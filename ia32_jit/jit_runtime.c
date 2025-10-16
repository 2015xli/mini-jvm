
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "vm_object.h"
#include "jit_runtime.h"
#include "exception.h"
#include "compile.h"

korp_java_call getaddress_native_to_java_call()
{
    korp_code* code;
    uint8* buf;
    korp_ia32_mem mem, *kmem = &mem;
    int buf_size = 60;
    uint8* branch_to_push_more;
    uint8* patch_to_finish_push;
    uint8* patch_to_no_exception;

    code = kenv->trampos->native_to_java_call;
	if(code != NULL ) {
        return (korp_java_call)(uint8*)(code->value);
    }

    code = new_vm_array(korp_code, buf_size);
    buf = code->value;
    
    /* start stack frame */
    buf = push(buf, ebp_reg, r_d);
    buf = mov(buf, ebp_reg, esp_reg, rr_d);

    /* save callee registers */
    buf = push(buf, ebx_reg, r_d);
    buf = push(buf, esi_reg, r_d);
    buf = push(buf, edi_reg, r_d);

    /* void (*kjava_call)( uint32 arg_words, uint32 *p_args, 
                            void *entry, uint32 arg_bytes,
                            uint32 *p_eax_var, uint32 *p_edx_var);
    */

    buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, ebp_reg, 0x8), rm_d );  /*  arg_words */
    buf = mov(buf, eax_reg, (uint32)mem_base_opnd(kmem, ebp_reg, 0xc), rm_d ); /*  p_args */

    branch_to_push_more = buf;

/* 
    push_more_args:
*/
    buf = binalu(buf, op_or, ecx_reg, ecx_reg, rr_d );  /* if arg_words==0 ? */
    buf = branch(buf, cc_eq, 0, i_b);       /* je finish_push */
    patch_to_finish_push = buf - 1;

    buf = push(buf, (uint32)mem_base_opnd(kmem, eax_reg, 0), m_d);
    buf = unialu(buf, op_dec, ecx_reg, r_d);
    buf = binalu(buf, op_add, eax_reg, 4, ri_d);
    buf = jump(buf, branch_to_push_more-buf-2, i_b);

/*    
    finish_push:
*/
    *patch_to_finish_push = buf-patch_to_finish_push-1;

    buf = call(buf, (uint32)mem_base_opnd(kmem, ebp_reg, 0x10), m_d );  /* method entry */
    buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, ebp_reg, 0x18), rm_d ); /* push retvals */
    buf = mov(buf, (uint32)mem_base_opnd(kmem, ecx_reg, 0), eax_reg, mr_d );
    buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, ebp_reg, 0x1c), rm_d );
    buf = mov(buf, (uint32)mem_base_opnd(kmem, ecx_reg, 0), edx_reg, mr_d );

    buf = call(buf, (uint32)get_last_exception, TARGET );
    buf = binalu(buf, op_or, eax_reg, eax_reg, rr_d);
    buf = branch(buf, cc_eq, 0, i_b); /* je no_exception */
    patch_to_no_exception = buf - 1;

    buf = binalu(buf, op_add, esp_reg, (uint32)mem_base_opnd(kmem, ebp_reg, 0x14), rm_d ); /* arg_bytes */
/*
    no_exception:
*/
    *patch_to_no_exception = buf - patch_to_no_exception - 1;

    buf = pop(buf, edi_reg, r_d);
    buf = pop(buf, esi_reg, r_d);
    buf = pop(buf, ebx_reg, r_d);
    buf = pop(buf, ebp_reg, r_d);
    buf = ret(buf, 0);

    assert((buf - code->value) <= buf_size);
    
    kenv->trampos->native_to_java_call = code;

    return (korp_java_call)(uint8*)(code->value);
}

korp_code* generate_compile_me_stub_ia32(korp_method* kmethod)
{
    /* korp_ia32_mem mem, *kmem = &mem; */
    int buf_size = 12;
    korp_code* code = new_vm_array(korp_code, buf_size);
    uint8* buf = (uint8*)code->value;

    buf = push(buf, (uint32)kmethod, i_d);
    buf = call(buf, (uint32)jit_a_method, TARGET);
    buf = jump(buf, eax_reg, r_d);

    assert((buf - code->value) <= buf_size);

    kmethod->jitted_code = code; // No use. code will be returned and jitted_code will be set outside again
    return code;
}

korp_code* generate_java_to_native_wrapper( korp_method* kmethod, uint8* funcptr)
{
    korp_code* code;
    uint8* buf;
    int offset, buf_size = 60;
    uint32 arg_words;
    const char* descriptor = string_to_chars( kmethod->descriptor );
    korp_ia32_mem mem, *kmem = &mem;
    char c;

    method_descriptor_get_param_info( kmethod->descriptor, &arg_words, (korp_java_type*)&c);
    code = new_vm_array(korp_code, buf_size);
    buf = (uint8*)code->value;
    
    if(!method_is_static(kmethod)) arg_words++;

    offset = arg_words * BYTES_OF_INT;

    if(!method_is_static(kmethod)){
        buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, esp_reg, offset), rm_d);
        buf = mov(buf, (uint32)mem_base_opnd(kmem, esp_reg, -offset), ecx_reg, mr_d);
        offset -= BYTES_OF_INT;
    }

    descriptor++;
    while( (c = *descriptor++) !=')'){
        switch( c ){
            case JAVA_TYPE_ARRAY:
                while( *descriptor == JAVA_TYPE_ARRAY ){
					descriptor++;
				}
                if(*descriptor++ != JAVA_TYPE_CLASS ){
                    buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, esp_reg, offset), rm_d);
                    buf = mov(buf, (uint32)mem_base_opnd(kmem, esp_reg, -offset), ecx_reg, mr_d);
                    offset -= BYTES_OF_INT;
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
                 buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, esp_reg, offset), rm_d);
                 buf = mov(buf, (uint32)mem_base_opnd(kmem, esp_reg, -offset), ecx_reg, mr_d);
                 offset -= BYTES_OF_INT;
                 break;

            case JAVA_TYPE_DOUBLE:
            case JAVA_TYPE_LONG:
                 buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, esp_reg, offset), rm_d);
                 buf = mov(buf, (uint32)mem_base_opnd(kmem, esp_reg, 4-offset), ecx_reg, mr_d);
                 offset -= BYTES_OF_INT;
                 buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, esp_reg, offset), rm_d);
                 buf = mov(buf, (uint32)mem_base_opnd(kmem, esp_reg, 8-offset), ecx_reg, mr_d);
                 offset -= BYTES_OF_INT;
                 break;

            default:
                return false;
        }
    }

    buf = binalu(buf, op_sub, esp_reg, arg_words*4, ri_d);
    if( method_is_static(kmethod))
        buf = push(buf, (uint32)kmethod->owner_class, i_d);
    else{
    }
    
    buf = call(buf, (uint32)funcptr, TARGET);
    buf = ret(buf, arg_words*4);

    assert((buf - code->value) <= buf_size);

    return code;    
}
