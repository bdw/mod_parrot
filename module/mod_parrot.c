#include "mod_parrot.h"

module mod_parrot;

static int mod_parrot_handler(request_rec *req) {
    mod_parrot_conf * conf = ap_get_module_config(req->server->module_config, &mod_parrot);
    char * fullName = apr_pstrdup(req->pool, req->filename);
    char * baseName = basename(fullName);
    const char * compiler = NULL;
    int code, idx = ap_rind(baseName, '.');
    if(idx > 0) {
        compiler = apr_table_get(conf->languages, baseName + idx);
    } 
    if(compiler) {
        Parrot_PMC interp = mod_parrot_interpreter(conf);
        code = mod_parrot_run(interp, req); // result code
        Parrot_api_destroy_interpreter(interp); 
        return code;
    } 
    return DECLINED;
}


static void * mod_parrot_create_config(apr_pool_t * pool, server_rec * server) {
    mod_parrot_conf * cfg = apr_pcalloc(pool, sizeof(mod_parrot_conf));
    if (cfg) {
        cfg->loaderPath = INSTALLDIR;
        cfg->loader = "mod_parrot.pbc";
        cfg->languages = apr_table_make(pool, 5);
    } 
    return cfg;
}

static const char * mod_parrot_set_loader_path(cmd_parms *cmd, void * dummy, const char * arg) {
    mod_parrot_conf * conf;
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    if(conf) {
        conf->loaderPath = arg;
    } /* we should check if the loaderpath is really a directory here */
    return NULL;
}

static const char * mod_parrot_set_loader(cmd_parms *cmd, void * dummy, const char * arg) {
    mod_parrot_conf * conf;
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    if(conf) {
        conf->loader = arg;
    }
    return NULL;
}

static const char * mod_parrot_add_language(cmd_parms *cmd, void * dummy, const char * compiler, const char * suffix) {
    mod_parrot_conf * conf;
    conf = ap_get_module_config(cmd->server->module_config, &mod_parrot);
    if(conf) {
        apr_table_set(conf->languages, suffix, compiler);
    }
    return NULL;
}

static void mod_parrot_register_hooks(apr_pool_t *p) {
	ap_hook_handler(mod_parrot_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

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


