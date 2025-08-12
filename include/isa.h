#ifndef ISA_H
#define ISA_H

#include "encoding.h"  /* for AddrMode */

/* 10-bit machine word */
#define WORD_MASK   0x3FFu

/* First word fields: [9:6]=opcode, [5:4]=src mode, [3:2]=dst mode, [1:0]=A/R/E */
#define OP_SHIFT    6u
#define SRC_SHIFT   4u
#define DST_SHIFT   2u
#define ARE_SHIFT   0u

/* A/R/E codes (2 bits) */
#define ARE_A  0u  /* 00 absolute */
#define ARE_E  1u  /* 01 external */
#define ARE_R  2u  /* 10 relocatable */

/* 8-bit payload words (extra words use [9:2] for data, [1:0] for A/R/E) */
#define FIELD8_SHIFT  2u
#define FIELD8_MASK   (0xFFu << FIELD8_SHIFT)

/* Registers: src in [9:6], dst in [5:2], A/R/E in [1:0] */
#define REG_SRC_SHIFT 6u
#define REG_DST_SHIFT 2u
#define REG_NIBBLE_MASK 0xFu  /* we store r0..r7 -> top bit stays 0 */

unsigned isa_pack_are(unsigned w, unsigned are2);
unsigned isa_mode_code(AddrMode m);
unsigned isa_first_word(int opcode, int src_mode_or_minus1, int dst_mode_or_minus1);
unsigned isa_word_imm(long v);
unsigned isa_word_reloc(int v);
unsigned isa_word_extern(void);
unsigned isa_word_regs_pair(int src_reg, int dst_reg);
unsigned isa_word_reg_src(int src_reg);
unsigned isa_word_reg_dst(int dst_reg);

#endif