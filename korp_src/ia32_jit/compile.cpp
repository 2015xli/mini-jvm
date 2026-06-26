
#include <assert.h>
#include <stdio.h>

#include "vm_object.h"
#include "method.h"
#include "compile.h"
#include "utils.h"

uint8* codebuf_get(korp_codebuf* kcodebuf)
{
    return (uint8*)kcodebuf->pos;
}

korp_codebuf* codebuf_set( korp_codebuf* kcodebuf, uint8* offset)
{
    int bufsize = GET_VM_ARRAY_LENGTH(kcodebuf->buf);
    int codesize;
    korp_code* newbuf;
    uint8* codebuf = (uint8*)kcodebuf->buf->value;

    codesize = offset - codebuf;

    /* check if there is enough buf left, 100 bytes is for all instructions of one bytecode */
    if( ( bufsize - codesize) < 100 ){        
        /* create new codebuf */
        newbuf = new_vm_array(korp_code, bufsize*2);
        memcpy(newbuf->value, codebuf, bufsize);
        free_vm_object((korp_object*)kcodebuf->buf);
        kcodebuf->buf = newbuf;
    }
    
    kcodebuf->pos = kcodebuf->buf->value + codesize;
    return kcodebuf;
    
}

static void gen_prolog(korp_comp_info* info, korp_codebuf* kcodebuf)
{
    uint8* buf = codebuf_get(kcodebuf);

    buf = push(buf, ebp_reg, r_d);
    buf = mov(buf, ebp_reg, esp_reg, rr_d);
    buf = push(buf, (uint32)info->method, i_d);
    buf = binalu(buf, op_add, esp_reg, info->frame_size+4, ri_d); /* esp changed 4 by last push */
    
    codebuf_set(kcodebuf, buf);

    return;
}

static void gen_epilog(korp_comp_info* info, korp_codebuf* kcodebuf)
{
    uint8* buf = codebuf_get(kcodebuf);
    korp_ia32_mem mem, *kmem=&mem;
    uint32 arg_num, arg_size;
    korp_method* kmethod = info->method;

    method_get_param_num_size(kmethod, &arg_num, &arg_size);
    
    if( !method_is_static(kmethod)) arg_size += 4;

    buf = mov(buf, esp_reg, ebp_reg, rr_d);
    buf = pop(buf, ebp_reg, r_d);
    buf = ret(buf, arg_size);
    
    codebuf_set(kcodebuf, buf);

    return;   
}

static void prepare_stackframe(korp_comp_info* info)
{
    korp_method* kmethod = info->method;
    uint32 args_num, arg_bytes, arg_words, offset, i;
    uint32 locals_num, stack_depth;

    method_get_param_num_size( kmethod, &args_num, &arg_bytes);
    arg_words = arg_bytes/4; 

    if( !method_is_static(kmethod) ){
        arg_words++;
    }

    locals_num = kmethod->max_locals; 

    /* set locals offset in stack frame */
    offset = (arg_words+2) * BYTES_OF_INT; /* account for saved eip and ebp */
    i=0;
    /* first one is this pointer */
    if( !method_is_static(kmethod) ){
        offset -= BYTES_OF_INT;
        info->locals_info->value[i] = offset;
        i++;
    }

    /* then go through arguments */
    /* double and long can never be used on its words seperately */
    for(; i<arg_words; i++){
        offset -= BYTES_OF_INT;
        info->locals_info->value[i] = offset; 
    }
    
    /* next go through rest local variables */
    offset = -BYTES_OF_INT; /* account for saved method korp_comp_info */
    //offset -= BYTES_OF_INT; /* account for saved method korp_comp_info */
    for(; i<locals_num; i++){
        offset -= BYTES_OF_INT;
        info->locals_info->value[i] = offset; 
    }

    /* continue from above, put some extra info in between */
    offset -= BYTES_OF_INT; 
    
    /* stack is used directly as evaluation stack,
       frame doesn't include it. 
     */
    info->frame_size = offset;
    
    /* now setup evaluation stack area */
    stack_depth = kmethod->max_stack;
    for(i=0; i<stack_depth; i++){
        offset -= BYTES_OF_UINT32;
        info->stack_info->value[i] = offset;
    }    
    
    return;
}

#if 0
/* not used */
static void prepare_stackframe_old(korp_comp_info* info)
{
    korp_method* kmethod = info->method;
    uint32 args_num=0, arg_bytes=0, offset, i;
    uint32 locals_num, stack_depth;
    const char* descriptor = string_to_chars(kmethod->descriptor);
    char c;

    method_get_param_num_size( kmethod, &args_num, &arg_bytes);

    if( !method_is_static(kmethod) ){
        arg_bytes++;
    }

    locals_num = kmethod->max_locals; 

    /* set locals offset in tack frame */
    offset = arg_bytes + BYTES_OF_INT; /* account for saved eip */
    i=0;
    /* first one is this pointer */
    if( !method_is_static(kmethod) ){
        offset -= BYTES_OF_INT;
        info->locals_info->value[i*2] = offset;
        i++;
    }

    /* then go through arguments */
    /* double and long can never be used on its words seperately */
    descriptor++; /* skip open bracket */
    while((c=*descriptor++) != ')' ){
        arg_bytes = typesize_type( (korp_java_type)c );
        offset -= arg_bytes;
        info->locals_info->value[i*2] = offset; 
        info->locals_info->value[i*2+1] = c; 
        if( arg_bytes == BYTES_OF_LONG )
            i++; 
    }
    
    /* next go through rest local variables, the size info is got from bytecode parsing */
    offset = -BYTES_OF_UINT32; /* account for saved ebp */
    //offset -= BYTES_OF_UINT32; /* account for saved ebp */
    for(; i<locals_num; i++){
        arg_bytes = typesize_type((korp_java_type)(info->locals_info->value[i*2+1]) );
        offset -= arg_bytes;      
        info->locals_info->value[i*2] = offset;
        if( arg_bytes == BYTES_OF_LONG )
            i++; 
    }

    /* now setup evaluation stack area */
    /* continue from above, put some extra info in between */
    offset -= BYTES_OF_UINT32; 
    stack_depth = kmethod->max_stack;
    for(i=0; i<stack_depth; i++){
        offset -= BYTES_OF_UINT32;
        info->stack_info->value[i] = offset;
    }    
    
    return;
}
#endif

static bool generate_code(korp_comp_info* info)
{
    uint32 num, i, codesize;
    uint8 *start;
    korp_codebuf codebuf, *kcodebuf=&codebuf;
    korp_method* kmethod = info->method;
    korp_stack kstack, *stack;
    uint16 code_len = METHOD_BYTE_CODE_LENGTH(kmethod);
    korp_bcinst_info* bcinst_info = info->bcinst_info;

    /* init temporary mimic stack */
    stack = &kstack;
//  stack->slot = (uint32*)stack_alloc( kmethod->max_stack*BYTES_OF_UINT32 );
    stack->slot = (uint32*)stack_alloc( (kmethod->max_stack + 2)*BYTES_OF_UINT32 );
	//xiaofeng
    stack_reset( stack );

    codesize = code_len*8;
    kcodebuf->buf = new_vm_array( korp_code, (100>codesize)?100:codesize );
    kcodebuf->pos = kcodebuf->buf->value;

    gen_prolog(info, kcodebuf);

    start = method_get_bytecode_entry(kmethod);
    /* parse main body of the method */
    select_code( info, start, kcodebuf, stack );
    

    /* parse exception handler of the method */
    num = METHOD_HANDLER_NUM( kmethod);
    for(i=0; i<num; i++){
        korp_handler* handler;
        handler = method_get_handler(kmethod, i);
        stack_reset( stack );
        stack_push( stack, YES_REF );
        select_code(info, start + handler->handler_pc,  kcodebuf, stack );
    }

    bcinst_info[code_len].native_offset = kcodebuf->pos - kcodebuf->buf->value;

    gen_epilog(info, kcodebuf);

    for(i=0; i<code_len; i++){
        uint16 branch_target = bcinst_info[i].branch_target;
        if( branch_target != 0 ){
            int32 patch_offset = bcinst_info[i].patch_offset;
            uint32* pos = (uint32 *)(kcodebuf->buf->value + patch_offset - 4 );
            *pos = bcinst_info[branch_target].native_offset - patch_offset;
            if( bcinst_info[i].is_jsr ){
                uint32 next_inst = kenv->bytecode_inst_len[start[i]] + i;
                pos = (uint32 *)(kcodebuf->buf->value + patch_offset - 9 );
                *pos = bcinst_info[next_inst].native_offset + (uint32)kcodebuf->buf->value;
            }
        }
    }
    
    kmethod->jitted_code = kcodebuf->buf;

    return true;
}

bool compile(korp_method *kmethod)
{
    korp_comp_info kcomp_info = {NULL, 0};
    korp_comp_info* info = &kcomp_info;
    uint16 code_len;

    info->method = kmethod;
    info->byte_code = method_get_bytecode_entry(kmethod);
    code_len = METHOD_BYTE_CODE_LENGTH(kmethod);
    info->bcinst_info = (korp_bcinst_info *)stack_alloc(sizeof(korp_bcinst_info) * (code_len+1) );    
    /* set the last slot as visited, so as to ease the bound checking */
    info->bcinst_info[code_len].is_visited = 1;

    info->locals_info = new_vm_array(korp_int16_array, kmethod->max_locals );
    info->stack_info = new_vm_array(korp_int16_array, kmethod->max_stack );
    
    /* currently our o0 ia32 jit needs only one pass
    prepass_bytecode(info ); 
    */
    
    prepare_stackframe(info);

	//printf("%s.%s\n", kmethod->owner_class->name->byte,kmethod->name->byte);

    generate_code(info);

    return true;
}
