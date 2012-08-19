#include "mod_parrot.h"
#include <sys/stat.h> 
#include <libgen.h>

extern module mod_parrot;

/* not portable! */
int file_exists(char * path) {
    struct stat buf;
    if(stat(path, &buf)) /* file is not stat-able */
        return 0;
    return buf.st_mode != S_IFDIR;
}

mod_parrot_route * app_route(request_rec * req, mod_parrot_spec * spec) {
    mod_parrot_route * route;
    if(!spec->application)
        return NULL;
    /* handle it if we have a specified application and the file does not exist 
     * or the file requested is the specified script for the application */
    if(file_exists(req->filename) && strcmp(req->filename, spec->application->script))
        return NULL; /* other file than the requested script, 
                        we shouldn't handle it */
    route = apr_pcalloc(req->pool, sizeof(mod_parrot_route));
    route->language = spec->application->language;
    if(spec->application->script) { /* append the script to the directory to find the relative path */
        char * path = apr_pstrdup(req->pool, req->filename);
        route->script = apr_pstrcat(req->pool, dirname(path), "/", spec->application->script, NULL);
    } else { /* use the filename */
        route->script = req->filename;
    }
    route->className = spec->application->className;
    route->routine = spec->application->routine;
    return route;
}

/* mime routing is really fallback routing as far as i'm concerned */
mod_parrot_route * mime_route(request_rec * req, mod_parrot_conf * conf) {
    if(req->content_type && !strcmp(req->content_type, MOD_PARROT_MIME_TYPE)) {
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

void route_dump(mod_parrot_route * route) {
    if(route->language)
        printf("Language is %s\n", route->language);
    else
        puts("Langauge is null");
    if(route->script) 
        printf("Script is %s\n", route->script);
    else
        puts("Script is null");
    if(route->className) 
        printf("Class name is %s\n", route->className);
    else
        puts("Class name is null");
    if(route->routine)
        printf("Routine name is %s\n", route->routine);
    else
        puts("Routine name is null");
}

/**
 * Parse a route 
 *
 * Proposed format:
 * <language>://<relative-filename>(/(<Class>/)?(#<Routine>?))?
 * or, the scheme specifies the language
 * and the path after it the class 
 * and the fragment specifies the routine 
 *
 * Thus:
 * winxed://foo.winxed/#bar specifies the routine named bar in foo.winxed
 * winxed://bar.winxed/Foo#bar specifies the bar method of a Foo object in bar.winxed
 * perl6://quix.p6/Quam specifies an (invokable) instance of class Quam in quix.p6
 * perl6://baz.p6/ specifies the main routine of baz.p6
 **/
mod_parrot_route * mod_parrot_parse_route(apr_pool_t * pool, const char * arg) {
    apr_uri_t uri;
    mod_parrot_route * route;
    if(apr_uri_parse(pool, arg, &uri) != APR_SUCCESS) 
        return NULL;
    route = apr_pcalloc(pool, sizeof(mod_parrot_route));
    route->language = uri.scheme; 
    route->script = uri.hostname; /* this is relative */
    /* ignore the first slash */
    route->className = (uri.path && strlen(uri.path) > 1) ? (uri.path + 1) : NULL;
    /* and the hash of the fragment */
    route->routine = (uri.fragment && strlen(uri.fragment) > 0) ? (uri.fragment) : NULL;
    return route;
}

/**
 * Find the current route by a couple of methods
 *
 * Currently, try app routing, suffix routing, and mime routing after that.
 * If nothing is found return NULL
 **/
mod_parrot_route * mod_parrot_find_route(request_rec * req) {
    mod_parrot_conf * conf = NULL;
    mod_parrot_spec * spec = NULL;
    mod_parrot_route * route = NULL;
    spec = ap_get_module_config(req->per_dir_config, &mod_parrot);
    conf = ap_get_module_config(req->server->module_config, &mod_parrot);

    if(!route && spec) 
        route = app_route(req, spec);
    if(!route && conf) 
        route = suffix_route(req, conf);
    if(!route)
        route = mime_route(req, conf);
    return route;
}

