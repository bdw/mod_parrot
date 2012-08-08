#include "mod_parrot.h"
/**
 * Interface first!
 * construct a pool via mod_parrot_pool_construct()
 * destroy a pool via mod_parrot_pool_destroy()
 * 
 * get an interpreter from mod_parrot_interpreter_acquire()
 * return an interpreter from mod_parrot_interpreter_release()
 * 
 * internally, use a root and child interpreter. Apparantly having two
 * interpreters that are not parent and child don't work out all that well
 * after all.
 * 
 * The internal interface is
 * interp_new() for getting the root interpreter
 * interp_init() for initializing an interpreter
 * interp_child() for getting a child interpreter
 * interp_destroy() for destroying any interpreter
 **/
typedef struct {
    Parrot_PMC root;
    apr_array_header_t queue;
    apr_thread_mutex_t mutex;
} mod_parrot_pool;


mod_parrot_pool * mod_parrot_pool_(apr_pool_t * mem) {
    mod_parrot_pool pool = apr_palloc(mem, sizeof(mod_parrot_pool));
    pool->root = interp_new();
    if(mod_parrot_mpm_is_threaded()) {
        pool->queue = apr_array_make(mem, 0, sizeof(Parrot_PMC));
        apr_thread_mutex_create(&(pool->mutex), APR_THREAD_MUTEX_UNNESTED, mem);
    }
    return pool;
}

Parrot_PMC interp_new() {
    Parrot_PMC interp_pmc;
    Parrot_api_make_interpreter(NULL, 0, NULL, &interp_pmc);    
    interp_init(interp_pmc);
    return interp_pmc;
}

void interp_init(Parrot_PMC interp_pmc) {
	Parrot_PMC hash_pmc = mod_parrot_hash_new(interp_pmc);
    /* this is to help parrot set up the right paths by itself,
     * and yes, i do agree this is a bit of unneccesesary magic.
     * Parrots, it appears, are magical birds after all. */
    mod_parrot_hash_put(interp_pmc, hash_pmc, "build_dir", BUILDDIR);
	mod_parrot_hash_put(interp_pmc, hash_pmc, "versiondir", VERSIONDIR);
	mod_parrot_hash_put(interp_pmc, hash_pmc, "libdir", LIBDIR);
	Parrot_api_set_configuration_hash(interp_pmc, hash_pmc);
    /* no pir without these calls ;-) */
	imcc_get_pir_compreg_api(interp_pmc, 1, &pir);
	imcc_get_pasm_compreg_api(interp_pmc, 1, &pasm);
    
}

Parrot_PMC interp_child(Parrot_PMC parent_interp_pmc) {
    Parrot_PMC child_interp_pmc;
    Parrot_api_make_interpreter(parent_interp_pmc, 0, NULL, &child_interp_pmc);
    interp_init(child_interp_pmc);
    return child_interp_pmc;
}

Parrot_PMC interp_destroy(Parrot_PMC interp_pmc) {
    Parrot_api_destroy_interpreter(interp_pmc);
}

/**
 * Strategy:
 * lock the pool
 * if the queue is empty, unlock, create a new interpreter
 * else, pop one from the queue, unlock
 **/
Parrot_PMC mod_parrot_acquire_interpreter(mod_parrot_pool * pool) {
    if(mod_parrot_mpm_is_threaded()) {
        Parrot_PMC child;
        apr_thread_mutex_lock(pool->mutex);
        if(apr_is_empty_array(pool->queue)) {
            apr_thread_mutex_unlock(pool->mutex);
            child = interp_child(pool->root); 
        } else {
            child = *(apr_array_pop(pool->queue));
            apr_thread_mutex_unlock(pool->mutex);
        }
        return child;
    } else {
        return pool->root;
    }
}

/**
 * Strategy:
 * lock the pool
 * push the interpreter on the queue
 * unlock the pool
 **/
void mod_parrot_release_interpreter(mod_parrot_pool * pool, Parrot_PMC interp_pmc) {
    if(mod_parrot_mpm_is_threaded()) {
        apr_thread_mutex_lock(pool->mutex);
        {
            Parrot_PMC * storage = apr_array_push(pool->queue);
            *storage = interp_pmc;
        }
        apr_thread_mutex_unlock(pool->mutex);
    } 
}
