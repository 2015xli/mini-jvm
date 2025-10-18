
/* this is temporary for test */
#include <stdlib.h>
#include <string.h>

#include "class_loader.h"
#include "vm_for_gc.h"
#include "utils.h"

static char* mem;
void free_vm_object(korp_object *kobj)
{
    return;
}

void free_java_object(korp_object* jobj)
{
    return;
}
