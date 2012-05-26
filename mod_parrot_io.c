#include "mod_parrot.h"

static Parrot_PMC new_stringhandle(Parrot_PMC interp);
static void open_stringhandle(Parrot_PMC interp, Parrot_PMC handlePMC, char * mode);
static Parrot_String read_stringhandle(Parrot_PMC interp, Parrot_PMC handle);

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

void mod_parrot_io_new_input_handle(Parrot_PMC interp, request_rec *r, Parrot_PMC *handle) {
	*handle = new_stringhandle(interp);
}

void mod_parrot_io_new_output_handle(Parrot_PMC interp, request_rec *r, Parrot_PMC *handle) {
	*handle = new_stringhandle(interp);
}

void mod_parrot_io_read_input_handle(Parrot_PMC interp, request_rec *r, Parrot_PMC handle) {
	// not yet implemented
}

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
