#include "mod_parrot.h"

/* The static functions below should be in some mod_parrot_util file */
static Parrot_PMC new_instance(Parrot_PMC i, char * class, Parrot_PMC initPMC) {
	Parrot_PMC classPMC, keyPMC;
	Parrot_String className;
	Parrot_PMC instancePMC;
	Parrot_api_string_import_ascii(i, class, &className);
	Parrot_api_pmc_box_string(i, className, &keyPMC);
	Parrot_api_pmc_get_class(i, keyPMC, &classPMC);
	Parrot_api_pmc_new_from_class(i, classPMC, initPMC, &instancePMC);
	return instancePMC;
}

static void hash_set(Parrot_PMC i, Parrot_PMC h, char * k, char * v) {
	Parrot_String kS, vS; // key string, value string
	Parrot_PMC vP;
	Parrot_api_string_import_ascii(i, k, &kS);
	Parrot_api_string_import_ascii(i, v, &vS);
	Parrot_api_pmc_box_string(i, vS, &vP);
	Parrot_api_pmc_set_keyed_string(i, h, kS, vP);
}


Parrot_PMC mod_parrot_interpreter(mod_parrot_conf * conf) {
	Parrot_PMC interp, configHash;
    Parrot_PMC pir, pasm;
    Parrot_api_make_interpreter(NULL, 0, NULL, &interp);
	configHash = new_instance(interp, "Hash", NULL);
	hash_set(interp, configHash, "build_dir", BUILDDIR);
	hash_set(interp, configHash, "versiondir", VERSIONDIR);
	hash_set(interp, configHash, "libdir", LIBDIR);
	Parrot_api_set_configuration_hash(interp, configHash);
	imcc_get_pir_compreg_api(interp, 1, &pir);
	imcc_get_pasm_compreg_api(interp, 1, &pasm);
    return interp;
}

extern module mod_parrot;

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
    if(Parrot_api_run_bytecode(interp, bytecodePMC, requestPMC)) {
        return OK;
    } else {
        return mod_parrot_report_error(interp, req);
    }
}
