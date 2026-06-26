#include <assert.h>
#include <memory.h>
#include <stdio.h>

#include "class_loader.h"
#include "string_pool.h"
#include "utils.h"
#include "vm_interface.h"
#include "method.h"

struct_jnc_field korp_jnc_fields[] = {
    { "java/lang/Thread",    "korp_thread",     "L",                  MOD_PRIVATE},
    { "java/lang/Throwable", "korp_stacktrace", "[L",   MOD_PRIVATE|MOD_TRANSIENT},
    { "java/lang/Class",     "korp_class",      "L",                  MOD_PRIVATE},
};

void load_jnc_fields()
{
    uint32 i;
    uint32 num = sizeof(korp_jnc_fields)/sizeof(struct_jnc_field);
    korp_ref_array* jnc_fields = new_vm_array(korp_ref_array, num);
    for( i=0; i<num; i++){
        korp_jnc_field* jnc_field = new_vm_object(korp_jnc_field);
        jnc_field->classname = string_pool_lookup(kenv->string_pool, korp_jnc_fields[i].classname );
        jnc_field->fieldname = string_pool_lookup(kenv->string_pool, korp_jnc_fields[i].fieldname );
        jnc_field->descriptor = string_pool_lookup(kenv->string_pool, korp_jnc_fields[i].descriptor );
        jnc_field->modifier = korp_jnc_fields[i].modifier;
        jnc_fields->value[i] = (korp_object *)jnc_field;
    }

    kenv->jnc_fields = jnc_fields;
}

typedef struct _class_buffer{
    const uint8* classfile;
    uint8*   pos;
    int     len;
} class_buffer;

static uint32 readbytes(class_buffer* buf, int num, korp_loader_exception* exc)
{
    int i=0;
    uint32 result=0;
    assert( num>0 && num<5 );

    if( buf->pos + num > buf->classfile + buf->len ){
        *exc = LD_ClassFormatError;
        return KORP_ERROR;
    }

    for(; i<num; i++, buf->pos++){
        uint32 x = (uint32)(*buf->pos);
        result = (result<<8) + (*buf->pos);
    }

    return result;
}

static korp_string* cp_class_read_name( korp_ref_array* cpool,
                                        class_buffer* buf,
                                        korp_loader_exception* e)
{
    uint32 cp_index;
    korp_object** cp_slot = cpool->value;
    uint32 cp_size = (uint32)GET_VM_ARRAY_LENGTH(cpool);

    cp_index = readbytes(buf, 2, e); if( *e != LD_OK ) return (korp_string *)KORP_ERROR;
    
    if( cp_index > cp_size ){
        *e = LD_ClassFormatError;
        return (korp_string *)KORP_ERROR;
    }

    if( cp_index == 0 ) return NULL;
    
    if( ((korp_char_array *)cp_slot[0])->value[cp_index] != TAG_Class ){ 
        *e = LD_ClassFormatError;
        return (korp_string *)KORP_ERROR;
    }
    /* classname's korp_string is there */
    return (korp_string *)cp_slot[cp_index];  

}/* cp_class_read_name */

static korp_string* cp_utf8_read_string( korp_ref_array* cpool,
                                         class_buffer* buf,
                                         korp_loader_exception* e)
{
    uint32 cp_index;
    korp_object** cp_slot = cpool->value;
    uint32 cp_size = (uint32)GET_VM_ARRAY_LENGTH(cpool);

    cp_index = readbytes(buf, 2, e); if( *e != LD_OK ) return (korp_string *)KORP_ERROR;
    
    if( cp_index > cp_size ){
        *e = LD_ClassFormatError;
        return (korp_string *)KORP_ERROR;
    }

    if( cp_index == 0 ) return NULL;
    
    if( ((((korp_char_array *)cp_slot[0])->value[cp_index])&TAG_MASK) != TAG_Utf8 ){ 
        *e = LD_ClassFormatError;
        return (korp_string *)KORP_ERROR;
    }
        
    return (korp_string *)cp_slot[cp_index];

}/* cp_utf8_read_string */

static bool parse_constant_pool(korp_class *kclass, 
                                class_buffer *buf)
{	
    korp_ref_array* cpool;
    korp_uint8_array* tags;
    korp_loader_exception e = LD_OK;
    uint32 i, result, quat[2],l, c;
    uint8 tag;
    korp_object *cp_item;
    char* chars;

    uint32 cp_size = readbytes( buf, 2, &e);
    if( e != LD_OK ) return false;
    
    cpool = new_vm_array(korp_ref_array, cp_size);
    tags = (korp_uint8_array *)new_vm_array(korp_uint8_array, cp_size);

    /* Entry 0 */
    cpool->value[0] = (korp_object *)tags;
    tags->value[0] = TAG_NULL; 

    /* parse the slots */
    for( i=1; i< cp_size; i++){
        tag = (uint8)readbytes(buf, 1, &e); 
		if( e != LD_OK ) return false;
        tags->value[i] = tag;

		switch( tag ){
			case TAG_Class:
			case TAG_String:

                result = readbytes(buf, 2, &e);
                if( e != LD_OK ) return false;

                cp_item = (korp_object *)new_vm_object(korp_uint16);
                ((korp_uint16 *)cp_item)->value = result;
                cpool->value[i] = cp_item;
				break;

            case TAG_Fieldref:
			case TAG_Methodref:
			case TAG_InterfaceMethodref:
            case TAG_NameAndType:

                result = readbytes(buf, 2, &e);
                if( e != LD_OK ) return false;
                cp_item = (korp_object *)new_vm_object(korp_2uint16);
                ((korp_2uint16 *)cp_item)->value[0] = result;

                result = readbytes(buf, 2, &e);
                if( e != LD_OK ) return false;
                ((korp_2uint16 *)cp_item)->value[1] = result;

                cpool->value[i] = cp_item;
				break;

            case TAG_Integer:
			case TAG_Float:

                result = readbytes(buf, 4, &e);
                if( e != LD_OK ) return false;

                cp_item = (korp_object *)new_vm_object(korp_int);
                ((korp_int *)cp_item)->value = result;
                cpool->value[i] = cp_item;
				break;

			case TAG_Long:
			case TAG_Double:
                /* pack high word and low word in one object */
                /* high word */
                result = readbytes(buf, 4, &e);
                if( e != LD_OK ) return false;
                quat[1] = result;

                cp_item = (korp_object *)new_vm_object(korp_long);
                cpool->value[i++] = cp_item;

                /* low word */
                result = readbytes(buf, 4, &e);
                if( e != LD_OK ) return false;
                quat[0] = result;
                ((korp_long *)cp_item)->value = *(uint64 *)quat;
                cpool->value[i] = cp_item;

                tags->value[i] = tag;

                break;

            case TAG_Utf8:
                {

                result = readbytes(buf, 2, &e);
                if( e != LD_OK ) return false;
                
                chars = stack_alloc(result+1);
                for(l=0; l<result; l++ ){
                    c = readbytes(buf, 1, &e);
                    if( e != LD_OK || c == 0 ) return false;
                    *chars++ = c;
                }

                *chars = '\0';
                chars -= result;
                /*
                cp_item = (korp_object *)new_vm_object(korp_ref);
                ((korp_ref *)cp_item)->value = (korp_object *)string_pool_lookup( kenv->string_pool, chars);
                */
                cp_item = (korp_object *)string_pool_lookup( kenv->string_pool, chars);
                cpool->value[i] = cp_item;
                
                }break;

			default:
				return false;
		}/* switch tag */
	}/* for each slot tag */
	
    /* check the consistency and let all slots pointing to Utf8 
       point to the string directly and marked as resolved except Class 
       which will be resolved later
     */

    for( i=1; i< cp_size; i++){       
        switch( tags->value[i] ){

            case TAG_String:
                cp_set_resolved(cpool, i);

            case TAG_Class: /* points to its literal index*/
                cp_item = cpool->value[i];
                result = (uint32)((korp_short *)cp_item)->value;
                if( ((tags->value[result])&TAG_MASK) != TAG_Utf8 ) return false;
                
                free_vm_object(cp_item);
                cpool->value[i] = cpool->value[result];

				break;

            case TAG_Fieldref:
			case TAG_Methodref:
			case TAG_InterfaceMethodref:
                cp_item = cpool->value[i];
                result = (uint32)((korp_2uint16 *)cp_item)->value[0];
                if( tags->value[result] != TAG_Class ) return false;

                result = (uint32)((korp_2uint16 *)cp_item)->value[1];
                if( ((tags->value[result])&TAG_MASK) != TAG_NameAndType ) return false;            

                break;

            case TAG_NameAndType:

                cp_item = cpool->value[i];
                result = (uint32)((korp_2uint16 *)cp_item)->value[0];
                if( ((tags->value[result])&TAG_MASK) != TAG_Utf8 ) return false;

                c = (uint32)((korp_2uint16 *)cp_item)->value[1];
                if( ((tags->value[result])&TAG_MASK) != TAG_Utf8 ) return false;            

                free_vm_object(cp_item);
                cp_item = (korp_object*)new_vm_object(korp_2ref);
                ((korp_2ref *)cp_item)->value[0] = cpool->value[result];
                ((korp_2ref *)cp_item)->value[1] = cpool->value[c];
                cpool->value[i] = cp_item;

                cp_set_resolved(cpool, i);
    
                break;

			default:
				break;
		}/* switch tag */
	}/* for each slot tag */

    kclass->constant_pool = cpool;
    return true;
}

static bool parse_class_member(class_buffer *buf,
                               korp_class *kclass,
                               uint16* modifier, 
                               korp_string** name, 
                               korp_string** descriptor)
{
    korp_loader_exception e=LD_OK;
    korp_ref_array *cpool = kclass->constant_pool;
    *modifier = (uint16)readbytes(buf, 2, &e); if( e != LD_OK ) return false;

    *name = cp_utf8_read_string(cpool,buf,&e); if( e != LD_OK ) return false;
    if( *name == NULL ) return false;

    *descriptor = cp_utf8_read_string(cpool,buf,&e); if( e != LD_OK ) return false;
    if( *descriptor == NULL ) return false;
    
    return true;
}

static bool parse_constant(korp_class* kclass, 
                           korp_field* kfield,
                           class_buffer* buf,
                           korp_loader_exception* e)
{   
    char const_tag, field_type;
    korp_ref_array *cpool = kclass->constant_pool;
    korp_object **cp_slot = cpool->value;
    uint32 cp_size = (uint32)GET_VM_ARRAY_LENGTH(cpool);
    korp_long *lconst;
    korp_float *fconst;
    korp_double *dconst;
    korp_int *iconst;
    korp_ref *rconst;

    uint32 cp_index = readbytes(buf, 2, e); if( *e != LD_OK || cp_index > cp_size) return false;
    
    const_tag = (char)(((korp_char_array *)cp_slot[0])->value[cp_index]);
    field_type = *(string_to_chars(kfield->descriptor));

    switch( const_tag ){
        case TAG_Long:{
            if(field_type != JAVA_TYPE_LONG) return false;
            lconst = (korp_long *)new_vm_object(korp_long);
            lconst->value = ((korp_long *)cp_slot[cp_index])->value;
            kfield->constval = (korp_object *)lconst;
            }break;

        case TAG_Float:{
            if(field_type != JAVA_TYPE_FLOAT) return false;
            fconst = (korp_float *)new_vm_object(korp_float);
            fconst->value = ((korp_float *)cp_slot[cp_index])->value;
            kfield->constval = (korp_object *)fconst;
            }break;

        case TAG_Double:{
            if(field_type != JAVA_TYPE_DOUBLE) return false;
            dconst = (korp_double *)new_vm_object(korp_double);
            dconst->value = ((korp_double *)cp_slot[cp_index])->value;
            kfield->constval = (korp_object *)dconst;
            }break;

        case TAG_Integer:{
            if( !(field_type == JAVA_TYPE_INT         || 
                  field_type == JAVA_TYPE_SHORT       ||
                  field_type == JAVA_TYPE_BOOLEAN     || 
                  field_type == JAVA_TYPE_BYTE        ||
                  field_type == JAVA_TYPE_CHAR ) 
              ) return false;

            iconst = (korp_int *)new_vm_object(korp_int);
            iconst->value = ((korp_int *)cp_slot[cp_index])->value;
            kfield->constval = (korp_object *)iconst;
            }break;

        case TAG_String:{
            if (field_type != JAVA_TYPE_CLASS) return false;
            rconst = (korp_ref *)new_vm_object(korp_ref);
            cp_index = (uint32)(((korp_short *)cp_slot[cp_index])->value);
            rconst->value = ((korp_ref *)cp_slot[cp_index])->value;
            
            kfield->constval = (korp_object *)rconst;
            }break;

        default:
            return false;
    }

    return true;
}

static bool parse_this_class(korp_class* kclass, 
                             class_buffer* buf,
                             korp_loader_exception* exc)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;

    korp_string* kclassname = cp_class_read_name(cpool, buf, &e);  if( e != LD_OK ) return false;

    if( kclass->name != NULL ){
        if( kclass->name != kclassname ){
            *exc = LD_NoClassDefFoundError;
            return false;
        }
    }else{
        kclass->name = kclassname;
    }

    return true;

}

static bool parse_super_class(korp_class *kclass, 
                              class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;
    korp_string* kclassname = cp_class_read_name(cpool, buf, &e);  if( e != LD_OK ) return false;

    if( kclassname == NULL ){
        /* java.lang.Object */
        kclass->super_name = NULL;
    }else{
        kclass->super_name = kclassname;
    }

    return true;

    /*
     korp_object **cp_slot = cpool->value;
     uint32 cp_size = (uint32)GET_VM_ARRAY_LENGTH( (korp_ref_array *)cpool );

    uint32 cp_index = readbytes(buf, 2, &e);  if( e != LD_OK || cp_index > cp_size ) return false;
    if(cp_index == 0){
        kclass->super_name = NULL;
    }else{
        korp_string* kclassname = cp_class_read_name(cpool, buf, &e);  if( e != LD_OK ) return false;
        if( kclassname == NULL )  return false;
        kclass->super_name = kclassname;
    }

    return true;
    */
}

static bool parse_interfaces(korp_class *kclass, 
                             class_buffer *buf)
{
    korp_string *intfc_name;
    uint32 i;    
    korp_loader_exception e = LD_OK;
    korp_ref_array* interface_info;
    korp_ref_array *cpool = kclass->constant_pool;

    uint32 num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    if( num == 0) return true;

    interface_info = (korp_ref_array *)new_vm_array(korp_ref_array, num);
    for(i=0; i<num; i++){
        intfc_name = cp_class_read_name(cpool, buf, &e);  if( e != LD_OK ) return false;
        if( intfc_name == NULL ) return false;
        interface_info->value[i] = (korp_object *)intfc_name;
    }

    kclass->interface_info = interface_info;
    return true;
}

static bool parse_fields(korp_class *kclass, 
                         class_buffer *buf)
{
    korp_loader_exception e = LD_OK;
    korp_field* field;
    korp_string* str;
    korp_ref_array* field_info;
    korp_ref_array *cpool = kclass->constant_pool;
    korp_object **cp_slot = cpool->value;
    uint32 cp_size = (uint32)GET_VM_ARRAY_LENGTH( (korp_ref_array *)cpool );
    korp_ref_array* jnc_fields = kenv->jnc_fields;
    uint32 num_all_jnc_field = (uint32)GET_VM_ARRAY_LENGTH(jnc_fields);
    uint32 i,j,attr_len, attr_num, num, num_jnc_field=0;
    korp_jnc_field* jfield = NULL;

    /* find if there are JNC fields */
    for(i=0; i<num_all_jnc_field; i++){
        jfield = (korp_jnc_field *)jnc_fields->value[i];
        if( jfield->classname == kclass->name ) num_jnc_field++;
    }

    num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    if( num==0 && num_jnc_field==0) return true;

    field_info = (korp_ref_array *)new_vm_array(korp_ref_array, num + num_jnc_field);
    for(i=0; i<num; i++){
        field = (korp_field *)new_vm_object(korp_field);
        if( parse_class_member(buf, kclass, &field->modifier, &field->name, &field->descriptor) == false ) return false;
        
        /* parse field attribute: ConstantValue */
        attr_num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    

        for(j=0; j<attr_num; j++){
            str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false; 
            if( str == NULL ) return false;
            
            attr_len=0;
            attr_len = readbytes(buf, 4, &e); if( e != LD_OK ) return false;
            if( field_is_static(field) &&
                str == string_pool_lookup( kenv->string_pool, "ConstantVlaue" ) ){
                if(attr_len != 2) return false;
                if( parse_constant(kclass, field, buf, &e ) == false ) return false;

            }else{ /* unrecognized */
                uint32 t;
                for(t=0; t<attr_len; t++ ){
                    readbytes(buf, 1, &e); if( e != LD_OK ) return false;
                } 
            }/* if ConstantValue else */
            
        }/* for each attribute */
        field_initialize(kclass, field);
        field_info->value[i] = (korp_object *)field;
    }/* for each field */

    /* for JNC fields */
    if( num_jnc_field != 0 ){
        for(j=0; j<num_all_jnc_field; j++){
            jfield = (korp_jnc_field *)jnc_fields->value[j];
            if( jfield->classname == kclass->name ){
                field = (korp_field *)new_vm_object(korp_field);        
                field->name = jfield->fieldname;
                field->descriptor = jfield->descriptor;
                field->modifier = jfield->modifier;
                field_info->value[i++]= (korp_object *)field;
            }
        }
    }
    kclass->field_info = field_info;
    return true;
}/* parse_fields */

static bool parse_handlers(korp_class *kclass, 
                           korp_method *kmethod,
                           class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;
    korp_object **cp_slot = cpool->value;
    uint32 cp_size = (uint32)GET_VM_ARRAY_LENGTH( (korp_ref_array *)cpool );

    uint32 i, byte_code_length; 
    korp_ref_array* handler_info;
    korp_handler* khan;

    uint16 exc_table_length = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    
    if(exc_table_length == 0) return true;

    handler_info = new_vm_array(korp_ref_array, exc_table_length);
    
    byte_code_length = METHOD_BYTE_CODE_LENGTH(kmethod);
	for (i=0; i<exc_table_length; i++) {
        khan = new_vm_object(korp_handler);
		khan->start_pc = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    
        khan->end_pc = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
        khan->handler_pc = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    
//        khan->name = cp_class_read_name(cpool, buf, &e ); if( e != LD_OK ) return false;    
		khan->class_type_index = readbytes(buf, 2, &e); if( e != LD_OK ) return false;          
        if( khan->start_pc > khan->end_pc || 
                khan->start_pc > byte_code_length ||
                khan->end_pc > byte_code_length )
            return false;
        
        handler_info->value[i] = (korp_object *)khan;
	}/* for each exception table slot */

    kmethod->handler_info = handler_info;
    return true;
}/* parse_exctable */


static bool parse_linenumtable(korp_method *kmethod,
                               class_buffer *buf)
{
    korp_loader_exception e = LD_OK;
    korp_ref_array *linenum_table;
    korp_linenum* linenum;
    uint32 byte_code_length;
    uint16 i;
    uint16 num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    linenum_table = new_vm_array(korp_ref_array, num);
    byte_code_length = METHOD_BYTE_CODE_LENGTH(kmethod);

    for(i=0; i<num; i++){
        linenum = new_vm_object(korp_linenum);
        linenum->start_pc = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
        linenum->line_num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
        if(linenum->start_pc > byte_code_length ) 
            return false;
        linenum_table->value[i] = (korp_object *)linenum;
    }

    kmethod->linenum_table = linenum_table;
    return true;

}/* parse_linenumtable */

static bool parse_localvartable(korp_class *kclass, 
                                korp_method *kmethod,
                                class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;
    korp_ref_array* localvar_table;
    korp_localvar* localvar;
    uint32 byte_code_length;
    korp_string* str;
    uint16 i;
    uint16 num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    localvar_table = new_vm_array(korp_ref_array, num);
    byte_code_length = METHOD_BYTE_CODE_LENGTH(kmethod);

    for(i=0; i<num; i++){
        localvar = new_vm_object(korp_localvar);
        localvar->start_pc = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
        localvar->length = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
        
        str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false;
        if( str == NULL ) return false;
        localvar->name = str;

        str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false;
        if( str == NULL ) return false;
        localvar->descriptor = str;
        
        localvar->index = readbytes(buf, 2, &e); if( e != LD_OK ) return false;

        if(localvar->start_pc > byte_code_length ||
           (uint32)(localvar->start_pc + localvar->length) > byte_code_length ||
           localvar->index > kmethod->max_locals )
            return false;

        localvar_table->value[i] = (korp_object *)localvar;
    }

    kmethod->localvar_table = localvar_table;
    return true;
}/* parse_localvartable */

static bool parse_code(korp_class *kclass, 
                       korp_method *kmethod,
                       class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;

    uint32 byte_code_length, i,attr_len=0; 
    korp_uint8_array* code;
    uint16 num;
    korp_string* str;

    /* max_stack */
    kmethod->max_stack = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    /* max_locals */
    kmethod->max_locals = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    /* code_length */
    byte_code_length = readbytes(buf, 4, &e); if( e != LD_OK ) return false;
    /* code */
    code = new_vm_array(korp_uint8_array, byte_code_length );
	for (i=0; i< byte_code_length; i++) {
        code->value[i] = (uint8)readbytes(buf, 1, &e); if( e != LD_OK ) return false;
	}
    kmethod->byte_code = code;

    /* parse exception table */
    if( parse_handlers(kclass, kmethod, buf) == false ) return false;

    /* parse attributes of Code */
    num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    

    for(i=0; i<num; i++){
        uint32 attr_len=0;
        str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false;    
        if( str == NULL ) return false;
        
        attr_len = readbytes(buf, 4, &e); if( e != LD_OK ) return false;

        if( str == string_pool_lookup( kenv->string_pool, "LineNumberTable")){
            if( parse_linenumtable( kmethod, buf) == false ) return false;

        }else if(str == string_pool_lookup( kenv->string_pool, "LocalVariableTable")){
            if( parse_localvartable(kclass, kmethod, buf) == false ) return false;            

        }else{ /* unrecognized */
            uint32 t;
            for( t=0; t<attr_len; t++ ){
                readbytes(buf, 1, &e); if( e != LD_OK ) return false;
            } 
        }
   
    }/* for each attribute*/
    
    return true;

}/* parse_code */

static bool parse_exceptions(korp_class *kclass,
                             korp_method *kmethod,
                             class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;
    korp_ref_array* exception_table;
    korp_string* str;
    uint32 i; 
    uint16 num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    exception_table = new_vm_array(korp_ref_array, num);

    for(i=0; i<num; i++){        
        str = cp_class_read_name(cpool, buf, &e); if( e != LD_OK ) return false;
        if( str == NULL ) return false;
        exception_table->value[i] = (korp_object *)str;
    }
    
    kmethod->exception_table = exception_table;
    return true; 

}/* parse_exceptions */

static bool parse_methods(korp_class *kclass, 
                          class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;
    korp_ref_array* method_info;
    korp_method* kmethod;

    uint32 num, i; 
    num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;
    method_info = (korp_ref_array *)new_vm_array(korp_ref_array, num);
    for(i=0; i<num; i++){
        uint32 j, attrnum;
        korp_string* str;
        korp_globals* globals;
        kmethod = (korp_method *)new_vm_object(korp_method);
        if( parse_class_member(buf, kclass, &kmethod->modifier, &kmethod->name, &kmethod->descriptor) == false ) return false;
        globals = kenv->globals;
        if( kmethod->name == globals->finalize_name_string ){
            if( kmethod->name == globals->finalize_descriptor_string ){
                kmethod->flags.is_finalize = 1;
            }
        }else if( kmethod->name == globals->init_string ){
            kmethod->flags.is_init = 1;

        }else if( kmethod->name == globals->clinit_string ){
            kclass->clinit = kmethod;
            kmethod->flags.is_clinit = 1;
        }

        /* parse method attributes: ConstantValue */
        attrnum = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    

        for(j=0; j<attrnum; j++){
            uint32 attr_len=0;
            str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false;    
            if( str == NULL ) return false;
            
            attr_len = readbytes(buf, 4, &e); if( e != LD_OK ) return false;

            if( str == string_pool_lookup( kenv->string_pool, "Code")){
                if( parse_code(kclass, kmethod, buf) == false ) return false;

            }else if(str == string_pool_lookup( kenv->string_pool, "Exceptions")){
                if( parse_exceptions(kclass, kmethod, buf) == false ) return false;            

            }else{ /* unrecognized */
                uint32 t;
                for( t=0; t<attr_len; t++ ){
                    readbytes(buf, 1, &e); if( e != LD_OK ) return false;
                } 
            }

        }/* for each attribute */

        method_initialize(kclass, kmethod);
        method_info->value[i] = (korp_object *)kmethod;
    }/* for each method */

    kclass->method_info = method_info;

    return true;
}/* parse_method */

static bool parse_innerclasses(korp_class *kclass, 
                               class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;
    korp_ref_array* innerclass_info;
    korp_innerclass* innerclass;
    korp_string* str;
    uint16 i; 
    uint16 num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    
    innerclass_info = new_vm_array(korp_ref_array, num);

    for(i=0; i<num; i++){
        innerclass = new_vm_object(korp_innerclass);

        str = cp_class_read_name(cpool, buf, &e); if( e != LD_OK ) return false;
        if( str != NULL )
            innerclass->inner_classname = str;

        str = cp_class_read_name(cpool, buf, &e); if( e != LD_OK ) return false;
        if( str != NULL )
            innerclass->outer_classname = str;
        
        str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false;
        if( str != NULL )
            innerclass->inner_name = str;

        innerclass->modifier = readbytes(buf, 2, &e); if( e != LD_OK ) return false;

    }/* for each innerclass */

    return true;

}/* parse_innerclasses */

static bool parse_attributes(korp_class *kclass, 
                             class_buffer *buf)
{
    korp_loader_exception e = LD_OK;

    korp_ref_array *cpool = kclass->constant_pool;
    korp_object **cp_slot = cpool->value;
    uint32 cp_size = (uint32)GET_VM_ARRAY_LENGTH( (korp_ref_array *)cpool );
    uint32 attr_len = 0;
    korp_string* str;

    uint16 i; 
    uint16 num = readbytes(buf, 2, &e); if( e != LD_OK ) return false;    

    for(i=0; i<num; i++){
        uint32 attr_len=0;
        str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false;
        if( str == NULL ) return false;
        
        attr_len = readbytes(buf, 4, &e); if( e != LD_OK ) return false;

        if( str == string_pool_lookup( kenv->string_pool, "SourceFile")){
            str = cp_utf8_read_string(cpool, buf, &e); if( e != LD_OK ) return false;            
            if( str == NULL ) return false;
            kclass->sourcefile_name = str;

        }else if(str == string_pool_lookup( kenv->string_pool, "InnerClasses")){
            
            if( parse_innerclasses(kclass, buf) == false )
                return false;

        }else{ /* unrecognized */
            uint32 t;
            for(t=0; t<attr_len; t++ ){
                readbytes(buf, 1, &e); if( e != LD_OK ) return false;
            }        
        }

    }/* for each attribute */

    return true;

}/* parse_attributes */

bool class_parse(korp_class* kclass, 
                 const char* classfile, 
                 int len,
                 korp_loader_exception* exc)
{
    uint16 major;
    class_buffer buffer = { (const uint8 *)classfile, (uint8 *)classfile, len }; 
    class_buffer *buf = &buffer;

    korp_loader_exception e = LD_OK;

    *exc = LD_ClassFormatError;

    /* magic */
    if( readbytes(buf, 4, &e) != CLASSFILE_MAGIC ) return false;

    /* minor */
    readbytes(buf, 2, &e); if( e != LD_OK ) return false;

    /* major */
    major = readbytes(buf, 2, &e);
    if( major != CLASSFILE_MAJOR && major != CLASSFILE_MAJOR+1 ) return false;

    /* constant pool */
    if( parse_constant_pool(kclass, buf) == false ) return false;

    /* modifiers */
    kclass->modifier = (uint16)readbytes(buf, 2, &e); if( e != LD_OK ) return false;

    /* this class */
    if( parse_this_class(kclass, buf, &e) == false ) return false;

    /* super class */
    if( parse_super_class(kclass, buf) == false ) return false;

    /* parse interfaces */
    if( parse_interfaces(kclass, buf) == false ) return false;

    /* parse fields */
    if( parse_fields(kclass, buf) == false ) return false;

    /* parse methods */
    if( parse_methods(kclass, buf) == false ) return false;

    /* parse classfile attributes */
    if( parse_attributes(kclass, buf) == false ) return false;

    *exc = LD_OK;
    return true;
}


