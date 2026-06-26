
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "natives.h"
#include "class_loader.h"
#include "utils.h"
#include "runtime.h"
#include "exception.h"

/*
 *  Compares this string to the specified object.
 *  The result is true if and only if the argument is not null and is a String object 
 *  that represents the same sequence of characters as this object.
 */

#ifdef __cplusplus
extern "C" {
#endif

void JNICALL com_sun_cldc_io_j2me_debug_PrivateOutputStream_putchar(korp_class* jclass, jchar ch)
{
    printf("%c", ch);
}

jobject JNICALL java_lang_Class_forName(korp_class* jclass, jobject jstr)
{
    korp_loader_exception exc;
    korp_string *kclassname = get_kstring_from_jstring( jstr );
    
    kclassname = classname_from_qualified_to_internal( kclassname);
    korp_class* kclass = load_class( get_sys_loader(), kclassname, &exc);

    return kclass->java_object;
}

jobject JNICALL java_lang_Class_newInstance(jobject jclass)
{
    korp_class* kclass = *(korp_class**)((uint8*)jclass + kenv->jclass_kclass_offset);
    
    return new_java_object(kclass);
}

jchar JNICALL java_lang_String_charAt(jobject jstr, jint index)
{
    korp_java_array* jarr;

    java_object_get_field( jstr, kenv->globals->jlString_value_field, (uint8*)&jarr);
    
    return (jchar)(uint16*)(jarr->value)[index];
}

jboolean JNICALL java_lang_String_equals(jobject jself, jobject jcomprand)
{
    int self_count, comp_count, self_off, comp_off;
    korp_java_array *self_val, *comp_val;

    if ( jself == jcomprand )
        return true;

    java_object_get_field( jself, kenv->globals->jlString_count_field, (uint8*)&self_count);
    java_object_get_field( jcomprand, kenv->globals->jlString_count_field, (uint8*)&comp_count);
    if( self_count != comp_count )
        return false;

    if( self_count == 0 )
        return true;

    java_object_get_field( jself, kenv->globals->jlString_offset_field, (uint8*)&self_off);
    java_object_get_field( jself, kenv->globals->jlString_value_field, (uint8*)&self_val);

    java_object_get_field( jcomprand, kenv->globals->jlString_offset_field, (uint8*)&comp_off);
    java_object_get_field( jcomprand, kenv->globals->jlString_value_field, (uint8*)&comp_val);
    
    while( self_count-- != 0 ){
        if( self_val->value[self_off++] != comp_val->value[comp_off++] )
            return false;
    }    

    return true;
}

jint JNICALL java_lang_String_indexOf_II(jobject jstr, jint unichar, jint from )
{
    korp_java_array* jarr;
    int i;

    java_object_get_field( jstr, kenv->globals->jlString_value_field, (uint8*)&jarr);

    if( from > (int)jarr->length ) return -1;
    if( from < 0) from = 0;

    for(i=0; i< (int)jarr->length; i++){
        if( jarr->value[i] == unichar )
            return i;
    }

    return -1;

}

jint JNICALL java_lang_String_indexOf(jobject jstr, jint unichar )
{
    return java_lang_String_indexOf_II(jstr, unichar, 0);
}

jobject JNICALL java_lang_StringBuffer_append(jobject jbuf, jobject jstr)
{
    korp_java_array *jbuf_value, *jstr_value;
    uint32 jbuf_count, new_count, jstr_offset, jstr_count;

    if(jstr==NULL) 
        jstr = get_jstring_from_kstring( string_pool_lookup(kenv->string_pool, "null"));

    java_object_get_field( jstr, kenv->globals->jlString_value_field, (uint8*)&jstr_value );
    java_object_get_field( jstr, kenv->globals->jlString_offset_field, (uint8*)&jstr_offset );
    java_object_get_field( jstr, kenv->globals->jlString_count_field, (uint8*)&jstr_count );
    java_object_get_field( jbuf, kenv->globals->jlStringBuffer_count_field, (uint8*)&jbuf_count );
    java_object_get_field( jbuf, kenv->globals->jlStringBuffer_value_field, (uint8*)&jbuf_value );
    
    new_count = jstr_count + jbuf_count;

    if( new_count >= jbuf_value->length ){
        char* temp = stack_alloc(new_count*2+1);
        korp_string *kstring;
        korp_java_array *jtemparr;
        memset(temp, 'a', new_count*2);
        temp[new_count*2] = '\0';
        kstring = string_pool_lookup(kenv->string_pool, temp);
        jtemparr = new_jarray_from_kstring( kstring);
        memcpy(jtemparr->value, jbuf_value->value, jbuf_value->length*BYTES_OF_CHAR);
        free_java_object( (korp_object*)jbuf_value );
        java_object_set_field( jbuf, kenv->globals->jlStringBuffer_value_field, (uint8*)&jtemparr );    
        jbuf_value = jtemparr;
    }

    memcpy( (uint16*)(jbuf_value->value)+jbuf_count, (uint16*)(jstr_value->value)+ jstr_offset, jstr_count*BYTES_OF_CHAR);
    java_object_set_field( jbuf, kenv->globals->jlStringBuffer_count_field, (uint8*)&new_count );    
    return jbuf;
}

jobject JNICALL java_lang_StringBuffer_toString(jobject jbuf)
{
    jobject jstr = new_java_object( kenv->globals->java_lang_String_class );
    korp_java_array* jarr;
    uint32 length;
    korp_string* kstring;
    char* chars;

    java_object_get_field( jbuf, kenv->globals->jlStringBuffer_value_field, (uint8*)&jarr );
    java_object_get_field( jbuf, kenv->globals->jlStringBuffer_count_field, (uint8*)&length );

    chars = stack_alloc(length*2);
    pack_utf8( chars, jarr->value, length);
    kstring = string_pool_lookup( kenv->string_pool, chars);

    return get_jstring_from_kstring( kstring);
}

void JNICALL java_lang_System_arraycopy(korp_class* kclass,
                                        jarray jsrc, jint src_offset,
                                        jarray jdst, jint dst_offset,
                                        jint length)
{
    korp_class *kc_src, *kc_dst;
    int32 src_len, dst_len;
    korp_java_type type;

    if(!(jsrc && jdst)) {
        throw_exception_name_native(kenv->globals->java_lang_NullPointerException_string);
    }

    kc_src = java_object_get_class( (korp_object*)jsrc );
    kc_dst = java_object_get_class( (korp_object*)jdst );

    if(!(kc_src->flags.is_array && kc_dst->flags.is_array)) {
        throw_exception_name_native(kenv->globals->java_lang_ArrayStoreException_string);
    }

    if(kc_src != kc_dst) {
        if(kc_src->array_info->is_primitive_array || kc_dst->array_info->is_primitive_array ) {
            throw_exception_name_native(kenv->globals->java_lang_ArrayStoreException_string);
        }            
    }

    if((src_offset < 0) || (dst_offset < 0) || (length < 0)) {
        throw_exception_name_native(kenv->globals->java_lang_ArrayIndexOutOfBoundsException_string);
    }

    if(!length)
        return;

    src_len = jsrc->length;
    dst_len = jdst->length;
    if(   (src_offset + length)>src_len
       || (dst_offset + length)>dst_len
      ) 
    {
        throw_exception_name_native(kenv->globals->java_lang_ArrayIndexOutOfBoundsException_string);
    }

    type = (korp_java_type)*string_to_chars(kc_src->array_info->array_element_class->name);

    if( is_ref_type_type(type)){
        korp_object** src_slot = (korp_object**)jsrc->value + src_offset;
        korp_object** dst_slot = (korp_object**)jdst->value + dst_offset;

        if(kc_src == kc_dst){ /* fast path */
           memmove(dst_slot, src_slot, length*BYTES_OF_REF);
       
        }else{ /* slow path, check each slot one by one */
            korp_class *kc_dst_elem = kc_dst->array_info->array_element_class;
                
            int i;
            for(i=0; i<length; i++){
                if( src_slot[i]) {
                    korp_class *kc_src_elem = java_object_get_class(src_slot[i]);

                    if( !java_instanceof_class(kc_src_elem, kc_dst_elem) )
                        throw_exception_name_native(kenv->globals->java_lang_ArrayStoreException_string);

                    dst_slot[i] = src_slot[i];
                }/* if slot not null */ 
            }/* for each slot */
        }/* slow path */
    
    }else{
        memmove( (void *)java_array_get_elem_addr(jdst, dst_offset), 
                 (const void*)java_array_get_elem_addr(jsrc, src_offset),
                 length*typesize_type(type)
               );
    }/* for primitive array */

    return;
}

jobject JNICALL java_lang_System_getProperty0(korp_class* kclass, jobject key_str)
{
    const char* key_chars;
    korp_string* kstr = get_kstring_from_jstring( key_str );
    
    key_chars = string_to_chars( kstr);
    
    /* j2me configuration */
    if ( strcmp(key_chars, "microedition.configuration") == 0 ) {
        kstr = string_pool_lookup(kenv->string_pool, "j2me");
    } 
    /* platform name */
    else if ( strcmp(key_chars, "microedition.platform") == 0) {
        kstr = string_pool_lookup(kenv->string_pool, "j2me");
    }
    /* javax.microedtion.io.Connector.protocol path */
    else if ( strcmp(key_chars, "javax.microedition.io.Connector.protocolpath") == 0) {
        kstr = string_pool_lookup(kenv->string_pool, "com.sun.cldc.io");
    }
    /* microedition.encoding default encoding name */
    else if ( strcmp(key_chars, "microedition.encoding") == 0) {
        kstr = string_pool_lookup(kenv->string_pool, "ISO8859_1");
    }
    /* com.sun.cldc.i18n.Helper.i18npath default encoding name */
    else if ( strcmp(key_chars, "com.sun.cldc.i18n.Helper.i18npath") == 0) {
        kstr = string_pool_lookup(kenv->string_pool, "com.sun.cldc.i18n.j2me");
    }
    /*    property = System.getProperty(internalName + "_InternalEncodingName"); */
    else if ( strcmp(key_chars, "ISO8859_1_InternalEncodingName") == 0) {
        kstr = string_pool_lookup(kenv->string_pool, "ISO8859_1");
    }
    /* lost native method */
    else {
        assert(0);
    }

    return (jobject)get_jstring_from_kstring(kstr);

}

#ifdef __cplusplus
}
#endif
