#ifndef ENCODING_PARSE_H
#define ENCODING_PARSE_H

#include "defaults.h"
#include "errors.h"
#include "encoding.h" /* for AddrMode */

/* Complete parse result for one instruction line */
typedef struct {
    int      argc;                    /* 0, 1, or 2 */
    int      opcode;                  /* 0..15 */

    /* Operand addressing modes */
    AddrMode src_mode;                /* valid when argc == 2 */
    AddrMode dst_mode;                /* valid when argc >= 1 */

    /* Immediate payloads (if used) */
    long     src_imm;
    long     dst_imm;

    char     src_sym[MAX_LABEL_LEN];  /* fits label + '\0' per defaults.h */
    char     dst_sym[MAX_LABEL_LEN];

    /* Register payloads (if used) */
    int      src_reg;                 /* ADR_REGISTER (single-reg) */
    int      dst_reg;

    /* Matrix payloads (if used): a label + two registers */
    int      src_mat_row;             /* row reg  (bits 9..6) when ADR_MATRIX */
    int      src_mat_col;             /* col reg  (bits 5..2) when ADR_MATRIX */
    int      dst_mat_row;
    int      dst_mat_col;
} ParsedInstr;

/* Parse mnemonic+operands & validate addressing legality.
 * On failure: returns false and reports into errs.
 */
bool encoding_parse_instruction(const char *line, ParsedInstr *out, Errors *errs, int lineno);

#endif /* ENCODING_PARSE_H */
