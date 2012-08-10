#include "mod_parrot.h"

typedef struct {
    Parrot_PMC root;
    apr_array_header_t queue;
    apr_thread_mutex_t mutex;
} mod_parrot_pool;

mod_parrot_pool * new_pool(apr_pool_t * mem) {
    mod_parrot_pool pool = apr_palloc(mem, sizeof(mod_parrot_pool));
    pool->root = new_interpreter();
    if(mod_parrot_is_threaded_mpm()) {
        pool->queue = apr_array_make(mem, 0, sizeof(Parrot_PMC));
        apr_thread_mutex_create(&(pool->mutex), APR_THREAD_MUTEX_UNNESTED, mem);
    }
    return pool;
}

Parrot_PMC new_interpreter() {
    Parrot_PMC interp_pmc, hash_pmc;
    Parrot_api_make_interpreter(NULL, 0, NULL, &interp_pmc);    
    /* this is to help parrot set up the right paths by itself,
     * and yes, i do agree this is a bit of unneccesesary magic.
     * Parrots, it appears, are magical birds after all. */
	hash_pmc = mod_parrot_hash_new(interp_pmc);
	mod_parrot_hash_put(interp_pmc, hash_pmc, "build_dir", BUILDDIR);
	mod_parrot_hash_put(interp_pmc, hash_pmc, "versiondir", VERSIONDIR);
	mod_parrot_hash_put(interp_pmc, hash_pmc, "libdir", LIBDIR);
	Parrot_api_set_configuration_hash(interp_pmc, hash_pmc);
    /* no pir without these calls ;-) */
	imcc_get_pir_compreg_api(interp_pmc, 1, &pir);
	imcc_get_pasm_compreg_api(interp_pmc, 1, &pasm);
    return interp_pmc;
}


Parrot_PMC child_interpreter(Parrot_PMC parent_interp_pmc) {
    

}


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

/**
 * Strategy:
 * lock the pool
 * if the queue is empty, unlock, create a new interpreter
 * else, pop one from the queue, unlock
 **/
Parrot_PMC get_interpreter(mod_parrot_pool * pool) {
    if(mod_parrot_mpm_is_threaded()) {
        Parrot_PMC child;
        apr_thread_mutex_lock(pool->mutex);
        if(apr_is_empty_array(pool->queue)) {
            apr_thread_mutex_unlock(pool->mutex);
            // it is important that this does not crash
            child = child_interpreter(pool->root); 
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
void release_interpreter(mod_parrot_pool * pool, Parrot_PMC interp_pmc) {
    if(mod_parrot_mpm_is_threaded()) {
        apr_thread_mutex_lock(pool->mutex);
        {
            Parrot_PMC * storage = apr_array_push(pool->queue);
            *storage = interp_pmc;
        }
        apr_thread_mutex_unlock(pool->mutex);
    } 
}
