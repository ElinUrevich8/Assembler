/* src/isa.c
 * Implementation of 10-bit word packing helpers for the assembler ISA.
 * Matches the layout documented in include/isa.h.
 */

#include "isa.h"

/* A/R/E field packer: clamp to 10 bits */
unsigned isa_pack_are(unsigned w, unsigned are2) {
    w &= ~((unsigned)3u << ARE_SHIFT);
    w |= ((are2 & 3u) << ARE_SHIFT);
    return (w & WORD_MASK);
}

/* Map high-level addressing mode to the 2-bit field code in the first word. */
unsigned isa_mode_code(AddrMode m) {
    switch (m) {
    case ADR_IMMEDIATE: return 0u; /* 00 */
    case ADR_DIRECT:    return 1u; /* 01 */
    case ADR_MATRIX:    return 2u; /* 10 */
    case ADR_REGISTER:  return 3u; /* 11 */
    default:            return 0u; /* absent => 0 */
    }
}

/* Build first instruction word.
 * Pass -1 for src/dst when that side is absent (keeps field 0).
 */
unsigned isa_first_word(int opcode, int src_mode_or_minus1, int dst_mode_or_minus1) {
    unsigned w = 0u;
    w |= ((unsigned)(opcode & 0xF)) << OP_SHIFT;
    if (src_mode_or_minus1 >= 0)
        w |= (isa_mode_code((AddrMode)src_mode_or_minus1) & 3u) << SRC_SHIFT;
    if (dst_mode_or_minus1 >= 0)
        w |= (isa_mode_code((AddrMode)dst_mode_or_minus1) & 3u) << DST_SHIFT;
    return isa_pack_are(w, ARE_A); /* first word is Absolute */
}

/* Extra words with 8-bit payload in [9:2] + A/R/E in [1:0] */
unsigned isa_word_imm(long v) {
    unsigned p = (unsigned)v & 0xFFu;
    return isa_pack_are(p << FIELD8_SHIFT, ARE_A);
}

unsigned isa_word_reloc(int v) {
    unsigned p = (unsigned)v & 0xFFu;
    return isa_pack_are(p << FIELD8_SHIFT, ARE_R);
}

unsigned isa_word_extern(void) {
    return isa_pack_are(0u, ARE_E);
}

/* Register-encoding words (src in [9:6], dst in [5:2]) */
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
