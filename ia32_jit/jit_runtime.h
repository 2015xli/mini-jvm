
#ifndef _JIT_RUNTIME_H
#define _JIT_RUNTIME_H

#include "compile.h"

korp_java_call getaddress_native_to_java_call(void);
korp_code* generate_java_to_native_wrapper(korp_method* kmethod, uint8* funcptr);

#endif /* #ifndef _JIT_RUNTIME_H */
