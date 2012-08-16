#include "mod_parrot.h"

extern module mod_parrot;

/* mime routing is really fallback routing as far as i'm concerned */
mod_parrot_route * mime_route(request_rec * req, mod_parrot_conf * conf) {
    if(req->content_type && 
       !strcmp(req->content_type, MOD_PARROT_MIME_TYPE)) {
        mod_parrot_route * route = 
            apr_pcalloc(req->pool, sizeof(mod_parrot_route));
        route->script = req->filename;
        return route;
    }
    return NULL;
}

/* suffix routing is our own special routing logic */
mod_parrot_route * suffix_route(request_rec * req, mod_parrot_conf * conf) {
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

/**
 * Find the current route by a couple of methods
 * Currently, try suffix routing, and mime routing after that.
 * I want to try application routing first, if supported by loader
 **/
mod_parrot_route * mod_parrot_find_route(request_rec * req) {
    mod_parrot_conf * conf = NULL;
    mod_parrot_route * route = NULL;
    conf = ap_get_module_config(req->server->module_config, &mod_parrot);
    if(!route && conf) 
        route = suffix_route(req, conf);
    if(!route)
        route = mime_route(req, conf);
    return route;
}



