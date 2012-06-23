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
static Parrot_PMC load_bytecode(Parrot_PMC interp, request_rec *req, char * filename)
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

int mod_parrot_run(Parrot_PMC interp, request_rec *req) {
    Parrot_PMC libraryPMC;
	Parrot_PMC bytecodePMC;
    Parrot_PMC requestPMC;
    libraryPMC = load_bytecode(interp, req, "apache.pbc");
    bytecodePMC = load_bytecode(interp, req, "mod_parrot.pbc");
    Parrot_api_wrap_pointer(interp, req, sizeof(request_rec), &requestPMC);
    if(!Parrot_api_run_bytecode(interp, libraryPMC, requestPMC)) {
        return mod_parrot_report_error(interp, req);
    } 
    /* TODO: build a more useful call signature than (request) for loaders */
    if(Parrot_api_run_bytecode(interp, bytecodePMC, requestPMC)) {
        return OK;
    } else {
        return mod_parrot_report_error(interp, req);
    }
}
