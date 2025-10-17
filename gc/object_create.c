
/* this is temporary for test */
#include <stdlib.h>
#include <string.h>

#include "class_loader.h"
#include "vm_for_gc.h"
#include "utils.h"

static char* mem;
void gc_init()
{
    //uint32 heap_size = 1024*1024*32;
	uint32 heap_size = 1024*256;
    mem = (char *)malloc( heap_size);
    memset(mem, 0, heap_size);
}

korp_java_array *gc_new_java_array(korp_class* kc_array, uint16 len)
{   
    uint32 size;
    korp_java_array* jarr = (korp_java_array *)mem;
    korp_java_type type = (korp_java_type)(string_to_chars(kc_array->name))[1];
    size = kc_array->instance_size + len* typesize_type(type);
    mem += size;
    memset(jarr, 0, size );
    jarr->header = (obj_info)kc_array|IS_JAVA_OBJ_MASK;
    jarr->length = len;
    return jarr;
}

korp_object *gc_new_java_object(korp_class* kclass )
{   
    uint32 size = kclass->instance_size;
    korp_object* jobj = (korp_object *)mem;
    mem += size;
    memset(jobj, 0, size );
    jobj->header = (obj_info)kclass|IS_JAVA_OBJ_MASK;
    return jobj;
}

korp_object *gc_new_vm_object(obj_info header)
{
    uint32 size = get_vm_object_size(header);
    korp_object* obj = (korp_object *)((uint32)(mem + 3)/4*4); /* 4 bytes aligned */
    mem = (char*)obj + size;
    memset(obj, 0, size );
    obj->header = header;
    return obj;
}

korp_object *gc_new_vm_array(obj_info header)
{
    return gc_new_vm_object(header);
}

