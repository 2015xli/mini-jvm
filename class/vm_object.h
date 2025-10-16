
#ifndef _VM_OBJECT_H
#define _VM_OBJECT_H

#include "vm_class.h"
typedef struct _korp_java_array{
    obj_info header;
    uint32 length;
    uint16 value[1];
}korp_java_array;

typedef struct _korp_4uint8{
    obj_info header;
    uint8  value[4];
}korp_4uint8;
#define korp_4uint8_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 0, NOT_RECYCLABLE)

typedef struct _korp_2uint16{
    obj_info header;
    uint16  value[2];
}korp_2uint16;
#define korp_2uint16_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 0, NOT_RECYCLABLE)

typedef struct _korp_uint32{
    obj_info header;
    uint32  value;
}korp_uint32;
#define korp_uint32_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 0, NOT_RECYCLABLE)

typedef struct _korp_2uint32{
    obj_info header;
    uint32  value[2];
}korp_2uint32;
#define korp_2uint32_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 0, NOT_RECYCLABLE)

typedef struct _korp_2ref{
    obj_info header;
    ref  value[2];
}korp_2ref;
#define korp_ref_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 2, NOT_RECYCLABLE)

typedef struct _korp_uint64{
    obj_info header;
    uint64  value;
}korp_uint64;
#define korp_uint64_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 0, NOT_RECYCLABLE)

typedef struct _korp_ref{
    obj_info header;
    ref value;
}korp_ref;
#define korp_2ref_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 1, NOT_RECYCLABLE)


typedef struct _korp_uint8_array{
    obj_info header;
    ref info;
    uint8  value[1];
}korp_uint8_array;
#define korp_uint8_array_ARR_GCINFO_RECYC BUILD_TYPE_INFO(YES_ARRAY, ARRAY_UINT8, NOT_RECYCLABLE)

typedef struct _korp_uint16_array{
    obj_info header;
    ref info;
    uint16  value[1];
}korp_uint16_array;
#define korp_uint16_array_ARR_GCINFO_RECYC BUILD_TYPE_INFO(YES_ARRAY, ARRAY_UINT16, NOT_RECYCLABLE)

typedef struct _korp_int16_array{
    obj_info header;
    ref info;
    int16  value[1];
}korp_int16_array;
#define korp_int16_array_ARR_GCINFO_RECYC BUILD_TYPE_INFO(YES_ARRAY, ARRAY_UINT16, NOT_RECYCLABLE)

typedef struct _korp_uint32_array{
    obj_info header;
    ref info;
    uint32  value[1];
}korp_uint32_array;
#define korp_uint32_array_ARR_GCINFO_RECYC BUILD_TYPE_INFO(YES_ARRAY, ARRAY_UINT32, NOT_RECYCLABLE)

typedef struct _korp_int32_array{
    obj_info header;
    ref info;
    int32  value[1];
}korp_int32_array;
#define korp_int32_array_ARR_GCINFO_RECYC BUILD_TYPE_INFO(YES_ARRAY, ARRAY_UINT32, NOT_RECYCLABLE)

typedef struct _korp_uint64_array{
    obj_info header;
    ref info;
    uint64  value[1];
}korp_uint64_array;
#define korp_uint64_array_ARR_GCINFO_RECYC BUILD_TYPE_INFO(YES_ARRAY, ARRAY_UINT64, NOT_RECYCLABLE)

typedef struct _korp_ref_array{
    obj_info header;
    ref info;
    ref value[1];
}korp_ref_array;
#define korp_ref_array_ARR_GCINFO_RECYC BUILD_TYPE_INFO(YES_ARRAY, ARRAY_REF, NOT_RECYCLABLE)

/* reflective info of an array */
typedef struct _korp_array_info{
    obj_info header;            /* object header (I) */
    korp_class *array_element_class;    /* 1 */
    korp_class *array_base_class;       /* 2 */

    uint16 is_primitive_array;  
    uint16 num_dimension;     

}korp_array_info;
#define korp_array_info_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 2, YES_RECYCLABLE)

/* not used actually, use korp_string directly for package */
typedef struct _korp_package{
    obj_info header;
    korp_string* name;
}korp_package;

/* innerclass */
typedef struct _korp_innerclass{
    obj_info header;            /* object header (I) */
    korp_string *inner_classname;    /* 1 */
    korp_string *outer_classname;    /* 2 */
    korp_string *inner_name;         /* 3 */
    uint8       modifier;            
    uint8       bogus[3];
}korp_innerclass;
#define korp_innerclass_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 3, NOT_RECYCLABLE)

/* reflective info of a class field */
typedef struct _korp_field{
    obj_info header;            /* object header (I) */
    korp_string *name;          /* 1 */    
    korp_string *descriptor;    /* 2 */
    korp_class *owner_class;    /* 3 */
    korp_object *constval;      /* 4 */

    uint16 offset;    
    uint16 modifier;

}korp_field;
#define korp_field_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 4, YES_RECYCLABLE)

/*
 * hopefully we can pack all refs in less than 16, 
 * if there are no less refs than 16, we can put the
 * infrequently used refs into a seperate struct.
 * the decision of which refs put alone depends on 
 * application behaviour.
 *
 */

/*
 * in kORP, there are some field overloadings. E.g.
 * super_class field is overloaded with super_name,
 * interface is also overloader with either class or name.
 * In any case, the defined type for the field is the 
 * (least) common super type of all overloading usages.
 * 
 */

/* reflective info of a class */
typedef struct _korp_class{
    obj_info header;            /* object header (I) */

/* put all ref fields at the beginning */
    /* class name */
    korp_string *name;                              /* 1 */
    korp_package *package;                          /* 2 */
    korp_class_loader *class_loader;                /* 3 */
    korp_array_info *array_info;                    /* 4 */

    korp_ref_array *field_info;                     /* 5 */
    korp_ref_array *interface_info;                 /* 6 */
    korp_ref_array *method_info;                    /* 7 */

    korp_object*   static_data;                      /* 16 */
/*    korp_ref_array *static_vtable; */                

    korp_uint32_array *vtable;                         /* 8 */
    korp_ref_array *vtable_info;

    korp_ref_array *interface_table;                   /* 9 */
    korp_ref_array *interface_table_info;                   /* 9 */

    union{                                          /* 10 */
        korp_class *super_class; 
        korp_string *super_name;
    }; 
    korp_string *classfile_name;                    /* 11 */
    korp_string *sourcefile_name;                   /* 12 */
    korp_ref_array *constant_pool;                  /* 14 */
    korp_object*  java_object;                       /* 15 */
	bool  initilizing_flag; //initilizing_thread;  //korp_thread*  initilizing_thread;
    korp_object*  exception;
    korp_method*  clinit;
/* without Kind, we aggregate ref fields at the beginning. 
   the following non-ref fields are aligned to word.
 */
    /* no header, no padding */
    uint16 instance_size;   /* (S) */
    uint8 gc_info;                   /* (C) */

    uint16 modifier;          /* class modifiers (C) */
    uint8 state;             /* class_state (C) */
    struct {
        unsigned is_array:1;          /* if class is array(C) */
        unsigned is_primitive:1;
        unsigned has_finalizer:1;
        unsigned has_subclass:1;
    }flags;

}korp_class;
#define korp_class_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 20, YES_RECYCLABLE)

#define CLASS_INTERFACE_NUM(C)  (GET_VM_ARRAY_LENGTH(((korp_class *)C)->interface_info))
#define CLASS_FIELD_NUM(C)      (GET_VM_ARRAY_LENGTH(((korp_class *)C)->field_info))
#define CLASS_METHOD_NUM(C)     (GET_VM_ARRAY_LENGTH(((korp_class *)C)->method_info))


/* class interfaces */
#define class_is_public(clss)           ((clss)->modifier & MOD_PUBLIC)
#define class_is_final(clss)            ((clss)->modifier & MOD_FINAL)
#define class_is_super(clss)            ((clss)->modifier & MOD_SUPER)
#define class_is_interface(clss)        ((clss)->modifier & MOD_INTERFACE)
#define class_is_abstract(clss)         ((clss)->modifier & MOD_ABSTRACT)
#define class_get_field_num(clss)       CLASS_FIELD_NUM(clss)
#define class_get_interface_num(clss)   CLASS_INTERFACE_NUM(clss)
#define class_get_method_num(clss)      CLASS_METHOD_NUM(clss)

#define field_is_static(F)      ((F)->modifier & MOD_STATIC )
#define field_is_public(F)      ((F)->modifier & MOD_PUBLIC)
#define field_is_protected(F)   ((F)->modifier & MOD_PROTECTED)
#define field_is_private(F)     ((F)->modifier & MOD_PRIVATE)
#define field_is_default(F)     (!((F)->modifier &(MOD_PUBLIC|MOD_PROTECTED|MOD_PRIVATE)))

/* Kind is not used currently 

typedef struct _korp_kind{
    obj_info header;           
    ref info;
    korp_gc_map gc_map;        
    uint32 instance_size;      
    korp_string *name; 
}
*/

#endif /* #ifndef _VM_OBJECT_H */
