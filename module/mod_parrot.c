#include "mod_parrot.h"

module mod_parrot;

/**
 * Lets try to make some sense of this.
 * First, find if I can handle it via mod_parrot_router.
 * If so, aquire an interpreter (pertaining to the server).
 * Run the script
 * Release the interpreter
 * Return a result
 **/
static apr_status_t mod_parrot_handler(request_rec *req) {
    mod_parrot_route * route = mod_parrot_find_route(req);
    if(route) {
        Parrot_PMC interp_pmc = mod_parrot_acquire_interpreter(req->server);
        apr_status_t code = mod_parrot_run(interp_pmc, req, route); 
        mod_parrot_release_interpreter(req->server, interp_pmc);
        return code;
    } 
    return DECLINED;
}



/* server configuration */
static void * mod_parrot_create_config(apr_pool_t * pool, server_rec * server) {
    mod_parrot_conf * conf = apr_pcalloc(pool, sizeof(mod_parrot_conf));
    if (conf) {
        conf->loaderPath = INSTALLDIR;
        conf->loader = DEFAULT_LOADER;
        conf->languages = apr_table_make(pool, 5);
    } 
    return conf;
}

static void * mod_parrot_merge_config(apr_pool_t * pool, void * base, void * add) {
    return NULL;
}

/* directory configuration */
static void * mod_parrot_create_spec(apr_pool_t * pool, char * context) {
    /* ignore context! mu ha */
    mod_parrot_spec * spec = apr_pcalloc(pool, sizeof(mod_parrot_conf));
    return spec;
}

static void * mod_parrot_merge_spec(apr_pool_t * pool, void * base, void * add) {
    mod_parrot_spec * parent = base;
    mod_parrot_spec * child = add;
    mod_parrot_spec * merged = mod_parrot_create_spec(pool, "merged spec");
    if(parent->application)
        merged->application = parent->application;
    if(child->application) 
        merged->application = child->application;
    return merged;
}

static const char * mod_parrot_set_application(cmd_parms *cmd,  void * conf, const char * arg) {
    mod_parrot_spec * spec = conf;
    spec->application = mod_parrot_parse_route(cmd->pool, arg);
    if(!spec->application) {
        return apr_pstrcat(cmd->pool, "Could not parse route ", arg, NULL);
    }
    return NULL;
}

/* i really really wan to push an array of paths here, but
 * unfortunately, that doesn't seem to work. I'll have to
 * investigate just why */
static const char * mod_parrot_set_loader_path(cmd_parms *cmd, void * dummy, const char * arg) {
    mod_parrot_conf * conf;
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    conf->loaderPath = arg;
    return NULL;
}

static const char * mod_parrot_set_loader(cmd_parms *cmd, void * dummy, 
                                          const char * arg) {
    mod_parrot_conf * conf = ap_get_module_config(cmd->server->module_config, 
                                                  &mod_parrot);
    /* if thisisnot a server configuration directive this is wrong */
    apr_pool_t * pool = cmd->server->process->pool;
    if(conf) {
        char * dup = apr_pstrdup(pool, arg);
        int dot = ap_rind(dup, '.');
        ap_str_tolower(dup);
        if(dot > 0) /* if we have a suffix its a complete file */
            conf->loader = dup;
        else /* otherwise add the standard bytecode lines */
            conf->loader = apr_pstrcat(pool, dup, ".pbc", NULL);
    }
    return NULL;
}

/* Add a given language with a suffix to the table */
static const char * mod_parrot_add_language(cmd_parms *cmd, void * dummy, 
                                            const char * compiler, 
                                            const char * suffix) {
    mod_parrot_conf * conf;
    /* If conf is null we crash. This is the correct behavior */
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    apr_table_set(conf->languages, suffix, compiler);
    return NULL;
}

static void mod_parrot_register_hooks(apr_pool_t *p) {
	ap_hook_handler(mod_parrot_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Todo: add ParrotError declaration */
static command_rec mod_parrot_directives[] = {
    AP_INIT_TAKE1("ParrotLoaderPath", mod_parrot_set_loader_path, NULL, RSRC_CONF, "Set the path for loader bytecodes"),
    AP_INIT_TAKE1("ParrotLoader", mod_parrot_set_loader, NULL, RSRC_CONF, "Set the loader bytecode for parrot (including extension)"),
    AP_INIT_ITERATE2("ParrotLanguage", mod_parrot_add_language, NULL, RSRC_CONF, "Register a language for parrot to use"),
    AP_INIT_TAKE1("ParrotApplication", mod_parrot_set_application, NULL, ACCESS_CONF, "Specify the application for PSGI"),
    { NULL },
};

module mod_parrot = {
	STANDARD20_MODULE_STUFF,
	mod_parrot_create_spec,
	mod_parrot_merge_spec,
	mod_parrot_create_config,
	mod_parrot_merge_config,
	mod_parrot_directives, 
	mod_parrot_register_hooks
};


