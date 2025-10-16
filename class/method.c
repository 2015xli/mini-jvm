
#include <string.h>
#include <stdio.h>

#include "korp_types.h"
#include "method.h"
#include "vm_interface.h"
#include "utils.h"
#include "runtime.h"

korp_handler* method_get_handler(korp_method* kmethod, uint16 handler_id)
{
    return (korp_handler*)kmethod->handler_info->value[handler_id];
}

uint8* method_get_bincode_entry( korp_method* kmethod )
{
    return (uint8*)(kmethod->jitted_code->value);
}

uint8* method_get_bincode_indirect_entry( korp_method* kmethod )
{
    return (uint8*)&(kmethod->jitted_code->value);
}

uint8* method_get_bytecode_entry( korp_method* kmethod )
{
    return (uint8*)(kmethod->byte_code->value);
}

bool method_initialize(korp_class* kclass, korp_method* kmethod)
{
   /* clinit init finalize max_locals ... */
    //if( METHOD_BYTE_CODE_LENGTH(kmethod) == 0 ){
    //    return false;
    //}

    kmethod->owner_class = kclass;
	//printf("initialize method %s for class %s\n", kmethod->name->byte, kclass->name->byte); //chunrong

    method_init_bincode(kmethod);

    return true;
}

bool method_get_param_num_size(korp_method* kmethod, uint32* arg_num, uint32* arg_size)
{
    uint32 size, num;
    const char* descriptor = string_to_chars(kmethod->descriptor);
    char c;
    size = 0; num = 0;
    descriptor++;
    while( (c = *descriptor++) !=')'){
        switch( c ){
            case JAVA_TYPE_ARRAY:
                while( *descriptor == JAVA_TYPE_ARRAY ){
					descriptor++;
				}
                if(*descriptor++ != JAVA_TYPE_CLASS ){
                    size+=4; num++;
                    break;                
                }

            case JAVA_TYPE_CLASS:
                descriptor = strchr(descriptor, ';');
                descriptor++;
            case JAVA_TYPE_BYTE:
            case JAVA_TYPE_CHAR:
            case JAVA_TYPE_FLOAT:
            case JAVA_TYPE_SHORT:
            case JAVA_TYPE_BOOLEAN:
            case JAVA_TYPE_INT:
                 size+=4; num++;
                 break;

            case JAVA_TYPE_DOUBLE:
            case JAVA_TYPE_LONG:
                 size+=8; num++;
                 break;

            default:
                return false;
        }        
    }
    
    *arg_num = num;
    *arg_size = size;
    return true;
}

bool method_descriptor_get_param_info(korp_string* kdesc, 
                                      uint32* n_words, 
                                      korp_java_type* ret_type)
{
    uint32 size = 0;
    const char* descriptor = string_to_chars(kdesc);
    char c;
    descriptor++;
    while( (c = *descriptor++) !=')'){
        switch( c ){
            case JAVA_TYPE_ARRAY:
                while( *descriptor == JAVA_TYPE_ARRAY ){
					descriptor++;
				}
                if(*descriptor++ != JAVA_TYPE_CLASS ){
                    size++;
                    break;                
                }

            case JAVA_TYPE_CLASS:
                descriptor = strchr(descriptor, ';');
                descriptor++;
            case JAVA_TYPE_BYTE:
            case JAVA_TYPE_CHAR:
            case JAVA_TYPE_FLOAT:
            case JAVA_TYPE_SHORT:
            case JAVA_TYPE_BOOLEAN:
            case JAVA_TYPE_INT:
                 size++;
                 break;

            case JAVA_TYPE_DOUBLE:
            case JAVA_TYPE_LONG:
                 size+=2;
                 break;

            default:
                return false;
        }        
    }
    
    *n_words = size;
    *ret_type = (korp_java_type)*descriptor;
    return true;
}

