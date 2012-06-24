#include "mod_parrot.h"


Parrot_PMC mod_parrot_interpreter(mod_parrot_conf * conf) {
	Parrot_PMC interp, configHash;
    Parrot_PMC pir, pasm;
    Parrot_api_make_interpreter(NULL, 0, NULL, &interp);
    /* this is to help parrot set up the right paths by itself,
     * and yes, i do agree this is a bit of unneccesesary magic.
     * Parrots, it appears, are magical birds after all. */
	configHash = mod_parrot_new_hash(interp);
	mod_parrot_hash_set(interp, configHash, "build_dir", BUILDDIR);
	mod_parrot_hash_set(interp, configHash, "versiondir", VERSIONDIR);
	mod_parrot_hash_set(interp, configHash, "libdir", LIBDIR);
	Parrot_api_set_configuration_hash(interp, configHash);
    /* no pir without these calls ;-) */
	imcc_get_pir_compreg_api(interp, 1, &pir);
	imcc_get_pasm_compreg_api(interp, 1, &pasm);
    return interp;
}

extern module mod_parrot;
/* this madness will be simplified in due time */ 
static Parrot_PMC load_bytecode(Parrot_PMC interp, request_rec *req, const char * filename)
{
    Parrot_PMC bytecodePMC;
    Parrot_String fileNameStr;
    mod_parrot_conf * conf = ap_get_module_config(req->server->module_config, &mod_parrot);
    char * fullName = (conf ? 
                       apr_pstrcat(req->pool, conf->loaderPath, "/", filename, NULL) :
                       apr_pstrcat(req->pool, INSTALLDIR, "/", filename, NULL));
    puts(fullName);
    Parrot_api_string_import_ascii(interp, fullName, &fileNameStr);
    Parrot_api_load_bytecode_file(interp, fileNameStr, &bytecodePMC);
    return bytecodePMC;
}

int mod_parrot_run(Parrot_PMC interp, request_rec *req, const char * compilerName) {
    Parrot_PMC library, loader, mainRoutine;
    Parrot_PMC request, contextObject;
    Parrot_String compiler, scriptName;
    mod_parrot_conf * conf = 
        ap_get_module_config(req->server->module_config, &mod_parrot);
    const char *preload[] = {
        "apache.pbc"
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
