
#ifndef _CLASS_TABLE_H
#define _CLASS_TABLE_H

#include "vm_object.h"

/* global class table */
typedef struct _korp_class_table{
    obj_info  header;
    korp_class_loader *loader;
    korp_entry *bucket[1];
}korp_class_table;

korp_class* class_table_search( korp_class_table *, const korp_string*);
void class_table_insert( korp_class_table *, korp_class*);
void class_table_remove( korp_class_table *, korp_class*);
korp_class_table* new_class_table(void);

#endif /* #ifndef _CLASS_TABLE_H */

