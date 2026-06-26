
#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include "korp_types.h"

typedef struct _korp_globals{
    obj_info    header;
    korp_string* java_lang_NullPointerException_string;
    korp_string* java_lang_ArrayStoreException_string;
    korp_string* java_lang_ArrayIndexOutOfBoundsException_string;
    korp_string* java_lang_Object_string;
    korp_string* java_lang_Class_string;
    korp_string* init_string;
    korp_string* clinit_string;
    korp_string* default_descriptor_string;
    korp_string* finalize_name_string;
    korp_string* finalize_descriptor_string;
    korp_string* main_name_string;
    korp_string* main_descriptor_string;
    
    korp_field* jlString_value_field;
    korp_field* jlString_offset_field;
    korp_field* jlString_count_field;
    korp_field* jlStringBuffer_value_field;
    korp_field* jlStringBuffer_count_field;

    korp_class* boolean_class;
    korp_class* char_class;
    korp_class* float_class;
    korp_class* double_class;
    korp_class* byte_class;
    korp_class* short_class;
    korp_class* int_class;
    korp_class* long_class;

    korp_class* void_class;
    
    korp_class* ArrayOfBoolean_class;
    korp_class* ArrayOfChar_class;
    korp_class* ArrayOfFloat_class;
    korp_class* ArrayOfDouble_class;
    korp_class* ArrayOfByte_class;
    korp_class* ArrayOfShort_class;
    korp_class* ArrayOfInt_class;
    korp_class* ArrayOfLong_class;
    
    korp_class* java_lang_Object_class;
    korp_class* java_lang_String_class;
    korp_class* java_lang_Class_class;
    korp_class* java_lang_StringBuffer_class;

    korp_class* java_lang_Throwable_class;
    korp_class* java_lang_Error_class;
    korp_class* java_lang_ExceptionInInitializerError_class;
    korp_class* java_lang_NullPointerException_class;
    korp_class* java_lang_ArrayIndexOutOfBoundsException_class;
    korp_class* java_lang_ArrayStoreException_class;
    korp_class* java_lang_ArithmeticException_class;
    korp_class* java_lang_ClassCastException_class;

    korp_class* java_io_Serializable_class;
    korp_class* java_lang_Cloneable_class;
    korp_class* java_lang_Thread_class;
    korp_class* java_util_Date_class;
    korp_class* java_lang_Runtime_class; 

    korp_string* classpath;
    korp_string* default_classpath;

}korp_globals;
#define korp_globals_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY,((uint8)(sizeof(korp_globals)/BYTES_OF_REF)-1),NOT_RECYCLABLE)

typedef struct _korp_trampos{
    obj_info header;
    korp_uint8_array* native_to_java_call;
}korp_trampos;
#define korp_trampos_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY,((uint8)(sizeof(korp_trampos)/BYTES_OF_REF)-1),YES_RECYCLABLE)

typedef struct _korp_entry{
    obj_info header;
    korp_entry* next;
    korp_object* value;
}korp_entry;
#define korp_entry_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY,2,NOT_RECYCLABLE)

typedef struct _korp_env{
    obj_info   header;
    korp_string_pool* string_pool;
    korp_entry* loader_list;
    korp_globals* globals;
    korp_ref_array* jnc_fields;
    korp_trampos* trampos; 
    uint16  jclass_kclass_offset;
    uint8*  bytecode_inst_len;
}korp_env;
#define korp_env_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY,7,NOT_RECYCLABLE)

extern korp_env* kenv;

#endif /* #ifndef _ENVIRONMENT_H */
