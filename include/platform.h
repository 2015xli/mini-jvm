
#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "korp_types.h"


#ifdef WIN32
#define stdcall__

#define DIR_SEPARATOR '\\'
#define PATH_SEPARATOR ';'

#else
#include <alloca.h>

#ifndef _alloca
#define _alloca alloca
#endif

#ifndef stdcall__
#define stdcall__ __attribute__ ((__stdcall__))
#endif

#ifndef __stdcall
#define __stdcall
#endif

#define DIR_SEPARATOR '/'
#define PATH_SEPARATOR ':'
#endif

#endif
