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

typedef struct { 
    const char * language;
    const char * script;
    const char * class;
    const char * routine;
} mod_parrot_route;

mod_parrot_route * mod_parrot_router(request_rec * req);



int mod_parrot_write(void * buf, size_t size, request_rec *req);
int mod_parrot_read(void * buf, size_t size, request_rec * req);

Parrot_PMC mod_parrot_acquire_interpreter(server_rec * srv);
void mod_parrot_release_interpreter(server_rec * srv, Parrot_PMC interp_pmc);
apr_status_t mod_parrot_run(Parrot_PMC interp_pmc, request_rec *req, mod_parrot_route * route);
apr_status_t mod_parrot_report_error(Parrot_PMC interp, request_rec *req);

void mod_parrot_setup_args(Parrot_PMC interp, request_rec *req, Parrot_PMC *args);

Parrot_PMC mod_parrot_hash_new(Parrot_PMC interp);
void mod_parrot_hash_put(Parrot_PMC interp, Parrot_PMC hash, char * key, char * value);
char * mod_parrot_export_cstring(Parrot_PMC interp_pmc, Parrot_PMC export_pmc);
void mod_parrot_free_cstring(Parrot_PMC interp_pmc, char * cstring);


typedef void (*mod_parrot_hash_callback)(void *, char *, char *);
void mod_parrot_hash_iterate(Parrot_PMC interp, Parrot_PMC hash, 
                             mod_parrot_hash_callback cb,  void * userData);
