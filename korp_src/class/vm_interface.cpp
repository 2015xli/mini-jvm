
#include <assert.h>

#include "korp_types.h"
#include "class_loader.h"
#include "method.h"
#include "utils.h"

uint16 get_vm_object_size(obj_info header)
{
    /* return GET_VM_OBJ_SIZE_H(header);*/
    uint32 h = header;
    uint16 size = 0;

    assert( IS_VM_OBJECT_H(h)  );

    if( h & IS_ARRAY_MASK ){
        uint8 array_type = ( h & ARRAY_TYPE_MASK ) >> ARRAY_TYPE_SHIFT;
        if( array_type < ARRAY_REF )
            size = 1<<array_type;
        else
            size = BYTES_OF_REF;

        return size * GET_VM_ARRAY_LENGTH_H(h) + VM_ARRAY_OVERHEAD;
            
    }else{
        return GET_VM_INSTANCE_SIZE_H(h);
    }

}
korp_java_type cp_get_field_type(korp_ref_array* cpool, uint16 index)
{
    korp_string* descriptor;
    
    if(cp_is_resolved(cpool, index)){
        descriptor = ((korp_field *)cpool->value[index])->descriptor;
    }else{
        /* get Methodref's NameAndType index */
        index = ((korp_2uint16 *)cpool->value[index])->value[1];
        /* get NameAndType's Type */
        descriptor = (korp_string*)((korp_2ref *)cpool->value[index])->value[1];    
    }

    return (korp_java_type)*string_to_chars(descriptor);
}

korp_string* cp_get_method_descriptor(korp_ref_array* cpool, uint16 index)
{
    if(cp_is_resolved(cpool, index))
        return ((korp_method *)cpool->value[index])->descriptor;

    /* get Methodref's NameAndType index */
    index = ((korp_2uint16 *)cpool->value[index])->value[1]; 
    /* get NameAndType's Type */
    return (korp_string*)((korp_2ref *)cpool->value[index])->value[1]; 
}

korp_java_type cp_get_const_type(korp_ref_array* cpool, uint16 index)
{
    switch (cp_get_tag(cpool, index)){
        case TAG_String:
            return JAVA_TYPE_STRING;
        case TAG_Integer:
            return JAVA_TYPE_INT;
        case TAG_Float:
            return JAVA_TYPE_FLOAT;
        case TAG_Long:
            return JAVA_TYPE_LONG;
        case TAG_Double:
            return JAVA_TYPE_DOUBLE;
    
        default:
            return JAVA_TYPE_WRONG;
    }

    return JAVA_TYPE_WRONG;
}

korp_object* cp_get_const_value(korp_ref_array* cpool, uint16 index)
{
    /* uint32 cp_item; */
    switch( cp_get_const_type(cpool, index) ){
        default:
            return cpool->value[index];
        case JAVA_TYPE_WRONG:
            return NULL;
    }
    return NULL;
}

/* for new(), the class can't be abstract. check it in context */
korp_class* class_resolve_class(korp_class* kclass, uint16 index, korp_loader_exception* exc)
{
    korp_string* kclassname;
    korp_class* kresult = NULL;
    korp_ref_array* cpool = kclass->constant_pool;
    korp_object* kvalue = cpool->value[index];
    if( cp_is_resolved(cpool,index))
        return (korp_class*)kvalue;

    /* class is not resolved, then the slot is its name */
    kclassname = (korp_string*)kvalue;

    kresult = load_class(kclass->class_loader, kclassname, exc);

    if( kresult == NULL ) return NULL;

    if (class_is_public(kresult) 
        || kresult == kclass 
        || kresult->package == kclass->package ) 
    {
        cpool->value[index] = (korp_object *)kresult;
        cp_set_resolved(cpool,index);
        return kresult;
	}

	*exc = LD_IllegalAccessError;

    return NULL;
}

/* should check static, virtual, special */
korp_field* class_resolve_field(korp_class* kclass, uint16 index, korp_loader_exception* exc)
{
    uint16 cp_item;
    korp_class *kresult = NULL;
    korp_string *ks_name, *ks_descriptor;
    korp_ref_array* cpool = kclass->constant_pool;
    korp_field* kfield = (korp_field*)cpool->value[index];
        
    if( cp_is_resolved(cpool,index))
        return kfield;

    /* get field info (class, signature) from cpool */
    /* class */
    cp_item = ((korp_2uint16 *)kfield)->value[0];
    if( !cp_is_resolved(cpool,cp_item)){
        kresult = class_resolve_class(kclass, cp_item, exc);
        if( kresult == NULL ) return NULL;
    }else{
        kresult = (korp_class*)cpool->value[cp_item];
    }
            
    /* signature */
    cp_item = ((korp_2uint16 *)kfield)->value[1];
    ks_name = (korp_string *)((korp_2ref *)(cpool->value[cp_item]))->value[0];
    ks_descriptor = (korp_string *)((korp_2ref *)(cpool->value[cp_item]))->value[1];

    kfield = class_lookup_field_recursive(kresult, ks_name, ks_descriptor);

    if(kfield == NULL){
        *exc = LD_NoSuchFieldError;
        return NULL;    
    }

    {   /*check access right */
        korp_class* kf_class = kfield->owner_class;
        korp_package* kf_pkg = kf_class->package;
        korp_package* kc_pkg = kresult->package;

        if( (member_is_public(kfield) || (kf_class == kresult)) 
                || ( member_is_protected(kfield) && ( (kf_pkg==kc_pkg) || is_subclass(kf_class, kresult) ) ) 
                || ( member_is_default(kfield)&& (kf_pkg==kc_pkg) )
          )
        {
            cpool->value[index] = (korp_object *)kfield;
            cp_set_resolved(cpool, index);
            return kfield;
        }
    }

    *exc = LD_IllegalAccessError;
    return NULL; 
}

/* should check static, virtual, special */
korp_method* class_resolve_method(korp_class* kclass, uint16 index, korp_loader_exception* exc)
{
    uint16 cp_item;
    korp_class *kresult = NULL;
    korp_string *ks_name, *ks_descriptor;
    korp_ref_array* cpool = kclass->constant_pool;
    korp_method* kmethod = (korp_method*)cpool->value[index];
        
    if( cp_is_resolved(cpool,index))
        return kmethod;

    /* get method info (class, signature) from cpool */
    /* class */
    cp_item = ((korp_2uint16 *)kmethod)->value[0];
    if( !cp_is_resolved(cpool,cp_item)){
        kresult = class_resolve_class(kclass, cp_item, exc);
        if( kresult == NULL ) return NULL;
    }else{
        kresult = (korp_class*)cpool->value[cp_item];
    }

    if( ( (cp_get_tag(cpool, index) == TAG_Methodref) 
          && class_is_interface( kresult) )
      ||( (cp_get_tag(cpool, index) == TAG_InterfaceMethodref) 
          && !class_is_interface( kresult) )
    ){
        *exc = LD_NoSuchMethodError;
        return NULL;
    }
            
    /* signature */
    cp_item = ((korp_2uint16 *)kmethod)->value[1];
    ks_name = (korp_string *)((korp_2ref *)(cpool->value[cp_item]))->value[0];
    ks_descriptor = (korp_string *)((korp_2ref *)(cpool->value[cp_item]))->value[1];

    kmethod = class_lookup_method_recursive(kresult, ks_name, ks_descriptor);

    if(kmethod == NULL){
        *exc = LD_NoSuchMethodError;
        return NULL;    
    }

    {   /*check access right */
        korp_class* km_class = kmethod->owner_class;
        korp_package* km_pkg = km_class->package;
        korp_package* kc_pkg = kresult->package;

        if( (member_is_public(kmethod) || (km_class == kresult)) 
                || ( member_is_protected(kmethod) && ( (km_pkg==kc_pkg) || is_subclass(km_class, kresult) ) ) 
                || ( member_is_default(kmethod)&& (km_pkg==kc_pkg) )
          )
        {
            cpool->value[index] = (korp_object *)kmethod;
            cp_set_resolved(cpool, index);
            return kmethod;
        }
    }

    *exc = LD_IllegalAccessError;
    return NULL; 
}
