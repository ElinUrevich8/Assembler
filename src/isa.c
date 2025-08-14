/*============================================================================
 *  isa.c
 *
 *  ISA word packers for the 10-bit format.
 *  First word: [9:6]=opcode, [5:4]=src mode, [3:2]=dst mode, [1:0]=A/R/E
 *  Extra word (8-bit payload): [9:2]=data, [1:0]=A/R/E
 *  Register pair word: src[9:6], dst[5:2], A/R/E[1:0]
 *============================================================================*/
#include "isa.h"

/* Set A/R/E bits (mask back to 10 bits) */
unsigned isa_pack_are(unsigned w, unsigned are2) {
    w &= ~((unsigned)3u << ARE_SHIFT);
    w |= ((are2 & 3u) << ARE_SHIFT);
    return (w & WORD_MASK);
}

/* Map addressing enum to the 2-bit mode field in first word */
unsigned isa_mode_code(AddrMode m) {
    switch (m) {
    case ADR_IMMEDIATE: return 0u; /* 00 */
    case ADR_DIRECT:    return 1u; /* 01 */
    case ADR_MATRIX:    return 2u; /* 10 */
    case ADR_REGISTER:  return 3u; /* 11 */
    default:            return 0u; /* absent -> 0 */
    }
}

/* Build first instruction word (pass -1 for missing src/dst to keep zero) */
unsigned isa_first_word(int opcode, int src_mode_or_minus1, int dst_mode_or_minus1) {
    unsigned w = 0u;
    w |= ((unsigned)(opcode & 0xF)) << OP_SHIFT;
    if (src_mode_or_minus1 >= 0)
        w |= (isa_mode_code((AddrMode)src_mode_or_minus1) & 3u) << SRC_SHIFT;
    if (dst_mode_or_minus1 >= 0)
        w |= (isa_mode_code((AddrMode)dst_mode_or_minus1) & 3u) << DST_SHIFT;
    return isa_pack_are(w, ARE_A); /* first word is Absolute */
}

/* Extra words (8-bit payload) */
unsigned isa_word_imm(long v) {
    unsigned p = (unsigned)v & 0xFFu;
    return isa_pack_are(p << FIELD8_SHIFT, ARE_A);
}
unsigned isa_word_reloc(int v) {
    unsigned p = (unsigned)v & 0xFFu;
    return isa_pack_are(p << FIELD8_SHIFT, ARE_R);
}
unsigned isa_word_extern(void) {
    return isa_pack_are(0u, ARE_E);  /* payload not used for extern */
}

/* Register encoders: pair or single-sided */
unsigned isa_word_regs_pair(int src_reg, int dst_reg) {
    unsigned w = 0u;
    w |= ((unsigned)(src_reg & REG_NIBBLE_MASK)) << REG_SRC_SHIFT;
    w |= ((unsigned)(dst_reg & REG_NIBBLE_MASK)) << REG_DST_SHIFT;
    return isa_pack_are(w, ARE_A);
}
unsigned isa_word_reg_src(int src_reg) {
    unsigned w = 0u;
    w |= ((unsigned)(src_reg & REG_NIBBLE_MASK)) << REG_SRC_SHIFT;
    return isa_pack_are(w, ARE_A);
}
unsigned isa_word_reg_dst(int dst_reg) {
    unsigned w = 0u;
    w |= ((unsigned)(dst_reg & REG_NIBBLE_MASK)) << REG_DST_SHIFT;
    return isa_pack_are(w, ARE_A);
}
