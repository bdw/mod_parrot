#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"
#include "apr.h"
#include "parrot/api.h"
#include "imcc/api.h"
#include <unistd.h>
#include "config.h"


void mod_parrot_io_new_input_handle(Parrot_PMC interp, request_rec *req, Parrot_PMC *handle);
void mod_parrot_io_new_output_handle(Parrot_PMC interp, request_rec *req, Parrot_PMC *handle);

void mod_parrot_io_read_input_handle(Parrot_PMC interp, request_rec *req, Parrot_PMC handle);
void mod_parrot_io_write_output_handle(Parrot_PMC interp, request_rec *req, Parrot_PMC handle);
void mod_parrot_io_report_error(Parrot_PMC interp, request_rec *req);

void mod_parrot_interpreter(Parrot_PMC *interp);
void mod_parrot_run(Parrot_PMC interp, request_rec *req);
void mod_parrot_setup_args(Parrot_PMC interp, request_rec *req, Parrot_PMC *args);

typedef struct {
 const char * loaderPath;
} mod_parrot_conf;
