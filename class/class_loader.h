
#ifndef _CLASS_LOADER_H
#define _CLASS_LOADER_H

#include "vm_object.h"
#include "string_pool.h"
#include "class_table.h"
#include "class_utils.h"

enum korp_cpool_tag{
	TAG_NULL				=0,	// pointer to tags array
	TAG_Utf8				=1,
	TAG_Integer			    =3,
	TAG_Float				=4,
	TAG_Long				=5,
	TAG_Double				=6,
	TAG_Class				=7,
	TAG_String				=8,
	TAG_Fieldref			=9,
	TAG_Methodref			=10,
	TAG_InterfaceMethodref	=11,
	TAG_NameAndType		    =12,
};

#define TAG_MASK 0x0F
#define RESOLVED_MASK 0x80

/**
 * definition of "korp_loader_exception"
 * class loader exceptions -- see pg 611 of the language definition to see list
 * of exceptions
 *
 * see pg 39 of the JVM spec for an overview of the LinkageError exceptions
 * see pg 45 of the JVM spec for an overview of the 
 * IncompatibleClassChangeError exceptions
 *
 * Error
 *   LinkageError
 *     ClassCircularityError		Loader
 *	   ClassFormatError				Loader
 *	   NoClassDefFoundError			Loader
 *	   IncompatibleClassChangeError			Linker				pg 45
 *       AbstractMethodError						Preperation
 *		 IllegalAccessError					Linker
 *		 InstantiationError					Linker
 *		 NoSuchFieldError					Linker
 *		 NoSuchMethodError					Linker
 *     UnsatisfiedLinkError									
 *     VerifyError
 *
 * IncompatibleClassChangeError exceptions
 *
 * IllegalAccessError occurs when a a field, method or class reference 
 * occurs and the referenced field (method) is private, protected or not public
 * or the accessed class is not public.
 *
 * InstantiationError occurs when an interface or abstract class is 
 * being instantiated by the byte codes.
 *
 * NoSuchField(Method)Error occurs when a reference refers to a specific field
 * (method) of a specific field or interface, but the class or interface does 
 * not declare a field of that name (it is specifically not sufficient for it 
 * simply to be an inherited field (method) of that class or interface).
 *
 * This is made anonymous just because we want to not expose its detail to JITs,
 * so we declare its opaque interface for JITs in interface/jit_intf.h, while gcc
 * in Linux doesn't permit two declarations with same name. please find jit_intf.h
 * for explanations there.
 */
#if 0
typedef enum _korp_loader_exception {
	LD_OK = 0,
	LD_NoClassDefFoundError,
	LD_ClassFormatError,
	LD_ClassCircularityError,
	LD_IncompatibleClassChangeError,
	LD_AbstractMethodError,		/* occurs during preparation */
	LD_IllegalAccessError,
	LD_InstantiationError,
	LD_NoSuchFieldError,
	LD_NoSuchMethodError,
	LD_UnsatisfiedLinkError,
	LD_VerifyError
}korp_loader_exception;
#endif

/* class states during loading and initialization*/
enum korp_class_state {
	CS_Start                = 0x01,	/* initial state */
	CS_LoadingSupers        = 0x02, /* loading super class and super interfaces */
	CS_Loaded               = 0x04,	/* successfully loaded */
    CS_InstanceSizeComputed = 0x08, /* still preparing the class but size of its instances is known */
	CS_Prepared             = 0x10,	/* successfully prepared */
	CS_Initializing         = 0x20,	/* initializing the class */
	CS_Initialized          = 0x40,	/* class initialized */
    CS_Error                = 0x80, /* bad class or the initializer failed */
};

#define CLASSFILE_MAGIC 0xCAFEBABE
#define CLASSFILE_MAJOR	45

/*
 * entry of class loader
 */
typedef struct _korp_class_loader{
    obj_info   header;
    korp_class_table* class_table;
    korp_object* java_object;
}korp_class_loader;
#define korp_class_loader_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 2, NOT_RECYCLABLE)

/* _struct_jnc_field and _korp_jnc_field are same */
typedef struct _struct_jnc_field{
    char *classname;
    char *fieldname;
    char *descriptor;
    uint16 modifier;
}struct_jnc_field;

typedef struct _korp_jnc_field{
    obj_info header;
    korp_string* classname;
    korp_string* fieldname;
    korp_string* descriptor;
    uint16 modifier;
}korp_jnc_field;
#define korp_jnc_field_ARR_GCINFO_RECYC BUILD_TYPE_INFO(NOT_ARRAY, 3, NOT_RECYCLABLE)
 
korp_class* load_class(korp_class_loader*,const korp_string*, korp_loader_exception*);
void class_add_to_package(korp_class*);
bool class_parse(korp_class*, const char*, int, korp_loader_exception*);
bool class_verify(korp_class* );
bool class_prepare(korp_class* );
korp_entry* new_class_loader(void);
bool prepare_java_class_object(korp_class*);
korp_class* preload_primitive_class(char* classname);
void load_jnc_fields(void);

#endif /* #ifndef _CLASS_LOADER_H */
