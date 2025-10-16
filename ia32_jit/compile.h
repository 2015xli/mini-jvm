
#ifndef _COMPILE_H
#define _COMPILE_H

#include "platform.h"
#include "korp_types.h"
#include "vm_object.h"
#include "method.h"
#include "runtime.h"
#include "utils.h"
#include "x86.h"

typedef struct _korp_bcinst_info {
    uint16  branch_target;
    int32 patch_offset;
    int32 native_offset;
    unsigned stack_depth:8;					/* stack depth */
    unsigned is_visited:1;              /* prepassing info */
    unsigned is_jsr:1;
    unsigned is_target:1;				/* is baisc block entry */
    unsigned is_try_start:1;			/* is start of a try block */
    unsigned is_try_end:1;				/* is end of a try block */
    unsigned is_exception_handler_entry:1;
    unsigned is_back_edge_entry:1;      /* is entry of a back edge */
}korp_bcinst_info;

typedef struct _korp_comp_info{
    korp_method* method;
    korp_int16_array* locals_info;
    korp_int16_array* stack_info;
    uint8* byte_code;
    korp_bcinst_info* bcinst_info;
    int16 frame_size;
}korp_comp_info;


typedef struct _korp_codebuf{
    korp_code* buf; 
    uint8* pos;
}korp_codebuf;
uint8* codebuf_get(korp_codebuf* kcodebuf);
korp_codebuf* codebuf_set( korp_codebuf* kcodebuf, uint8* offset);

void execute_java_method_ia32(korp_method*, uint32* args, uint32* retval);
korp_code* generate_compile_me_stub_ia32(korp_method* kmethod);
void* __stdcall jit_a_method(korp_method* kmethod) stdcall__;

bool compile(korp_method*);
void select_code(korp_comp_info*, uint8*, korp_codebuf*, korp_stack*);
bool prepass_bytecode(korp_comp_info*);

uint8 read_uint8_index( uint8* );
uint16 read_uint16_index( uint8* );
int16 read_int16_offset( uint8* );
int32 read_int32_offset( uint8* );
int8 read_int8_value( uint8* bc );

typedef void(*korp_java_call)( uint32, uint32*,void*, uint32, uint32*, uint32* ); 

#endif /* #ifndef _COMPILE_H */
