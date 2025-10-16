
#include <assert.h>

#include "x86.h"

static korp_ia32_unialu alu_inc = {op_inc, 0xFE, 0xFF, 0x40};
static korp_ia32_unialu alu_dec = {op_dec, 0xFE, 0xFF, 0x48};
static korp_ia32_unialu alu_not = {op_not, 0xF6, 0xF7};
static korp_ia32_unialu alu_neg = {op_neg, 0xF6, 0xF7};

static korp_ia32_unialu* unialu_op[4] = { 
    &alu_inc, 
    &alu_dec, 
    &alu_not, 
    &alu_neg, 
};

#define NUM_ALU_OPS 8
static korp_ia32_binalu alu_op[NUM_ALU_OPS]; 
/* alu_add, alu_or;alu_adc;alu_sbb;alu_and;alu_sub;alu_xor;alu_cmp;*/

static korp_ia32_binalu* binalu_op[NUM_ALU_OPS];

bool x86_init()
{
    uint32 i;

    alu_op[0].mod_opcode = op_add;
    alu_op[0].mr_b_bits = 0;
    alu_op[0].mr_d_bits = 1;
    alu_op[0].rm_b_bits = 2;
    alu_op[0].rm_d_bits = 3;
    alu_op[0].al_bits = 4;
    alu_op[0].eax_bits = 5;
    binalu_op[0] = &alu_op[0];

    for(i=1; i< NUM_ALU_OPS; i++ ){
        uint8 inc = 8*i;
        binalu_op[i]=&alu_op[i];
        binalu_op[i]->mod_opcode = (korp_binalu_kind)i;
        binalu_op[i]->al_bits = alu_op[0].al_bits + inc;
        binalu_op[i]->eax_bits = alu_op[0].eax_bits + inc;
        binalu_op[i]->mr_b_bits = alu_op[0].mr_b_bits + inc;
        binalu_op[i]->mr_d_bits = alu_op[0].mr_d_bits + inc;
        binalu_op[i]->rm_b_bits = alu_op[0].rm_b_bits + inc;
        binalu_op[i]->rm_d_bits = alu_op[0].rm_d_bits + inc;
    }

    return true;
}

/* To form the SIB byte 
 * the byte contains  scale index  base
 *                    [7 6][5 4 3][2 1 0]
 */
static uint8 sib_byte(uint8 scale,uint8 index,uint8 base)
{
  return ((scale&0x3)<<6)|((index&0x7)<<3)|(base&0x7);
}

/* To form the MODR/M byte 
 * the byte cotains  mod   reg/op   r/m 
 *                  [7 6] [5 4 3] [2 1 0]
 */
static uint8 modrm_byte(uint8 mod,uint8 regop,uint8 rm)
{
  return ((mod&0x3)<<6)|((regop&0x7)<<3)|(rm&0x7);
}

korp_ia32_mem* mem_base_opnd(korp_ia32_mem* kmem, korp_ia32_reg reg, int32 disp)
{
    return mem_opnd(kmem, IS_BASE_DISP, 0, 0, reg, disp);
}

korp_ia32_mem* mem_addr_opnd(korp_ia32_mem* kmem, int32 disp)
{
    return mem_opnd(kmem, IS_DEFAULT, 0, 0, 0, disp);
}

korp_ia32_mem* mem_opnd(korp_ia32_mem* mem,
                              uint8 kind,
                              uint8 scale,
                              uint8 index, 
                              uint8 base, 
                              int32 disp)
{
    mem->kind = kind;
    mem->base = base;
    mem->disp = disp;
    mem->index = index;
    mem->scale = scale;
    return mem;
} 

static uint8* imm_opnd_inst(uint8* buf, uint32 opnd, korp_ia32_mode mode)
{
    switch(mode){
        case r_b: case rr_b: case ri_b: case m_b: case mr_b: case mi_b:
        case i_b:{
            *buf++ = (uint8)opnd;
            return buf;
        }
        case r_d: case rr_d: case ri_d: case m_d: case mr_d: case mi_d: 
        case rri_d: case rrr_d: case rmi_d: case rrm_d: case i_d:{
            uint32* tbuf = (uint32*)buf;
            *tbuf++ = opnd;
            return (uint8*)tbuf;
        }
        case r_w: case rr_w: case ri_w: case m_w: case mr_w: case mi_w: 
        case rri_w: case i_w:{
            uint16* tbuf = (uint16*)buf;
            *tbuf++ = opnd;                   
            return (uint8*)tbuf;
        }
    }
    assert(0);
    return buf;
}

static uint8* mem_inst(uint8 *buf, korp_ia32_mem* mem, uint8 regop)
{
    switch(mem->kind){
        case IS_BASE:
            if(mem->base==ebp_reg){
                *buf++=modrm_byte(MODERM_REG_DISP8_MEM,regop,ebp_reg);
                *buf++=0;
                return buf;
            }else if(mem->base==esp_reg){
                *buf++=modrm_byte(MODERM_REG_MEM,regop,esp_reg);
                *buf++=sib_byte(SIB_SCALE_1,esp_reg,esp_reg);
                return buf;    
            }
            *buf++=modrm_byte(MODERM_REG_MEM,regop,mem->base);
            return buf;

        case IS_SIB:
            if(mem->base==ebp_reg)  {
                *buf++=modrm_byte(MODERM_REG_DISP8_MEM,regop,esp_reg);
                *buf++=sib_byte(mem->scale,mem->index,mem->base);
                *buf++=0;
                return buf;
            }
            *buf++=modrm_byte(MODERM_REG_MEM,regop,esp_reg);
            *buf++=sib_byte(mem->scale,mem->index,mem->base);
            return buf;

        case IS_BASE_DISP:
            if((mem->disp>=-128)&&(mem->disp<=127)){
                *buf++=modrm_byte(MODERM_REG_DISP8_MEM,regop,mem->base);
                if(mem->base==esp_reg){
                    *buf++=sib_byte(SIB_SCALE_1,esp_reg,esp_reg);
                }
                buf = imm_opnd_inst(buf, mem->disp, i_b);
            }else{
                *buf++=modrm_byte(MODERM_REG_DISP32_MEM,regop,mem->base);
                if(mem->base==esp_reg){
                    *buf++=sib_byte(SIB_SCALE_1,esp_reg,esp_reg);
                }
                buf = imm_opnd_inst(buf, mem->disp, i_d);
            }
            return buf;
                
        case IS_SIB_DISP:
            if ((mem->disp>=-128)&&(mem->disp<=127)){
                *buf++=modrm_byte(MODERM_REG_DISP8_MEM,regop,esp_reg);
                *buf++=sib_byte(mem->scale,mem->index,mem->base);
                buf = imm_opnd_inst(buf, mem->disp, i_b);
            }else{
                *buf++=modrm_byte(MODERM_REG_DISP32_MEM,regop,esp_reg);
                *buf++=sib_byte(mem->scale,mem->index,mem->base);
                buf = imm_opnd_inst(buf, mem->disp, i_d);
            }
            return buf;
        
        case IS_DEFAULT:
            *buf++=modrm_byte(MODERM_REG_REG,regop,ebp_reg);
            buf = imm_opnd_inst(buf, mem->disp, i_d);
            return buf;
    }
    assert(0);
    return buf;
}

static uint8* reg_inst(uint8* buf, 
                korp_ia32_reg opnd1, 
                uint8 mod_op)
{
    *buf++ = modrm_byte(MODERM_REG_REG, mod_op, opnd1);
    return buf;
}

static uint8* rm_opnd_inst(uint8* buf, 
               uint32 opnd, 
               uint8 regop,
               korp_ia32_mode mode )
{       
    switch(mode){
        case r_w:  case r_d:  case r_b:  case rri_w: case rri_d:
        case rr_w: case rr_d: case rr_b: case rrr_w: case rrr_d:
        case ri_w: case ri_d: case ri_b:
            return reg_inst(buf, (korp_ia32_reg)opnd, regop);
        
        case m_w:  case m_d:  case m_b:  case rmi_w: case rmi_d:
        case mr_w: case mr_d: case mr_b: case rrm_w: case rrm_d:
        case mi_w: case mi_d: case mi_b: case rm_b:  case rm_w:
            return mem_inst(buf, (korp_ia32_mem*)opnd, regop);
    }    
    assert(0);
    return buf;
}

static uint8* binalu_inst(uint8 *buf,
                korp_ia32_binalu* alu,
                uint32 opnd1,
                uint32 opnd2,
                korp_ia32_mode mode)
{
    switch(mode){
        /* reg, mem */
        case rm_w:
            *buf++=opnd_size_prefix; /* fall through */
        case rm_d:
            *buf++=alu->rm_d_bits;
            return mem_inst(buf, (korp_ia32_mem*)opnd2, (uint8)opnd1 );

        case rm_b:
            *buf++=alu->rm_b_bits;
            return mem_inst(buf, (korp_ia32_mem*)opnd2, (uint8)opnd1);

        /* rm, reg */
        case mr_w: case rr_w:
            *buf++=opnd_size_prefix;
        case mr_d: case rr_d:
            *buf++=alu->mr_d_bits;
            return rm_opnd_inst(buf, opnd1, (uint8)opnd2, mode);
        case mr_b: case rr_b:
            *buf++=alu->mr_b_bits;
            return rm_opnd_inst(buf, opnd1, (uint8)opnd2, mode);
        
        /* rm imm */
        case mi_w: case ri_w:
            *buf++=opnd_size_prefix;
        case mi_d: case ri_d:
            if(opnd1==eax_reg){
                *buf++=alu->eax_bits;
            }else{
                *buf++=0x81;
                buf = rm_opnd_inst(buf, opnd1, alu->mod_opcode, mode);
            }
            return buf = imm_opnd_inst(buf, opnd2, mode);
        case mi_b: case ri_b:
            if(opnd1==al_reg){
                *buf++=alu->al_bits;
            }else{
                *buf++=0x80;
                buf = rm_opnd_inst(buf, (korp_ia32_reg)opnd1, alu->mod_opcode, mode);
            }
            return buf = imm_opnd_inst(buf, opnd2, mode);

    }
    assert(0);
    return buf;
}

static uint8* unialu_inst(uint8 *buf,
                korp_ia32_unialu* alu,
                uint32 opnd1,
                korp_ia32_mode mode)
{
    if( alu->mod_opcode == op_inc || alu->mod_opcode == op_dec ){
        switch(mode){
            case r_w:
                *buf++ = opnd_size_prefix;
            case r_d:
                *buf++ = alu->r_bits + (uint8)opnd1;
                return buf;
            case r_b:
                *buf++ = alu->b_bits; 
                *buf++=modrm_byte(MODERM_REG_REG, alu->mod_opcode, opnd1);
                return buf;
        }
    }

    switch(mode){
        case r_w: case m_w: *buf++ = opnd_size_prefix;
        case r_d: case m_d: *buf++ = alu->d_bits; 
            return rm_opnd_inst(buf, opnd1, alu->mod_opcode, mode);
        case r_b: case m_b: *buf++ = alu->b_bits; 
            return rm_opnd_inst(buf, opnd1, alu->mod_opcode, mode);
    }

    assert(0);
    return buf;
}

/**
 * code emitter interfaces
 */

/* prefix */
uint8 *prefix(uint8 *buf, korp_ia32_prefix prefix) 
{
    *buf++ = (uint8)prefix;
    return buf;
}

/* mov instruction */
uint8* mov(uint8 *buf,const uint32 opnd1,const uint32 opnd2,const korp_ia32_mode mode)
{
    switch(mode){

        /* reg <- imm */
        case ri_w:
            *buf++=opnd_size_prefix; 
         case ri_d:
            *buf++=0xb8+(opnd1&0x7);
            return imm_opnd_inst(buf, opnd2, mode);            
        case ri_b:
            *buf++=0xb0+(opnd1&0x7);
            return imm_opnd_inst(buf, opnd2, mode);
        
        /* reg <-mem */
        case rm_w:
            *buf++=opnd_size_prefix;
        case rm_d:
            if(opnd1==eax_reg){
                korp_ia32_mem* mem=(korp_ia32_mem*)opnd2;
                if(mem->kind==IS_DEFAULT){
                    *buf++ = 0xa1;
                    return imm_opnd_inst(buf, mem->disp, i_d);
                }
            }
            *buf++=0x8b;
            return mem_inst(buf, (korp_ia32_mem*)opnd2, (uint8)opnd1 );
        case rm_b:
            if(opnd1==al_reg){
                korp_ia32_mem* mem=(korp_ia32_mem*)opnd2;
                if(mem->kind==IS_DEFAULT){
                    *buf++=0xa0;
                    return imm_opnd_inst(buf, mem->disp, i_d);
                }
            }        
            *buf++=0x8a;
            return mem_inst(buf, (korp_ia32_mem*)opnd2, (uint8)opnd1 );

        
        /* mem <- imm (mem IS_DEFAULT) */
        case mi_w:
            *buf++=opnd_size_prefix;
        case mi_d: 
            *buf++=0xc7;
            buf = mem_inst(buf, (korp_ia32_mem*)opnd1, 0);
            buf = imm_opnd_inst(buf, opnd2, mode);
            return buf;
        case mi_b:
            *buf++=0xc6;
            buf = mem_inst(buf, (korp_ia32_mem*)opnd1, 0);
            return imm_opnd_inst(buf, opnd2, i_b);

        /* rm <- reg */
        case mr_w: case rr_w:
            *buf++=opnd_size_prefix;
        case mr_d: case rr_d:
            if(opnd2==eax_reg){
                korp_ia32_mem* mem=(korp_ia32_mem*)opnd1;

	        	if( mode == mr_w) *buf++=addr_mode_prefix;

                if(mem->kind==IS_DEFAULT){
                    *buf++=0xa3;
                    return imm_opnd_inst(buf, mem->disp, i_d);
                }
            }
            *buf++=0x89;
            return rm_opnd_inst(buf, opnd1, (uint8)opnd2, mode );
        case mr_b: case rr_b:          
            if(opnd2==al_reg) {
                korp_ia32_mem* mem=(korp_ia32_mem *)opnd1;
                if(mem->kind==IS_DEFAULT){
                    *buf++=0xa2;
                    return imm_opnd_inst(buf, mem->disp, i_d);
                }
            }        
            *buf++=0x88;
            return rm_opnd_inst(buf, opnd1, (uint8)opnd2, mode );
    }
    return buf;
}

/* Binary ALU Operation */
uint8* binalu(uint8* buf, 
              korp_binalu_kind alukind, 
              uint32 opnd1, 
              uint32 opnd2, 
              korp_ia32_mode mode )
{
    return binalu_inst( buf, binalu_op[alukind], opnd1, opnd2, mode );
}

uint8* unialu(uint8* buf, 
              korp_unialu_kind alukind, 
              uint32 opnd1, 
              korp_ia32_mode mode )
{
    return unialu_inst( buf, unialu_op[alukind], opnd1, mode );
}

uint8* nop(uint8* buf)
{
	*buf++ = (uint8)0x90;
	return buf;
}

uint8* muldiv(uint8* buf, 
              korp_muldiv_kind mdkind, 
              uint32 opnd, 
              korp_ia32_mode mode)
{
    uint8 opcode = mdkind+4;
    switch( mode ) {
        case r_w: case m_w: *buf++=(uint8)opnd_size_prefix;
        case r_d: case m_d:
            *buf++=0xf7;
            return rm_opnd_inst(buf, opnd, opcode, mode);
        case r_b: case m_b:
            *buf++=0xf6;
            return rm_opnd_inst(buf, opnd, opcode, mode);        
    }
    assert(0);
    return buf;
}

uint8* imul( uint8 *buf,
             uint32 opnd1,
             uint32 opnd2,
             uint32 opnd3,
             korp_ia32_mode mode) 
{
    switch( mode ) {
        /* reg <- rm*Imm */
        case rri_w: case rmi_w:
            *buf++=opnd_size_prefix;
        case rri_d: case rmi_d:
            *buf++=0x69;
            buf = rm_opnd_inst(buf, opnd2, (uint8)opnd1, mode);
            return imm_opnd_inst(buf, opnd3, mode);

        /* reg <- reg*rm */
        case rrr_w: case rrm_w:
            *buf++=opnd_size_prefix;
        case rrr_d: case rrm_d:
            assert(opnd1==opnd2);
            *buf++ = 0x0F;
            *buf++ = 0xAF;
            return rm_opnd_inst(buf, opnd3, (uint8)opnd2, mode);

    }
    assert(0);
    return buf;
}

uint8* lea(uint8 *buf, 
            uint8 opnd1, 
            uint32 opnd2, 
            korp_ia32_mode mode )
{

    switch( mode ) {
        case rm_w: *buf++=opnd_size_prefix;
        case rm_d: 
            *buf++=0x8d;
            return mem_inst(buf, (korp_ia32_mem*)opnd2, opnd1);
    }
    assert(0);
    return buf;
}

uint8* push(uint8* buf, uint32 opnd, korp_ia32_mode mode)
{
    switch(mode){
        case r_w: *buf++=opnd_size_prefix;
        case r_d:
            *buf++ = 0x50 + (uint8)opnd;
            return buf;

        case m_w: *buf++=opnd_size_prefix;
        case m_d:
            *buf++ = 0xFF;
            return mem_inst(buf, (korp_ia32_mem*)opnd, 6);
    
        case i_w: *buf++=opnd_size_prefix;
        case i_d:
            *buf++ = 0x68;
            return imm_opnd_inst(buf, opnd, mode);
    }
    assert(0);
    return buf;
}

uint8* pop(uint8* buf, uint32 opnd, korp_ia32_mode mode)
{
    switch(mode){
        case r_w: *buf++=opnd_size_prefix;
        case r_d:
            *buf++ = 0x58 + (uint8)opnd;
            return buf;

        case m_w: *buf++=opnd_size_prefix;
        case m_d:
            *buf++ = 0x8F;
            return mem_inst(buf, (korp_ia32_mem*)opnd, 6);
    
    }
    assert(0);
    return buf;
}

uint8* ret(uint8* buf, uint16 imm ) 
{
    switch( imm ){
        case 0:
            *buf++ = 0xc3;
            return buf;
        default:
            *buf++ = 0xc2;
            imm_opnd_inst(buf, imm, i_w);
            return buf;
    }
    assert(0);
    return buf;
}

uint8* call(uint8* buf, uint32 target, korp_ia32_mode mode )
{
    switch(mode){
        case i_w: *buf++=opnd_size_prefix;
        case i_d:
            *buf++ = 0xE8;
            return imm_opnd_inst(buf, target, mode);

        case m_w: case r_w: *buf++=opnd_size_prefix;
        case m_d: case r_d:
            *buf++ = 0xFF;
            return rm_opnd_inst(buf, target, 2, mode);

        case TARGET:
            return call(buf, (int32)((uint8*)target-buf-5), i_d); 
    }
    assert(0);
    return buf;
}

uint8* branch(uint8* buf, korp_cc_kind cc, uint32 target, korp_ia32_mode mode)
{
    switch( mode ){
        case i_b:
            *buf++ = cc;
            *buf++ = (uint8)target;
            return buf;
        case i_d:
            *buf++ = 0x0f;
            *buf++ = cc+0x10;
            return imm_opnd_inst(buf, target, mode);
        case TARGET:
            return branch(buf, cc, (int32)((uint8*)target-buf-6), i_d); 
    }
    assert(0);
    return buf;
}

uint8* jump(uint8* buf, uint32 target, korp_ia32_mode mode)
{
    switch( mode ){
        case i_b:
            *buf++ = 0xEB;
            *buf++ = (uint8)target;
            return buf;
        case i_d:
            *buf++ = 0xE9;
            return imm_opnd_inst(buf, target, mode);
        case r_d:
        case m_d:
            *buf++ = 0xFF;
            buf = rm_opnd_inst(buf, target, 4, mode);
            return buf;
        case TARGET:
            return jump(buf, (int32)((uint8*)target-buf-5), i_d);

    }
    assert(0);
    return buf;
}

uint8* widen_to_full(uint8* buf, korp_ia32_reg r, uint32 opnd2, int32 is_signed, korp_ia32_mode mode)
{
    uint8 opcode = is_signed? 0xBE:0xB6;
    switch(mode){
        case rm_w: opcode++;
        case rm_b: break;
        default: assert(0);
    }
    *buf++ = 0x0F;
    *buf++ = opcode;
    return rm_opnd_inst(buf, opnd2, r, mode);
}

uint8* widen_to_half(uint8* buf, korp_ia32_wreg r, uint32 opnd2, int32 is_signed, korp_ia32_mode mode)
{
    uint8 opcode = is_signed? 0xBE:0xB6;
    switch(mode){
        case rm_b: *buf++=opnd_size_prefix; break;
        default: assert(0);
    }
    *buf++ = 0x0F;
    *buf++ = opcode;
    return rm_opnd_inst(buf, opnd2, r, mode);
}
