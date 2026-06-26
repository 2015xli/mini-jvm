
#include "korp_types.h"
#include "class_table.h"
#include "utils.h"
#include "string_pool.h"
#include "vm_interface.h"

korp_class_table* new_class_table()
{
    return (korp_class_table *)new_vm_array(korp_ref_array, HASH_TABLE_SIZE);
}


korp_class* class_table_search( korp_class_table* ktable, const korp_string *kclassname)
{
   	uint16 key = 0;
    korp_entry* entry;
    korp_class* c;
    const char *chars = string_to_chars(kclassname);
    key = get_chars_hash(chars, NULL);
    
    for(entry=ktable->bucket[key]; entry!=NULL; entry=entry->next){
            c = (korp_class *)entry->value;
            /* using simple kstring address comparison */
            if( kclassname == c->name ) return c;
    }

    return NULL;   
}

void class_table_insert( korp_class_table* ktable, korp_class *kclass)
{
    korp_string* kclassname = kclass->name;
    uint16 key = 0;
    const char *chars;
    korp_entry* entry;
    korp_class* c;

    chars = string_to_chars(kclassname);
    key = get_chars_hash(chars, NULL);

    for(entry=ktable->bucket[key]; entry!=NULL; entry=entry->next){
            c = (korp_class *)entry->value;
            /* using simple kstring address comparison */
            if( kclassname == c->name ){
                return;
            }
    }

    entry = new_vm_object(korp_entry);
    entry->value = (korp_object*)kclass;
    entry->next = ktable->bucket[key];
    ktable->bucket[key] = entry;

    return;   
}

void class_table_remove(korp_class_table* ktable, korp_class *kclass )
{

    korp_string* kclassname = kclass->name;
    uint16 key = 0;
    const char *chars = string_to_chars(kclassname);
    korp_entry* entry;
    korp_entry** pre;
    korp_class* c;
    key = get_chars_hash(chars, NULL);

    entry = ktable->bucket[key];
    pre = &(ktable->bucket[key]);

    for(; entry!=NULL; pre=&entry->next, entry=entry->next){
        c = (korp_class *)entry->value;
        /* using simple kstring address comparison */
        if( kclassname == c->name ){
            *pre = entry->next;
            break;
        }
    }

    return;   
}
