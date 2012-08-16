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


static void * mod_parrot_create_config(apr_pool_t * pool, server_rec * server) {
    mod_parrot_conf * cfg = apr_pcalloc(pool, sizeof(mod_parrot_conf));
    if (cfg) {
        cfg->loaderPath = INSTALLDIR;
        cfg->loader = DEFAULT_LOADER;
        cfg->languages = apr_table_make(pool, 5);
    } 
    return cfg;
}

/* i really really wan to push an array of paths here, but
 * unfortunately, that doesn't seem to work. I'll have to
 * investigate just why */
static const char * mod_parrot_set_loader_path(cmd_parms *cmd, void * dummy, const char * arg) {
    mod_parrot_conf * conf;
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    if(conf) { 
        conf->loaderPath = arg;
    } /* we should check if the loaderpath is really a directory here */
    return NULL;
}

static const char * mod_parrot_set_loader(cmd_parms *cmd, void * dummy, 
                                          const char * arg) {
    mod_parrot_conf * conf = ap_get_module_config(cmd->server->module_config, 
                                                  &mod_parrot);
    /* if thisisnot a server configuration directive this is wrong */
    apr_pool_t pool = cmd->server->process->pool;
    if(conf) {
        int dot = ap_rind(arg, '.');
        if(dot > 0) /* if we have a suffix its a complete file */
            conf->loader = arg;
        else /* otherwise add the standard bytecode lines */
            conf->loader = apr_pstrcat(pool, arg, ".pbc", NULL);
    }
    return NULL;
}

/* Add a given language with a suffix to the table */
static const char * mod_parrot_add_language(cmd_parms *cmd, void * dummy, 
                                            const char * compiler, 
                                            const char * suffix) {
    mod_parrot_conf * conf;
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    if(conf) {
        apr_table_set(conf->languages, suffix, compiler);
    } else {
        return "Something is wrong in initialization";
    }
    return NULL;
}

static void mod_parrot_register_hooks(apr_pool_t *p) {
	ap_hook_handler(mod_parrot_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

/* Todo: add ParrotApplication and ParrotError directives */
static command_rec mod_parrot_directives[] = {
    AP_INIT_TAKE1("ParrotLoaderPath", mod_parrot_set_loader_path, NULL, RSRC_CONF, "Set the path for loader bytecodes"),
    AP_INIT_TAKE1("ParrotLoader", mod_parrot_set_loader, NULL, RSRC_CONF, "Set the loader bytecode for parrot (including extension)"),
    AP_INIT_ITERATE2("ParrotLanguage", mod_parrot_add_language, NULL, RSRC_CONF, "Register a language for parrot to use"),
    { NULL },
};

module mod_parrot = {
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	mod_parrot_create_config,
	NULL,
	mod_parrot_directives, 
	mod_parrot_register_hooks
};


