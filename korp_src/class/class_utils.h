
#ifndef _CLASS_UTILS_H
#define _CLASS_UTILS_H

#include "korp_types.h"
#include "vm_object.h"
#include "platform.h"

bool field_initialize(korp_class* , korp_field* );

korp_class *array_get_base_class(korp_class_loader*, korp_class *);
uint16 array_name_get_dimension(const char *);
bool class_name_is_array(const char*);
korp_string* class_create_package_name(korp_class*);

#define class_is_primitive(kclass) ((kclass)->flags.is_primitive)
korp_class* java_object_get_class( korp_object *);
bool is_subclass(korp_class *, korp_class *);

korp_field* class_lookup_field_recursive(korp_class*, korp_string*, korp_string*);
korp_field* class_lookup_field_own(korp_class*, korp_string*, korp_string*);

korp_method* class_lookup_method_recursive(korp_class*, korp_string* , korp_string*);
korp_method* class_lookup_method_own(korp_class* , korp_string* , korp_string*);

uint32 __stdcall class_get_interface_method_offset( korp_class* kclass, korp_method* km_intf ) stdcall__ ;

#endif /* #ifndef _CLASS_UTILS_H */
