/*============================================================================
 *  encoding_parse.h
 *
 *  Stage: Parsing and validating a single assembly instruction.
 *
 *  This module defines the ParsedInstr structure and the main parser
 *  for converting a tokenized instruction line into:
 *    - Opcode
 *    - Operand addressing modes
 *    - Immediate / register / matrix / symbol payloads
 *
 *  Used by:
 *    - Pass 1: to determine instruction size and check addressing legality.
 *    - Pass 2: to emit the correct binary words for each operand.
 *
 *  Validation:
 *    encoding_parse_instruction() also checks that addressing modes
 *    are legal for the given opcode (using ISA tables) and reports
 *    descriptive errors if not.
 *============================================================================*/

#ifndef ENCODING_PARSE_H
#define ENCODING_PARSE_H

#include "defaults.h"
#include "errors.h"
#include "encoding.h" /* for AddrMode */

/*----------------------------------------------------------------------------
 * ParsedInstr
 *
 * Fully-decoded representation of a single instruction line, as used by
 * later passes to either size or emit the instruction.
 *
 * Fields are populated according to the detected addressing modes.
 * Unused fields are left 0.
 *---------------------------------------------------------------------------*/
typedef struct {
    int      argc;                    /* Operand count: 0, 1, or 2. */
    int      opcode;                  /* Numeric opcode (0..15).    */

    /* Addressing modes detected for each operand. */
    AddrMode src_mode;                /* Valid only if argc == 2.   */
    AddrMode dst_mode;                /* Valid if argc >= 1.        */

    /* Immediate constants (#imm syntax).
       Only meaningful if the corresponding mode is ADR_IMMEDIATE. */
    long     src_imm;                 /* Source immediate value.    */
    long     dst_imm;                 /* Destination immediate.     */

    /* Direct/matrix symbol labels (max length per defaults.h). */
    char     src_sym[MAX_LABEL_LEN];  /* Source label (direct/matrix). */
    char     dst_sym[MAX_LABEL_LEN];  /* Destination label.            */

    /* Single register operands (ADR_REGISTER).
       Only meaningful if the corresponding mode is ADR_REGISTER. */
    int      src_reg;                 /* Source register number. */
    int      dst_reg;                 /* Destination register.    */

    /* Matrix addressing (LABEL[rX][rY]).
       The row/col fields hold the register numbers. */
    int      src_mat_row;             /* Source matrix row register. */
    int      src_mat_col;             /* Source matrix col register. */
    int      dst_mat_row;             /* Dest matrix row register.   */
    int      dst_mat_col;             /* Dest matrix col register.   */
} ParsedInstr;

/*----------------------------------------------------------------------------
 * encoding_parse_instruction()
 *
 * Parse a raw instruction line into a ParsedInstr structure.
 *
 * Responsibilities:
 *   - Tokenize line into mnemonic and operand list.
 *   - Look up mnemonic -> opcode and legal addressing modes (ISA table).
 *   - Parse each operand, determining:
 *       * Addressing mode
 *       * Associated payload (imm constant, register index, label name, matrix parts)
 *   - Validate addressing legality for this opcode (per source/dest).
 *
 * On success:
 *   - Returns true.
 *   - 'out' is filled with a fully populated ParsedInstr.
 *
 * On failure:
 *   - Returns false.
 *   - Adds one or more messages to 'errs' describing why parsing failed.
 *---------------------------------------------------------------------------*/
bool encoding_parse_instruction(const char *line,
                                ParsedInstr *out,
                                Errors *errs,
                                int lineno);

#endif /* ENCODING_PARSE_H */
