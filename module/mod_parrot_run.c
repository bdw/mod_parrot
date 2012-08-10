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

apr_status_t mod_parrot_run(Parrot_PMC interp_pmc, request_rec *req, 
                            mod_parrot_route * route) {
    mod_parrot_conf * conf;
    Parrot_PMC loader_pmc, args_pmc;
    Parrot_PMC request_pmc, route_pmc;
    char * path;
    /* get the configuration */
    conf = ap_get_module_config(req->server->module_config, &mod_parrot);    
    /* load libraries, preferably move this somewhere earlier */
    if(!mod_parrot_preload(interp_pmc, req->pool, conf)) {
        return mod_parrot_report(interp_pmc, req);
    }
    /* get the actual loader script  */
    path = loader_path(req->pool, conf, (char*)conf->loader);
    loader_pmc = loader_get(interp_pmc, path);
    /* wrap the structures */
    Parrot_api_wrap_pointer(interp_pmc, req, sizeof(*req), &request_pmc);
    Parrot_api_wrap_pointer(interp_pmc, route, sizeof(*route), &route_pmc);
    /* setup the arguments */
    args_pmc = mod_parrot_array_new(interp_pmc);
    mod_parrot_array_push(interp_pmc, args_pmc, request_pmc);
    mod_parrot_array_push(interp_pmc, args_pmc, route_pmc);
    /* run the script! maybe we want to move the responsibility 
     * for calling mod_parrot_report up to mod_parrot_handler. */
    if(Parrot_api_run_bytecode(interp_pmc, loader_pmc, args_pmc)) {
        return OK;
    } else {
        return mod_parrot_report(interp_pmc, req);
    }
}


/**
 * Report an error (with backtrace) to the apache logs (via stderr).
 * Sometimes these things are so easy.  (This routine used to print the
 * error messages to the as well, but apache overrides that because I
 * return an error code. Which is just as well for security, really).
 * 
 * Note, I want to replace this with a function that might run a script.
 *
 * @param Parrot_PMC interp The interpreter on which the error occured
 * @param request_rec * req The request on which the error occured. 
 * @return int the http code of the error (HTTP_INTERNAL_SERVER_ERROR)
 **/
apr_status_t mod_parrot_report(Parrot_PMC interp, request_rec *req) {
  Parrot_Int is_error, exit_code;
  Parrot_PMC exception;
  Parrot_String error_message, backtrace;
  if (Parrot_api_get_result(interp, &is_error, &exception, &exit_code, &error_message) && is_error) {
      /* do something useful on the basis of this information */
      char *rrmsg,  *bcktrc;
      Parrot_api_get_exception_backtrace(interp, exception, &backtrace);
      Parrot_api_string_export_ascii(interp, error_message, &rrmsg);
      Parrot_api_string_export_ascii(interp, backtrace, &bcktrc);
      fputs(rrmsg, stderr);
      fputs(bcktrc, stderr);
  }
  /**
   * This right here, is pretty much wrong 
   **/
  return HTTP_INTERNAL_SERVER_ERROR;
}
