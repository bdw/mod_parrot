#include "parrot/parrot.h"
#include "mod_parrot.h"

Parrot_PMC mod_parrot_hash_new(Parrot_PMC interp_pmc) {
    Parrot_Interp interp = Parrot_interp_get_from_pmc(interp_pmc);
    Parrot_PMC hash_pmc = Parrot_pmc_new(interp, enum_class_Hash);
    Hash * hash = VTABLE_get_pointer(interp, hash_pmc);
    /* set the correct key type */
    hash->key_type = Hash_key_type_STRING;
    hash->entry_type = enum_type_STRING;
    return hash_pmc;
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

                                                                              
