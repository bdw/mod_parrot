#include "mod_parrot.h"
#include <strings.h>
#include <ctype.h>

/* Some of the functions below should be in mod_parrot_util.c. */

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

static char * header_convert(apr_pool_t *pool, char * header)  {
	int idx;
	char * word;
	word = (strncasecmp("content", header, 7) ? 
			apr_pstrcat(pool, "HTTP_", header, NULL) :
			apr_pstrdup(pool, header));

	for(idx = 0; word[idx]; idx++) {
		if(isalpha(word[idx]) && islower(word[idx]))
			word[idx] = toupper(word[idx]);
		else if(!isalnum(word[idx]))
			word[idx] = '_';
	}
	return word;
}

static char * ipaddr(apr_sockaddr_t *a) {
	char * string;
	apr_sockaddr_ip_get(&string, a);
	return string;
}

void mod_parrot_setup_args(Parrot_PMC i, request_rec *req, Parrot_PMC *args) {
	const apr_array_header_t *array;
    mod_parrot_conf * conf;
	apr_table_entry_t * entries;
	int idx;

	*args = new_instance(i, "Hash", NULL);
	
	hash_set(i, *args, "REQUEST_METHOD", (char*)req->method);
	hash_set(i, *args, "REQUEST_URI", req->unparsed_uri);
	hash_set(i, *args, "QUERY_STRING", req->args ? req->args : ""); 
	hash_set(i, *args, "HTTP_HOST", (char*)req->hostname);
    hash_set(i, *args, "SCRIPT_NAME", req->filename);
    hash_set(i, *args, "PATH_INFO", req->path_info);
	hash_set(i, *args, "SERVER_NAME", req->server->server_hostname);
	hash_set(i, *args, "SERVER_PROTOCOL", req->protocol);

	/* Network parameters. This should be simpler, but it isn't. */
	hash_set(i, *args, "SERVER_ADDR", ipaddr(req->connection->local_addr));
	hash_set(i, *args, "SERVER_PORT", apr_itoa(req->pool, req->connection->local_addr->port));

	hash_set(i, *args, "REMOTE_ADDR", ipaddr(req->connection->remote_addr));
	hash_set(i, *args, "REMOTE_PORT", apr_itoa(req->pool, req->connection->remote_addr->port));

	if(req->server->server_admin) /* I don't believe this is ever NULL, but anyway */
		hash_set(i, *args, "SERVER_ADMIN", req->server->server_admin);

	/* Read headers. It may be worthwhile to extract this into its' own
	 * routine. Update: because then it can be called with NCI */
	array = apr_table_elts(req->headers_in);
	entries = (apr_table_entry_t *) array->elts;
	for(idx = 0; idx < array->nelts; idx++) {
		if(!strcasecmp(entries[idx].key, "host"))
			continue;
		hash_set(i, *args, header_convert(req->pool, entries[idx].key), entries[idx].val);
	}
    
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
