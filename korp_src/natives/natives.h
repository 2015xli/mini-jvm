
#ifndef _NATIVES_H
#define _NATIVES_H

#include "korp_types.h"

#ifdef WIN32
#define JNICALL __stdcall
#else
#define JNICALL
#endif

typedef uint32   jboolean;
typedef int32    jbyte;
typedef uint32   jchar;
typedef int32    jshort;
typedef int32    jint;
typedef int64    jlong;
typedef int64    jfloat;
typedef int64    jdouble;

typedef korp_object*  jobject;
typedef korp_java_array* jarray;

typedef struct _korp_native_entry{
    char* methodname;
    uint8* funcptr;
}korp_native_entry;

bool method_forced_native(korp_method*);
uint8* method_lookup_native_entry(korp_method* kmethod, bool jit_caller);

#endif /* #ifndef _NATIVES_H */
