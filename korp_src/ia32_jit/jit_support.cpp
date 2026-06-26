
#include <assert.h>
#include <stdio.h>

#ifdef _DEBUG
#include <string.h>
#endif

#include "method.h"
#include "natives.h"
#include "exception.h"
#include "compile.h"
#include "jit_runtime.h"

uint8 read_uint8_index( uint8* bc )
{
    return bc[0];
}

uint16 read_uint16_index( uint8* bc )
{
    return ((uint16)(bc[0]<<8) | (uint16)bc[1]) ;
}

int8 read_int8_value( uint8* bc )
{
    return ((int8*)bc)[0];
}

int16 read_int16_offset( uint8* bc )
{
    return (int16)((uint16)(bc[0]<<8) | (uint16)bc[1]) ;
}

int32 read_int32_offset( uint8* bc )
{
    return (int)( (uint32)(bc[0]<<24) | (uint32)bc[1]<<16  
                    | (uint32)(bc[2]<<8) | (uint32)bc[3] ) ;
}

static void method_update_vtable(korp_method* kmethod, uint8* funcptr)
{
    uint8** vtable_slot = (uint8**)((uint8*)kmethod->owner_class + kmethod->offset);

        *vtable_slot = funcptr;
    
    return;
}

void* __stdcall jit_a_method(korp_method* kmethod)
{
    bool ok;
    uint8* funcptr= NULL;

#ifdef _DEBUG
    if( strcmp(string_to_chars(kmethod->name), "toString")==0 
        && strcmp(string_to_chars(kmethod->owner_class->name), "java/lang/Integer")==0 
      )
    {

        int k =0;
    }    
#endif

    /* when it is being compiled, no other method can hold the lock 
     * when it holds the lock, the method is either Compiled or NotCompiled 
     */

    /* Test: if already compiled, simply return */
    /* exclusive compilation */
    if( kmethod->state == MS_Compiled ) return method_get_bincode_entry(kmethod);
    if( kmethod->state == MS_Compiled ){
        return method_get_bincode_entry(kmethod);
    }

    kmethod->state = MS_BeingCompiled;
    class_initialize( kmethod->owner_class );

    if( method_is_native(kmethod) || method_forced_native(kmethod) ){
        funcptr = method_lookup_native_entry(kmethod, true);
        if( funcptr == NULL ){
            korp_string* kexcname = string_pool_lookup(kenv->string_pool,"java/lang/UnsatisfiedLinkError");
            throw_exception_name_native( kexcname );
            assert(0);
        }

        
    }else{
        ok = compile(kmethod);
        if( ok == false){
            korp_string* kexcname = string_pool_lookup(kenv->string_pool, "java/lang/InternalError");
            throw_exception_name_native( kexcname );
            assert(0);
        }
    }

    funcptr = method_get_bincode_entry(kmethod);
    method_update_vtable(kmethod, funcptr);
    kmethod->state = MS_Compiled;
    return funcptr;
}

/*
    void (*kjava_call)( uint32 arg_words, uint32 *p_args, 
                   void *entry, uint32 arg_bytes,
                   uint32 *p_eax_var, uint32 *p_edx_var);
*/
void execute_java_method_ia32(korp_method* kmethod, uint32* args, uint32* retval)
{
    uint32 eax_var, edx_var;
    korp_java_call kjava_call;

    uint32 arg_words;
    korp_java_type ret_type;
    korp_2uint32 *ret_val;
    uint8* entry; 

    method_descriptor_get_param_info(kmethod->descriptor, &arg_words, &ret_type);
    entry = method_get_bincode_entry(kmethod);

    kjava_call = getaddress_native_to_java_call();

    (*kjava_call)(arg_words, args, entry, arg_words*BYTES_OF_INT, &eax_var, &edx_var);

    /* handle return value */
	if (retval != NULL)
	{
		ret_val = new_vm_object(korp_2uint32);
		ret_val->value[0] = eax_var;
		ret_val->value[1] = edx_var;
		//retval = ret_val->value;
		*retval = (uint32)(ret_val->value);
	}
}
