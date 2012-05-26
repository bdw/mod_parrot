#include "mod_parrot.h"

static int mod_parrot_handler(request_rec *req) {
	Parrot_PMC interp;
	if(Parrot_api_make_interpreter(NULL, 0, NULL, &interp)) {
		mod_parrot_run(interp, req);
		Parrot_api_destroy_interpreter(interp);
		return OK;
	}
	return DECLINED;
}


static void mod_parrot_register_hooks(apr_pool_t *p) {
	ap_hook_handler(mod_parrot_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA mod_parrot = {
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, 
	mod_parrot_register_hooks
};
