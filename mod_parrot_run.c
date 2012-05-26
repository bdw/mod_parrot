#include "mod_parrot.h"

void mod_parrot_setup_args(Parrot_PMC interp, request_rec *req, Parrot_PMC *args) {
	*args = NULL;
}

void mod_parrot_run(Parrot_PMC interp, request_rec *req) {
	Parrot_PMC bytecodePMC, argumentsPMC;
	Parrot_PMC inputPMC, outputPMC;
	Parrot_PMC stdinPMC, stdoutPMC;
	Parrot_String fileName;
	mod_parrot_io_new_input_handle(interp, req, &inputPMC);
	mod_parrot_io_new_output_handle(interp, req, &outputPMC);
	mod_parrot_io_read_input_handle(interp, req, inputPMC);

	mod_parrot_setup_args(interp, req, &argumentsPMC);

	Parrot_api_set_stdhandle(interp, inputPMC, 0, &stdinPMC);
	Parrot_api_set_stdhandle(interp, outputPMC, 1, &stdoutPMC);
	


	Parrot_api_string_import_ascii(interp, "mod_parrot.pbc", &fileName);
	Parrot_api_load_bytecode_file(interp, fileName, &bytecodePMC);
	
	Parrot_api_run_bytecode(interp, bytecodePMC, argumentsPMC);
	
	mod_parrot_io_write_output_handle(interp, req, outputPMC);
}
