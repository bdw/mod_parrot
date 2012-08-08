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
 * Determine the script to run
 *
 * Note, I want to make this different, more flexible, maybe even run a
 * script. For now, just copy the old logic here. 
 **/
mod_parrot_route * mod_parrot_router(request_rec * req) {
    mod_parrot_conf * conf = ap_get_module_config(req->server->module_config, &mod_parrot);
    char * fullName = apr_pstrdup(req->pool, req->filename);
    char * baseName = basename(fullName);
    char * compiler = NULL;
    int code, idx = ap_rind(baseName, '.'); // find the file suffix 
    if(idx > 0) {
        compiler = apr_table_get(conf->languages, baseName + idx);
    } 
    if(compiler) {
        mod_parrot_route * route = apr_pcalloc(req->pool, sizeof(mod_parrot_route));
        route->compiler = compiler;
        route->script = fullName;
        return route;
    } else {
        return NULL;
    }
}

/* this madness will be simplified in due time */ 
static Parrot_PMC load_bytecode(Parrot_PMC interp, request_rec *req, 
                                const char * filename) {
    Parrot_PMC bytecodePMC;
    Parrot_String fileNameStr;
    mod_parrot_conf * conf = ap_get_module_config(req->server->module_config, &mod_parrot);
    char * fullName = (conf ? 
                       apr_pstrcat(req->pool, conf->loaderPath, "/", filename, NULL) :
                       apr_pstrcat(req->pool, INSTALLDIR, "/", filename, NULL));
    Parrot_api_string_import_ascii(interp, fullName, &fileNameStr);
    Parrot_api_load_bytecode_file(interp, fileNameStr, &bytecodePMC);
    return bytecodePMC;
}


Parrot_Int mod_parrot_preload(Parrot_PMC interp_pmc, mod_parrot_conf * conf) {
    
}

apr_status_t mod_parrot_run(Parrot_PMC interp, request_rec *req, const char * compilerName) {
    Parrot_PMC library, loader, mainRoutine;
    Parrot_PMC request, contextObject;
    Parrot_String compiler, scriptName;
    mod_parrot_conf * conf = 
        ap_get_module_config(req->server->module_config, &mod_parrot);
    const char *preload[] = {
        "apache.pbc",
        "common.pbc",
    };
    int i;

    Parrot_api_string_import_ascii(interp, compilerName, &compiler);
    Parrot_api_string_import_ascii(interp, req->filename, &scriptName);
    for(i = 0; i < (sizeof(preload)/sizeof(*preload)); i++) {
        library = load_bytecode(interp, req, preload[i]);
        if(!Parrot_api_ready_bytecode(interp, library, &mainRoutine)) {
            return mod_parrot_report_error(interp, req);
        } 
    }
    if(!Parrot_api_wrap_pointer(interp, req, sizeof(request_rec), &request)) {
        return mod_parrot_report_error(interp, req);
    }
    loader = load_bytecode(interp, req, conf->loader);
    Parrot_api_ready_bytecode(interp, loader, &mainRoutine);
    Parrot_api_pmc_new_call_object(interp, &contextObject);
    Parrot_api_pmc_setup_signature(interp, contextObject, "PSS->", 
                                   request, compiler, scriptName);
    if(Parrot_api_pmc_invoke(interp, mainRoutine, contextObject)) {
        return OK;
    } else {
        return mod_parrot_report_error(interp, req);
    }
}
