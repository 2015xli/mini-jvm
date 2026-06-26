
#include <string.h>

#include "platform.h"
#include "string_pool.h"
#include "vm_interface.h"
#include "utils.h"

korp_string_pool* new_string_pool()
{
    return (korp_string_pool *)new_vm_array(korp_ref_array, HASH_TABLE_SIZE);
}

/*
 * search a char string in string pool, assume read is much more than write
 * return: the corresponding korp_string of the char string
 */
korp_string *string_pool_lookup(korp_string_pool *spool, 
                                const char *chars)
{
   	uint16 key = 0;
	uint16 l, len = 0;
    korp_entry* head;
    korp_entry* entry;
    korp_string* s;

    key = get_chars_hash( chars, &len); /* doesnt count '\0' */

    head = spool->bucket[key];

	for( entry = head; entry != NULL; entry = entry->next ){
        s = (korp_string *)entry->value;
        l = get_string_len(s);
        if( (l==len) && strcmp(string_to_chars(s), chars) == 0){
            return s;
        }
	}

    /* no matched string, create one */
	for( entry = head; entry != NULL; entry = entry->next ){
        s = (korp_string *)entry->value;
        l = get_string_len(s);
        if( (l==len) && strcmp(string_to_chars(s), chars) == 0){
            return s;
        }
	}

    /* char array length is 1 bigger than real string length for '\0' */
    s = (korp_string *)new_vm_array(korp_uint8_array, len+1);
    memcpy(s->byte, chars, len+1);
    s->byte[len] = '\0'; 

    entry = new_vm_object(korp_entry);
    entry->value = (korp_object *)s;

    entry->next = head;
    //sfence();
    spool->bucket[key] = entry;

    return s;
}

void string_pool_remove(korp_string_pool* spool, const korp_string* kstring)
{
   	uint16 key = 0;
	uint16 l, len = 0;
    korp_entry *head;
    korp_entry **pre;
    korp_entry *entry; 
    korp_string *s;

    const char* chars = string_to_chars(kstring);
    key = get_chars_hash( chars, &len);

    head = spool->bucket[key];
    pre = &(spool->bucket[key]);
	for(entry = head; entry != NULL; pre = &(entry->next), entry = entry->next ){
        s = (korp_string *)entry->value;
        l = get_string_len(s);
        if( (l==len) && strcmp(string_to_chars(s), chars) == 0){
            *pre = entry->next;
            break;
        }
	}
    return;
}
