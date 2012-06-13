#include "mod_parrot.h"

module mod_parrot;

static int mod_parrot_handler(request_rec *req) {
	Parrot_PMC interp;
    /* TODO: do something more intelligent, i.e. figure out which
     * request I want to handle compared to those I don't */
	mod_parrot_interpreter(&interp);
	mod_parrot_run(interp, req);
	Parrot_api_destroy_interpreter(interp); 
	return OK;
}


static void * mod_parrot_create_config(apr_pool_t * pool, server_rec * server) {
    mod_parrot_conf * cfg = apr_pcalloc(pool, sizeof(mod_parrot_conf));
    if (cfg) {
        cfg->loaderPath = INSTALLDIR;
    } 
    return cfg;
}

static const char * mod_parrot_set_loader_path(cmd_parms *cmd, void * dummy, const char * arg) {
    mod_parrot_conf * conf;
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    if(conf) {
        conf->loaderPath = arg;
    } 
    return NULL;
}

static void mod_parrot_register_hooks(apr_pool_t *p) {
	ap_hook_handler(mod_parrot_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

static command_rec mod_parrot_directives[] = {
    AP_INIT_TAKE1("ParrotLoaderPath", mod_parrot_set_loader_path, NULL, RSRC_CONF, "Set the path for loader bytecodes"),
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


