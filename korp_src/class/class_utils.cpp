
#include <string.h>
#include <assert.h>

#include "korp_types.h"
#include "class_loader.h"
#include "method.h"
#include "utils.h"

bool field_initialize(korp_class* kclass, korp_field* kfield)
{
    
    kfield->owner_class = kclass;
    return true;
}

korp_class *array_get_base_class(korp_class_loader* loader, korp_class *array)
{
    korp_string* kbase_name;
    const char* array_name = string_to_chars( array->name );
    char *base_name;
    char* pos;

    while( *array_name ++ == '[');
    array_name--;
    
    base_name = (char*)array_name;
    if( *base_name =='L'){
        base_name = stack_alloc(256);
        strcpy(base_name, ++array_name);
        pos = strchr(base_name, ';');
        *pos = '\0';
    }

    kbase_name = string_pool_lookup( kenv->string_pool, base_name );
    return class_table_search( loader->class_table, kbase_name );
}

uint16 array_name_get_dimension(const char *array_name)
{   
    uint8 dimension = 0;
    while( *array_name ++ == '[') dimension++;
    
    return dimension;
}

bool class_name_is_array(const char*array_name)
{
    if( *array_name == '[') return true;
    else return false;
}


bool is_subclass(korp_class *super, korp_class *sub)
{
    while( super != sub ){
        if( sub->super_class != NULL ) 
            sub = sub->super_class;
        else
           return false;
    }

    return true;
}

korp_string* class_create_package_name(korp_class* kclass)
{    
    char* c;
    char* p=NULL; 
    char* classname = stack_alloc(256);
    strcpy(classname, string_to_chars(kclass->name));
    c= classname;

    while( c = strchr(c ,'/') ){
        p=c;
        c++;
    }
    if( p!=NULL) *p = '\0';
    return string_pool_lookup(kenv->string_pool, classname);

}

korp_field* class_lookup_field_recursive(korp_class* kclass, korp_string* ks_name, korp_string* ks_desc)
{
    korp_field *kf = NULL;
	korp_class *kc_temp, *kintf;
    korp_ref_array* intf_info = NULL;
    uint32 intf_num, i;

    for(kc_temp=kclass; kc_temp!=NULL; kc_temp = kc_temp->super_class ) {
        kf = class_lookup_field_own(kc_temp, ks_name, ks_desc);
        if(kf != NULL) return kf;
    }

    /* not class, lookup interface */
    intf_info = kclass->interface_info;
    intf_num = GET_VM_ARRAY_LENGTH(intf_info);    

    for(i=0; i<intf_num; i++){
        kintf = (korp_class *)intf_info->value[i];
        kf = class_lookup_field_recursive(kclass, ks_name, ks_desc);
        if( kf!= NULL) return kf;
    }

    return NULL;
}

korp_field* class_lookup_field_own(korp_class* kclass, korp_string* kname, korp_string* kdesc)
{
    uint16 i;
    korp_field* kfield;
    korp_ref_array* field_info = kclass->field_info;
    uint16 num = GET_VM_ARRAY_LENGTH(field_info);
    for(i=0; i<num; i++){
        kfield = (korp_field *)field_info->value[i];
        if(kfield->name == kname && kfield->descriptor == kdesc ){
            return kfield;
        }
    }

    return NULL;
}

korp_method* class_lookup_method_own(korp_class* kclass, korp_string* ks_name, korp_string* ks_desc)
{
    korp_method* kmethod;
    uint32 i;
    korp_ref_array *method_info = kclass->method_info;
    uint32 num = class_get_method_num(kclass);
    for(i=0; i<num; i++){
        kmethod = (korp_method*)method_info->value[i];
        if(kmethod->name == ks_name && kmethod->descriptor == ks_desc)
            return kmethod;
    }
    
    return NULL;
}

korp_method* class_lookup_method_recursive(korp_class* kclass, korp_string* ks_name, korp_string* ks_desc)
{
    korp_method *km = NULL;
	korp_class *kc_temp, *kintf;
    korp_ref_array* intf_info = NULL;
    uint32 intf_num, i;

    for(kc_temp=kclass; kc_temp!=NULL; kc_temp = kc_temp->super_class ) {
        km = class_lookup_method_own(kc_temp, ks_name, ks_desc);
        if(km != NULL) return km;
    }

    /* not class, lookup interface */
    intf_info = kclass->interface_info;
    intf_num = GET_VM_ARRAY_LENGTH(intf_info);    

    for(i=0; i<intf_num; i++){
        kintf = (korp_class *)intf_info->value[i];
        km = class_lookup_method_recursive(kclass, ks_name, ks_desc);
        if( km!= NULL) return km;
    }

    return NULL;
    
}

korp_class* java_object_get_class( korp_object *jobj)
{
    return (korp_class *)((*(obj_info *)jobj) & POINTER_MASK);
}

uint32 __stdcall class_get_interface_method_offset( korp_class* kclass, korp_method* km_intf )
{
    korp_ref_array* intf_table_info = kclass->interface_table_info;
    korp_ref_array* intf_table = kclass->interface_table;
    korp_class* kc_intf = km_intf->owner_class;
    int n_intf = GET_VM_ARRAY_LENGTH( intf_table_info ); 
    int i;
    
    for( i=0; i<n_intf; i++){
        if( kc_intf == (korp_class*)intf_table_info->value[i] ) break;
    }
    assert( i<n_intf );

    intf_table = (korp_ref_array*)intf_table->value[i];
    i = (km_intf->offset-VM_ARRAY_OVERHEAD)/BYTES_OF_REF;
    return ((korp_method*)intf_table->value[i])->offset;
}
