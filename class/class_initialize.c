
#include "korp_types.h"
#include "class_loader.h"
#include "class_utils.h"
#include "utils/exception.h"
#include "runtime.h"
#include "platform.h"

#include <assert.h>
#include <stdio.h>

#define object_unlock_return(X) \
    do{                         \
        return;                 \
    }while(0)

#define complete_gracefully(X)              \
    do{                                     \
        kclass->state |= CS_Initialized;      \
		kclass->initilizing_flag = false; \
        return;                             \
    }while(0)

#define complete_abruptly(X)                \
    do{                                     \
        kclass->state |= CS_Error;            \
        kclass->initilizing_flag = false; \
        throw_exception_object_native(kclass->exception);   \
    }while(0)



void class_initialize( korp_class *kclass)
{
    korp_class* ksuper_class;
    korp_object* jexc;
    korp_class *kclass_exc;

    /* 1: Synchronize ($14.18) on the Class object */
    korp_object* jclass = kclass->java_object;

    /* 2: Wait if the class intialization is in progress */
    //while(  kclass->initilizing_thread && kclass->initilizing_thread != korp_thread_self ){
    //    object_wait(jclass );
    //}
    
    /* 3: If initilizing thread is self, return happily */
    //if( kclass->initilizing_thread == korp_thread_self )
    //    object_unlock_return(jclass );
	if( kclass->initilizing_flag ) object_unlock_return(jclass );
    
    /* 4: If class is initialized, return */
    if( kclass->state & CS_Initialized )
        object_unlock_return(jclass );

    /* 5: If class is in err state, throw exception */
    if( kclass->state & CS_Error ){
        //object_unlock( jclass );
        throw_exception_name_native(string_pool_lookup(kenv->string_pool, "java/lang/NoClassDefFoundError"));
        assert(0);
    }

    /* 6: It's self to initiate the class */
    kclass->state |= CS_Initializing;
    //kclass->initilizing_thread = korp_thread_self;
	kclass->initilizing_flag = true;

    /* 7: Initialize superclasses */
    ksuper_class = kclass->super_class;
    if( ksuper_class ){
        if( ksuper_class->state != CS_Initialized ){
            class_initialize(ksuper_class );
            if( ksuper_class->state & CS_Error ){
                kclass->exception = ksuper_class->exception;
                complete_abruptly(kclass);
            }
        }
    }

    /* 8: execute initilizers. constants are prepared already except refs */
    if( !kclass->clinit )  complete_gracefully(kclass);
    
    execute_class_initializer(kclass->clinit);
    jexc = get_last_exception();

    /* 9: if nothing wrong, return normally */
    if( jexc == NULL ) complete_gracefully(kclass);

    /* 10: else if something wrong, produce expcetion */
    kclass_exc = java_object_get_class(jexc);
    if( !is_subclass(kenv->globals->java_lang_Error_class, kclass_exc)){
        kclass_exc = kenv->globals->java_lang_ExceptionInInitializerError_class;
        jexc = new_java_object(kclass_exc);
    }

    /* 11: throw the produced exception */
    kclass->exception = jexc;
    complete_abruptly(kclass);
    
    assert(0);    
}
