
#ifndef _VM_INTREFACE_H
#define _VM_INTREFACE_H

#include "vm_class.h"
#include "string_pool.h"


#define new_vm_object(T)            ((T *)gc_new_vm_object( OBJ_INFO(T) | sizeof(T)<<SIZE_SHIFT  ) )

#define new_special_vm_object(T, obj_size)  ((T *)gc_new_vm_object( OBJ_INFO(T) | (obj_size)<<SIZE_SHIFT ) )

#define new_vm_array(T, arr_len)    ((T *)gc_new_vm_array( OBJ_INFO(T) | (arr_len)<<SIZE_SHIFT ) )

#define new_dynamic_object(header)  ((korp_object *)gc_new_vm_object( header ));
#define build_vm_obj_info( ref_type, \
                           is_array, \
                           refnum_arrtype, \
                           arrlen_or_unheaded_objsize )   BUILD_VM_OBJ_HEADER( ref_type, is_array, refnum_arrtype, arrlen_or_unheaded_objsize )

#define get_vm_array_length(kobj)    GET_VM_ARRAY_LENGTH(kobj)
#define get_string_len(kstring)      (GET_VM_ARRAY_LENGTH(kstring)-1) /* exclude '\0' */

#define cp_get_tag(cp, i) \
 ((((korp_uint8_array *)((cp)->value[0]))->value[i]) & TAG_MASK)

#define	cp_is_resolved(cp,i)	\
 ((((korp_uint8_array *)((cp)->value[0]))->value[i]) & RESOLVED_MASK)

#define cp_set_resolved(cp,i)	\
do { (((korp_uint8_array *)((cp)->value[0]))->value[i]) |= RESOLVED_MASK; }while(0)

#define member_is_public(M)     ((M)->modifier & MOD_PUBLIC)
#define member_is_protected(M)  ((M)->modifier & MOD_PROTECTED)
#define member_is_private(M)    ((M)->modifier & MOD_PRIVATE)
#define member_is_default(M)    (!((M)->modifier &(MOD_PUBLIC|MOD_PROTECTED|MOD_PRIVATE)))

korp_java_type cp_get_const_type(korp_ref_array* cp, uint16 index);
korp_object* cp_get_const_value(korp_ref_array* cp, uint16 index);

korp_java_type cp_get_field_type(korp_ref_array* cpool, uint16 index);
korp_string* cp_get_method_descriptor(korp_ref_array* cpool, uint16 index);

korp_class* class_resolve_class(korp_class*, uint16, korp_loader_exception*);
korp_method* class_resolve_method(korp_class*, uint16, korp_loader_exception*);
korp_field* class_resolve_field(korp_class*, uint16, korp_loader_exception*);

void class_initialize(korp_class*);

#endif /* #ifndef _VM_INTREFACE_H */
