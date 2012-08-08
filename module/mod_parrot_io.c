#include "mod_parrot.h"
#include <strings.h>
#include <ctype.h>


static char * ipaddr(apr_sockaddr_t *a) {
	char * string;
	apr_sockaddr_ip_get(&string, a);
	return string;
}



/**
 * Fetch the request parameters
 * @param Parrot_PMC i The interpreter
 * @param request_rec * r The request
 * @return Parrot_PMC hash  
 **/
Parrot_PMC mod_parrot_request_parameters(Parrot_PMC interp, request_rec * req) {
    Parrot_PMC hash;
    /* todo, make this non-nasty. although it works */
	hash = mod_parrot_hash_new(interp);
	
	mod_parrot_hash_put(interp, hash, "REQUEST_METHOD", (char*)req->method);
	mod_parrot_hash_put(interp, hash, "REQUEST_URI", req->unparsed_uri);
	mod_parrot_hash_put(interp, hash, "QUERY_STRING", req->args ? req->args : ""); 
	mod_parrot_hash_put(interp, hash, "HTTP_HOST", (char*)req->hostname);
    mod_parrot_hash_put(interp, hash, "SCRIPT_NAME", req->filename);
    mod_parrot_hash_put(interp, hash, "PATH_INFO", req->path_info);
	mod_parrot_hash_put(interp, hash, "SERVER_NAME", req->server->server_hostname);
	mod_parrot_hash_put(interp, hash, "SERVER_PROTOCOL", req->protocol);

	/* Network parameters. This should be simpler, but it isn't. */
	mod_parrot_hash_put(interp, hash, "SERVER_ADDR", ipaddr(req->connection->local_addr));
	mod_parrot_hash_put(interp, hash, "SERVER_PORT", 
             apr_itoa(req->pool, req->connection->local_addr->port));

	mod_parrot_hash_put(interp, hash, "REMOTE_ADDR", 
             ipaddr(req->connection->remote_addr));
	mod_parrot_hash_put(interp, hash, "REMOTE_PORT", 
             apr_itoa(req->pool, req->connection->remote_addr->port));

    if(req->server->server_admin) /* I don't believe this is ever NULL */
		mod_parrot_hash_put(interp, hash, "SERVER_ADMIN", req->server->server_admin);
    return hash;
}



/**
 * Read the input headers 
 * @param Parrot_PMC i 
 * @param request_rec * r
 * @return the input headers as a hash 
 **/
Parrot_PMC mod_parrot_headers_in(Parrot_PMC interp, request_rec * req) {
	const apr_array_header_t *array;
	apr_table_entry_t * entries;
    Parrot_PMC hash;
	int idx;
    hash = mod_parrot_hash_new(interp);
	array = apr_table_elts(req->headers_in);
	entries = (apr_table_entry_t *) array->elts;
	for(idx = 0; idx < array->nelts; idx++) {
		mod_parrot_hash_put(interp, hash, entries[idx].key, entries[idx].val);
	}
    return hash;
}


/**
 * I laugh in the face of inefficiency 
 * (although the efficient version would have been fun, as well)
 **/
void mod_parrot_header_out(Parrot_PMC interp_pmc, Parrot_PMC key_pmc, 
                           Parrot_PMC val_pmc, request_rec *req) {
    char * key = mod_parrot_export_cstring(interp_pmc, key_pmc);
    char * val = mod_parrot_export_cstring(interp_pmc, val_pmc);
    apr_table_set(req->headers_out, key, val);
    mod_parrot_free_cstring(interp_pmc, key);
    mod_parrot_free_cstring(interp_pmc, val);
}


void mod_parrot_set_status(request_rec * req, int code) {
    req->status = code;
}

/**
 * Write a buffer of specific length to apache
 *
 * @param void * b The buffer
 * @param size_t s The number of bytes
 * @param request_rec * r The request on which to respond
 * @return the number of bytes written
 **/
int mod_parrot_write(void * b, size_t s, request_rec * r) {
    return ap_rwrite(b, s, r);
}

/**
 * Setup reading of a client block (i.e., POST or PUT data)
 * @param request_rec * req The request from which to read
 * @return int the amount of bytes remainig to be read
 **/
int mod_parrot_open_input(request_rec *req) {
    if(ap_setup_client_block(req, REQUEST_CHUNKED_ERROR))
        return 0;
    if(!ap_should_client_block(req))
        return 0;
    return req->remaining;
}

/**
 * Read s bytes into buffer b from request r
 * @param void * b a pre-allocated buffer
 * @param size_t s the amount of bytes to read
 * @param request_rec r The 
 * @return int the amount of bytes read or -1 in case of an error
 **/
int mod_parrot_read(void * b, size_t s, request_rec * r) {
    apr_off_t count = r->read_length;
    if (ap_get_client_block(r, b, s) > 0) {
        return r->read_length - count;
    } 
    return -1;
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
int mod_parrot_report_error(Parrot_PMC interp, request_rec *req) {
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
