
#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include "korp_types.h"
#include "platform.h"

void throw_exception_name_native(korp_string *);
void throw_exception_object_native(korp_object *);
void __stdcall throw_exception_object_java(korp_object* ) stdcall__;

korp_object* get_last_exception(void);

#endif /* #ifndef _EXCEPTION_H */
