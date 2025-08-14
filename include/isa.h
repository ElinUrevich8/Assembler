/*============================================================================
 *  isa.h
 *
 *  Instruction Set Architecture packing helpers (10-bit words).
 *
 *  Word layout:
 *    - First word: [9:6]=opcode, [5:4]=src_mode, [3:2]=dst_mode, [1:0]=ARE
 *    - Extra words with 8-bit payloads: [9:2]=data, [1:0]=ARE
 *    - Register words (pair): src in [9:6], dst in [5:2], ARE in [1:0]
 *
 *  ARE codes:
 *    A=00 (absolute), E=01 (external), R=10 (relocatable)
 *
 *  These helpers centralize packing so Pass 1 sizing matches Pass 2 emission.
 *============================================================================*/

#ifndef ISA_H
#define ISA_H

#include "encoding.h"  /* for AddrMode */

/* 10-bit machine word */
#define WORD_MASK   0x3FFu

/* First word field positions */
#define OP_SHIFT    6u
#define SRC_SHIFT   4u
#define DST_SHIFT   2u
#define ARE_SHIFT   0u

/* A/R/E codes (2 bits) */
#define ARE_A  0u  /* 00 absolute */
#define ARE_E  1u  /* 01 external */
#define ARE_R  2u  /* 10 relocatable */

/* 8-bit payload words (data in [9:2], ARE in [1:0]) */
#define FIELD8_SHIFT  2u
#define FIELD8_MASK   (0xFFu << FIELD8_SHIFT)

/* Register word packing (pair or single) */
#define REG_SRC_SHIFT 6u
#define REG_DST_SHIFT 2u
#define REG_NIBBLE_MASK 0xFu  /* r0..r7 -> fits in 4 bits (top stays 0) */

/*------- Packing helpers (all return 10-bit words masked by WORD_MASK) ------*/

/* Set the A/R/E bits on an already-packed word. */
unsigned isa_pack_are(unsigned w, unsigned are2);

/* Map AddrMode to the 2-bit code used in the first word. */
unsigned isa_mode_code(AddrMode m);

/* Build first word from opcode and addressing modes (or -1 if absent). */
unsigned isa_first_word(int opcode, int src_mode_or_minus1, int dst_mode_or_minus1);

/* Extra words for immediates, relocatables, externs. */
unsigned isa_word_imm(long v);        /* A + 8-bit payload                   */
unsigned isa_word_reloc(int v);       /* R + 8-bit relocated address         */
unsigned isa_word_extern(void);       /* E (payload usually 0 for linker)    */

/* Register words: pair (src,dst) or single (src-only / dst-only). */
unsigned isa_word_regs_pair(int src_reg, int dst_reg);
unsigned isa_word_reg_src(int src_reg);
unsigned isa_word_reg_dst(int dst_reg);

#endif
