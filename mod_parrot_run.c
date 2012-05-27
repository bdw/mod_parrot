#include "mod_parrot.h"
#include <stdio.h>

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


void mod_parrot_setup_args(Parrot_PMC i, request_rec *req, Parrot_PMC *args) {
	const apr_array_header_t *array;
	apr_table_entry_t * entries;
	int idx;

	*args = new_instance(i, "Hash", NULL);

	hash_set(i, *args, "SERVER_NAME", req->server->server_hostname);
	hash_set(i, *args, "REQUEST_METHOD", (char*)req->method);
	hash_set(i, *args, "REQUEST_URI", req->unparsed_uri);
	hash_set(i, *args, "QUERY_STRING", req->args ? req->args : ""); 
	hash_set(i, *args, "HTTP_HOST", (char*)req->hostname);
	hash_set(i, *args, "SERVER_PROTOCOL", req->protocol);
	/*	hash_set(i, *args, "REMOTE_ADDR", req->useragent_ip); */
	
	array = apr_table_elts(req->headers_in);
	entries = (apr_table_entry_t *) array->elts;
	for(idx = 0; idx < array->nelts; idx++) {
		hash_set(i, *args, entries[idx].key, entries[idx].val);
	}

}

void mod_parrot_run(Parrot_PMC i, request_rec *req) {
	Parrot_PMC bytecodePMC, argumentsPMC;
	Parrot_PMC inputPMC, outputPMC;
	Parrot_PMC stdinPMC, stdoutPMC;
	Parrot_String fileName;
	mod_parrot_io_new_input_handle(i, req, &inputPMC);
	mod_parrot_io_new_output_handle(i, req, &outputPMC);
	mod_parrot_io_read_input_handle(i, req, inputPMC);

	mod_parrot_setup_args(i, req, &argumentsPMC);

	Parrot_api_set_stdhandle(i, inputPMC, 0, &stdinPMC);
	Parrot_api_set_stdhandle(i, outputPMC, 1, &stdoutPMC);
	
	Parrot_api_string_import_ascii(i, "mod_parrot.pbc", &fileName);
	Parrot_api_load_bytecode_file(i, fileName, &bytecodePMC);
	
	Parrot_api_run_bytecode(i, bytecodePMC, argumentsPMC);
	
	mod_parrot_io_write_output_handle(i, req, outputPMC);
}
