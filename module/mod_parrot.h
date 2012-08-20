#include <unistd.h>
#include <libgen.h> 
#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "ap_config.h"
#include "ap_mpm.h"
#include "apr.h"
#include "apr_file_info.h"
#include "apr_strings.h"
#include "apr_uri.h"

/* here there be dragons */
#include "parrot/api.h"
#include "imcc/api.h"
#include "config.h"

#define DEFAULT_LOADER "echo.pbc"

/* Specify what script was requested. */
typedef struct { 
    char * language;
    char * script;
    char * className; /* for those who compile with c++ */
    char * routine;
} mod_parrot_route;

/* Server configuration */
typedef struct {
  const char * loaderPath;
  char * loader;
  apr_table_t * languages;
} mod_parrot_conf;

/* Directory configuration */
typedef struct {
    mod_parrot_route * application;
} mod_parrot_spec;


#define MOD_PARROT_MIME_TYPE "application/x-httpd-parrot"

/* determine if we run in a threaded mpm */
int mod_parrot_mpm_is_threaded();

/* determine if and how a request should be handled */
mod_parrot_route * mod_parrot_find_route(request_rec * req);
mod_parrot_route * mod_parrot_parse_route(apr_pool_t * pool, const char * arg);

/* IO */
int mod_parrot_write(void * buf, size_t size, request_rec *req);
int mod_parrot_read(void * buf, size_t size, request_rec * req);

/* interpreter interface */
 Parrot_PMC mod_parrot_acquire_interpreter(server_rec * srv);
void mod_parrot_release_interpreter(server_rec * srv, Parrot_PMC interp_pmc);

/* Run and report */
Parrot_Int mod_parrot_preload(Parrot_PMC interp_pmc, apr_pool_t * pool,
                              mod_parrot_conf * conf);
apr_status_t mod_parrot_run(Parrot_PMC interp_pmc, request_rec *req, 
                            mod_parrot_route * route);
apr_status_t mod_parrot_report(Parrot_PMC interp_pmc, request_rec *req);

/* Utility */
Parrot_PMC mod_parrot_array_new(Parrot_PMC interp_pmc);
void mod_parrot_array_push(Parrot_PMC interp_pmc, Parrot_PMC array_pmc, 
                           Parrot_PMC data_pmc);
Parrot_PMC mod_parrot_hash_new(Parrot_PMC interp_pmc);
void mod_parrot_hash_put(Parrot_PMC interp_pmc, Parrot_PMC hash_pmc, 
                         char * key, char * value);
char * mod_parrot_export_cstring(Parrot_PMC interp_pmc, Parrot_PMC export_pmc);
void mod_parrot_free_cstring(Parrot_PMC interp_pmc, char * cstring);

/* Cool in principle, unused due to poor mens polymorphism */
typedef void (*mod_parrot_hash_callback)(void *, char *, char *);
void mod_parrot_hash_iterate(Parrot_PMC interp, Parrot_PMC hash, 
                             mod_parrot_hash_callback cb,  void * userData);
