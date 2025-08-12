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

/* ---------- helpers ---------- */

static inline unsigned isa_pack_are(unsigned w, unsigned are2) {
    w &= ~(3u << ARE_SHIFT);
    w |= ((are2 & 3u) << ARE_SHIFT);
    return (w & WORD_MASK);
}

/* Map addressing mode -> 2-bit field code in first word */
static inline unsigned isa_mode_code(AddrMode m) {
    switch (m) {
    case ADR_IMMEDIATE: return 0u;
    case ADR_DIRECT:    return 1u;
    case ADR_MATRIX:    return 2u;
    case ADR_REGISTER:  return 3u;
    default:            return 0u; /* absent => 0 */
    }
}

/* First word builder; pass -1 when a side is absent (so its field stays 0) */
static inline unsigned isa_first_word(int opcode, int src_mode_or_minus1, int dst_mode_or_minus1) {
    unsigned w = 0u;
    w |= ((unsigned)(opcode & 0xF)) << OP_SHIFT;
    if (src_mode_or_minus1 >= 0) w |= (isa_mode_code((AddrMode)src_mode_or_minus1) & 3u) << SRC_SHIFT;
    if (dst_mode_or_minus1 >= 0) w |= (isa_mode_code((AddrMode)dst_mode_or_minus1) & 3u) << DST_SHIFT;
    return isa_pack_are(w, ARE_A);
}

/* 8-bit payload words */
static inline unsigned isa_word_imm(long v)      { unsigned p=(unsigned)v & 0xFFu; return isa_pack_are(p<<FIELD8_SHIFT, ARE_A); }
static inline unsigned isa_word_reloc(int v)     { unsigned p=(unsigned)v & 0xFFu; return isa_pack_are(p<<FIELD8_SHIFT, ARE_R); }
static inline unsigned isa_word_extern(void)     { return isa_pack_are(0u, ARE_E); }

/* Register words */
static inline unsigned isa_word_regs_pair(int src_reg, int dst_reg) {
    unsigned w = 0u;
    w |= ((unsigned)(src_reg & REG_NIBBLE_MASK)) << REG_SRC_SHIFT; /* [9:6] */
    w |= ((unsigned)(dst_reg & REG_NIBBLE_MASK)) << REG_DST_SHIFT; /* [5:2] */
    return isa_pack_are(w, ARE_A);
}
static inline unsigned isa_word_reg_src(int src_reg) {
    unsigned w = ((unsigned)(src_reg & REG_NIBBLE_MASK)) << REG_SRC_SHIFT;
    return isa_pack_are(w, ARE_A);
}
static inline unsigned isa_word_reg_dst(int dst_reg) {
    unsigned w = ((unsigned)(dst_reg & REG_NIBBLE_MASK)) << REG_DST_SHIFT;
    return isa_pack_are(w, ARE_A);
}

#endif /* ISA_H */
