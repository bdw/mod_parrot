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

void mod_parrot_interpreter(Parrot_PMC *interp) {
	Parrot_PMC configHash, pir, pasm;
	Parrot_api_make_interpreter(NULL, 0, NULL, interp);
	configHash = new_instance(*interp, "Hash", NULL);
	hash_set(*interp, configHash, "build_dir", BUILDDIR);
	hash_set(*interp, configHash, "versiondir", VERSIONDIR);
	hash_set(*interp, configHash, "libdir", LIBDIR);
	Parrot_api_set_configuration_hash(*interp, configHash);
    Parrot_api_add_library_search_path(*interp, INSTALLDIR);
	imcc_get_pir_compreg_api(*interp, 1, &pir);
	imcc_get_pasm_compreg_api(*interp, 1, &pasm);
}

extern module mod_parrot;

int mod_parrot_run(Parrot_PMC interp, request_rec *req) {
	Parrot_PMC bytecodePMC;
    Parrot_PMC requestPMC;
	Parrot_PMC inputPMC;
	Parrot_PMC stdinPMC;
	Parrot_String fileNameStr;
    char * filename;
    mod_parrot_conf * conf = NULL;
    /** 
     * This is here, because, as it turns out, they can only be called once.
     * And i have no idea how many times mod_parrot_read will be called. 
     **/
    ap_setup_client_block(req, REQUEST_CHUNKED_DECHUNK);
    ap_should_client_block(req);
    
    conf = ap_get_module_config(req->server->module_config, &mod_parrot);
    if(conf) {
        filename = apr_pstrcat(req->pool, conf->loaderPath, 
                               "/", conf->loader, NULL);
        /* should not be here. no, no no */
        Parrot_api_add_library_search_path(interp, conf->loaderPath);
    } else {
        filename = apr_pstrcat(req->pool, INSTALLDIR, 
                               "/", "mod_parrot.pbc", NULL);
    }

    /* initialize the loader script */
    Parrot_api_string_import_ascii(interp, filename, &fileNameStr);
    Parrot_api_load_bytecode_file(interp, fileNameStr, &bytecodePMC);
    Parrot_api_wrap_pointer(interp, req, sizeof(request_rec), &requestPMC);
    
    if(Parrot_api_run_bytecode(interp, bytecodePMC, requestPMC)) {
        return OK;
    } else {
        return mod_parrot_report_error(interp, req);
    }
}
