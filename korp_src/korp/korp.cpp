
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "korp_types.h"
#include "environment.h"
#include "class_loader.h" 
#include "platform.h"
#include "utils.h"
#include "runtime.h"
 
korp_env* kenv;

static bool preload_class()
{
    korp_class_loader* sys_loader;
    korp_string* jlc_string;
    korp_loader_exception exc = LD_OK;
    korp_class* jlc;

    sys_loader = (korp_class_loader *)kenv->loader_list->value;
    jlc_string = string_pool_lookup(kenv->string_pool, "java/lang/Class");
    kenv->globals->java_lang_Class_string = jlc_string;
    kenv->globals->java_lang_Object_string = string_pool_lookup(kenv->string_pool, "java/lang/Object");

    jlc = load_class(sys_loader, jlc_string, &exc); 

    if( exc!= LD_OK ){
        printf("can not load Java.lang.Class class\n");
        return false;
    } 
    
    kenv->globals->java_lang_Class_class = jlc;
    kenv->globals->java_lang_Object_class = jlc->super_class;
    {
        korp_string* name = string_pool_lookup(kenv->string_pool, "korp_class");
        korp_string* desc = string_pool_lookup(kenv->string_pool, "L");
        korp_field* kclass_field = class_lookup_field_own(jlc, name, desc);
        kenv->jclass_kclass_offset = kclass_field->offset;

        prepare_java_class_object(jlc);
        prepare_java_class_object(jlc->super_class );
    }
    return true;

}

static bool prepare_globals()
{
    korp_string_pool* spool = kenv->string_pool;
    korp_class_loader* sys_loader = get_sys_loader();
    korp_loader_exception exc;
    korp_string *kclassname, *kname, *kdesc;

    if( !preload_class() )
        return false;
    
    kenv->globals->main_name_string = string_pool_lookup(spool, "main" );
    kenv->globals->main_descriptor_string = string_pool_lookup(spool, "([Ljava/lang/String;)V" );
    kenv->globals->finalize_name_string = string_pool_lookup(spool, "finalize" );
    kenv->globals->init_string = string_pool_lookup(spool, "<init>" );
    kenv->globals->clinit_string = string_pool_lookup(spool, "<clinit>" );
    kenv->globals->default_descriptor_string = string_pool_lookup(spool, "()V" );

    kenv->globals->boolean_class = preload_primitive_class("Z");
    kenv->globals->char_class = preload_primitive_class("C");
    kenv->globals->float_class = preload_primitive_class("F");
    kenv->globals->double_class = preload_primitive_class("D");
    kenv->globals->byte_class = preload_primitive_class("B");
    kenv->globals->short_class = preload_primitive_class("S");
    kenv->globals->int_class = preload_primitive_class("I");
    kenv->globals->long_class = preload_primitive_class("J");
    kenv->globals->long_class = preload_primitive_class("V");

    kclassname = string_pool_lookup(spool, "[Z");
    kenv->globals->ArrayOfBoolean_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "[C");
    kenv->globals->ArrayOfChar_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "[F");
    kenv->globals->ArrayOfFloat_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "[D");
    kenv->globals->ArrayOfDouble_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "[B");
    kenv->globals->ArrayOfByte_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "[S");
    kenv->globals->ArrayOfShort_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "[I");
    kenv->globals->ArrayOfInt_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "[J");
    kenv->globals->ArrayOfLong_class = load_class(sys_loader, kclassname, &exc);

    kclassname = string_pool_lookup(spool, "java/lang/String");
    kenv->globals->java_lang_String_class = load_class(sys_loader, kclassname, &exc);
    kclassname = string_pool_lookup(spool, "java/lang/StringBuffer");
    kenv->globals->java_lang_StringBuffer_class = load_class(sys_loader, kclassname, &exc);

    kname = string_pool_lookup(spool, "value" );
    kdesc = string_pool_lookup(spool, "[C" );
    kenv->globals->jlString_value_field  = class_lookup_field_own( kenv->globals->java_lang_String_class, kname, kdesc);
    kenv->globals->jlStringBuffer_value_field  = class_lookup_field_own( kenv->globals->java_lang_StringBuffer_class, kname, kdesc);

    kname = string_pool_lookup(spool, "count" );
    kdesc = string_pool_lookup(spool, "I" );
    kenv->globals->jlString_count_field  = class_lookup_field_own( kenv->globals->java_lang_String_class, kname, kdesc);
    kenv->globals->jlStringBuffer_count_field = class_lookup_field_own( kenv->globals->java_lang_StringBuffer_class, kname, kdesc);

    kname = string_pool_lookup(spool, "offset" );
    kdesc = string_pool_lookup(spool, "I" );
    kenv->globals->jlString_offset_field  = class_lookup_field_own( kenv->globals->java_lang_String_class, kname, kdesc);

    kname = string_pool_lookup(spool, "count" );
    kdesc = string_pool_lookup(spool, "I" );
    
    return true;

}

static bool korp_init()
{
    load_jnc_fields();
    
    prepare_globals();

    return true;
}

static bool system_init()
{
    char classpath[3] = ".";
    classpath[1] = PATH_SEPARATOR;
    classpath[2] = '\0';

    kenv = new_vm_object(korp_env);

    kenv->string_pool = new_string_pool();
    kenv->loader_list = new_class_loader();
    kenv->globals = new_vm_object(korp_globals);
    kenv->trampos = new_vm_object(korp_trampos);

    kenv->globals->default_classpath = string_pool_lookup(kenv->string_pool, classpath );
    
    return true;

}

korp_string* normalize_classpath(char* cp )
{
    char* classpath = stack_alloc(256);
    char* c;

    c = classpath;
    if( cp != NULL ){
        convert_path_separator(classpath, cp);

        /* check whether last char is PATH_SEPARATOR */
        c = strchr(classpath, '\0');

        if( *(c-1) != PATH_SEPARATOR ){ /*  ! dir; */
            if( *(c-1) == DIR_SEPARATOR ) /*  dir/ */
                c--;
            *c++ = PATH_SEPARATOR;
        }
    }

    *c = '\0';
    strcat(classpath, string_to_chars(kenv->globals->default_classpath));

    return string_pool_lookup(kenv->string_pool, classpath);
}

static int parse_arguments(int argc, char* argv[], int* used_argc)
{
    int i;
    char* classpath=NULL;

    for(i=1; i<argc; i++){
        char* arg = argv[i];
        if( !strcmp(arg, "-classpath") || !strcmp(arg, "-cp")){
            if(++i == argc ) return (--i);
            classpath = argv[i];
            continue;
        }
        if( *argv[i] != '-') break;
    }

    if(i<argc && *argv[i]== '-') return i;
    
    if(i==argc) return -1;

    kenv->globals->classpath = normalize_classpath(classpath);

    *used_argc = i;

    return 0;
}

static bool start_java_execution( int argc, char* argv[])
{
    korp_class_loader* sysloader;
    korp_string *kclassname, *ks_arrstr;
    korp_class *kc_arrstr, *kclass;
    korp_loader_exception exc;
    korp_method* km_main;
    
    sysloader = get_sys_loader();
    kclassname = string_pool_lookup(kenv->string_pool, argv[0]);
    kclass = load_class(sysloader, kclassname,&exc);
    if(kclass == NULL ){
        printf("can not load application class %s.\n", argv[0]);
        return false;
    }
    
    km_main = class_lookup_method_own(kclass, kenv->globals->main_name_string, kenv->globals->main_descriptor_string);
    //km_main = class_lookup_method_own(kclass, kenv->globals->init_string, kenv->globals->default_descriptor_string);

    ks_arrstr = string_pool_lookup(kenv->string_pool, "[Ljava/lang/String;");
    kc_arrstr = load_class(sysloader, ks_arrstr,&exc);

	// To support user input. Modified by Chengrui, 20091112
	// ***args = new_java_array(kc_arrstr, (uint16)(argc-1));
	// Prepare the arguments for java app
	korp_java_array* mainargs = NULL;
	++argv;  // class name arg has been used
	--argc;

	if (argc > 0) 
	{
		mainargs = new_java_array(kc_arrstr, (uint16)(argc));
		for (int i = 0; i < argc; i++)
		{
			korp_string* ks_argstr = string_pool_lookup(kenv->string_pool, argv[i]);
			ref jobj = get_jstring_from_kstring(ks_argstr);
			//korp_string* temp = get_kstring_from_jstring(jobj);
			uint32 addr = java_array_get_elem_addr(mainargs, i);
			memcpy((void*)addr, (void*)&jobj, sizeof(ref));
			//int t = 0;
			//memcpy((void*)&t, (void*)addr, sizeof(ref));
			//assert(t ==(int)jobj);
		}
	}
	// To support user input. Modified by Chengrui, 20091112
    
    run_java_main(km_main, mainargs);

    return true;
}

int main(int argc, char *argv[]) 
{
    int err_pos;
    int used_argc;

    gc_init();

    system_init();
    
    err_pos = parse_arguments(argc, argv, &used_argc );

    if( err_pos == -1 ){
        printf("class not specified.\n");
        return -1;
    }else if(  err_pos!= 0 ){
        printf("arguments parse error after %s.\n", argv[err_pos]);
        return -1;
    }

    korp_init();

    //thread_init();

    start_java_execution( argc-used_argc, argv+used_argc);

    //thread_start();

    return 0;

}

