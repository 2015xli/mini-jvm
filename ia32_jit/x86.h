
#ifndef _X86_H
#define _X86_H

/* b: BYTE, w: WORD, d: DWORD */

#include "vm_object.h"

typedef enum korp_ia32_reg{
    eax_reg=0,ecx_reg=1,edx_reg=2,ebx_reg=3,esp_reg=4,ebp_reg=5,esi_reg=6,edi_reg=7
} korp_ia32_reg;

typedef enum korp_ia32_wreg{
    ax_reg=0,cx_reg=1,dx_reg=2,bx_reg=3,sp_reg=4,bp_reg=5,si_reg=6,di_reg=7
} korp_ia32_wreg;

typedef enum korp_ia32_breg{
    al_reg=0,cl_reg=1,dl_reg=2,bl_reg=3,ah_reg=4,ch_reg=5,dh_reg=6,bh_reg=7
} korp_ia32_breg;

/*
 * ia32 instruction operation kind,
 * First part denotes the destination, source operand:
 *      r register; i immediate; m memory;
 * Second part denotes operation size.
 */
typedef enum korp_ia32_mode{
    TARGET=0,
    i_d,    i_w,    i_b,
	r_d,    r_w,    r_b,
	m_d,    m_w,    m_b,
	rr_d,   rr_w,   rr_b,
	ri_d,   ri_w,   ri_b,
	mr_d,   mr_w,   mr_b,
	rm_d,   rm_w,   rm_b,
	mi_d,   mi_w,   mi_b,
	rri_d,  rri_w,  
	rmi_d,  rmi_w,
    rrr_d,  rrr_w,
    rrm_d,  rrm_w
} korp_ia32_mode;

#define MODERM_REG_MEM	0
#define MODERM_REG_DISP8_MEM	1
#define MODERM_REG_DISP32_MEM	2
#define MODERM_REG_REG	3

#define SIB_SCALE_1	0
#define SIB_SCALE_2	1
#define SIB_SCALE_4	2
#define SIB_SCALE_8	3

#define BASE     0x01
#define INDEX    0x02
#define SCALE	 0x04
#define DISP	 0x08

#define IS_BASE         (BASE)
#define IS_SIB          (BASE|INDEX|SCALE)
#define IS_BASE_DISP    (BASE|DISP)
#define IS_SIB_DISP     (IS_SIB|DISP)
#define IS_DEFAULT      (DISP)

typedef struct _korp_ia32_mem{
    uint8 kind;
    uint8 base; 
    uint8 index; 
    uint8 scale;
    int32 disp;
}korp_ia32_mem;

typedef enum korp_unialu_kind{
    op_inc, op_dec, op_not, op_neg
} korp_unialu_kind;

typedef struct _korp_ia32_unialu{
    korp_unialu_kind mod_opcode;
    uint8 b_bits;
    uint8 d_bits;
    uint8 r_bits;    
}korp_ia32_unialu;

typedef enum korp_binalu_kind{
    op_add=0, op_or, op_adc, op_sbb, op_and, op_sub, op_xor, op_cmp
} korp_binalu_kind;

typedef struct _korp_ia32_binalu{
	korp_binalu_kind mod_opcode;

	uint8 mr_b_bits;
	uint8 mr_d_bits;

	uint8 rm_b_bits;
	uint8 rm_d_bits;

    uint8 al_bits;
	uint8 eax_bits;
    
}korp_ia32_binalu;

typedef enum korp_muldiv_kind{
    op_mul, op_imul, op_div, op_idiv
} korp_muldiv_kind;

typedef enum korp_ia32_prefix{
    es_prefix = 0x26,
    cs_prefix = 0x2E,
    ss_prefix = 0x36,
    ds_prefix = 0x3E,
    fs_prefix = 0x64,
    gs_prefix = 0x65,

    opnd_size_prefix = 0x66,
    addr_mode_prefix = 0x67,

    lock_prefix = 0xF0,
    repnz_prefix = 0xF2,
    repz_prefix = 0xF3, 
    rep_prefix = 0xF3
} korp_ia32_prefix;

/* ult means "unsigned less than". 
   those having no "u" prefix can be used for either 
 */
typedef enum korp_cc_kind{
    cc_ult = 0x72, 
    cc_uge = 0x73,
    cc_eq =  0x74,
    cc_ne =  0x75, 
    cc_ule = 0x76, 
    cc_ugt = 0x77, 
    cc_lz  = 0x78, 
    cc_gez = 0x79,
    cc_p  =  0x7a, 
    cc_np =  0x7b,
    cc_lt =  0x7c, 
    cc_ge =  0x7d, 
    cc_le =  0x7e, 
    cc_gt =  0x7f 

} korp_cc_kind;

korp_ia32_mem* mem_opnd(korp_ia32_mem*, uint8 kind, uint8 S, uint8 I, uint8 B, int32 disp); 
korp_ia32_mem* mem_base_opnd(korp_ia32_mem* kmem, korp_ia32_reg reg, int32 disp);
korp_ia32_mem* mem_addr_opnd(korp_ia32_mem* kmem, int32 disp);

uint8* prefix(uint8* buf, korp_ia32_prefix);
uint8* mov(uint8* buf, uint32 opnd1, uint32 opnd2, korp_ia32_mode);
uint8* binalu(uint8* buf, korp_binalu_kind, uint32 opnd1, uint32 opnd2, korp_ia32_mode);
uint8* unialu(uint8* buf, korp_unialu_kind, uint32 opnd1, korp_ia32_mode);
uint8* widen_to_full(uint8* buf, korp_ia32_reg r, uint32, int32 is_signed, korp_ia32_mode);
uint8* widen_to_half(uint8* buf, korp_ia32_wreg r, uint32, int32 is_signed, korp_ia32_mode);

uint8* noop(uint8* buf);
uint8* muldiv(uint8* buf, korp_muldiv_kind, uint32 opnd, korp_ia32_mode mode);
uint8* imul(uint8 *buf, uint32 opnd1, uint32 opnd2, uint32 opnd3, korp_ia32_mode);
uint8* lea(uint8 *buf, uint8 opnd1, uint32 opnd2, korp_ia32_mode mode);
uint8* push(uint8* buf, uint32 opnd, korp_ia32_mode mode);
uint8* pop(uint8* buf, uint32 opnd, korp_ia32_mode mode);
uint8* ret(uint8* buf, uint16 imm ) ;
uint8* call(uint8* buf, uint32 opnd, korp_ia32_mode mode );
uint8* branch(uint8* buf, korp_cc_kind cc, uint32 target, korp_ia32_mode mode);
uint8* jump(uint8* buf, uint32 target, korp_ia32_mode mode);

bool x86_init(void);

#endif /* #ifndef _X86_H */

