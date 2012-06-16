#include "mod_parrot.h"

/**
 * Write a buffer of specific length to apache
 * @param void * b The buffer
 * @param size_t s The number of bytes
 * @param request_rec * r The request on which to respond
 * @return the number of bytes written
 **/
int mod_parrot_write(void * b, size_t s, request_rec * r) {
    return ap_rwrite(b, s, r);
}

/**
 * Setup refading of a client block (i.e., POST or PUT data)
 * Returns the amount of bytes left to read.
 **/
int mod_parrot_setup_input(request_rec *req) {
    if(ap_setup_client_block(req, REQUEST_CHUNKED_ERROR))
        return 0;
    if(!ap_should_client_block(req))
        return 0;
    return req->remaining;
}

/**
 * Read s bytes into buffer b from request r
 * Return the amount of bytes read
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
 * Also prints it out. Should not do that, but anyway.

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
      ap_rputs(rrmsg, req);
      fputs(rrmsg, stderr);
      ap_rputs("\n", req);
      ap_rputs(bcktrc, req);
      fputs(bcktrc, stderr);
  } 
  return HTTP_INTERNAL_SERVER_ERROR;

}
