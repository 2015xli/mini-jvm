

#include <stdio.h>
#if 0
#ifndef WIN32
#else
#include <io.h>
#include <fcntl.h>
#endif /* #ifdef __POSIX__ #else */
#endif

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "vm_interface.h"
#include "gc_for_vm.h"
#include "vm_for_gc.h"
#include "runtime.h"
#include "class_loader.h"
#include "class_table.h"
#include "class_utils.h"
#include "method.h"
#include "utils.h"
//#include "jar.h"

/* array_elem_size, see korp_types.h for the type defination */
uint8 array_elem_size[13] = {
   /* ARRAY_UINT8     =0,  */   1,
   /* ARRAY_UINT16    =1,  */   2,
   /* ARRAY_UINT32    =2,  */   4,
   /* ARRAY_UINT64    =3,  */   8,
   /* ARRAY_BOOLEAN   =4,  */   1,
   /* ARRAY_CHAR      =5,  */   2,
   /* ARRAY_FLOAT     =6,  */   4,
   /* ARRAY_DOUBLE    =7,  */   8,
   /* ARRAY_BYTE      =8,  */   1,
   /* ARRAY_SHORT     =9,  */   2,
   /* ARRAY_INT       =10, */   4,
   /* ARRAY_LONG      =11, */   8,
   /* ARRAY_REF       =12  */   4   /* for calculation */
};    

/* create Class's Java object */
bool prepare_java_class_object(korp_class* kclass)
{
    korp_class** kclass_slot;
    korp_object* jclass = new_java_object(kenv->globals->java_lang_Class_class);
    kclass_slot = (korp_class**)((uint8*)jclass + kenv->jclass_kclass_offset);
    *kclass_slot = kclass;
    kclass->java_object = jclass;
    return true;
}

korp_entry* new_class_loader()
{
    korp_entry* entry;
    korp_class_loader* sys_loader = new_vm_object(korp_class_loader);
    sys_loader->class_table = new_class_table();
    entry = new_vm_object(korp_entry);
    entry->value = (korp_object *)sys_loader;
    return entry;
}

korp_class* collocate_class_vtable(korp_class* kclass)
{
    korp_class* newclass;
    uint32 vtable_size = get_vm_object_size(*(obj_info *)(kclass->vtable));
    newclass = new_special_vm_object(korp_class, sizeof(korp_class)+vtable_size );
    uint8* vtable = ((uint8 *)newclass + sizeof(korp_class));
    memcpy(newclass, kclass, sizeof(korp_class));
    memcpy(vtable, kclass->vtable, vtable_size);

    {/* adjust ownerclass pointer */
        korp_method* kmethod;
        uint32 i;
        korp_ref_array *method_info = kclass->method_info;
        uint32 num = class_get_method_num(kclass);
        for(i=0; i<num; i++){
            kmethod = (korp_method*)method_info->value[i];
            kmethod->owner_class = newclass;
        }       
    }

    {/* adjust ownerclass pointer */
        uint16 i;
        korp_field* kfield;
        korp_ref_array* field_info = kclass->field_info;
        uint16 num = GET_VM_ARRAY_LENGTH(field_info);
        for(i=0; i<num; i++){
            kfield = (korp_field *)field_info->value[i];
            kfield->owner_class = newclass;
        }
    }

    free_vm_object((korp_object *)kclass->vtable);
    free_vm_object((korp_object *)kclass);

    return newclass;
}
/*
 * set fields of a field structure
 */
bool field_set(korp_field *field,
               const char *name,
               const char *descriptor,
               unsigned modifier,
               korp_class *kclass)
{
    field->name = string_pool_lookup( kenv->string_pool, name);
    field->descriptor = string_pool_lookup( kenv->string_pool, descriptor);
    field->modifier = modifier;
    field->owner_class = kclass;
    field->offset = 0;

    return true;
} /* field_set */

/*
 * set fields of an array class
 */
bool set_array_class(korp_class_loader* loader,
                     korp_class *array_class,
                     const char *classname,
                     korp_class *array_element_class)
{
    korp_class *array_base_class;
    korp_array_info *array_info;
    korp_ref_array *field_info;
    korp_field *field;

    array_class->flags.is_array = true;
    array_class->name = string_pool_lookup(kenv->string_pool, classname);

    array_base_class = array_get_base_class(loader, array_class);

    array_info = new_vm_object(korp_array_info);
    {
        array_info->num_dimension = array_name_get_dimension(classname);
        array_info->array_element_class = array_element_class;
        array_info->array_base_class = array_base_class;
        array_info->is_primitive_array = class_is_primitive(array_base_class);
    }

    array_class->array_info = array_info;

    field_info = new_vm_array(korp_ref_array, 1);
    array_class->field_info = field_info;
    field = new_vm_object(korp_field); 
    field_set(field, "length", "I", MOD_PUBLIC|MOD_FINAL, array_class);
    
    array_class->field_info->value[0] = (korp_object *)field;
    array_class->super_class = kenv->globals->java_lang_Object_class;
    array_class->package = array_base_class->package;
    array_class->class_loader = array_base_class->class_loader;

    if(array_info->is_primitive_array){
        array_class->modifier |= MOD_PUBLIC;
    }
    else {
        array_class->modifier |= (array_base_class->modifier & MOD_PUBLIC);
    }

    return true;
} /* set_array_class */

korp_class* preload_primitive_class(char* classname)
{
    korp_class* kclass;
    korp_string* kclassname = string_pool_lookup(kenv->string_pool, classname);
    
    kclass = new_vm_object(korp_class);
    kclass->name = kclassname;
    kclass->flags.is_primitive = 1;
    class_table_insert(get_sys_loader()->class_table, kclass); 

    kclass->super_class = kenv->globals->java_lang_Object_class;
    
    //if( class_prepare(kclass) == NULL ) 
    if( !class_prepare(kclass) )
        return NULL;

    return kclass;
}

/*
 * load an array class by creating it
 */

korp_class *load_class_array_creation(korp_class_loader *loader,
                                      const korp_string *kclassname, 
                                      korp_loader_exception *exc )
{
    korp_class *array_class = NULL;
    korp_class *array_elem_class;
    char *elem_classname; 
    korp_java_type type;
    const char *classname = string_to_chars( kclassname );
    korp_string *kelem_classname;

    type = (korp_java_type)classname[1];
    elem_classname = stack_alloc(256);
    if( is_ref_type_type(type) ){
        if(type==JAVA_TYPE_CLASS){
            char* c;
            strcpy(elem_classname, classname+2);
            c = strchr(elem_classname+2, ';');
            *c = '\0';
        }else{
            assert(type==JAVA_TYPE_ARRAY);
            strcpy(elem_classname, classname+1);
        }  

        kelem_classname = string_pool_lookup(kenv->string_pool, elem_classname);
        array_elem_class = load_class(loader, kelem_classname, exc );   

    }else{

        array_elem_class = get_primitive_class_from_type(type); 
    }

    if( array_elem_class == NULL ) return NULL;

    array_class = new_vm_object(korp_class);
    set_array_class(loader, array_class, classname, array_elem_class);

    //class_loader_check( array_class->class_loader, loader);
    class_table_insert(loader->class_table, array_class);

    return array_class;
} /*load_class_array_creation*/

/*
 * load a class from Jar/Zip file
 */

korp_class *load_class_from_jarfile(korp_class_loader *loader,
                                    korp_string *pathname,
                                    const korp_string *classname, 
                                    korp_loader_exception *exc )

{
    return NULL;
} /*load_class_from_jarfile*/

/*
 * load a class from Classfile 
 */

korp_class *load_class_from_classfile(korp_class_loader *loader,
                                      const korp_string *kfilename,
                                      const korp_string *kclassname, 
                                      korp_loader_exception *exc )

{
    int buf_len, file_handle, buf_read;
    char *buffer;
    korp_class *kclass;
    bool ok;

    const char* filename = string_to_chars(kfilename);

#ifndef WIN32
	struct stat tstat;
	if (stat(filename, &tstat) != 0) return NULL;
#else
	struct _stat tstat;
    if (_stat(filename, &tstat) != 0) return NULL;
#endif

    buf_len = tstat.st_size;

#ifndef WIN32
	file_handle = open(filename, O_RDONLY);
#else
	file_handle = _open(filename, _O_RDONLY | _O_BINARY);
#endif

    if(file_handle == -1){
        printf("Error: open file %s, %s\n", filename, strerror (errno));
		return NULL;
    }

    buffer = stack_alloc(buf_len);
    buf_read = 0;

#ifndef WIN32 
    buf_read = read(file_handle, buffer, buf_len);
#else
    buf_read = _read(file_handle, buffer, buf_len);
#endif
    
    if( buf_read<0 || buf_read<buf_len){
        printf("Error: read file %s, %s\n", filename, strerror (errno));
		return NULL;
    }

    kclass = new_vm_object(korp_class);
    kclass->classfile_name = string_pool_lookup(kenv->string_pool, filename);
    kclass->name = (korp_string *)kclassname;
    kclass->class_loader = loader;

    ok = class_parse(kclass, buffer, buf_len, exc);

    if( !ok ) {
        /* delete_object(kclass); we have GC.*/
        kclass = NULL;
    }

#ifndef WIN32
    close(file_handle);
#else
    _close(file_handle);
#endif

    //if(kclass != NULL )
    //    class_add_to_package(kclass);

    return kclass;

} /*load_class_from_jarfile*/


/*
 * load a class from media
 */

korp_class *load_class_from_media(korp_class_loader *loader,
                                  const korp_string *kclassname, 
                                  korp_loader_exception *exc )
{
    const char *classpath, *classname = NULL;
    korp_string *kfilename, *kpathname, *kfullpath;
    char *temp_pathes;
    korp_class *kclass;

    if ( kclassname !=NULL )
        classname = string_to_chars(kclassname);

    if ( kclassname==NULL || *classname == 0){
        *exc = LD_ClassCircularityError;
        return NULL;
    }


    kfilename = classname_get_classfile_name(kclassname );
        
    classpath = string_to_chars( kenv->globals->classpath );
    temp_pathes = stack_alloc( 256);
    strcpy(temp_pathes, classpath);
    
    kpathname = NULL;
    kclass = NULL;
    while( (temp_pathes = get_next_path( temp_pathes)) != NULL ){
        
        kpathname = string_pool_lookup(kenv->string_pool, temp_pathes );

//        if(is_jar(temp_pathes)){
//            kclass = load_class_from_jarfile(loader, kpathname, kclassname, exc );
//        }else{
            kfullpath = get_full_filename(kpathname, kfilename);
            kclass = load_class_from_classfile(loader, kfullpath, kclassname, exc );
//        }  //chunrong, 02-24-2006
        if( kclass != NULL){
            return kclass;
        }
    }

    *exc = LD_NoClassDefFoundError;
    return NULL;

} /*load_class_from_media*/

/*
 * load superclass and interfaces of a class
 */

bool load_class_supers(korp_class *kclass, 
                       korp_loader_exception* exc )
{
    unsigned i;
    char *intfc_name=NULL;
    korp_class* kinterface=NULL;
    korp_object** intfc_slot;
    uint32 intfc_num;

    korp_class *super_class = NULL;
    korp_string *super_name = kclass->super_name;
    kclass->state |= CS_LoadingSupers;

    if(  super_name == NULL ){
        if( kclass->name != kenv->globals->java_lang_Object_string){
            *exc = LD_ClassFormatError;
            return false;
        }
    
    }else{
        super_class = load_class(kclass->class_loader, super_name, exc );   
        if(super_class == NULL ){
            return false;
        }
        if (class_is_interface(super_class) || class_is_final(super_class) ) {
            *exc = LD_IncompatibleClassChangeError;
            return false;
        }
    }

    kclass->super_class = super_class;

    intfc_num = GET_VM_ARRAY_LENGTH(kclass->interface_info);
    for(i=0; i<intfc_num; i++ ){
        intfc_slot = &(kclass->interface_info->value[i]);
        kinterface = load_class(kclass->class_loader, (korp_string *)*intfc_slot, exc );
        if( kinterface == NULL ){
            return false;
        }

        *intfc_slot = (korp_object *)kinterface;
    }

    kclass->state ^= CS_LoadingSupers;
    return true;
}

/*
 * entry of class loader
 */

korp_class *load_class(korp_class_loader *loader,
                       const korp_string *kclassname, 
                       korp_loader_exception *exc )
{
    bool ok=false; 
    korp_class *kclass=NULL;
    const char* classname = string_to_chars(kclassname);
    kclass = class_table_search(loader->class_table, kclassname);

#ifdef _DEBUG
    if( strcmp(classname, "com/sun/cldc/io/ConnectionBase")==0 )
        int t = 0;
#endif

    if(kclass != NULL ){
        if(kclass->state & CS_LoadingSupers ){
            *exc = LD_ClassCircularityError;
            return NULL;
        }
        return kclass;

    }else{        
        if(class_name_is_array(classname)){
            kclass = load_class_array_creation(loader, kclassname, exc );
        }else{
            kclass = load_class_from_media(loader, kclassname, exc );
        }
    }
    
    if(kclass == NULL ) return NULL;

    if(! kclass->flags.is_array){
        ok = load_class_supers(kclass, exc);
        if( !ok ) return NULL;
    }

    ok = class_verify(kclass);
    if( !ok ){
        *exc = LD_VerifyError;
        return NULL;
    }
   
    ok = class_prepare(kclass);
    if( !ok ){
        *exc = LD_IncompatibleClassChangeError;
        return NULL;
    }

    /* pack class and vtable structure */
    if( !class_is_interface(kclass) )
        kclass = collocate_class_vtable(kclass);

    if( kclass->name != kenv->globals->java_lang_Object_string &&
        kclass->name != kenv->globals->java_lang_Class_string )
    {
        prepare_java_class_object(kclass);
    }

    kclass->package = (korp_package *)class_create_package_name(kclass);
    kclass->state |= CS_Loaded;
    class_table_insert(loader->class_table, kclass);
    return kclass;

} /*load_class*/

