#include "mod_parrot.h"

extern module mod_parrot;
/**
 * Determine the script to run
 *
 * Note, I want to make this different, more flexible, maybe even run a
 * script. For now, just copy the old logic here. 
 **/
mod_parrot_route * mod_parrot_find_route(request_rec * req) {
    mod_parrot_conf * conf = 
        ap_get_module_config(req->server->module_config, &mod_parrot);
    char * path = apr_pstrdup(req->pool, req->filename);
    char * base = basename(path); /* basename is not allways reentrant */
    const char * compiler = NULL;
    int dot = ap_rind(base, '.'); /* find the file suffix */
    if(dot > 0) {
        compiler = apr_table_get(conf->languages, base + dot);
    } 
    if(compiler) {
        mod_parrot_route * route = 
            apr_pcalloc(req->pool, sizeof(mod_parrot_route));
        route->language = (char*)compiler;
        route->script = path;
        return route;
    } else {
        return NULL;
    }
}
