
#include <assert.h>
#include <string.h>

#include "compile.h"
#include "exception.h"

#define select_code_branch(X) 1

static korp_ia32_mem* localvar_opnd(korp_comp_info* info, korp_ia32_mem* kmem, korp_stack* stack, uint16 index)
{
    return mem_opnd(kmem, IS_BASE_DISP, 0, 0, ebp_reg, (int32)info->locals_info->value[index]);
} 

static korp_ia32_mem* stack_opnd(korp_comp_info* info, korp_ia32_mem* kmem, korp_stack* stack, uint16 index)
{
    return mem_opnd(kmem, IS_BASE_DISP, 0, 0, ebp_reg, (int32)info->stack_info->value[stack_depth(stack)-index]);
} 

#define localvar_ith_opnd(index) localvar_opnd(info, kmem, stack, index)  

#define stack_ith_opnd(index) stack_opnd(info, kmem, stack, index)

#define stack_top_opnd() stack_opnd(info, kmem, stack, 1)

static uint8* stack_push_const(uint8* buf, 
                               korp_comp_info* info, 
                               korp_stack* estack,
                               int32 val,
                               bool is_ref)
{
/*
    korp_ia32_mem mem, *stack_top_opnd;
    int16 stack_top_offset = info->stack_info->value[stack_depth(estack)-1];
    
    stack_push( estack, is_ref );
    stack_top_opnd =  mem_opnd(&mem, IS_BASE_DISP, 0, 0, ebp_reg, stack_top_offset);
    buf = mov( buf, (uint32)stack_top_opnd, val, mi_d );  
    return buf;
*/
    stack_push( estack, is_ref );
    buf = push( buf, val, i_d );
    return buf;
}

static uint8* stack_push_reg(uint8* buf, 
                             korp_comp_info* info, 
                             korp_stack* estack,
                             uint8 reg,
                             bool is_ref)
{
/*
    korp_ia32_mem mem, *stack_top_opnd;
    int16 stack_top_offset;
    
    stack_push( estack,  is_ref);
    stack_top_offset = info->stack_info->value[stack_depth(estack)-1];
    stack_top_opnd =  mem_opnd(&mem, IS_BASE_DISP, 0, 0, ebp_reg, stack_top_offset);
    buf = mov( buf, (uint32)stack_top_opnd, reg, mr_d );  
    return buf;
*/
    stack_push( estack, is_ref );
    buf = push( buf, reg, r_d );
    return buf;
}

static uint8* stack_pop_reg(uint8* buf, 
                            korp_comp_info* info, 
                            korp_stack* estack,
                            uint8 reg)
{
/*
    korp_ia32_mem mem, *stack_top_opnd;
    int16 stack_top_offset;
    
    stack_top_offset = info->stack_info->value[stack_depth(estack)-1];
    stack_top_opnd =  mem_opnd(&mem, IS_BASE_DISP, 0, 0, ebp_reg, stack_top_offset);
    buf = mov( buf, reg, (uint32)stack_top_opnd, rm_d );  
    stack_pop( estack);
    return buf;
*/
    buf = pop( buf, reg, r_d );
    stack_pop( estack );
    return buf;
}

static uint8* local_var_load(uint8* buf, 
                             korp_comp_info* info, 
                             korp_stack* estack,
                             uint16 index,
                             bool is_ref)
{
    korp_ia32_mem mem, *local_var_opnd;
    int16 local_var_offset;

    local_var_offset = info->locals_info->value[index];
    local_var_opnd = mem_opnd(&mem, IS_BASE_DISP, 0, 0, ebp_reg, local_var_offset);
    buf = mov(buf, eax_reg, (uint32)local_var_opnd, rm_d); 
    buf = stack_push_reg(buf, info, estack, eax_reg, is_ref ); 
    return buf;
}

static uint8* local_var_store(uint8* buf, 
                              korp_comp_info* info, 
                              korp_stack* estack,
                              uint16 index)
{
    korp_ia32_mem mem, *local_var_opnd;
    int16 local_var_offset;

    buf = stack_pop_reg(buf, info, estack, eax_reg);
    local_var_offset = info->locals_info->value[index];
    local_var_opnd = mem_opnd(&mem, IS_BASE_DISP, 0, 0, ebp_reg, local_var_offset);
    buf = mov(buf, (uint32)local_var_opnd, eax_reg, mr_d); 
    return buf;
}


void select_code(korp_comp_info* info, 
                 uint8* current_inst, 
                 korp_codebuf* kcodebuf, 
                 korp_stack* stack)
{

    korp_method* kmethod = info->method;
    korp_class* kclass = kmethod->owner_class;

    int32 offset; /* relative branch distance */
    int32 index;   /* index in constant pool or localvars */
    int32 value; /* temp value used */
    uint8 bytecode; /* butecode content */
    korp_ia32_mem mem, *kmem = &mem; /* temp space for mem oprand */

    uint8* first_bc = info->byte_code;
    korp_bcinst_info* bcinst_info = info->bcinst_info;
    korp_bcinst_info* cur_bcinst_info = &bcinst_info[current_inst-first_bc];


#ifdef _DEBUG
    if( strcmp(string_to_chars(kmethod->name), "toString")==0 
        && strcmp(string_to_chars(kmethod->owner_class->name), "java/lang/Integer")==0 
      )
    {
        int k =0;
    }    
#endif

    if( cur_bcinst_info->is_visited ) return;
    
    /* loop for each bytecode in current BB */
    while(! cur_bcinst_info->is_visited ) {
        /* get the codebuf for this bytecode */
        uint8* buf = codebuf_get(kcodebuf);
        uint8* next_inst;
        next_inst = 0;
        cur_bcinst_info->native_offset = buf - kcodebuf->buf->value;
        cur_bcinst_info->is_visited = 1;
        bytecode = *current_inst;
        
        switch (bytecode) {
        case NOP:
            break;

        /* constant loads */
        case ACONST_NULL:
            buf = stack_push_const(buf, info, stack, 0, YES_REF);
            break;
        case ICONST_M1: 
        case ICONST_0: 
        case ICONST_1: 
        case ICONST_2:
        case ICONST_3: 
        case ICONST_4: 
        case ICONST_5:
            value = bytecode - ICONST_0;
            buf = stack_push_const(buf, info, stack, value, NOT_REF);
            break;
        case LCONST_0: 
        case LCONST_1:
            value = bytecode - LCONST_0;
            buf = stack_push_const(buf, info, stack, 0, NOT_REF);
            buf = stack_push_const(buf, info, stack, value, NOT_REF);
            break;
        case FCONST_0: 
        case FCONST_1: 
        case FCONST_2:
            value = bytecode - FCONST_0;
            buf = stack_push_const(buf, info, stack, value, NOT_REF);
            break;
        case DCONST_0: 
        case DCONST_1:
            value = bytecode - DCONST_0;
            buf = stack_push_const(buf, info, stack, 0, NOT_REF);
            buf = stack_push_const(buf, info, stack, value, NOT_REF);
            break;
        
        /* stack pushes */
        case BIPUSH:
            value = read_uint8_index( current_inst+1 );
            buf = stack_push_const(buf, info, stack, value, NOT_REF);
            break;
        case SIPUSH:
            value = read_uint16_index( current_inst+1 );
            buf = stack_push_const(buf, info, stack, value, NOT_REF);
            break;
        case LDC:{
            korp_java_type type;
            korp_object* kvalue;

            index = read_uint8_index( current_inst+1 );
            type = cp_get_const_type(kclass->constant_pool, index);
            kvalue = cp_get_const_value(kclass->constant_pool, index);
            if( type == JAVA_TYPE_STRING )
                kvalue = (korp_object*)get_jstring_from_kstring( (korp_string*)kvalue );

            if( is_ref_type_type(type) ){
                buf = stack_push_const(buf, info, stack, (uint32)kvalue, YES_REF);
            }else{
                buf = stack_push_const(buf, info, stack, ((korp_uint32 *)kvalue)->value, NOT_REF);
            }

            }break;
        case LDC_W:{
            korp_java_type type;
            korp_object* kvalue;

            index = read_uint16_index( current_inst+1 );
            type = cp_get_const_type(kclass->constant_pool, index);
            kvalue = cp_get_const_value(kclass->constant_pool, index);
            if( type == JAVA_TYPE_STRING )
                kvalue = (korp_object*)get_jstring_from_kstring( (korp_string*)kvalue );
            if( is_ref_type_type(type) ){
                buf = stack_push_const(buf, info, stack, (uint32)kvalue, YES_REF);
            }else{
                buf = stack_push_const(buf, info, stack, ((korp_uint32 *)kvalue)->value, NOT_REF);
            }
            
            }break;
        case LDC2_W:{
            korp_object* kvalue;
            index = read_uint16_index( current_inst+1 );
            kvalue = cp_get_const_value(kclass->constant_pool, index);
            buf = stack_push_const(buf, info, stack, ((korp_2uint32 *)kvalue)->value[1], NOT_REF);
            buf = stack_push_const(buf, info, stack, ((korp_2uint32 *)kvalue)->value[0], NOT_REF);
            }break;
        case ILOAD:
        case FLOAD:
            index = read_uint8_index( current_inst+1 );
            buf = local_var_load(buf, info, stack, index, NOT_REF);
            break;
        case ALOAD:
            index = read_uint8_index( current_inst+1 );
            buf = local_var_load(buf, info, stack, index, YES_REF);
            break;
        case LLOAD:
        case DLOAD:
            index = read_uint8_index( current_inst+1 );
            buf = local_var_load(buf, info, stack, index+1, NOT_REF);
            buf = local_var_load(buf, info, stack, index, NOT_REF);
            break;
        case ILOAD_0: 
        case ILOAD_1: 
        case ILOAD_2: 
        case ILOAD_3:
            index = bytecode-ILOAD_0;
            buf = local_var_load(buf, info, stack, index, NOT_REF);
            break;
        case FLOAD_0: 
        case FLOAD_1: 
        case FLOAD_2: 
        case FLOAD_3:
            index = bytecode-FLOAD_0;
            buf = local_var_load(buf, info, stack, index, NOT_REF);
            break;
        case ALOAD_0: 
        case ALOAD_1: 
        case ALOAD_2: 
        case ALOAD_3:
            index = bytecode-ALOAD_0;
            buf = local_var_load(buf, info, stack, index, YES_REF);
            break;
        case LLOAD_0: 
        case LLOAD_1: 
        case LLOAD_2: 
        case LLOAD_3:
            index = bytecode-LLOAD_0;
            buf = local_var_load(buf, info, stack, index+1, NOT_REF);
            buf = local_var_load(buf, info, stack, index, NOT_REF);
            break;
        case DLOAD_0: 
        case DLOAD_1: 
        case DLOAD_2: 
        case DLOAD_3:
            index = bytecode-DLOAD_0;
            buf = local_var_load(buf, info, stack, index+1, NOT_REF);
            buf = local_var_load(buf, info, stack, index, NOT_REF);
            break;
        
        /* array load */
        case IALOAD:
        case AALOAD:
        case FALOAD:
        case DALOAD:
        case LALOAD:
        case BALOAD:
        case CALOAD:
        case SALOAD:{
            korp_java_type type = (korp_java_type)"IJFDLBCS"[bytecode - IALOAD];
            uint16 n_bytes = typesize_type(type);
            uint8 first_elem_offset = JAVA_ARRAY_FIRST_ELEM_OFFSET;
            uint8 scale = log2( typesize_type(type) );
            int32 disp = first_elem_offset;
            assert( stack_get_ith( stack, 2 ) == YES_REF);
            assert( stack_get_ith( stack, 1) == NOT_REF);
            buf = stack_pop_reg(buf, info, stack, eax_reg); /* get array index */
            buf = stack_pop_reg(buf, info, stack, ecx_reg); /* get array objref */
            buf = binalu(buf, op_xor, edx_reg, edx_reg, rr_d);
            kmem = mem_opnd(kmem, IS_SIB_DISP, scale, eax_reg, ecx_reg, disp); 
            if( n_bytes==1)
                buf = mov(buf, dl_reg, (uint32)kmem, rm_b );
            else if( n_bytes==2)
                buf = mov(buf, dx_reg, (uint32)kmem, rm_w );
            else if( n_bytes==4)
                buf = mov(buf, edx_reg, (uint32)kmem, rm_d );
            else{
                disp = first_elem_offset + BYTES_OF_UINT32;
                kmem = mem_opnd(kmem, IS_SIB_DISP, scale, eax_reg, ecx_reg, disp);
                buf = mov(buf, eax_reg, (uint32)kmem, rm_d );
                buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
                buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);
            }

            if( bytecode == AALOAD )
                    buf = stack_push_reg(buf, info, stack, edx_reg, YES_REF);
            else
                    buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);

            }break;

        /* stack pops */
        case ISTORE: 
        case ASTORE:
        case FSTORE:
            index = read_uint8_index( current_inst+1 );
            buf = local_var_store(buf, info, stack, index);
            break;

        case DSTORE:
        case LSTORE:
            index = read_uint8_index( current_inst+1 );
            buf = local_var_store(buf, info, stack, index);
            buf = local_var_store(buf, info, stack, index+1);
            break;

        case ISTORE_0: 
        case ISTORE_1: 
        case ISTORE_2: 
        case ISTORE_3:
        case LSTORE_0: 
        case LSTORE_1: 
        case LSTORE_2: 
        case LSTORE_3:
        case FSTORE_0: 
        case FSTORE_1: 
        case FSTORE_2: 
        case FSTORE_3:
        case DSTORE_0: 
        case DSTORE_1: 
        case DSTORE_2: 
        case DSTORE_3:
        case ASTORE_0: 
        case ASTORE_1: 
        case ASTORE_2: 
        case ASTORE_3:{
            uint8 basevals[5] = {ISTORE_0, LSTORE_0, FSTORE_0, DSTORE_0, ASTORE_0}; 
            uint8 var_base = basevals[(bytecode - ISTORE_0)/4];
            index = bytecode - var_base; 
            buf = local_var_store(buf, info, stack, index);
            if( var_base==LSTORE_0 || var_base==DSTORE_0 ){
                buf = local_var_store(buf, info, stack, index+1);
            }
                
            }break;

        /* store into array */
        case IASTORE:
        case AASTORE:
        case FASTORE:
        case DASTORE:
        case LASTORE:
        case BASTORE:
        case CASTORE:
        case SASTORE:{
            korp_java_type type = (korp_java_type)"IJFDLBCS"[bytecode - IASTORE];
            /* uint8 first_elem_offset = java_array_first_elem_offset(type); */
            uint8 first_elem_offset = JAVA_ARRAY_FIRST_ELEM_OFFSET;
            uint16 n_bytes = typesize_type(type);
            uint8 scale = log2( n_bytes );
            int32 disp = first_elem_offset;
            uint32 n_words =  (n_bytes+3)/4;

            assert( stack_get_ith( stack, n_words+2 ) == YES_REF); /* array objref */
            assert( stack_get_ith( stack, n_words+1 ) == NOT_REF); /* array index */
            
            buf = mov(buf, ecx_reg, (uint32)stack_ith_opnd(n_words+1), rm_d); /* get array index without pop */
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(n_words+2), rm_d); /* get array objref without pop */

            buf = stack_pop_reg(buf, info, stack, edx_reg); /* get array elem value */
            kmem = mem_opnd(kmem, IS_SIB_DISP, scale, ecx_reg, eax_reg, disp);
            
            if( n_bytes==1 )
                buf = mov(buf, (uint32)kmem, dl_reg, mr_b);
            else if( n_bytes==2 )
                buf = mov(buf, (uint32)kmem, dx_reg, mr_w);
            else if( n_bytes==4 )
                buf = mov(buf, (uint32)kmem, edx_reg, mr_d);
            else{
                assert(n_bytes==8); 
                buf = stack_pop_reg(buf, info, stack, edx_reg); /* get array elem value hi_byte */
                kmem = mem_opnd(kmem, IS_SIB_DISP, scale, ecx_reg, eax_reg, disp+BYTES_OF_UINT32);
                buf = mov(buf, (uint32)kmem, edx_reg, mr_d);
            }
            buf = stack_pop_reg(buf, info, stack, eax_reg); /* drop array index */
            buf = stack_pop_reg(buf, info, stack, eax_reg); /* drop array objref */

            }break;

        /* stack operations */
        case POP:
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            break;
        case POP2:
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            break;
        case DUP:
            buf = mov(buf, eax_reg, (uint32)stack_top_opnd(), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, (stack_top(stack)==1));
            break;
        case DUP_X1:  /* dup_x1: stack: ...,w2,w1 ==> ...,w1,w2,w1 */
            buf = mov(buf, eax_reg, (uint32)stack_top_opnd(), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg,  (stack_top(stack)==1));
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(3), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(2), eax_reg, mr_d);
            stack_set_ith(stack, 2, stack_get_ith(stack, 3)==1);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(1), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(3), eax_reg, mr_d);
            stack_set_ith(stack, 3, stack_get_ith(stack, 1)==1);
            break;
        case DUP_X2:  /* dup_x2: stack: ...,w3,w2,w1 ==> ...,w1,w3,w2,w1 */
            buf = mov(buf, eax_reg, (uint32)stack_top_opnd(), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg,  (stack_top(stack)==1));
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(3), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(2), eax_reg, mr_d);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(4), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(3), eax_reg, mr_d);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(1), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(4), eax_reg, mr_d);
            break;
        case DUP2:  /* dup2: stack: ...,w2,w1 ==> ...,w2,w1,w2,w1 */
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, (stack_get_ith(stack, 2)==1));
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, (stack_get_ith(stack, 2)==1));
            break;
        case DUP2_X1:  /* dup2_x1: stack: ...,w3,w2,w1 ==> ...,w2,w1,w3,w2,w1 */
            /* push the top 2 stack operands */
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, (stack_get_ith(stack, 2)==1));
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, (stack_get_ith(stack, 2)==1));
            /* exchange three operands */
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(5), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(3), eax_reg, mr_d);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(1), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(4), eax_reg, mr_d);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(5), eax_reg, mr_d);
            break;
        case DUP2_X2:  /* dup2_x2: stack: ...,w4,w3,w2,w1 ==> ...,w2,w1,w4,w3,w2,w1 */
            /* push the top 2 stack operands */
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, (stack_get_ith(stack, 2)==1));
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, (stack_get_ith(stack, 2)==1));
            /* exchange four operands */
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(5), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(3), eax_reg, mr_d);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(6), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(4), eax_reg, mr_d);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(1), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(5), eax_reg, mr_d);
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = mov(buf, (uint32)stack_ith_opnd(6), eax_reg, mr_d);
            break;
        case SWAP: /* swap: stack:  ...,word2,word1 ==> ...,word1,word2 */
            buf = mov(buf, eax_reg, (uint32)stack_top_opnd(), rm_d);
            buf = mov(buf, edx_reg, (uint32)stack_ith_opnd(2), rm_d);
            buf = mov(buf, (uint32)stack_top_opnd(), edx_reg, mr_d);
            buf = mov(buf, (uint32)stack_ith_opnd(2), eax_reg, mr_d);
            break;

        /* arithmetic operations */
        case IADD: 
        case ISUB: 
        case IAND:
        case IOR:
        case IXOR:{
            korp_binalu_kind op_kind;

            if(bytecode == IADD) op_kind = op_add;
            else if(bytecode == ISUB) op_kind = op_sub;
            else if(bytecode == IAND) op_kind = op_and;
            else if(bytecode == IOR)  op_kind = op_or;
            else if(bytecode == IXOR) op_kind = op_xor;
            else assert(0);
            assert( stack_top(stack)==NOT_REF );
            assert( stack_get_ith(stack, 2)==NOT_REF );
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = binalu(buf, op_kind, (uint32)stack_top_opnd(), eax_reg, mr_d);
            /* buf = mov(buf, (uint32)stack_top_opnd(), eax_reg, mr_d);*/

            }
			break;
        case IDIV: 
        case IREM: 
            buf = stack_pop_reg(buf, info, stack, ecx_reg);
			buf = stack_pop_reg(buf, info, stack, eax_reg);
			*buf++ = 0x99; // Opcode of CDQ on IA-32
            //buf = binalu(buf, op_xor, edx_reg, edx_reg, rr_d);
            
            buf = muldiv(buf, op_idiv, ecx_reg, r_d);
            if(bytecode == IDIV)
                buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
            else if(bytecode == IREM)
                buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);
            
            break;

        case IMUL: 
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = muldiv(buf, op_imul, (uint32)stack_top_opnd(), m_d);
            buf = mov(buf, (uint32)stack_top_opnd(), eax_reg, mr_d);

			break;
            
        case FADD: 
        case FSUB:
        case FMUL: 
        case FDIV:
        case FREM:
            assert(0);
            stack_pop( stack );
            break;
        case LADD: 
        case DADD: 
        case LSUB: 
        case DSUB:
        case DMUL: 
        case DDIV: 
        case DREM: /* stack:  ...,v1.w1,v1.w2,v2.w1,v2.w1 ==> result.w1,result.w2 */
            assert(0);
            stack_times_pop( stack, 2 ); 
            break;
        case LMUL: 
        case LDIV: 
        case LREM:
            assert(0);
            stack_times_pop( stack, 2 );
            break;

        case INEG: 
            buf = unialu(buf, op_neg, (uint32)stack_top_opnd(), m_d);
            break;
        case LNEG: 
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = stack_pop_reg(buf, info, stack, edx_reg);
            buf = unialu(buf, op_neg, eax_reg, r_d);
            buf = binalu(buf, op_adc, edx_reg, 0, ri_d);
            buf = unialu(buf, op_neg, edx_reg, r_d);
            buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);
            buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
            break;
        case FNEG: 
        case DNEG:
            assert(0);
            break;

        /* logical operations */
        case ISHL: 
        case ISHR: 
        case IUSHR:
            assert(0);
            stack_pop( stack );
            break;
        case LSHL:
        case LSHR:
        case LUSHR: /* stack:  ...,v1.w1,v1.w2,shift ==> result.w1,result.w2 */
            assert(0);
            stack_pop( stack ); 
            break;
        case LAND:
        case LOR:
        case LXOR: 
            assert(0);
            stack_times_pop( stack, 2 ); 
            break;

        case IINC:{
            int constval;
            index = read_uint8_index( current_inst+1 );
            constval = read_int8_value( current_inst+2 );
            buf = binalu(buf, op_add, (uint32)localvar_ith_opnd(index), constval, mi_d);

            }break;
        case I2B:
        case I2C:
            buf = widen_to_full(buf, eax_reg, (uint32)stack_top_opnd(), (I2C-bytecode), rm_b);
            buf = mov(buf, (uint32)stack_top_opnd(), eax_reg, mr_d);
            break;
        case I2S:
            buf = widen_to_full(buf, eax_reg, (uint32)stack_top_opnd(), 1, rm_w);
            buf = mov(buf, (uint32)stack_top_opnd(), eax_reg, mr_d);
            break;            
        case L2I:
            assert(0);
            stack_pop( stack );
            break;
        case I2L:
            assert(0);
            stack_push( stack, NOT_REF );
            break;
        case I2D:
        case F2L:
        case F2D: 
            assert(0);
            stack_push( stack, NOT_REF );
            break;
        case I2F:
        case F2I:
        case L2D:
        case D2L:
            assert(0);
            break;
        case L2F:
        case D2I:
        case D2F:
            assert(0);
            stack_pop( stack );
            break;

        /* compare and/or branch */
        case LCMP:
        case DCMPL:
        case DCMPG:
            assert(0);
            stack_times_pop( stack, 3 );
            break;
        case FCMPL:
        case FCMPG:
            assert(0);
            stack_pop( stack );
            break;
        case IFEQ: 
        case IFNE: 
        case IFLT: 
        case IFGE: 
        case IFGT: 
        case IFLE:{
            /* stack:  ...,objectref ==> ... */
            korp_stack kbranch_stack, *branch_stack = &kbranch_stack;
            next_inst = current_inst + kenv->bytecode_inst_len[bytecode];
                        
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = binalu(buf, op_or, eax_reg, eax_reg, rr_d);
            switch( bytecode){
                case IFEQ: buf = branch(buf, cc_eq, 0, i_d); break;
                case IFNE: buf = branch(buf, cc_ne, 0, i_d); break;
                case IFLT: buf = branch(buf, cc_lt, 0, i_d); break;
                case IFGE: buf = branch(buf, cc_ge, 0, i_d); break;
                case IFGT: buf = branch(buf, cc_gt, 0, i_d); break;
                case IFLE: buf = branch(buf, cc_le, 0, i_d); break;
                default: assert(0);
            }
            branch_stack->slot = (uint32*)stack_alloc( kmethod->max_stack*BYTES_OF_UINT32 );
            branch_stack->depth = stack->depth;
            memcpy(branch_stack->slot, stack->slot, kmethod->max_stack*BYTES_OF_UINT32);
    
            offset = read_int16_offset( current_inst+1 ); 
            cur_bcinst_info->branch_target = current_inst- first_bc+offset;
            cur_bcinst_info->patch_offset = buf - kcodebuf->buf->value;
            select_code(info, next_inst, codebuf_set(kcodebuf, buf), branch_stack);
            buf = codebuf_get(kcodebuf);
            next_inst = current_inst+offset;
            /* determine if the branch is a back edge */
            if (offset < 0){
                ;
            }
            if( bcinst_info[next_inst-first_bc].is_visited )
                return;

            }break;
        case IF_ICMPEQ: 
        case IF_ICMPNE: 
        case IF_ICMPLT: 
        case IF_ICMPGE: 
        case IF_ICMPGT: 
        case IF_ICMPLE: 
        case IF_ACMPEQ: 
        case IF_ACMPNE:{
            korp_stack kbranch_stack, *branch_stack = &kbranch_stack;
            next_inst = current_inst + kenv->bytecode_inst_len[bytecode];

#ifdef _DEBUG
            if(bytecode==IF_ACMPEQ || bytecode==IF_ACMPNE){
                assert( stack_top(stack)==YES_REF );
                assert( stack_get_ith(stack, 2)==YES_REF );
            }else{
                assert( stack_top(stack)==NOT_REF );
                assert( stack_get_ith(stack, 2)==NOT_REF );
            }
#endif /* _DEBUG */

            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = stack_pop_reg(buf, info, stack, ecx_reg);
            buf = binalu(buf, op_sub, ecx_reg, eax_reg, rr_d);
            switch( bytecode){
                case IF_ICMPEQ: case IF_ACMPEQ: buf = branch(buf, cc_eq, 0, i_d); break;
                case IF_ICMPNE: case IF_ACMPNE: buf = branch(buf, cc_ne, 0, i_d); break;
                case IF_ICMPLT: buf = branch(buf, cc_lt, 0, i_d); break;
                case IF_ICMPGE: buf = branch(buf, cc_ge, 0, i_d); break;
                case IF_ICMPGT: buf = branch(buf, cc_gt, 0, i_d); break;
                case IF_ICMPLE: buf = branch(buf, cc_le, 0, i_d); break;
                default: assert(0);
            }
            branch_stack->slot = (uint32*)stack_alloc( kmethod->max_stack*BYTES_OF_UINT32 );
            branch_stack->depth = stack->depth;
            memcpy(branch_stack->slot, stack->slot, kmethod->max_stack*BYTES_OF_UINT32);
    
            offset = read_int16_offset( current_inst+1 ); 
            cur_bcinst_info->branch_target = current_inst- first_bc+offset;
            cur_bcinst_info->patch_offset = buf - kcodebuf->buf->value;
            select_code(info, next_inst, codebuf_set(kcodebuf, buf), branch_stack);
            buf = codebuf_get(kcodebuf);
            next_inst = current_inst+offset;
            /* determine if the branch is a back edge */
            if (offset < 0){
                ;
            }
            if( bcinst_info[next_inst-first_bc].is_visited )
                return;

            }break;
        case GOTO:
        case GOTO_W:{
            if(bytecode == GOTO)
                offset = read_int16_offset( current_inst+1 );
            else
                offset = read_int32_offset( current_inst+1 );
            
            buf = jump(buf, 0, i_d); 
            cur_bcinst_info->branch_target = current_inst- first_bc+offset;
            cur_bcinst_info->patch_offset = buf - kcodebuf->buf->value;

            codebuf_set(kcodebuf, buf);
            next_inst = current_inst+offset;
            /* determine if the branch is a back edge */
            if (offset < 0){
                ;
            }
            if( bcinst_info[next_inst-first_bc].is_visited )
                return;

            }break;

        case JSR:
        case JSR_W:{
            int target_off;

            if( bytecode == JSR )
                offset = read_int16_offset( current_inst+1 );
            else
                offset = read_int32_offset(current_inst);

            target_off = current_inst - first_bc + offset;
            
            buf = stack_push_const(buf, info, stack, 0, NOT_REF );            
            buf = jump(buf, 0, i_d);
            cur_bcinst_info->is_jsr = 1;
            cur_bcinst_info->branch_target = target_off;
            cur_bcinst_info->patch_offset = buf - kcodebuf->buf->value;

            // visit target (no edge)
            select_code( info, current_inst+offset, codebuf_set(kcodebuf, buf), stack);   /* target BB */
            buf = codebuf_get(kcodebuf);
                                      
            }break;

        case RET:

            index = read_uint8_index( current_inst+1 );
            buf = jump(buf, (uint32)localvar_ith_opnd(index), m_d); 
            codebuf_set(kcodebuf, buf);

            return; 
            break;

        case TABLESWITCH:{ /* tableswitch, stack: ...,index ==> ... */
            int default_offset, low, high, n_entries; 
            uint8* bc;
            assert(0);
            
            stack_pop( stack );

            /* skip over padding bytes to align on 4 byte boundary */
            bc = (current_inst+1) + ((4-(current_inst-first_bc+1)) & 0x03);
            default_offset = read_int32_offset( bc );
            low = read_int32_offset( bc+4 );
            high = read_int32_offset( bc+8 );
            
            bc += 12;
            n_entries = high - low + 1;    

            /* visit each target */
            for (int i=0; i<n_entries; i++) {
                select_code_branch( bc );   /* target BB */
                bc += 4;
            }
            /* default target */
            select_code( info, current_inst+default_offset, codebuf_set(kcodebuf, buf), stack );   /* target BB */
            return; /* end of the BB */
            
            }break;

        case LOOKUPSWITCH:{ /* lookupswitch (key match and jump) stack: ...,key ==> ... */
            int default_offset, n_pairs;
            uint8* bc;
            assert(0);

            stack_pop( stack );

            /* skip over padding bytes to align on 4 byte boundary */
            bc = (current_inst+1) + ((4-(current_inst-first_bc+1)) & 0x03);
            default_offset = read_int32_offset( bc );
            n_pairs = read_int32_offset( bc+4 );
            bc += 8; bc += 4;
            
            /* visit each target */
            for (int i=0; i<n_pairs; i++) {
                select_code_branch( bc );   /* target BB */
                bc += 8;
            }
            /* default target */
            select_code( info, current_inst+default_offset, codebuf_set(kcodebuf, buf), stack );   /* target BB */
            return; /* end of the BB */

            }break;

        case IRETURN:
        case FRETURN:
        case ARETURN:
        case LRETURN: 
        case DRETURN:
        case RETURN:{
            /* end of the BB */
            uint16 codelen = METHOD_BYTE_CODE_LENGTH( kmethod );
            if( bytecode != RETURN)
                buf = stack_pop_reg(buf, info, stack, eax_reg);
            if( bytecode==LRETURN || bytecode==DRETURN )
                buf = stack_pop_reg(buf, info, stack, edx_reg);
            buf = jump(buf, 0, i_d); 
            cur_bcinst_info->branch_target = codelen;
            cur_bcinst_info->patch_offset = buf - kcodebuf->buf->value;
            codebuf_set(kcodebuf, buf);
            return; 

            }break;

        case GETSTATIC: {
            korp_field* kfield;
            korp_java_type type;
            int32 addr, size;
            korp_loader_exception exc;

            index = read_uint16_index( current_inst+1 );
            kfield = class_resolve_field(kclass, index, &exc);
            class_initialize(kfield->owner_class);

            type = (korp_java_type)*string_to_chars(kfield->descriptor);
            size = typesize_type(type);

            addr = (int32)kfield->owner_class->static_data + kfield->offset;

            buf = binalu(buf, op_xor, eax_reg, eax_reg, rr_d);
            if( size == 1 )
                buf = mov(buf, al_reg, (uint32)mem_addr_opnd(kmem, addr), rm_b);
            else if( size == 2 )
                buf = mov(buf, ax_reg, (uint32)mem_addr_opnd(kmem, addr), rm_w);
            else if( size == 4 )
                buf = mov(buf, eax_reg, (uint32)mem_addr_opnd(kmem, addr), rm_d);
            else{
                assert(size==8);
                buf = mov(buf, edx_reg, (uint32)mem_addr_opnd(kmem, addr+BYTES_OF_INT), rm_d);
                buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF );
                buf = mov(buf, eax_reg, (uint32)mem_addr_opnd(kmem, addr), rm_d);
            }
            
            if( is_ref_type_type(type) ){
                buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF );
            }else{
                buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF );
            }
            
            }break;

        case PUTSTATIC: { 
            korp_field* kfield;
            korp_java_type type;
            int32 addr, size;
            korp_loader_exception exc;

            index = read_uint16_index( current_inst+1 );
            kfield = class_resolve_field(kclass, index, &exc);
            class_initialize(kfield->owner_class);

            addr = (int32)kfield->owner_class->static_data + kfield->offset;
            type = (korp_java_type)*string_to_chars(kfield->descriptor);
            size = typesize_type(type);

            buf = stack_pop_reg(buf, info, stack, eax_reg);
    
            if( size==1 )
                buf = mov(buf, (uint32)mem_addr_opnd(kmem, addr), al_reg, mr_b);
            else if( size==2 )
                buf = mov(buf, (uint32)mem_addr_opnd(kmem, addr), ax_reg, mr_w);
            else if( size==4 )
                buf = mov(buf, (uint32)mem_addr_opnd(kmem, addr), eax_reg, mr_d);
            else{
                assert( size==8 );
                buf = mov(buf, (uint32)mem_addr_opnd(kmem, addr), eax_reg, mr_d);
                buf = stack_pop_reg(buf, info, stack, edx_reg);
                buf = mov(buf, (uint32)mem_addr_opnd(kmem, addr+BYTES_OF_INT), edx_reg, mr_d);
            }

            }break;
        case GETFIELD: 
        case GETFIELD_Q_W:{
            korp_field* kfield;
            korp_java_type type;
            int32 offset, size;
            korp_loader_exception exc;

            index = read_uint16_index( current_inst+1 );
            kfield = class_resolve_field(kclass, index, &exc);
            class_initialize(kfield->owner_class);

            offset = (int32)kfield->offset;
            type = (korp_java_type)*string_to_chars(kfield->descriptor);
            size = typesize_type(type);
    
            buf = binalu(buf, op_xor, ecx_reg, ecx_reg, rr_d);
            buf = stack_pop_reg(buf, info, stack, eax_reg); /* get objref */
            if( size == 1)
                buf = mov(buf, cl_reg, (uint32)mem_base_opnd(kmem, eax_reg, offset), rm_b); /* get field */
            else if( size == 2 )
                buf = mov(buf, cx_reg, (uint32)mem_base_opnd(kmem, eax_reg, offset), rm_w); /* get field */
            else if( size == 4 )
                buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, eax_reg, offset), rm_d); /* get field */
            else{
                buf = mov(buf, edx_reg, (uint32)mem_base_opnd(kmem, eax_reg, offset+BYTES_OF_INT), rm_d);
                buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF ); /* push hi word */
                buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, eax_reg, offset), rm_d); /* get field */
            }

            if( is_ref_type_type(type) ){
                buf = stack_push_reg(buf, info, stack, ecx_reg, YES_REF );
            }else{
                buf = stack_push_reg(buf, info, stack, ecx_reg, NOT_REF ); /* push low word */
            }
            
            }break;
        case PUTFIELD: 
        case PUTFIELD_Q_W:{ 
            korp_field* kfield;
            korp_java_type type;
            int32 offset;
            int32 n_bytes, n_words;
            korp_loader_exception exc;

            index = read_uint16_index( current_inst+1 );
            kfield = class_resolve_field(kclass, index, &exc);
            class_initialize(kfield->owner_class);
            type = (korp_java_type)*string_to_chars(kfield->descriptor);
            n_bytes = typesize_type(type);
            n_words = (n_bytes+3)/BYTES_OF_INT;

            offset = (int32)kfield->offset;
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(n_words+1), rm_d); /* get objref */
            buf = stack_pop_reg(buf, info, stack, ecx_reg); /* pop low word */

            if( n_bytes == 1 )
                buf = mov(buf, (uint32)mem_base_opnd(kmem, eax_reg, offset), cl_reg, mr_b);
            else if( n_bytes == 2 )
                buf = mov(buf, (uint32)mem_base_opnd(kmem, eax_reg, offset), cx_reg, mr_w);
            else if( n_bytes == 4 )
                buf = mov(buf, (uint32)mem_base_opnd(kmem, eax_reg, offset), ecx_reg, mr_d);
            else{
                buf = mov(buf, (uint32)mem_base_opnd(kmem, eax_reg, offset), ecx_reg, mr_d);
                buf = stack_pop_reg(buf, info, stack, ecx_reg); /* pop high word */
                buf = mov(buf, (uint32)mem_base_opnd(kmem, eax_reg, offset+BYTES_OF_INT), ecx_reg, mr_d);
            }
            buf = stack_pop_reg(buf, info, stack, eax_reg);            

            }break;
        case INVOKEVIRTUAL: { 
            /* stack: ..,objectref,[arg1,[arg2,...]] ==> ..., retval1(, retval2) */
            unsigned n_words, n_bytes;
            korp_java_type ret_type;
            korp_method *kmethod;
            korp_loader_exception exc;
            uint16 offset;

            index = read_uint16_index( current_inst+1 );

            kmethod = class_resolve_method(kclass, index, &exc);
            class_initialize(kmethod->owner_class);
            assert(kmethod);
            method_descriptor_get_param_info(kmethod->descriptor, &n_words, &ret_type);
    
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(n_words+1), rm_d); /* get objref */
            buf = mov(buf, eax_reg, (uint32)mem_base_opnd(kmem, eax_reg, 0), rm_d); /* get vtable */
            buf = binalu(buf, op_and, eax_reg, POINTER_MASK, ri_d); /* get pointer */

            offset = kmethod->offset;
            buf = call(buf, (uint32)mem_base_opnd(kmem, eax_reg, offset), m_d);
            stack_times_pop(stack, n_words+1);

            /* the return value is pushed onto the stack */
            n_bytes = typesize_type(ret_type);
            if( is_ref_type_type(ret_type) ){
               buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF);
            }else if( n_bytes != 0 ){
                if (n_bytes == 8 ){
                    buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);
                }
                buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
            }

            }break;

        case INVOKESPECIAL: { 
            /* stack: ..,objectref,[arg1,[arg2,...]] ==> ..., retval1(, retval2) */
            unsigned n_words, n_bytes;
            korp_java_type ret_type;
            korp_method *kmethod;
            korp_loader_exception exc;
            korp_code** entry;

            index = read_uint16_index( current_inst+1 );
            kmethod = class_resolve_method(kclass, index, &exc);
            class_initialize(kmethod->owner_class);
            assert(kmethod);
            method_descriptor_get_param_info(kmethod->descriptor, &n_words, &ret_type);
    
            entry = &(kmethod->jitted_code);
            
            buf = mov(buf, eax_reg, (uint32)mem_addr_opnd(kmem, (int32)entry), rm_d);
            buf = binalu(buf, op_add, eax_reg, VM_ARRAY_OVERHEAD, ri_d);
            buf = call(buf, eax_reg, r_d);
            stack_times_pop(stack, n_words+1);
            /* the return value is pushed onto the stack */
            n_bytes = typesize_type(ret_type);
            if (is_ref_type_type(ret_type)) {
                buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF);
            } else if (n_bytes != 0) {
                if(n_bytes == 8 ){
                    buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);
                }
                buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
            }

            }break;
        case INVOKESTATIC: 
        case INVOKESTATIC_Q: {
            /* stack: ..,[arg1,[arg2,...]] ==> ..., retval1(, retval2) */
            unsigned n_words, n_bytes;
            korp_java_type ret_type;
            korp_method *kmethod;
            korp_loader_exception exc;
            korp_code** entry;

            index = read_uint16_index( current_inst+1 );
            kmethod = class_resolve_method(kclass, index, &exc);
            class_initialize(kmethod->owner_class);
            assert(kmethod);
            method_descriptor_get_param_info(kmethod->descriptor, &n_words, &ret_type);
    
            entry = &(kmethod->jitted_code);
            
            buf = mov(buf, eax_reg, (uint32)mem_addr_opnd(kmem, (int32)entry), rm_d);
            buf = binalu(buf, op_add, eax_reg, VM_ARRAY_OVERHEAD, ri_d);
            buf = call(buf, eax_reg, r_d);
            stack_times_pop(stack, n_words);

            /* the return value is pushed onto the stack */
            n_bytes = typesize_type(ret_type);
            if (is_ref_type_type(ret_type)) {
                buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF);
            } else if (n_bytes != 0) {
                if(n_bytes == 8 ){
                    buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);
                }
                buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
            }

            }break;

        case INVOKEINTERFACE: 
        case INVOKEINTERFACE_Q:{ 
            /* stack: ..,objectref,[arg1,[arg2,...]] ==> ... */
            unsigned n_words, n_bytes;
            korp_java_type ret_type;
            korp_method *km_new;
            korp_loader_exception exc;

            index = read_uint16_index( current_inst+1 );

            km_new = class_resolve_method(kclass, index, &exc);
            class_initialize(kmethod->owner_class);
            assert(km_new);
            method_descriptor_get_param_info(km_new->descriptor, &n_words, &ret_type);
    
            buf = mov(buf, eax_reg, (uint32)stack_ith_opnd(n_words+1), rm_d); /* get objref */
            buf = mov(buf, eax_reg, (uint32)mem_base_opnd(kmem, eax_reg, 0), rm_d); /* get vtable */
            buf = binalu(buf, op_and, eax_reg, POINTER_MASK, ri_d); /* get class pointer */
            buf = stack_push_const(buf, info, stack, (uint32)km_new, YES_REF); /* push interface method */            
            buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF); /* push class */
            buf = mov(buf, eax_reg, (uint32)class_get_interface_method_offset, ri_d);
            buf = call(buf, eax_reg, r_d);
            stack_times_pop(stack, 2);
            
            buf = mov(buf, ecx_reg, (uint32)stack_ith_opnd(n_words+1), rm_d); /* get objref */
            buf = mov(buf, ecx_reg, (uint32)mem_base_opnd(kmem, ecx_reg, 0), rm_d); /* get vtable */
            buf = binalu(buf, op_and, ecx_reg, POINTER_MASK, ri_d); /* get vtable pointer */
            
            buf = binalu(buf, op_add, eax_reg, ecx_reg, rr_d);
            buf = call(buf, (uint32)mem_base_opnd(kmem, eax_reg, 0), m_d);
            stack_times_pop(stack, n_words+1);

            /* the return value is pushed onto the stack */
            n_bytes = typesize_type(ret_type);
            if( is_ref_type_type(ret_type) ){
               buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF);
            }else if( n_bytes != 0 ){
                if (n_bytes == 8 ){
                    buf = stack_push_reg(buf, info, stack, edx_reg, NOT_REF);
                }
                buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
            }

            }break;

        case NEW: case NEW_Q:{
            /* stack: ... ==> ...,objectref */
            korp_class* kc_new;
            korp_loader_exception exc;
            index = read_uint16_index(current_inst+1);
            kc_new = class_resolve_class(kclass, index, &exc);    
            buf = stack_push_const(buf, info, stack, (uint32)kc_new, YES_REF );
            /* the direct call will be used after patching works 
            buf = call(buf, (uint32)new_java_object, TARGET);*/
            buf = mov(buf, eax_reg, (int32)new_java_object, ri_d);
            buf = call(buf, eax_reg, r_d);
            stack_pop(stack);
            buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF );

            }break;
        case NEWARRAY: 
            /* newarray : stack: ...,count ==> ...,arrayref */
            index = read_uint8_index(current_inst+1);            
            buf = stack_push_const(buf, info, stack, index, NOT_REF);
            buf = mov(buf, eax_reg, (uint32)new_java_array_fast, ri_d);
            buf = call(buf, eax_reg, r_d);
            stack_times_pop(stack, 2); /* count and atype */
            buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF );
            break;
        case ANEWARRAY: 
        case ANEWARRAY_Q:
            /* stack: ...,count ==> ...,arrayref */
            assert(0);
            stack_pop( stack );
            stack_push( stack, YES_REF );
            break;
        case ARRAYLENGTH:  
            /* arraylength : stack: ...,arrayref ==> ...,length */
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = mov(buf, eax_reg, (uint32)mem_base_opnd(kmem, eax_reg, (int32)&((korp_java_array*)0)->length), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, NOT_REF);
            break;
        case ATHROW:
            buf = mov(buf, eax_reg, (uint32)throw_exception_object_java, ri_d);
            buf = call(buf, eax_reg, r_d);
            codebuf_set(kcodebuf, buf);
            stack_pop(stack); 
            return;
        case CHECKCAST: 
        case CHECKCAST_Q:{
            /* stack: ...,objectref ==> ...,objectref */
            korp_class* kc_new;
            korp_loader_exception exc;

            index = read_uint16_index(current_inst+1);
            kc_new = class_resolve_class(kclass, index, &exc);  

            buf = mov(buf, eax_reg, (uint32)stack_top_opnd(), rm_d);
            buf = stack_push_reg(buf, info, stack, eax_reg, YES_REF);
            buf = stack_push_const(buf, info, stack, (uint32)kc_new, YES_REF);
            buf = mov(buf, eax_reg, (uint32)java_object_checkcast, ri_d);
            buf = call(buf, eax_reg, r_d);
            stack_times_pop(stack, 2);

            }break;
        case INSTANCEOF: 
        case INSTANCEOF_Q:
            /* stack: ...,objectref ==> ...,result */
            /* not enumerate the objectref */
            assert(0);
            stack_pop( stack );
            stack_push( stack, NOT_REF );
            break;
        case MONITORENTER: 
            /* stack: ...,objecterf ==> ... ; not enumerate the objectref */
//            buf = mov(buf, eax_reg, (uint32)object_lock, ri_d);
//            buf = call(buf, eax_reg, r_d);
//            stack_pop(stack);
            break;
        case MONITOREXIT:
            /* stack: ...,objecterf ==> ... ; not enumerate the objectref */
//            buf = mov(buf, eax_reg, (uint32)object_unlock, ri_d);
//            buf = call(buf, eax_reg, r_d);
//            stack_pop(stack);
            break;
        case WIDE:
            assert(0);
            bytecode = *(current_inst+1);
            index = read_uint16_index( current_inst+2 );
            current_inst += 2;

            switch (bytecode){
                case IINC:
                    current_inst += 2;
                    break;
                case ALOAD:
                    stack_push( stack, YES_REF );
                    break;
                case ILOAD:
                case FLOAD:
                    stack_push( stack, NOT_REF );
                    break;
                case LLOAD:
                case DLOAD:
                    stack_push( stack, NOT_REF );
                    stack_push( stack, NOT_REF );
                    break;
                case ISTORE:
                case FSTORE:
                case ASTORE:
                    stack_pop( stack );
                    break;
                case LSTORE:
                case DSTORE:
                    stack_times_pop( stack, 2 ); 
                    break;
                case RET: /* ret: end of BB */
                    return; 
                    break;
            } /* switch */
            break;
        case MULTIANEWARRAY: 
        case MULTIANEWARRAY_Q:{
            /* stack: ...,count1,[count2,...] ==> ...,arrayref */
            uint8 n_dimen;
            assert(0);
            n_dimen = read_uint8_index( current_inst+2 );
            stack_times_pop( stack,  n_dimen ); /* dimension for popping count */
            stack_push( stack, YES_REF );     /* for pushing arrayref */

            }break;
        case IFNULL: 
        case IFNONNULL:{
            /* stack:  ...,objectref ==> ... */
            korp_stack kbranch_stack, *branch_stack = &kbranch_stack;
            next_inst = current_inst + kenv->bytecode_inst_len[bytecode];
                        
            buf = stack_pop_reg(buf, info, stack, eax_reg);
            buf = binalu(buf, op_or, eax_reg, eax_reg, rr_d);
            if(bytecode == IFNULL)
                buf = branch(buf, cc_eq, 0, i_d);
            else
                buf = branch(buf, cc_ne, 0, i_d);

            branch_stack->slot = (uint32*)stack_alloc( kmethod->max_stack*BYTES_OF_UINT32 );
            branch_stack->depth = stack->depth;
            memcpy(branch_stack->slot, stack->slot, kmethod->max_stack*BYTES_OF_UINT32);

            offset = read_int16_offset( current_inst+1 ); 
            cur_bcinst_info->branch_target = current_inst- first_bc+offset;
            cur_bcinst_info->patch_offset = buf - kcodebuf->buf->value;
            select_code(info, next_inst, codebuf_set(kcodebuf, buf), branch_stack);
            buf = codebuf_get(kcodebuf);
            next_inst = current_inst+offset;
            /* determine if the branch is a back edge */
            if (offset < 0){
                ;
            }
            if( bcinst_info[next_inst-first_bc].is_visited )
                return;

            }break;
        default:
            assert(0);
            break;
        } /* switch bytecode */

        codebuf_set( kcodebuf, buf );

        if( next_inst == 0 )
            next_inst = current_inst + kenv->bytecode_inst_len[bytecode];
        
        offset = next_inst-first_bc;
        if( bcinst_info[offset].is_visited ){
            offset = bcinst_info[offset].native_offset;
            assert(offset);
            offset -= buf - kcodebuf->buf->value; 
            offset -= 5;
            buf = jump(buf, offset, i_d);
            codebuf_set( kcodebuf, buf );
        }

        current_inst = next_inst ;
        cur_bcinst_info = &bcinst_info[current_inst-first_bc];

    }/* while */
}


