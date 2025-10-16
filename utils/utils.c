
#include <assert.h>
#include <string.h>

#include "platform.h"
#include "korp_types.h"
#include "utils.h"
#include "class_utils.h"
#include "runtime.h"
#include "exception.h"

bool is_ref_type_type( korp_java_type t)
{
    return t == JAVA_TYPE_CLASS || 
           t == JAVA_TYPE_ARRAY ||
           t == JAVA_TYPE_STRING;

}/* is_ref_type_type */

bool is_ref_type_chars( char* d)
{
    korp_java_type t = (korp_java_type)*d;
    return is_ref_type_type( t );

}/* is_ref_type_chars */

bool is_ref_type_string( korp_string* type)
{
    korp_java_type t = (korp_java_type)*string_to_chars(type);
    return is_ref_type_type( t );

}/* is_ref_type_string */

uint16 typesize_type( korp_java_type t)
{
    switch( t ){
        case JAVA_TYPE_BYTE: 
        case JAVA_TYPE_BOOLEAN:
            return 1;

        case JAVA_TYPE_CHAR:
        case JAVA_TYPE_SHORT:
            return 2;

        case JAVA_TYPE_FLOAT:
        case JAVA_TYPE_INT:
            return 4;

        case JAVA_TYPE_DOUBLE:
        case JAVA_TYPE_LONG:
            return 8;

        case JAVA_TYPE_CLASS:
        case JAVA_TYPE_ARRAY:
            return 4;
        
        default:
            return 0; 
    }
            
}

uint16 typesize_string( korp_string* type )
{
    korp_java_type t = (korp_java_type)*string_to_chars(type);
    return typesize_type(t);
}/* typesize_string */

uint16 typesize_chars(char* d )
{
    korp_java_type t = (korp_java_type)*d;
    return typesize_type(t);

}/* typesize_string */

void convert_path_separator(char* dest, const char* src)
{
    char* d;
    char c;
    assert( dest!= NULL);

    d = dest;
    while( (c=*src++) != '\0'){

        if( c == '\\' || c == '/')
            *d++ = DIR_SEPARATOR;
        else
            *d++ = c;
    }
    
    *d = '\0';
    return;
}

korp_string* classname_get_classfile_name(const korp_string* kclassname)
{
    char* filename = stack_alloc(256);
    convert_path_separator(filename, string_to_chars(kclassname));
    strcpy( (char *)(filename + get_string_len(kclassname)), ".class");
    return string_pool_lookup( kenv->string_pool, filename);
}

korp_string* get_full_filename(const korp_string* kpathname, 
                               const korp_string* kclassname)
{
    char* filename = stack_alloc(256);
    uint16 path_strlen = get_string_len(kpathname);
    strcpy(filename, string_to_chars(kpathname));
    filename[path_strlen] = DIR_SEPARATOR;
    strcpy( (char *)(filename + path_strlen + 1), string_to_chars(kclassname));
    return string_pool_lookup( kenv->string_pool, filename);
}

korp_string* classname_from_qualified_to_internal( korp_string* kqualified )
{
    char *temp, *internal = stack_alloc(256);
    const char* qualified = string_to_chars( kqualified );
    char c;

    temp = internal;
    while( ( c= *qualified++ ) != '\0' )
        if( c=='.' ) *internal++ = '/';
        else *internal++ = c;
        
    *internal = '\0';

    return string_pool_lookup( kenv->string_pool, temp);
}

char* get_next_path( char* pathes)
{
    /* classpath last char must be PATH_SEPARATOR then '\0' */

    char *cur_str_end;
    char *next_path_end;
    
    cur_str_end = strchr(pathes, '\0');

    if( *(cur_str_end-1) == PATH_SEPARATOR ){
        /* first time come in */
        next_path_end = strchr(pathes, PATH_SEPARATOR); 
        *next_path_end = '\0';
        return pathes;
    }

    if( *(++cur_str_end) == '\0' ){
        /*last path */
        return NULL;
    }
    
    /* more path after \0 */
    next_path_end = strchr( cur_str_end, PATH_SEPARATOR);
    *next_path_end = '\0';
    return cur_str_end;
}

uint16 get_chars_hash(const char* chars, uint16 *len)
{
  	const char *charp = chars;
    uint16 key=0;
    unsigned char c;
    while ((c = *charp++) != '\0') key = (key+c)<<1;

    if(len != NULL ) *len = (uint16) ((charp - chars) - 1);
	return key % HASH_TABLE_SIZE;

}

uint8 log2(uint32 scale)
{
    uint32 i=0;
    while( (scale = scale>>1) != 0 ) i++;
    
    return i;
}

static uint16 get_unicode_length_of_utf8(const char *utf8)
{
    unsigned len = 0;
    uint8 ch;
    while(ch = *utf8++) {
        len++;
        if(ch & 0x80) {
            assert(ch & 0x40);
            if(ch & 0x20) {
                uint8 x = ch;
                uint8 y = *utf8++;
                uint8 z = *utf8++;
            } else {
                uint8 x = ch;
                uint8 y = *utf8++;
            }
        } 
    }
    return len;
} /* get_unicode_length_of_utf8 */

static uint16 get_utf8_length_of_unicode(const uint16 *unicode, unsigned unicode_length)
{
    unsigned len = 0;
    for(unsigned i = 0; i < unicode_length; i++) {
        uint16 ch = unicode[i];
        if(ch == 0) {
            len += 2;
        } else if(ch < 0x80) {
            len += 1;
        } else if(ch < 0x800) {
            len += 2;
        } else {
            len += 3;
        }
    }
    return len;
} /* get_utf8_length_of_unicode */

void pack_utf8(char *utf8_string, const uint16 *unicode, unsigned unicode_length)
{
    unsigned utf8_len = get_utf8_length_of_unicode(unicode, unicode_length);
    char *s = utf8_string;
    for(unsigned i = 0; i < unicode_length; i++) {
        unsigned ch = unicode[i];
        if(ch == 0) {
            *s++ = (char)0xc0;
            *s++ = (char)0x80;
        } else if(ch < 0x80) {
            *s++ = ch;
        } else if(ch < 0x800) {
            unsigned b5_0 = ch & 0x3f;
            unsigned b10_6 = (ch >> 6) & 0x1f;
            *s++ = 0xc0 | b10_6;
            *s++ = 0x80 | b5_0;
        } else {
            unsigned b5_0 = ch & 0x3f;
            unsigned b11_6 = (ch >> 6) & 0x3f;
            unsigned b15_12 = (ch >> 12) & 0xf;
            *s++ = 0xe0 | b15_12;
            *s++ = 0x80 | b11_6;
            *s++ = 0x80 | b5_0;
        }
    }
    *s = 0;
} 

static void unpack_utf8(uint16 *unicode, const char *utf8_string)
{
    const uint8 *utf8 = (const uint8 *)utf8_string;
    unsigned len = 0;
    uint16 ch;
    while(ch = (uint16)*utf8++) {
        len++;
        if(ch & 0x80) {
            assert(ch & 0x40);
            if(ch & 0x20) {
                uint16 x = ch;
                uint16 y = (uint16)*utf8++;
                uint16 z = (uint16)*utf8++;
                uint16 u =
                *unicode++ = ((0x0f & x) << 12) + ((0x3f & y) << 6) + ((0x3f & z));
            } else {
                uint16 x = ch;
                uint16 y = (uint16)*utf8++;
                uint16 u =
                *unicode++ = ((0x1f & x) << 6) + (0x3f & y);
            }
        } else {
            uint16 u =
            *unicode++ = ch;
        }
    }
} /* unpack_utf8 */

korp_string* new_kstring_from_jarray(korp_java_array* jarr)
{
    char chars[256];
    korp_string* kstring;
    
    pack_utf8(chars, (uint16*)(jarr->value), jarr->length);
    kstring = string_pool_lookup(kenv->string_pool, chars);

    kstring->intern = get_jstring_from_kstring(kstring);

    return kstring;
}

korp_java_array* new_jarray_from_kstring(korp_string* kstring)
{
    korp_java_array* jarr;
    const char* chars = string_to_chars(kstring);
    uint16 len;

    if( kstring->intern ){
        java_object_get_field( kstring->intern, kenv->globals->jlString_value_field, (uint8*)&jarr);
        return jarr;
    }

    len = get_unicode_length_of_utf8( chars );
    jarr = new_java_array(kenv->globals->ArrayOfChar_class, len);    

    unpack_utf8(jarr->value, chars );
    jarr->length = len;

    return jarr;
}

korp_string* get_kstring_from_jstring( korp_object* jstr)
{
    korp_java_array* jvalue;
    
    java_object_get_field(jstr, kenv->globals->jlString_value_field, (uint8*)&jvalue);

    return new_kstring_from_jarray( jvalue);

}

korp_object* get_jstring_from_kstring(korp_string *kstring)
{
    korp_java_array* jarr;
    korp_object* jstring;
    uint32 len;

    if(kstring->intern != NULL)
        return kstring->intern;
        
    jarr = new_jarray_from_kstring(kstring);
    jstring = new_java_object( kenv->globals->java_lang_String_class);
    java_object_set_field(jstring, kenv->globals->jlString_value_field, (uint8*)&jarr);

    len = jarr->length;
    java_object_set_field(jstring, kenv->globals->jlString_count_field, (uint8*)&len);

    //len = 0;
    //java_object_set_field(jstring, kenv->globals->jlString_offset_field, (uint8*)&len);

    kstring->intern = jstring;
    return jstring;
}

void java_object_set_field(korp_object* jobj, 
                           korp_field* kfield, 
                           uint8* value )
{
    uint16 size = typesize_string(kfield->descriptor);     
    uint32 fieldaddr = (uint32)jobj + (uint32)kfield->offset;
	memmove((void*)fieldaddr, (void*)value, size);
	/*
    while(size--){
        *(uint8*)fieldaddr++ = *value++;
    }*/

    //return jobj;
}

void java_object_get_field(korp_object* jobj, 
                                   korp_field* kfield, 
                                   uint8* value )
{
    uint16 size = typesize_string(kfield->descriptor);     
    uint32 fieldaddr = (uint32)jobj + (uint32)kfield->offset;
	memmove((void*)value, (void*)fieldaddr, size);
	/*
    while(size--){
        *value++ = *(uint8*)fieldaddr++;
    }*/

    //return jobj;
}

uint32 java_array_get_elem_addr(korp_java_array* jarr, uint16 index)
{
    korp_class* kclass;
    korp_java_type type;

    assert( IS_JAVA_OBJECT_H(*(obj_info*)jarr) );
    
    kclass = java_object_get_class((korp_object*)jarr);
    type = (korp_java_type)*(string_to_chars((kclass->name))+1);
	return (uint32)(jarr->value) + (typesize_type(type))*index; 
}

korp_class* get_primitive_class_from_type(korp_java_type type)
{
    switch(type){
        case JAVA_TYPE_BOOLEAN:
            return kenv->globals->boolean_class;
        case JAVA_TYPE_BYTE:
            return kenv->globals->byte_class;
        case JAVA_TYPE_CHAR:
            return kenv->globals->char_class;
        case JAVA_TYPE_SHORT:
            return kenv->globals->short_class;
        case JAVA_TYPE_INT:
            return kenv->globals->int_class;
        case JAVA_TYPE_FLOAT:
            return kenv->globals->float_class;
        case JAVA_TYPE_LONG:
            return kenv->globals->long_class;
        case JAVA_TYPE_DOUBLE:
            return kenv->globals->double_class;
    }

    assert(0);
    return NULL;
}

bool java_instanceof_class(korp_class* kc_self, korp_class* kclass)
{
    if(kc_self == kclass) 
        return true;
    
    if( kc_self->flags.is_array ){
        if( kclass = kenv->globals->java_lang_Object_class ) return true;
        if( !kclass->flags.is_array ) return false;
        
        return java_instanceof_class( kc_self->array_info->array_element_class, 
                                      kclass->array_info->array_element_class);    
    }else{
        if(!class_is_interface( kclass )){
            return is_subclass( kclass, kc_self);

        }else{
            while( kc_self ){
                unsigned n_intf = GET_VM_ARRAY_LENGTH( kc_self->interface_info );
		        for(unsigned i = 0; i < n_intf; i++) {
                    if( java_instanceof_class( (korp_class*)kc_self->interface_info->value[i], kclass) == true )
                        return true;
                }
                kc_self = kc_self->super_class;
            }/* while */
        }/* else !interface */
    }/* else array */

    return false;
}

void __stdcall java_object_checkcast(korp_class* kclass, korp_object* jobj )
{
    korp_class* kc_self;
    korp_object* jexc;

    if( jobj == NULL ) return;

    kc_self = java_object_get_class( jobj );
    if(kc_self == kclass) return;

    if( java_instanceof_class(kc_self, kclass) == true) return;

    jexc = new_java_object( kenv->globals->java_lang_ClassCastException_class );
    throw_exception_object_native(jexc);

    assert(0);
    return;
}
