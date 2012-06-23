#include "mod_parrot.h"
#define HASH_CLASS_NAME "Hash"


Parrot_PMC mod_parrot_new_hash(Parrot_PMC interp) {
    Parrot_PMC classKey, classObj, hashObj;
    Parrot_String hashName;
    Parrot_api_string_import_ascii(interp, HASH_CLASS_NAME, &hashName);
    Parrot_api_pmc_box_string(interp, hashName, &classKey);
    Parrot_api_pmc_get_class(interp, classKey, &classObj);
    if(Parrot_api_pmc_new_from_class(interp, classObj, NULL, &hashObj))
        return hashObj;
    return NULL;
}

void mod_parrot_hash_set(Parrot_PMC interp, Parrot_PMC hash, char * key, char * value) {
    Parrot_String keyString, valueString;
    Parrot_PMC valueObj;
    Parrot_api_string_import_ascii(interp, key, &keyString);
    Parrot_api_string_import_ascii(interp, value, &valueString);
    Parrot_api_pmc_box_string(interp, valueString, &valueObj);
    Parrot_api_pmc_set_keyed_string(interp, hash, keyString, valueObj);
}
