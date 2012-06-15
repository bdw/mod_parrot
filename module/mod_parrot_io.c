#include "mod_parrot.h"

static Parrot_PMC new_stringhandle(Parrot_PMC interp);
static void open_stringhandle(Parrot_PMC interp, Parrot_PMC handlePMC, char * mode);
static Parrot_String read_stringhandle(Parrot_PMC interp, Parrot_PMC handle);
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

static Parrot_String read_stringhandle(Parrot_PMC interp, Parrot_PMC handlePMC) {
	Parrot_String methodName;
	Parrot_PMC methodPMC, callPMC, resultPMC;
	Parrot_String result;
	Parrot_api_string_import_ascii(interp, "readall", &methodName);
	Parrot_api_pmc_find_method(interp, handlePMC, methodName, &methodPMC);
	Parrot_api_pmc_new_call_object(interp, &callPMC);
	Parrot_api_pmc_setup_signature(interp, callPMC, "Pi->S", handlePMC);
	Parrot_api_pmc_invoke(interp, methodPMC, callPMC);
	Parrot_api_pmc_get_keyed_int(interp, callPMC, 0, &resultPMC);
	Parrot_api_pmc_get_string(interp, resultPMC, &result);
	return result;
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

void mod_parrot_io_new_output_handle(Parrot_PMC interp, request_rec *r, Parrot_PMC *handle) {
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
 * This will obviously be an awesome routine one day.
 * Right now it assumes stringhandles.
 **/
void mod_parrot_io_write_output_handle(Parrot_PMC interp, request_rec *req, Parrot_PMC handle) {
	Parrot_String output;
	Parrot_Int length;
	char * buffer;
	output = read_stringhandle(interp, handle);
	Parrot_api_string_export_ascii(interp, output, &buffer);
	Parrot_api_string_byte_length(interp, output, &length);
	ap_rwrite(buffer, length, req);
	Parrot_api_string_free_exported_ascii(interp, buffer);
}

void mod_parrot_report_error(Parrot_PMC interp, request_rec *req) {
  Parrot_Int is_error, exit_code;
  Parrot_PMC exception;
  Parrot_String error_message, backtrace;
  req->status = 500;
  if (Parrot_api_get_result(interp, &is_error, &exception, &exit_code, &error_message) && is_error) {
      /* do something useful on the basis of this information */
      char *rrmsg,  *bcktrc;
      Parrot_api_get_exception_backtrace(interp, exception, &backtrace);
      Parrot_api_string_export_ascii(interp, error_message, &rrmsg);
      Parrot_api_string_export_ascii(interp, backtrace, &bcktrc);
      ap_rputs(rrmsg, req);
      ap_rputs("\n", req);
      ap_rputs(bcktrc, req);
  } 
      

}
