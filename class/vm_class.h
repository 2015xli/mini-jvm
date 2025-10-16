
#ifndef _VM_CLASS_H
#define _VM_CLASS_H

#include "korp_types.h"
#include "gc_for_vm.h"
#include "vm_interface.h"

/*
 *   header definition:
 *   
 *   Java object:
 *   |31|30...............................3 2 | 1 0|
 *             class pointer                    type      
 *
 *   VM object (without kind):
 *   |31|30 ... ... ... 16| 15....8| 7..4|3 2 | 1 0|
 *        instance_size     ref_num             type
 *        array_length      arr_type      
 *
 *   type: bit 0: Java(1) or VM(0) object
 *         bit 1: lock bit
 *         bit 31: gc bit(mark/forward) 
 *
 * ===== Below Only for VM Object ======================================
 *
 *   array: bit 3: object type:  YES_ARRAY(1) or NOT_ARRAY(0) 
 *
 *   prop: bit 7..4 
 *         properties of the kind, currently undefined.
 *         bit 4: recyclable or not:  NOT_RECYCLABLE(1) or YES_RECYCLABLE(0)
 *   
 *   (### only for not-array VM object ###)
 *       gc_info: bit 8..15 
 *            ref fields number (used when we aggregate refs ),
 *
 *       instance_size: bit 16..30
 *           instance size of the VM object including header
 *
 *   (### only for yes_array VM object ###)
 *       array_type: bit 8..15
 *            see array_type definition
 *
 *       array_length: bit 16..30
 *            array length of the VM array object
 *       Note: the instance size is:
 *            array_length * array_elem_size + VM_ARRAY_OVERHEAD
 *           
 *
 */
#define OBJ_TYPE_SHIFT      0
#define OBJ_TYPE_MASK       (0x00000001)
#define IS_JAVA_OBJ_MASK    OBJ_TYPE_MASK

#define POINTER_MASK        (0x7fFFffFC)

#define LOCK_BIT            1
#define LOCK_SHIFT          1
#define IS_LOCKED_MASK      (0x00000002)

#define MARK_SHIFT          31
#define MARK_MASK           (0x80000000)
#define IS_OBJ_MARK_MASK    MARK_MASK   

#define JAVA_OBJ_INFO       (0x00000003)

/* below only for VM object */
#define IS_ARRAY_SHIFT      2
#define IS_ARRAY_MASK       (0x00000004)

#define PROP_MASK           (0x00000070)

#define IS_RECYCLABLE_SHIFT 4
#define IS_RECYCLABLE_MASK  (0x00000010)

#define ARRAY_TYPE_MASK     (0x0000FF00)
#define REF_NUM_MASK        ARRAY_TYPE_MASK
#define REFNUM_ARRTYPE_MASK ARRAY_TYPE_MASK
#define ARRAY_TYPE_SHIFT     8
#define REF_NUM_SHIFT        ARRAY_TYPE_SHIFT
#define REFNUM_ARRTYPE_SHIFT ARRAY_TYPE_SHIFT

#define SIZE_MASK           (0x7FFF0000)
#define LENGTH_MASK         SIZE_MASK
#define SIZE_SHIFT          16

#define HEAD(h)  (*(obj_info *)h)

#define GET_VM_INSTANCE_SIZE_H(h)   ((uint16)( (h & SIZE_MASK)>>SIZE_SHIFT ))
#define GET_VM_INSTANCE_SIZE(kobj)  (kobj?GET_VM_INSTANCE_SIZE_H(HEAD(kobj)):0)
#define GET_VM_ARRAY_LENGTH_H(h)    GET_VM_INSTANCE_SIZE_H(h)
#define GET_VM_ARRAY_LENGTH(kobj)   (kobj?GET_VM_INSTANCE_SIZE(kobj):0)    

#define VM_OBJ_OVERHEAD     BYTES_OF_OBJ_INFO
#define VM_ARRAY_OVERHEAD   (BYTES_OF_OBJ_INFO + BYTES_OF_REF)

/* this one is used when building a VM object, obj->header = OBJ_INFO(X),
   hopefully user doesn't need to use it directly; 
   instead, new_vm_object() or new_vm_array() will be used. 
 */
#define OBJ_INFO(X)  X##_ARR_GCINFO_RECYC
#define BUILD_TYPE_INFO(is_array, refnum_arrtype, is_recyclable ) \
            ((obj_info)(is_recyclable << IS_RECYCLABLE_SHIFT | \
                        is_array << IS_ARRAY_SHIFT | \
                        refnum_arrtype << REFNUM_ARRTYPE_SHIFT ) )

/* interface of new_object */
#define JAVA_OBJ 1
#define VM_OBJ 0
#define NOT_RECYCLABLE 1
#define YES_RECYCLABLE 0
#define NOT_ARRAY 0
#define YES_ARRAY 1

#define SIZE_INFO(is_array, arrlen_or_unheaded_objsize) \
            (is_array ? arrlen_or_unheaded_objsize : arrlen_or_unheaded_objsize+VM_OBJ_OVERHEAD)

#define BUILD_VM_OBJ_HEADER( is_array, refnum_arrtype, is_recyclable, arrlen_or_unheaded_objsize ) \
    ((uint32)( (is_recyclable << IS_RECYCLABLE_SHIFT) |         \
               (is_array << IS_ARRAY_SHIFT) |                   \
               (refnum_arrtype<< REFNUM_ARRTYPE_SHIFT) |        \
               (SIZE_INFO(is_array, arrlen_or_unheaded_objsize) << SIZE_SHIFT ) \
             ) \
    ) 

/* some interfaces with (obj_info h) */
#define IS_JAVA_OBJECT_H(h)         (h & IS_JAVA_OBJ_MASK )
#define IS_VM_OBJECT_H(h)           ((bool)!IS_JAVA_OBJECT_H(h))
#define IS_ARRAY_H(h)               (h & IS_ARRAY_MASK )

#define VM_ARRAY_TYPE_H(h)          ((korp_array_type)((h & ARRAY_TYPE_MASK)>>REFNUM_OR_ARRTYPE_SHIFT) )
#define VM_ARRAY_ELEMSIZE_H(h)      ((uint16)(VM_ARRAY_TYPE_H(h)<ARRAY_REF)? 1<<(VM_ARRAY_TYPE_H(h)) : BYTES_OF_REF )
#define IS_PRI_VM_ARRAY_H(h)        ((bool)(IS_ARRAY_H(h) && (VM_ARRAY_TYPE_H(h)<ARRAY_REF) ) )
#define IS_REF_VM_ARRAY_H(h)        ((bool)(IS_ARRAY_H(h) && (VM_ARRAY_TYPE_H(h)==ARRAY_REF) ) )

#define GET_VM_OBJ_SIZE_H(h)                \
    ((uint16)                               \
      (                                     \
        IS_ARRAY_H(h)?                      \
        (                                   \
            GET_VM_ARRAY_LENGTH_H(h)*       \
            GET_VM_ARRAY_ELEMSIZE_H(h) +    \
            VM_ARRAY_OVERHEAD               \
        )                                   \
        :                                   \
        GET_VM_INSTANCE_SIZE_H(h)           \
      )                                     \
    )

#define GET_VM_OBJ_REF_NUM_H(h)             \
    ((uint16)                               \
      (                                     \
         IS_REF_VM_ARRAY_H(h)?              \
         (GET_VM_ARRAY_LENGTH_H(h)+1)       \
         :                                  \
         (h & REFNUM_ARRTYPE_MASK )>>REFNUM_ARRTYPE_SHIFT   \
      )                                     \
    )

#endif /* #ifndef _VM_CLASS_H */
