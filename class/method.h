
#ifndef _METHOD_H
#define _METHOD_H

#include "korp_types.h"
#include "class_loader.h"

/* method states during compilation */
enum korp_method_state {
	MS_NotCompiled          = 0x01,	/* initial state */
	MS_BeingCompiled        = 0x02, /* being compiled */
	MS_Compiled             = 0x04,	/* successfully finish compilation */
    MS_Error                = 0x08, /* bad method or compilation failed */
};

/* reflective info of a class method */
typedef struct _korp_method{
    obj_info header;            /* object header (I) */
    korp_string *name;          /* 1 */    
    korp_string *descriptor;    /* 2 */
    korp_class *owner_class;    /* 3 */
    korp_char_array *byte_code;      /* 4 */
    korp_ref_array *handler_info; /* 5 */
    korp_ref_array *linenum_table; /* 6 */
    korp_ref_array *localvar_table; /* 6 */
    korp_ref_array *exception_table; /* 5 */

    korp_uint8_array *jitted_code;

    uint16 offset;    
    uint16 modifier;
    uint16 max_stack;
    uint16 max_locals;
    uint8 state;

    struct {
        unsigned is_init        : 1;
        unsigned is_clinit      : 1;
        unsigned is_finalize    : 1;	
        unsigned is_overridden  : 1;	
        unsigned is_nop         : 1;
	} flags;

}korp_method;
#define korp_method_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 9, YES_RECYCLABLE)

typedef struct _korp_handler{
    obj_info header;
    uint16 start_pc;
    uint16 end_pc;
    uint16 handler_pc;
	uint16 class_type_index;
    
}korp_handler;
#define korp_handler_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 1, YES_RECYCLABLE)

typedef struct _korp_linenum{
    obj_info header;
    uint16 start_pc;
    uint16 line_num;
    
}korp_linenum;
#define korp_linenum_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 0, YES_RECYCLABLE)

typedef struct _korp_localvar{
    obj_info header;
    korp_string* name;
    korp_string* descriptor;
    uint16 start_pc;
    uint16 length;
    uint16 index;

}korp_localvar;
#define korp_localvar_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 2, YES_RECYCLABLE)

#define METHOD_BYTE_CODE_LENGTH(M)  GET_VM_ARRAY_LENGTH((M)->byte_code)
#define METHOD_HANDLER_NUM(M)       GET_VM_ARRAY_LENGTH((M)->handler_info)
#define METHOD_EXCEPTION_NUM(M)     GET_VM_ARRAY_LENGTH((M)->exception_table)
#define METHOD_LINENUM_TABLE_LENGTH(M)     GET_VM_ARRAY_LENGTH((M)->linenum_table)
#define method_is_static(M)     ((M)->modifier & MOD_STATIC)
#define method_is_public(M)     ((M)->modifier & MOD_PUBLIC)
#define method_is_protected(M)  ((M)->modifier & MOD_PROTECTED)
#define method_is_private(M)    ((M)->modifier & MOD_PRIVATE)
#define method_is_native(M)     ((M)->modifier & MOD_NATIVE)
#define method_is_synchronized(M)     ((M)->modifier & MOD_SYNCHRONIZED)
#define method_is_default(M)    (!((M)->modifier &(MOD_PUBLIC|MOD_PROTECTED|MOD_PRIVATE)))

#define method_set_overridden(M) ((M)->flags.is_overridden = 1)
#define method_is_overridden(M) ((M)->flags.is_overridden == 1)

#define method_equal_signature(M1, M2) (( (M1)->name == (M2)->name) && ( (M1)->descriptor == (M2)->descriptor))
#define method_get_slot_index(M)    (uint16)(((M)->offset - VM_ARRAY_OVERHEAD - sizeof(korp_class))>>2 ) /* BYTES_OF_REF=4 */

uint8* method_get_bincode_entry( korp_method*);
uint8* method_get_bytecode_entry( korp_method*);

bool method_get_param_num_size(korp_method*, uint32* num, uint32* size);
bool method_descriptor_get_param_info(korp_string* kdesc, uint32* n_words, korp_java_type* ret_type);


#define method_get_descriptor(M) ((M)->descriptor)
#define method_get_name(M)       ((M)->name)

korp_handler* method_get_handler(korp_method*, uint16 handler_id);

bool method_initialize(korp_class*, korp_method*);

#endif /* #ifndef _METHOD_H */

