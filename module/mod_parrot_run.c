#include "mod_parrot.h"

/** 
 * How this works:
 * - we get an interpreter from mod_parrot_interpreter, which is brand new
 * - we set it up in mod_parrot_run
 * - and then we immediately run the script
 * - if there is an error it goes to mod_parrot_report_error
 * - and otherwise mod_parrot_handler simply clears our interpreter
 *
 * How this is going to work 
 * - upon startup we create a pool of interpreters, all children of one
 * - we get an interpreter to serve a request via mod_parrot_get_interpreter(request_rec)
 * - this tries to get one from mod_parrot_interpreter_pool_get()
 * - if thats not possible it creates a new one from the root interpreter
 * - this new interpreter is set up via mod_parrot_interpreter_setup()
 * - this interpreter is passed to mod_parrot_start_loader
 * - the loader creates a child interpreter and starts the scripts
 * - afterwards, the child interpreter is destroyed
 * - if an exception is thrown, it is freezed and thawed to the loader 
 * - which passes it to a (default or specified error handler)
 **/

extern module mod_parrot;

/**
 * Interface-first design
 *
 * Internally:
 * get the path to a loader script via loader_path()
 * obtain a loader / library via loader_get()
 *
 * wrap the route into a parrot structure with wrap_route()
 * wrap the request into a parrot pointer with wrap_request()
 *
 * Publicly:
 * preload libraries with mod_parrot_preload()
 * run a loader to a route with mod_parrot_run()
 * report an error with mod_parrot_report()
 **/

/* This is the perfect place to extend with a more thorough search */
static char * loader_path(apr_pool_t * pool, mod_parrot_conf * conf, 
                          char * filename) {
    return conf ? apr_pstrcat(pool, conf->loaderPath, "/", filename, NULL) :
        apr_pstrcat(pool, INSTALLDIR, "/", filename, NULL);
}

static Parrot_PMC loader_get(Parrot_PMC interp_pmc, char * path) {
    Parrot_String path_str;
    Parrot_PMC bytecode_pmc;
    Parrot_api_string_import_ascii(interp_pmc, path, &path_str);
    if(Parrot_api_load_bytecode_file(interp_pmc, path_str, &bytecode_pmc))
        return bytecode_pmc;
    return NULL; /* crash and crash hard! */
}


/* yes we should have a more consistent calling interface */
/* and for those who wonder, this is public to allow for calling it earlier */
Parrot_Int mod_parrot_preload(Parrot_PMC interp_pmc, apr_pool_t * pool, 
                              mod_parrot_conf * conf) {
    /* put the libraries in a separate function? */
    char * libraries[] = {
        "apache.pbc",
        "common.pbc",
        NULL
    };
    Parrot_PMC dummy_pmc;
    int i;
    for(i = 0; libraries[i]; i++) {
        char * path = loader_path(pool, conf, libraries[i]);
        Parrot_PMC bytecode_pmc = loader_get(interp_pmc, path);
        if(!Parrot_api_ready_bytecode(interp_pmc, bytecode_pmc, &dummy_pmc))
            return 0;
    }
    return 1;
}

/* evil, maniacal laughter */
Parrot_PMC wrap_route(Parrot_PMC interp_pmc, mod_parrot_route * route) {
    Parrot_PMC route_pmc;
    int argc = sizeof(*route) / sizeof(char*);
    const char ** argv = (const char**)(route); /* this line right here */
    Parrot_api_pmc_wrap_string_array(interp_pmc, argc, argv, &route_pmc);
    return route_pmc;
}

Parrot_PMC wrap_request(Parrot_PMC interp_pmc, request_rec *req) {
    Parrot_PMC request_pmc;
    Parrot_api_wrap_pointer(interp_pmc, req, sizeof(*req), &request_pmc);
    return request_pmc;
}

/**
 * Start handling a request with a given interpreter to a determined route 
 *
 * @param Parrot_PMC interp_pmc
 * @param request_rec * request
 * @param mod_parrot_route * route
 * @return apr_status_t a status code (OK or a HTTP code)
 **/
apr_status_t mod_parrot_run(Parrot_PMC interp_pmc, request_rec *request, 
                            mod_parrot_route * route) {
    mod_parrot_conf * conf;
    Parrot_PMC loader_pmc, args_pmc;
    Parrot_PMC request_pmc, route_pmc;
    char * path;
    /* get the configuration */
    conf = ap_get_module_config(request->server->module_config, &mod_parrot);    
    /* load libraries, preferably move this somewhere earlier */
    /* 
     * for the record, that cannot be done yet because of:
     *
     * interpreters must be short-lived (long story :-))
     * hence, i must create them on every request
     * i need to allocate some memory to search for the libraries
     * thus I need to use the request pool (as opposed to the process pool)
     * because only the request pool is cleaned after every request
     */
    if(!mod_parrot_preload(interp_pmc, request->pool, conf)) {
        return mod_parrot_report(interp_pmc, request);
    }

    /* get the actual loader script  */
    path = loader_path(request->pool, conf, (char*)conf->loader);
    loader_pmc = loader_get(interp_pmc, path);
    /* wrap the structures */
    request_pmc = wrap_request(interp_pmc, request);
    route_pmc = wrap_route(interp_pmc, route);
    /* setup the arguments */
    args_pmc = mod_parrot_array_new(interp_pmc);
    mod_parrot_array_push(interp_pmc, args_pmc, request_pmc);
    mod_parrot_array_push(interp_pmc, args_pmc, route_pmc);
    /* run the script! maybe we want to move the responsibility 
     * for calling mod_parrot_report up to mod_parrot_handler. */
    if(Parrot_api_run_bytecode(interp_pmc, loader_pmc, args_pmc)) {
        return OK;
    } else {
        return mod_parrot_report(interp_pmc, request);
    }
}


/**
 * The calling signature on this thing is so incredibly wrong.
 * This function SHOULD have been passed the route and the exception,
 * and perhaps the request as well.
 *
 * Instead it gets the exception for itself, just dumping the backtrace
 * (w/o proper information) in the server error log. I'd really like to fix
 * this one day.
 *
 * @param Parrot_PMC interp The interpreter on which the error occured
 * @param request_rec * req The request on which the error occured. 
 * @return int the http code of the error (HTTP_INTERNAL_SERVER_ERROR)
 **/
apr_status_t mod_parrot_report(Parrot_PMC interp_pmc, request_rec *req) {
  Parrot_Int is_error = 0, exit_code = 0;
  Parrot_PMC exception_pmc;
  Parrot_String error_str, backtrace_str;
  Parrot_Int status = Parrot_api_get_result(interp_pmc, &is_error, 
                                            &exception_pmc, &exit_code, &error_str);
  if(status && is_error) {
      /* report the error  */
      char *error,  *backtrace;
      Parrot_api_get_exception_backtrace(interp_pmc, exception_pmc, &backtrace_str);
      Parrot_api_string_export_ascii(interp_pmc, error_str, &error);
      Parrot_api_string_export_ascii(interp_pmc, backtrace_str, &backtrace);
      /* print them to the log */
      fputs(error, stderr);
      fputs(backtrace, stderr);
      /* free the exported string */
      Parrot_api_string_free_exported_ascii(interp_pmc, error);
      Parrot_api_string_free_exported_ascii(interp_pmc, backtrace);
      /* oh, and get the real exit code */
      Parrot_api_pmc_get_integer(interp_pmc, exception_pmc, &exit_code);
  }
  return (exit_code > 100 ? exit_code : 
          (is_error ? HTTP_INTERNAL_SERVER_ERROR : OK));
}
