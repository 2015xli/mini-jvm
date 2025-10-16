
#ifndef _GC_FOR_VM_H
#define _GC_FOR_VM_H

#include <stdlib.h>
#include <memory.h>

#ifdef WIN32
#include <malloc.h>
#endif

#include "korp_types.h"
korp_object *gc_new_java_object(korp_class*);
korp_java_array *gc_new_java_array(korp_class*, uint16);
korp_object *gc_new_vm_object(obj_info);
korp_object *gc_new_vm_array(obj_info);
void free_vm_object(korp_object*);
void free_java_object(korp_object*);


#define stack_alloc(size)  ((char*)memset(_alloca(size), 0, size) )

void gc_init(void);


#endif //#ifndef _GC_FOR_VM_H
