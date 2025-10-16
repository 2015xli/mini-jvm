
#include <assert.h>

#include "gc_for_vm.h"
#include "runtime.h"
#include "class_loader.h"
#include "compile.h"
//#include "interpreter.h"
//#include "interp_stat.h"

/*
#define INTERPRETER
*/
#ifdef INTERPRETER
#define INTERP_STAT
Interpreter interp;
#endif

korp_object* __stdcall new_java_object(korp_class* kclass)
{
    char *param = NULL;
    korp_object* jobj = gc_new_java_object(kclass);
    return jobj;
}

korp_java_array* __stdcall new_java_array_fast(korp_array_type atype, uint32 len)
{
    korp_class* kc_array;

    switch( atype ){
        case ARRAY_BOOLEAN: kc_array = kenv->globals->ArrayOfBoolean_class; break;
        case ARRAY_CHAR: kc_array = kenv->globals->ArrayOfChar_class; break;
        case ARRAY_FLOAT: kc_array = kenv->globals->ArrayOfFloat_class; break;
        case ARRAY_DOUBLE: kc_array = kenv->globals->ArrayOfDouble_class; break;
        case ARRAY_BYTE: kc_array = kenv->globals->ArrayOfByte_class; break;
        case ARRAY_SHORT: kc_array = kenv->globals->ArrayOfShort_class; break;
        case ARRAY_INT: kc_array = kenv->globals->ArrayOfInt_class; break;
        case ARRAY_LONG: kc_array = kenv->globals->ArrayOfLong_class; break;
        default: assert(0);
    
    };
    
    return gc_new_java_array(kc_array, len); 
}

korp_java_array* new_java_array(korp_class* kc_array, uint16 len)
{
    korp_java_array* jobj = gc_new_java_array(kc_array, len);
    return jobj; 
}

korp_class_loader *get_sys_loader()
{
    return (korp_class_loader*)kenv->loader_list->value;
}

void run_java_main(korp_method* km, korp_java_array* args)
{
    bytecode_init();
    x86_init();

#ifdef INTERPRETER
#ifdef INTERP_STAT
    interp_stat_init();
#endif

    korp_slot ks;
    interp.init_interpreter();

    /* fake main's caller frame */
    interp.fake_mains_caller_frame();

    ks.type = JAVA_TYPE_ARRAY;
    ks.value = (uint32)args;
    interp.interpret_java_method(km, &ks, true);
#ifdef INTERP_STAT
    dump_coverage_ops();
#endif // INTERP_STAT
#else // ia32 jitO0
    execute_java_method_ia32( km, (uint32 *)&args, NULL);
#endif // INTERPRETER

    return;
}

bool method_init_bincode(korp_method* kmethod)
{

    kmethod->jitted_code = generate_compile_me_stub_ia32(kmethod);

    return true;
}

/*
 *  run class <init> & <clinit>
 */
void execute_class_initializer(korp_method *clinit)
{
#ifdef INTERPRETER
    interp.interpret_java_method(clinit, NULL, false);
#else
    execute_java_method_ia32(clinit, NULL, NULL);
#endif
}

