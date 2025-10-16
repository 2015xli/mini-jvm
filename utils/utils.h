
#ifndef _UTILS_H
#define _UTILS_H

#include "platform.h"

/* file utilities */
korp_string* classname_get_classfile_name(const korp_string* kclassname);
korp_string* get_full_filename(const korp_string* pathname, const korp_string* classname);
void convert_path_separator(char* dest, const char* src);
char* get_next_path(char* pathes);
uint16 get_chars_hash(const char* chars, uint16 *len);

/* string utilities */
#define string_to_chars(s) ((const char*)s->byte)

void pack_utf8(char *utf8_string, const uint16 *unicode, unsigned unicode_length);
uint32 java_array_get_elem_addr(korp_java_array*, uint16 index);
void java_object_set_field(korp_object* jobj, korp_field* kfield, uint8* value );
void java_object_get_field(korp_object* jobj, korp_field* kfield, uint8* value );
korp_object* get_jstring_from_kstring(korp_string *kstring);
korp_string* get_kstring_from_jstring( korp_object* jstr);
korp_java_array* new_jarray_from_kstring(korp_string* kstring);
korp_string* new_kstring_from_jarray(korp_java_array* jarr);
korp_class* get_primitive_class_from_type(korp_java_type);
korp_string* classname_from_qualified_to_internal( korp_string* );

void __stdcall java_object_checkcast(korp_class*, korp_object*) stdcall__;
bool java_instanceof_class(korp_class* kc_self, korp_class* kclass);

bool is_ref_type_string( korp_string*);
bool is_ref_type_type( korp_java_type);
bool is_ref_type_chars( char*);
uint16 typesize_string( korp_string*);
uint16 typesize_type( korp_java_type);
uint16 typesize_chars( char* );

typedef struct _korp_stack{
    uint32* slot;
    uint32 depth; 
}korp_stack;

#define stack_pop(s)            ((s)->depth--)
#define stack_times_pop(s, i)   ((s)->depth -= (i))
#define stack_push(s, i)        ((s)->slot[(s)->depth++] = (i))
#define stack_get_ith(s, i)     ((s)->slot[(s)->depth-(i)])
#define stack_set_ith(s, i, b)  ((s)->slot[(s)->depth-(i)] = (b))
#define stack_reset(s)          ((s)->depth = 0)
#define stack_depth(s)          ((s)->depth)
#define stack_top(s)            (stack_get_ith((s), 1))

uint8 log2(uint32); 

#endif /* #ifndef _UTILS_H */
