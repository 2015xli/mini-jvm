
#include <string.h>

#include "natives.h"
#include "java_lang.h"
#include "utils.h"
#include "method.h"
#include "jit_runtime.h"

static korp_native_entry builtin_natives[] = {

    "com/sun/cldc/io/j2me/debug/PrivateOutputStream.putchar", (uint8*)com_sun_cldc_io_j2me_debug_PrivateOutputStream_putchar,

    "java/lang/Class.forName", (uint8*)java_lang_Class_forName,
    "java/lang/Class.newInstance", (uint8*)java_lang_Class_newInstance,

    "java/lang/String.charAt", (uint8*)java_lang_String_charAt,
    "java/lang/String.equals", (uint8*)java_lang_String_equals,
    "java/lang/String.indexOf", (uint8*)java_lang_String_indexOf,

    "java/lang/StringBuffer.append", (uint8*)java_lang_StringBuffer_append,
    "java/lang/StringBuffer.toString", (uint8*)java_lang_StringBuffer_toString,
    
    "java/lang/System.arraycopy", (uint8*)java_lang_System_arraycopy,
    "java/lang/System.getProperty0", (uint8*)java_lang_System_getProperty0
};

bool method_forced_native(korp_method* kmethod)
{
    return false;
}

uint8* method_lookup_native_entry(korp_method* kmethod, bool jit_caller)
{
    char methname[256];
    int L, R;
    uint8* funcptr = NULL;

    strcpy(methname, string_to_chars( kmethod->owner_class->name ));    
    strcat(methname, ".");
    strcat(methname, string_to_chars( kmethod->name ));

    L = 0;
    R = sizeof(builtin_natives)/sizeof(korp_native_entry);
    
    while( L<R ){
        int M = (L + R) / 2;
        char *tryname = builtin_natives[M].methodname;
        int cmp = strcmp(methname, tryname);
        if( cmp<0){
            R = M;
        }else if( cmp>0 ){
            L = M + 1;
        }else{
            funcptr = builtin_natives[M].funcptr;
            break;
        }
    }

    /* if the caller is from jitted code, set the jitted code entry */
    if( funcptr && jit_caller ) {
        kmethod->jitted_code = generate_java_to_native_wrapper(kmethod, funcptr);
    }
    
    return funcptr;
}
