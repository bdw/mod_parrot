#include <unistd.h>
#include <libgen.h> 
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"
#include "apr.h"
#include "apr_strings.h"
/* here there be dragons */
#include "parrot/api.h"
#include "imcc/api.h"

#include "config.h"

#define DEFAULT_LOADER "echo.pbc"

typedef struct {
  const char * loaderPath;
  const char * loader;
  apr_table_t * languages;
} mod_parrot_conf;

int mod_parrot_report_error(Parrot_PMC interp, request_rec *req);
int mod_parrot_write(void * buf, size_t size, request_rec *req);
int mod_parrot_read(void * buf, size_t size, request_rec * req);

Parrot_PMC mod_parrot_interpreter(mod_parrot_conf * conf);
int mod_parrot_run(Parrot_PMC interp, request_rec *req, const char * compiler);
void mod_parrot_setup_args(Parrot_PMC interp, request_rec *req, Parrot_PMC *args);
Parrot_PMC mod_parrot_new_hash(Parrot_PMC interp);
void mod_parrot_hash_set(Parrot_PMC interp, Parrot_PMC hash, char * key, char * value);
