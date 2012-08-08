#include "parrot/parrot.h"
#include "mod_parrot.h"

int mod_parrot_mpm_is_threaded() {
    int is_threaded;
    ap_mpm_query(AP_MPMQ_IS_THREADED, &is_threaded);
    return is_threaded;
}

Parrot_PMC mod_parrot_hash_new(Parrot_PMC interp_pmc) {
    Parrot_Interp interp = Parrot_interp_get_from_pmc(interp_pmc);
    Parrot_PMC hash_pmc = Parrot_pmc_new(interp, enum_class_Hash);
    Hash * hash = VTABLE_get_pointer(interp, hash_pmc);
    /* set the correct key type */
    hash->key_type = Hash_key_type_STRING;
    hash->entry_type = enum_type_STRING;
    return hash_pmc;
} 

void mod_parrot_eval(Parrot_PMC interp_pmc, Parrot_PMC bytecode_pmc, 
                     Parrot_PMC argument_pmc) {
    if(!Parrot_api_run_bytecode(interp_pmc, bytecode_pmc, argument_pmc)) {
        Parrot_PMC exception_pmc;
        Parrot_Int is_error, exit_code;
        Parrot_String error_message;
        Parrot_Interp interp = Parrot_interp_get_from_pmc(interp_pmc);
        Parrot_api_get_result(interp_pmc, &is_error, &exception_pmc,
                              &exit_code, &error_message);
        if(interp->parent_interpreter) {
            interp = interp->parent_interpreter;
        }
        exception_pmc = Parrot_ex_build_exception(interp, EXCEPT_error, 
                                                  exit_code, error_message);
        Parrot_ex_throw_from_c(interp, exception_pmc);
    }
}
                                   
/* this is not much less verbose than the original version but whatever */
void mod_parrot_hash_put(Parrot_PMC interp_pmc, Parrot_PMC hash_pmc, 
                         char * key_cstr, char * val_cstr) {
    Parrot_Interp interp = Parrot_interp_get_from_pmc(interp_pmc);
    Hash * hash = VTABLE_get_pointer(interp, hash_pmc);
    STRING * key = Parrot_str_new(interp, key_cstr, 0);
    STRING * val = Parrot_str_new(interp, val_cstr, 0);
    Parrot_hash_put(interp, hash, key, val);
}

char * mod_parrot_export_cstring(Parrot_PMC interp_pmc, Parrot_PMC export_pmc) { 
    Parrot_Interp interp = Parrot_interp_get_from_pmc(interp_pmc);
    STRING * export = VTABLE_get_string(interp, export_pmc);
    return Parrot_str_to_cstring(interp, export);
}

void mod_parrot_free_cstring(Parrot_PMC interp_pmc, char * cstring) {
    Parrot_str_free_cstring(cstring);
}

                                                                              
