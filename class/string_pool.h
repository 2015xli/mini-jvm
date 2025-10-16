
#ifndef _STRING_POOL_H
#define _STRING_POOL_H

#include "korp_types.h"
#include "vm_object.h"

/* interned representation of string */
typedef struct _korp_string{
    obj_info header;               /* object header (I) */
    korp_object* intern;
    uint8 byte[1]; 
}korp_string;

/* global string pool */
typedef struct _korp_string_pool{
    obj_info  header;
    korp_object* info;
    korp_entry *bucket[1];
}korp_string_pool;

korp_string *string_pool_lookup(korp_string_pool*, const char*);
void string_pool_remove(korp_string_pool*, const korp_string*);
korp_string_pool* new_string_pool(void);

#endif /* #ifndef _STRING_POOL_H */
