
#include <assert.h>

#include "compile.h"
#include "method.h"
#include "utils.h"

#define parse_bytecode_branch( offset_bc ) \
    do{ korp_stack kbranch_stack, *branch_stack;                                        \
        branch_stack = &kbranch_stack;                                                  \
        offset = read_int16_offset( offset_bc );                                        \
        branch_stack->slot = (uint32*)stack_alloc( kmethod->max_stack*BYTES_OF_UINT32 );  \
        branch_stack->depth = stack->depth;                                             \
        memcpy(branch_stack->slot, stack->slot, kmethod->max_stack*BYTES_OF_UINT32);    \
        parse_bytecode( info, current_inst+offset, branch_stack);                       \
    }while(0)


#define inc_local_use(i)  ( use_count[2*i+1]++ )

static void gcinfo_at_call_site(korp_comp_info* info, 
                                uint8* inst, 
                                korp_stack* stack)
{
    return;
}

static void gcinfo_at_back_edge(korp_comp_info* info, 
                                uint8* inst,
                                int offset,
                                korp_stack* stack)
{
    return;
}

static void parse_bytecode(korp_comp_info* info, uint8* current_inst, korp_stack* stack)
{
    /* info for prepass */
    uint32 num_blocks=0;
    uint32 num_edges=0;
    uint32 num_call_sites=0;
    uint32 num_returns=0;

    korp_method* kmethod = info->method;
    korp_class* kclass = kmethod->owner_class;

    int offset; /* relative branch distance */
    uint8* first_bc = info->byte_code;
    uint32 index;   /* index in constant pool or localvars */
    uint8 bytecode; /* butecode content */

    korp_bcinst_info* bcinst_info = info->bcinst_info;
    korp_bcinst_info* cur_bcinst_info = &bcinst_info[current_inst-first_bc];
    int16* use_count = (int16 *)info->locals_info->value;

    if(!cur_bcinst_info->is_target ) {
        cur_bcinst_info->is_target = 1;
        num_blocks++;
        if( cur_bcinst_info->is_visited ) num_edges++;
    }   

    if( cur_bcinst_info->is_visited ) return;
    
    /* loop for each bytecode in current BB */
    while(! cur_bcinst_info->is_visited ) {

        cur_bcinst_info->is_visited = 1;
        bytecode = *current_inst;
        
        switch (bytecode) {
        case NOP:
            break;

        /* constant loads */
        case ACONST_NULL:
            stack_push( stack, YES_REF ); /* subtle */
            break;
        case ICONST_M1: 
        case ICONST_0: 
        case ICONST_1: 
        case ICONST_2:
        case ICONST_3: 
        case ICONST_4: 
        case ICONST_5:
            stack_push( stack, NOT_REF );
            break;
        case LCONST_0: 
        case LCONST_1:
            stack_push( stack, NOT_REF );
            stack_push( stack, NOT_REF );
            break;
        case FCONST_0: 
        case FCONST_1: 
        case FCONST_2:
            stack_push( stack, NOT_REF );
            break;
        case DCONST_0: 
        case DCONST_1:
            stack_push( stack, NOT_REF );
            stack_push( stack, NOT_REF );
            break;
        
        /* stack pushes */
        case BIPUSH:
            stack_push( stack, NOT_REF );
            break;
        case SIPUSH:
            stack_push( stack, NOT_REF );
            break;
        case LDC:{
            korp_java_type t;
            index = read_uint8_index( current_inst+1 );
            t = cp_get_const_type(kclass->constant_pool, index);
            if( is_ref_type_type(t) ){
                gcinfo_at_call_site( info, current_inst, stack );
                stack_push( stack, YES_REF );
                num_call_sites++;
            }else{
                stack_push( stack, NOT_REF );
            }

            }break;
        case LDC_W:{
            korp_java_type t;
            index = read_uint16_index( current_inst+1 ); 
            t = cp_get_const_type(kclass->constant_pool, index);
            if( is_ref_type_type(t) ) {
                gcinfo_at_call_site( info, current_inst, stack );
                num_call_sites++;
                stack_push( stack, YES_REF );
            } else {
                stack_push( stack, NOT_REF );
            }
            
            }break;
        case LDC2_W:
            stack_push( stack, NOT_REF );
            stack_push( stack, NOT_REF );
            break;
        case ILOAD:
        case FLOAD:
            stack_push( stack, NOT_REF );
            index = read_uint8_index( current_inst+1 );
            if (bytecode == ILOAD) inc_local_use( index );
            break;
        case ALOAD:
            stack_push( stack, YES_REF );
            inc_local_use( index );
            break;
        case LLOAD:
        case DLOAD:
            stack_push( stack, NOT_REF );
            stack_push( stack, NOT_REF );
            break;
        case ILOAD_0: 
        case ILOAD_1: 
        case ILOAD_2: 
        case ILOAD_3:
            stack_push( stack, NOT_REF );
            index = (bytecode-ILOAD_0)&0x03;
            inc_local_use( index );
            break;
        case FLOAD_0: 
        case FLOAD_1: 
        case FLOAD_2: 
        case FLOAD_3:
            stack_push( stack, NOT_REF );
            break;
        case ALOAD_0: 
        case ALOAD_1: 
        case ALOAD_2: 
        case ALOAD_3:
            stack_push( stack, YES_REF );
            index = (bytecode-ALOAD_0)&0x03;
            inc_local_use( index );
            break;
        case LLOAD_0: 
        case LLOAD_1: 
        case LLOAD_2: 
        case LLOAD_3:
        case DLOAD_0: 
        case DLOAD_1: 
        case DLOAD_2: 
        case DLOAD_3:
            stack_push( stack, NOT_REF );
            stack_push( stack, NOT_REF );
            break;
        
        /* array load */
        case IALOAD:
        case FALOAD:
        case AALOAD:
        case BALOAD:
        case CALOAD:
        case SALOAD:
            assert( stack_get_ith( stack, 2 ) == YES_REF);
            assert( stack_get_ith( stack, 1) == NOT_REF);
            gcinfo_at_call_site(info, current_inst, stack ); /* ? no arrayref ? */
            num_call_sites++;
            stack_times_pop( stack, 2 ); 
            stack_push( stack, NOT_REF );
            break;
        case LALOAD:
        case DALOAD:
            assert( stack_get_ith( stack, 2 ) == YES_REF);
            assert( stack_get_ith( stack, 1 ) == NOT_REF);
            gcinfo_at_call_site(info, current_inst, stack ); /* ? no arrayref ? */
            num_call_sites++;
            stack_times_pop( stack, 2 ); 
            stack_push( stack, NOT_REF ); 
            stack_push( stack, NOT_REF );
            break;

        /* stack pops */
        case ISTORE: 
        case ASTORE:
        case FSTORE:
            stack_pop( stack );
            index = read_uint8_index( current_inst+1 );
            if (bytecode != FSTORE) inc_local_use(index);
            break;
        case DSTORE:
        case LSTORE:
            stack_times_pop( stack, 2 ); 
            break;
        case ISTORE_0: 
        case ISTORE_1: 
        case ISTORE_2: 
        case ISTORE_3:
            stack_pop( stack );
            index = (bytecode - ISTORE_0) & 0x03;
            inc_local_use(index);
            break;
        case ASTORE_0: 
        case ASTORE_1: 
        case ASTORE_2: 
        case ASTORE_3:
            stack_pop( stack );
            break;
        case FSTORE_0: 
        case FSTORE_1: 
        case FSTORE_2: 
        case FSTORE_3:
            stack_pop( stack );
            break;
        case LSTORE_0: 
        case LSTORE_1: 
        case LSTORE_2: 
        case LSTORE_3:
        case DSTORE_0: 
        case DSTORE_1: 
        case DSTORE_2: 
        case DSTORE_3:
            stack_times_pop( stack, 2 );
            break;

        /* store into array */
        case IASTORE:
        case FASTORE:
        case AASTORE:
        case BASTORE:
        case CASTORE:
        case SASTORE:
            gcinfo_at_call_site(info, current_inst, stack);
            num_call_sites++;
            stack_times_pop( stack, 3 );
            break;
        case LASTORE:
        case DASTORE:
            gcinfo_at_call_site(info, current_inst, stack);
            num_call_sites++;
            stack_times_pop( stack, 4 );
            break;

        /* stack operations */
        case POP:
            stack_pop( stack );
            break;
        case POP2:
            stack_times_pop( stack, 2 ); 
            break;
        case DUP:
            stack_push( stack, stack_get_ith( stack, 1 ) );
            break;
        case DUP_X1:  /* dup_x1: stack: ...,w2,w1 ==> ...,w1,w2,w1 */
            stack_push( stack, stack_get_ith( stack, 1 ) );
            stack_set_ith( stack, 2, stack_get_ith( stack, 3 ) );
            stack_set_ith( stack, 3, stack_get_ith( stack, 1 ) );
            break;
        case DUP_X2:  /* dup_x2: stack: ...,w3,w2,w1 ==> ...,w1,w3,w2,w1 */
            stack_push( stack, stack_get_ith( stack, 1 ) );
            stack_set_ith( stack, 2, stack_get_ith( stack, 3 ) );
            stack_set_ith( stack, 3, stack_get_ith( stack, 4 ) );
            stack_set_ith( stack, 4, stack_get_ith( stack, 1 ) );
            break;
        case DUP2:  /* dup2: stack: ...,w2,w1 ==> ...,w2,w1,w2,w1 */
            stack_push( stack, stack_get_ith( stack, 2 ) );
            stack_push( stack, stack_get_ith( stack, 2 ) );
            break;
        case DUP2_X1:  /* dup2_x1: stack: ...,w3,w2,w1 ==> ...,w2,w1,w3,w2,w1 */
            /* push the top 2 stack operands */
            stack_push( stack, stack_get_ith( stack, 2 ) );
            stack_push( stack, stack_get_ith( stack, 2 ) );
            /* exchange three operands */
            stack_set_ith( stack, 3, stack_get_ith( stack, 5 ) );
            stack_set_ith( stack, 4, stack_get_ith( stack, 1 ) );
            stack_set_ith( stack, 5, stack_get_ith( stack, 2 ) );            
            break;
        case DUP2_X2:  /* dup2_x2: stack: ...,w4,w3,w2,w1 ==> ...,w2,w1,w4,w3,w2,w1 */
            /* push the top 2 stack operands */
            stack_push( stack, stack_get_ith( stack, 2 ) );
            stack_push( stack, stack_get_ith( stack, 2 ) );
            /* exchange four operands */
            stack_set_ith( stack, 3, stack_get_ith( stack, 5 ) );
            stack_set_ith( stack, 4, stack_get_ith( stack, 6 ) );
            stack_set_ith( stack, 5, stack_get_ith( stack, 1 ) );            
            stack_set_ith( stack, 6, stack_get_ith( stack, 2 ) );            
            break;
        case SWAP: /* swap: stack:  ...,word2,word1 ==> ...,word1,word2 */
            index = stack_get_ith( stack, 1 );
            stack_set_ith( stack, 1, stack_get_ith( stack, 2 ) );
            stack_set_ith( stack, 2, index );
            break;

        /* arithmetic operations */
        case IADD: 
        case FADD: 
        case ISUB: 
        case FSUB:
        case IMUL: 
        case FMUL: 
        case IDIV: 
        case FDIV:
        case IREM: 
        case FREM: /* stack:  ...,value1,value2 ==> ...,result */
            stack_pop( stack );
            break;
        case LADD: 
        case DADD: 
        case LSUB: 
        case DSUB:
        case DMUL: 
        case DDIV: 
        case DREM: /* stack:  ...,v1.w1,v1.w2,v2.w1,v2.w1 ==> result.w1,result.w2 */
            stack_times_pop( stack, 2 ); 
            break;
        case LMUL: 
        case LDIV: 
        case LREM:
            gcinfo_at_call_site(info, current_inst, stack);
            num_call_sites++;
            stack_times_pop( stack, 2 );
            break;

        case INEG: 
        case FNEG: 
        case LNEG: 
        case DNEG:
            break;

        /* logical operations */
        case ISHL: 
        case ISHR: 
        case IUSHR:
        case IAND:
        case IOR:
        case IXOR:
            stack_pop( stack );
            break;
        case LSHL:
        case LSHR:
        case LUSHR: /* stack:  ...,v1.w1,v1.w2,shift ==> result.w1,result.w2 */
            stack_pop( stack ); 
            break;
        case LAND:
        case LOR:
        case LXOR: 
            stack_times_pop( stack, 2 ); 
            break;

        case IINC:
            index = read_uint8_index( current_inst+1 );
            inc_local_use(index);
            inc_local_use(index);
            break;

        case I2L:
        case I2D:
        case F2L:
        case F2D: 
            stack_push( stack, NOT_REF );
            break;
        case I2F:
        case F2I:
        case I2B:
        case I2C:
        case I2S:
        case L2D:
        case D2L:
            break;
        case L2I:
        case L2F:
        case D2I:
        case D2F:
            stack_pop( stack );
            break;

        /* compare and/or branch */
        case LCMP:
        case DCMPL:
        case DCMPG:
            stack_times_pop( stack, 3 );
            break;
        case FCMPL:
        case FCMPG:
            stack_pop( stack );
            break;
        case IFEQ: 
        case IFNE: 
        case IFLT: 
        case IFGE: 
        case IFGT: 
        case IFLE:
            stack_pop( stack );
            num_edges++;
            parse_bytecode_branch( current_inst+1 );   /* target BB */
            if (! bcinst_info[current_inst - first_bc ].is_target ) {
                num_edges++;
                bcinst_info[current_inst - first_bc ].is_target = 1;
                num_blocks++;
            }

            /* determine if the branch is a back edge */
            if (offset < 0){
                gcinfo_at_back_edge(info, current_inst, offset, stack);
            }

            break;

        case IF_ICMPEQ: 
        case IF_ICMPNE: 
        case IF_ICMPLT: 
        case IF_ICMPGE: 
        case IF_ICMPGT: 
        case IF_ICMPLE: 
        case IF_ACMPEQ: 
        case IF_ACMPNE:
            stack_times_pop( stack, 2 ); 
            num_edges++;
            parse_bytecode_branch( current_inst+1 );   /* target BB */
            if (! bcinst_info[current_inst - first_bc ].is_target ) {
                num_edges++;
                bcinst_info[current_inst - first_bc ].is_target = 1;
                num_blocks++;
            }

            /* determine if the branch is a back edge */
            if (offset < 0){
                gcinfo_at_back_edge(info, current_inst, offset, stack);
            }
            break;

        case GOTO:
            num_edges++;    /* target edge */
            offset = read_int16_offset( current_inst+1 );
            parse_bytecode( info, current_inst+offset, stack );   /* target BB */

            /* determine if the branch is a back edge */
            if (offset < 0){
                gcinfo_at_back_edge(info, current_inst, offset, stack);
            }

            /* the end of the BB */ 
            return; 
            break;

        case JSR:
            stack_push( stack, NOT_REF );
            /* search target (no edge to target) */
            offset = read_int16_offset( current_inst+1 );
            parse_bytecode( info, current_inst+offset, stack );  /* target BB */
            break;

        case RET:
            /* the end of the BB */ 
            return; 
            break;

        case TABLESWITCH:{ /* tableswitch, stack: ...,index ==> ... */
            int default_offset, low, high, n_entries; 
            uint8* bc;
            
            stack_pop( stack );

            /* skip over padding bytes to align on 4 byte boundary */
            bc = (current_inst+1) + ((4-(current_inst-first_bc+1)) & 0x03);
            default_offset = read_int32_offset( bc );
            low = read_int32_offset( bc+4 );
            high = read_int32_offset( bc+8 );
            
            bc += 12;
            n_entries = high - low + 1;    
            num_edges += n_entries + 1;

            /* visit each target */
            for (int i=0; i<n_entries; i++) {
                parse_bytecode_branch( bc );   /* target BB */
                bc += 4;
            }
            /* default target */
            parse_bytecode( info, current_inst+default_offset, stack );   /* target BB */
            return; /* end of the BB */
            
            }break;

        case LOOKUPSWITCH:{ /* lookupswitch (key match and jump) stack: ...,key ==> ... */
            int default_offset, n_pairs;
            uint8* bc;

            stack_pop( stack );

            /* skip over padding bytes to align on 4 byte boundary */
            bc = (current_inst+1) + ((4-(current_inst-first_bc+1)) & 0x03);
            default_offset = read_int32_offset( bc );
            n_pairs = read_int32_offset( bc+4 );
            bc += 8; bc += 4;

            num_edges += n_pairs + 1;
            
            /* visit each target */
            for (int i=0; i<n_pairs; i++) {
                parse_bytecode_branch( bc );   /* target BB */
                bc += 8;
            }
            /* default target */
            parse_bytecode( info, current_inst+default_offset, stack );   /* target BB */
            return; /* end of the BB */

            }break;

        case IRETURN:
        case FRETURN:
        case ARETURN:
        case LRETURN: 
        case DRETURN:
        case RETURN:
            /* end of the BB */
            num_returns++;
            return; 
            break;

        case GETSTATIC: {
            korp_java_type t;
            uint16 n_bytes;            
            index = read_uint16_index( current_inst+1 );
            t = cp_get_field_type( kclass->constant_pool, index);
            n_bytes = typesize_type(t);
            if (is_ref_type_type(t)) {
                stack_push( stack, YES_REF );
            } else {
                stack_push( stack, NOT_REF );
                if ( n_bytes == 8) 
                    stack_push( stack, NOT_REF );
            }
            num_call_sites += 2;
            
            }break;

        case PUTSTATIC: { 
            korp_java_type t;
            index = read_uint16_index( current_inst+1 );
            t = cp_get_field_type( kclass->constant_pool, index);
            stack_times_pop( stack,  typesize_type(t)/4 );
            num_call_sites += 2;
            
            }break;

        case GETFIELD: 
        case GETFIELD_Q_W:{ 
            korp_java_type t;
            uint32 n_bytes;
            index = read_uint16_index( current_inst+1 );
            stack_pop( stack );    /* pop objectref */
            t = cp_get_field_type( kclass->constant_pool, index);
            n_bytes = typesize_type(t);
            if( is_ref_type_type(t) ){
                stack_push( stack, YES_REF );
            }else{
                stack_push( stack, NOT_REF );
                if ( n_bytes == 8) stack_push( stack, NOT_REF );
            }
            num_call_sites ++;
            
            }break;
        case PUTFIELD: 
        case PUTFIELD_Q_W:{ 
            korp_java_type t;
            index = read_uint16_index( current_inst+1 );
            t = cp_get_field_type( kclass->constant_pool, index);
            stack_times_pop( stack,  typesize_type(t)/4 + 1 );
            num_call_sites ++;
            
            }break;

        case INVOKEVIRTUAL: 
        case INVOKESPECIAL: { 
            /* stack: ..,objectref,[arg1,[arg2,...]] ==> ..., retval1(, retval2) */
            unsigned n_words, n_bytes;
            korp_string* kdesc;
            korp_java_type ret_type;

            index = read_uint16_index( current_inst+1 );
            num_call_sites++;

            kdesc = cp_get_method_descriptor(kclass->constant_pool, index );
            method_descriptor_get_param_info(kdesc, &n_words, &ret_type);
            stack_times_pop( stack,  n_words + 1 );
            /* store gc info after popping args */
            gcinfo_at_call_site(info, current_inst, stack);
            /* the return value is pushed onto the stack */
            n_bytes = typesize_type(ret_type);
            if (is_ref_type_type(ret_type)) {
                stack_push( stack, YES_REF );
            } else if (n_bytes != 0) {
                stack_push( stack, NOT_REF );
                if (n_bytes == 8 ) stack_push( stack, NOT_REF );
            }

            }break;
        case INVOKESTATIC: 
        case INVOKESTATIC_Q: {
            unsigned n_words, n_bytes;
            korp_string* kdesc;
            korp_java_type ret_type;

            num_call_sites++;
            index = read_uint16_index( current_inst+1 );
            kdesc = cp_get_method_descriptor(kclass->constant_pool, index );
            method_descriptor_get_param_info(kdesc, &n_words, &ret_type);
            stack_times_pop( stack,  n_words );
            /* store gc info after popping args */
            gcinfo_at_call_site(info, current_inst, stack);
            /* the return value is pushed onto the stack */
            n_bytes = typesize_type(ret_type);
            if (is_ref_type_type(ret_type)) {
                stack_push( stack, YES_REF );
            } else if (n_bytes != 0) {
                stack_push( stack, NOT_REF );
                if (n_bytes==8) stack_push( stack, NOT_REF );
            }

            }break;
        case INVOKEINTERFACE: 
        case INVOKEINTERFACE_Q:{ 
            /* stack: ..,objectref,[arg1,[arg2,...]] ==> ... */
            unsigned n_args, n_bytes;
            korp_string* kdesc;
            korp_java_type ret_type;

            num_call_sites += 2;
            index = read_uint16_index( current_inst+1 );
            n_args = read_uint8_index( current_inst+3 );;
            /* store garbage collection info before popping args (helper call) */
            gcinfo_at_call_site(info, current_inst, stack);
            stack_times_pop( stack,  n_args );
            /* store garbage collection info after popping args (actual invoke) */
            gcinfo_at_call_site(info, current_inst, stack);

            kdesc = cp_get_method_descriptor(kclass->constant_pool, index );
            method_descriptor_get_param_info(kdesc, &n_args, &ret_type);
            /* the return value is pushed onto the stack */
            n_bytes = typesize_type(ret_type);
            if (is_ref_type_type(ret_type)) {
                stack_push( stack, YES_REF );
            } else if (n_bytes != 0) {
                stack_push( stack, NOT_REF );
                if (n_bytes==8) stack_push( stack, NOT_REF );
            }

            }break;

        case NEW: case NEW_Q:
            /* stack: ... ==> ...,objectref */
            gcinfo_at_call_site(info, current_inst, stack);
            stack_push( stack, YES_REF );
            num_call_sites++;
            break;
        case NEWARRAY: 
            /* newarray : stack: ...,count ==> ...,arrayref */
            gcinfo_at_call_site(info, current_inst, stack);
            stack_pop( stack );
            stack_push( stack, YES_REF );
            num_call_sites++;
            break;
        case ANEWARRAY: 
        case ANEWARRAY_Q:
            /* stack: ...,count ==> ...,arrayref */
            gcinfo_at_call_site(info, current_inst, stack);
            stack_pop( stack );
            stack_push( stack, YES_REF );
            num_call_sites++;
            break;
        case ARRAYLENGTH:  
            /* arraylength : stack: ...,arrayref ==> ...,length */
            stack_pop( stack );
            stack_push( stack, NOT_REF );
            break;
        case ATHROW:
            num_call_sites++;
            gcinfo_at_call_site(info, current_inst, stack);
            return;
            break;
        case CHECKCAST: 
        case CHECKCAST_Q:
            /* stack: ...,objectref ==> ...,objectref */
            num_call_sites++;
            stack_pop( stack );
            gcinfo_at_call_site(info, current_inst, stack);
            stack_push( stack, YES_REF );
            break;
        case INSTANCEOF: 
        case INSTANCEOF_Q:
            /* stack: ...,objectref ==> ...,result */
            /* not enumerate the objectref */
            stack_pop( stack );
            gcinfo_at_call_site(info, current_inst, stack);
            stack_push( stack, NOT_REF );
            num_call_sites++;
            break;
        case MONITORENTER: 
        case MONITOREXIT:
            /* stack: ...,objecterf ==> ... ; not enumerate the objectref */
            stack_pop( stack );
            gcinfo_at_call_site(info, current_inst, stack);
            num_call_sites++;
            break;
        case WIDE:
            bytecode = *(current_inst+1);
            index = read_uint16_index( current_inst+2 );
            inc_local_use(index);
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
            gcinfo_at_call_site(info, current_inst, stack);
            num_call_sites++;
            n_dimen = read_uint8_index( current_inst+2 );
            stack_times_pop( stack,  n_dimen ); /* dimension for popping count */
            stack_push( stack, YES_REF );     /* for pushing arrayref */

            }break;
        case IFNULL: 
        case IFNONNULL:
            /* stack:  ...,objectref ==> ... */
            stack_pop( stack );
            num_edges++;    /* target edge */
            parse_bytecode_branch( current_inst+1 );   /* target BB */
            if (! bcinst_info[current_inst - first_bc ].is_target ) {
                num_edges++;
                bcinst_info[current_inst - first_bc ].is_target = 1;
                num_blocks++;
            }

            /* determine if the branch is a back edge */
            if (offset < 0){
                gcinfo_at_back_edge(info, current_inst, offset, stack);
            }

            break;
        case GOTO_W:
            num_edges++;    /* target edge */
            offset = read_int32_offset(current_inst);
            parse_bytecode( info, current_inst+offset, stack );   /* target BB */

            /* determine if the branch is a back edge */
            if (offset < 0){
                gcinfo_at_back_edge(info, current_inst, offset, stack);
            }

            return;  /* end of the BB */
            break;
        case JSR_W:
            stack_push( stack, NOT_REF );
            // visit target (no edge)
            offset = read_int32_offset(current_inst);
            parse_bytecode( info, current_inst+offset, stack);   /* target BB */
            break;
        default:
            assert(0);
            break;
        } /* switch bytecode */

        current_inst += kenv->bytecode_inst_len[bytecode] ;
        cur_bcinst_info = &(info->bcinst_info[current_inst - first_bc]);

    }/* while */
}

bool prepass_bytecode(korp_comp_info* info)
{
    uint32 num, i;
    uint8 *start;
    korp_method* kmethod = info->method;
    korp_stack kstack, *stack;
    stack = &kstack;
    stack->slot = (uint32*)stack_alloc( kmethod->max_stack*BYTES_OF_UINT32 );
    stack_reset( stack );

    /* parse main body of the method */
    start = method_get_bytecode_entry( kmethod );
    parse_bytecode( info, start, stack );
    
    /* parse exception handler of the method */
    num = METHOD_HANDLER_NUM( kmethod);
    for(i=0; i<num; i++){
        korp_handler* handler;
        handler = method_get_handler(kmethod, i);
        stack_reset( stack );
        stack_push( stack, YES_REF );
        parse_bytecode(info, start + handler->handler_pc, stack );
    }

    return true;
}
