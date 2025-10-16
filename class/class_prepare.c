
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "class_loader.h"
#include "vm_interface.h"
#include "utils.h"
#include "gc_for_vm.h"
#include "method.h"

static bool prepare_static_data(korp_class *kclass, uint16 refnum, uint16 unheaded_size)
{
    korp_field* kfield;
    korp_object* constval;
    uint32 i, field_size;

    /* Prepare const fields */
    korp_ref_array *field_info = kclass->field_info;
    uint32 num = class_get_field_num(kclass);

    obj_info header = build_vm_obj_info(YES_RECYCLABLE, NOT_ARRAY, refnum, unheaded_size );
    korp_object* block = (korp_object *)new_dynamic_object(header); 
    
    for(i=0; i<num; i++){
        kfield = (korp_field *)field_info->value[i];
        constval = kfield->constval;
        if( constval == NULL ) continue;
        /* 
         * leave ref constant construction till compiling ldc or class initilizer 
         */
        if( is_ref_type_string( kfield->descriptor ) ) continue;
        field_size = GET_VM_INSTANCE_SIZE( constval ) - VM_OBJ_OVERHEAD;
        memcpy(((uint8 *)block + kfield->offset), ((uint8 *)constval + VM_OBJ_OVERHEAD), field_size );  
        
    }
    
    kclass->static_data = block;
    
    return true;
}

static bool prepare_fields(korp_class* kclass)
{
    korp_ref_array *field_info = kclass->field_info;
    uint32 num = class_get_field_num(kclass);
    
    uint16 static_data_size = 0;
    uint16 instance_data_size = VM_OBJ_OVERHEAD;
    uint8 instance_ref_num = 0;
    uint8 static_ref_num = 0;

    uint16 field_size = 0;
    korp_field* field;
    uint32 i;

    if( kclass->super_class ){
        instance_data_size = kclass->super_class->instance_size;
        instance_ref_num = kclass->super_class->gc_info;
    }   

    /* first go over static ref fields */
    for(i=0; i<num; i++){
        field = (korp_field *)field_info->value[i];
        if( field_is_static(field) ){ 
            if( is_ref_type_string(field->descriptor )){
                static_ref_num ++;
                field->offset = static_data_size;
                static_data_size += BYTES_OF_REF;
            }/* static ref */
        }/* static */
    }/* for each field */

    /* next go over static non-ref fields */
    for(i=0; i<num; i++){
        field = (korp_field *)field_info->value[i];
        if( field_is_static(field) ){ 
            if( !is_ref_type_string(field->descriptor )){
                static_ref_num ++;
                field->offset = static_data_size;
                field_size = typesize_string( field->descriptor ); if(field_size == 0) return false;
                static_data_size += field_size;
            }/* static non-ref */
        }/* static */
    }/* for each field */
    
    if(static_data_size != 0){
        prepare_static_data(kclass, static_ref_num, static_data_size);
    }

    if(class_is_interface(kclass)) return true;

    /* then go through instance ref fields */
    for(i=0; i<num; i++){
        field = (korp_field *)field_info->value[i];
        if( !field_is_static(field) ){ 
            if( is_ref_type_string(field->descriptor )){
                instance_ref_num ++;
                field->offset = instance_data_size;
                instance_data_size += BYTES_OF_REF;
            }/* instance ref */
        }/* instance */
    }/* for each field */

    /* finally go through instance non-ref fields */
    for(i=0; i<num; i++){
        field = (korp_field *)field_info->value[i];
        if( !field_is_static(field) ){ 
            if( !is_ref_type_string(field->descriptor )){
                field->offset = instance_data_size;
                field_size = typesize_string( field->descriptor ); if(field_size == 0) return false;
                instance_data_size += field_size;
            }/* instance ref */
        }/* instance */
    }/* for each field */

    kclass->instance_size = instance_data_size;
    kclass->gc_info = instance_ref_num;

    return true;
}


static bool prepare_supers(korp_class* kclass)
{
    korp_ref_array* interface_info = kclass->interface_info;
    uint32 num = GET_VM_ARRAY_LENGTH( interface_info );
    uint32 i;
    for(i=0; i<num; i++){
        if( class_prepare((korp_class *)interface_info->value[i]) == false ) 
            return false;
    }

    if( !class_is_interface( kclass ) ){
        if( kclass->super_class != NULL ){
            if( class_prepare(kclass->super_class ) == false )
                return false;
        }            
    }

    return true;
}/* prepare_supers */

static bool override_superclass_method( korp_class* kclass, korp_method* kmethod )
{
    bool same_package_as_super = true;
    korp_ref_array* super_vtable_info;
    uint32 num_instance_method;
    uint32 i;
    korp_method *m;

    if( class_is_interface(kclass) ) return false;

    if( kclass->super_class == NULL ) return false;

    super_vtable_info = kclass->super_class->vtable_info;
    num_instance_method = GET_VM_ARRAY_LENGTH(super_vtable_info);

    if( kclass->package != kclass->super_class->package )
        same_package_as_super = false;

    for( i=0; i<num_instance_method; i++){
        m = (korp_method *)super_vtable_info->value[i];
        
        if ( method_equal_signature( kmethod, m ) == false )
            continue;

        if( !same_package_as_super && method_is_default(m) ) 
            return false;
        
        kmethod->offset = m->offset;        
        method_set_overridden( m );
        return true;
    }
 
    return false;

}/* override_superclass_method */

/* Nonsense
static bool prepare_static_vtable( korp_class* kclass, uint32 slotnum)
{
    korp_ref_array* static_vtable = new_vm_array(korp_ref_array, slotnum);
    korp_ref_array *method_info = kclass->method_info;
    uint32 num_method = class_get_method_num(kclass);

    korp_method* method;
    uint32 i;
    for(i=0; i<num_method; i++){
        method = (korp_method *)method_info->value[i];
        assert( method->offset == VM_ARRAY_OVERHEAD + i*BYTES_OF_REF );

        if( ! method_is_static(method) ) continue;

        static_vtable->value[i] = (korp_object *)(korp_object*)method_get_code_entry( method );

    }

    kclass->static_vtable = static_vtable;

    return true;

}
*/

static bool prepare_instance_vtable(korp_class* kclass, uint32 slotnum)
{
    korp_uint32_array* instance_vtable = new_vm_array(korp_uint32_array, slotnum);
    korp_ref_array* instance_vtable_info = new_vm_array(korp_ref_array, slotnum);
    uint32 i,j, num_method;
    korp_ref_array *method_info;

    korp_class* ksuper_class = kclass->super_class;
    uint32 super_slotnum = 0;
    korp_ref_array* super_vtable_info;
    korp_uint32_array* super_vtable;
    korp_method* method;

    if( ksuper_class != NULL ){
        super_vtable_info = ksuper_class->vtable_info;
        super_vtable = ksuper_class->vtable;
        super_slotnum = GET_VM_ARRAY_LENGTH(super_vtable);
    }

    /* memcpy(vtable_info, super_vtable_info) */
    /* memcpy(vtable, super_vtable) */
    for(i=0; i< super_slotnum; i++ ){
        instance_vtable_info->value[i] = super_vtable_info->value[i];
        instance_vtable->value[i] = super_vtable->value[i];
    }

    method_info = kclass->method_info;
    num_method = class_get_method_num(kclass);

    for(i=0,j=0; i<num_method; i++){
        method = (korp_method *)method_info->value[i];
        if( method_is_static(method) ) continue;

        j = method_get_slot_index( method );
        instance_vtable_info->value[j] = (korp_object*)method;
        instance_vtable->value[j] = (uint32)method_get_bincode_entry( method );

    }/* for each method */

    kclass->vtable = instance_vtable;
    kclass->vtable_info = instance_vtable_info;

    return true;

}/* prepare_instance_vtable */

static bool prepare_methods(korp_class* kclass)
{

    korp_method* method;
    uint32 i;

    korp_ref_array *method_info = kclass->method_info;
    uint32 num = class_get_method_num(kclass);

    uint16 num_instance_method = 0;
    uint16 instance_vtable_offset = VM_ARRAY_OVERHEAD;

    korp_ref_array *super_vtable_info = NULL; 
    korp_class *super_class = kclass->super_class;

    if( !class_is_interface(kclass) ){
        instance_vtable_offset += sizeof(korp_class);
        if( super_class != NULL ){
            num_instance_method = GET_VM_ARRAY_LENGTH(super_class->vtable);
            instance_vtable_offset += num_instance_method*BYTES_OF_REF; 
        }
    }

    for(i=0; i<num; i++){
        method = (korp_method *)method_info->value[i];
        if( ! method_is_static(method) ){
            if( !class_is_interface(kclass) ) 
                if( kclass->super_class && override_superclass_method( kclass, method ) == true )
                    continue;
            
            method->offset = instance_vtable_offset;
            instance_vtable_offset += BYTES_OF_REF;     
            num_instance_method ++;

        }/* non-static */
    }/* for each method */
    
    if( class_is_interface(kclass) ) 
        return true;

    if( prepare_instance_vtable(kclass, num_instance_method) == false ) 
        return false;

    return true;

}/* prepare_methods */

static bool prepare_interface_table( korp_class* kclass)
{
    korp_ref_array* intf_table_info = kclass->interface_table_info;
    uint32 intf_num = GET_VM_ARRAY_LENGTH(intf_table_info);    

    korp_ref_array* intf_table = new_vm_array(korp_ref_array, intf_num);
    uint32 num_method = GET_VM_ARRAY_LENGTH(kclass->vtable_info);

    uint32 i, j, k, num_intf_method;
    korp_class* kintf;
    korp_ref_array* intf_method_table;

    for(i=0; i<intf_num; i++){
        korp_method* kintf_method;
        kintf = (korp_class *)intf_table_info->value[i];
	    num_intf_method = class_get_method_num(kintf);
	    intf_method_table = new_vm_array(korp_ref_array, num_intf_method); /* may have clinit */

        for(j=0; j<num_intf_method; j++){
            korp_method* kmethod;
            kintf_method = (korp_method *)kintf->method_info->value[j];
            if( kintf_method->flags.is_clinit ) continue;

            for(k=0; k<num_method; k++){
                kmethod = (korp_method *)kclass->vtable_info->value[k];
                if( method_equal_signature(kmethod, kintf_method) )
                    break;                    
            }/* for each instance method */
            assert( (k<num_method) || class_is_abstract(kclass) );

            intf_method_table->value[j] = (korp_object *)kmethod;
            assert( j == (uint32)(kintf_method->offset - VM_ARRAY_OVERHEAD)/BYTES_OF_REF );

        }/* for each method of the interface */

        intf_table->value[i] = (korp_object *)intf_method_table;

    }/* for each interface */

    kclass->interface_table = intf_table;

    return true;

}/* prepare_interface_table */

static bool prepare_interfaces( korp_class* kclass)
{
    korp_ref_array *super_intf_table_info = NULL; 
    korp_ref_array *super_intf_table = NULL; 
    korp_class *ksuper_class = kclass->super_class;
    uint32 super_slotnum = 0;
    uint32 intf_num, intf_table_slotnum, i, j;
    korp_class *kinterface, **temp_intf_table_info, **table;
    korp_ref_array *kinterface_info, *interface_table_info;
    uint32 last, index = 0;

    if( ksuper_class != NULL ){
        super_intf_table_info = ksuper_class->interface_table_info;
        super_intf_table = ksuper_class->interface_table;
        super_slotnum = GET_VM_ARRAY_LENGTH(super_intf_table);
    }

    intf_num = class_get_interface_num(kclass);
    intf_table_slotnum = super_slotnum;

    kinterface_info = kclass->interface_info;
    for(i=0; i<intf_num; i++){
        kinterface = (korp_class*)kinterface_info->value[i];
        intf_table_slotnum += GET_VM_ARRAY_LENGTH(kinterface->interface_table_info);
    }

    if(class_is_interface(kclass)) 
        intf_table_slotnum++; /* include self */

    if( intf_table_slotnum==0 ) return true;

    /* this is temp because there might be duplicate items */
    //temp_intf_table_info = (korp_class **)stack_alloc(sizeof(intf_table_slotnum * BYTES_OF_REF));
    temp_intf_table_info = (korp_class **)stack_alloc(intf_table_slotnum * BYTES_OF_REF); //chunrong

    /* copy superclass interface_vtable_info */
    for(i=0; i<super_slotnum; i++ ){
        temp_intf_table_info[index] = (korp_class *)super_intf_table_info->value[index];
        index ++;
    }

    /* copy interfaces interface_vtable_info */
    for(i=0; i<intf_num; i++ ){
        korp_ref_array* superintf_intf_table_info;
        uint32 superintf_intf_table_num;
        kinterface = (korp_class*)kinterface_info->value[i];
        superintf_intf_table_info = kinterface->interface_table_info;
        superintf_intf_table_num = GET_VM_ARRAY_LENGTH(superintf_intf_table_info);
        for( j=0; j<superintf_intf_table_num; j++ ){
            temp_intf_table_info[index] = (korp_class *)superintf_intf_table_info->value[j];
            index ++;
        }
    }

    /* include self */
    if(class_is_interface(kclass)) 
        temp_intf_table_info[index] = kclass;

    /* eliminate duplicates */
    table = temp_intf_table_info;
    last = intf_table_slotnum;
    for(i=0; i<last; i++){
        for(j=i+1; j<last; j++ ){
            if(table[i] == table[j]){
                /* replace the duplicate with last one */
                while ( table[j] == table[--last] );
                if(j!=last) table[j] = table[last];

            }/* if duplicate */
        }/* j */
    }/* i */
    intf_table_slotnum = last;

    /* copy intf_table_info from temp to kclass */
    interface_table_info = new_vm_array(korp_ref_array, intf_table_slotnum);
    for( i=0; i<intf_table_slotnum; i++ ){
        interface_table_info->value[i] = (korp_object *)temp_intf_table_info[i];  
    }

    kclass->interface_table_info = interface_table_info;

    if( class_is_interface(kclass) )
        return true;
    
    if( prepare_interface_table(kclass) == false )
        return false;

    return true;

}/* prepare_interfaces */

#define unlock_return(X)                        \
    do{                                         \
        return X;                               \
    }while(0)

/*
 * this is tedious but essential, it decides how JIT will
 * access the fields and the methods
 *
 */
bool class_prepare(korp_class *kclass)
{
	/* Test: if already prepared, simply return */
    /* exclusive prepraration */
    if( kclass->state & CS_Prepared ) return true;
    if( kclass->state & CS_Prepared ) unlock_return(true);
    
    /* Prepare supers interfaces and super classes */
    if( prepare_supers(kclass) == false ) unlock_return(false);

    /* Prepare fields for both static and instance */
    if( prepare_fields(kclass) == false ) unlock_return(false);

    /* Prepare instance methods */
    if( prepare_methods(kclass) == false ) unlock_return(false);

    /* Prepare interface methods */
    if( prepare_interfaces(kclass) == false ) unlock_return(false);

    /**
     * Finish Preparation  
     *   set class special properties --> what are they?
     */
    
    kclass->state |= CS_Prepared;
    unlock_return(true);
}
