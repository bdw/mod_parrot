#include "mod_parrot.h"

static Parrot_PMC new_stringhandle(Parrot_PMC interp);
static void open_stringhandle(Parrot_PMC interp, Parrot_PMC handlePMC, char * mode);
static void write_stringhandle(Parrot_PMC interp, Parrot_PMC handle, void * buf, size_t size);

static Parrot_PMC new_stringhandle(Parrot_PMC interp) {
	Parrot_PMC classPMC, keyPMC;
	Parrot_PMC handlePMC;
	Parrot_String className;
	Parrot_api_string_import_ascii(interp, "StringHandle", &className);
	Parrot_api_pmc_box_string(interp, className, &keyPMC);
	Parrot_api_pmc_get_class(interp, keyPMC, &classPMC);
	Parrot_api_pmc_new_from_class(interp, classPMC, NULL, &handlePMC);
	open_stringhandle(interp, handlePMC, "w+");
	return handlePMC;
}

static void open_stringhandle(Parrot_PMC interp, Parrot_PMC handlePMC, char * mode) {
	Parrot_String fileName, fileMode;
	Parrot_String methodName;
	Parrot_PMC methodPMC, callPMC;

	Parrot_api_string_import_ascii(interp, "open", &methodName);
	Parrot_api_pmc_find_method(interp, handlePMC, methodName, &methodPMC);

	Parrot_api_string_import_ascii(interp, "", &fileName);
	Parrot_api_string_import_ascii(interp, mode, &fileMode);

	Parrot_api_pmc_new_call_object(interp, &callPMC);
	Parrot_api_pmc_setup_signature(interp, callPMC, "PiSS", handlePMC, fileName, fileMode);
	Parrot_api_pmc_invoke(interp, methodPMC, callPMC);
}

static void write_stringhandle(Parrot_PMC interp, Parrot_PMC handlePMC, void * buffer, size_t size) {
	Parrot_PMC methodPMC, callPMC;
	Parrot_String methodName;
	Parrot_String importedString;
	Parrot_api_string_import_binary(interp, buffer, size, "binary", &importedString);
	Parrot_api_string_import_ascii(interp, "puts", &methodName);
	Parrot_api_pmc_find_method(interp, handlePMC, methodName, &methodPMC);
	Parrot_api_pmc_new_call_object(interp, &callPMC);
	Parrot_api_pmc_setup_signature(interp, callPMC, "PiS->I", handlePMC, importedString);
	Parrot_api_pmc_invoke(interp, methodPMC, callPMC);
}

void mod_parrot_io_new_input_handle(Parrot_PMC interp, request_rec *r, Parrot_PMC *handle) {
	*handle = new_stringhandle(interp);
}

void mod_parrot_io_read_input_handle(Parrot_PMC interp, request_rec *r, Parrot_PMC handle) {
	char buffer[1024];
	size_t count;
	if (ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)) 
		return;
	if (ap_should_client_block(r)) {
		count = r->remaining;
		/* current (post-2.4) apache doumentation will tell you that
		 * ap_get_client_block returns the number of bytes read (if
		 * everything is correct, that is). For apache 2.2, which is what I
		 * use for development, it simply returns 1 in case of success.
		 * Hence, the amount of bytes read should be deducted from the
		 * difference in remaining bytes between successive calls */
		while (ap_get_client_block(r, buffer, sizeof(buffer)) > 0) {
			write_stringhandle(interp, handle, buffer, count - r->remaining);
			count = r->remaining;
		}
	}

}

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
    printf("I am to read %d bytes\n", s);
    apr_off_t count = r->remaining;
    if(ap_get_client_block(r, b, s) > 0){
        printf("%d bytes requested, %d read, %d remaining\n", s, count - r->remaining, r->remaining);
        return count - r->remaining;
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
