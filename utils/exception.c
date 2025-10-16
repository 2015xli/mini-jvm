
#include <assert.h>

#include "exception.h"

void throw_exception_name_native(korp_string* sename)
{
    assert(0);
    return;
}

void __stdcall throw_exception_object_java(korp_object* jeobj)
{
    assert(0);
    return;
}

void throw_exception_object_native(korp_object* jeobj)
{
    assert(0);
    return;
}

korp_object* get_last_exception()
{
    return NULL;
}
