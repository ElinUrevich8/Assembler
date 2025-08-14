/*============================================================================
 *  encoding.h
 *
 *  Parsing utilities for directives and instruction sizing.
 *
 *  Responsibilities:
 *  - Strip comments (handles quoted strings safely)
 *  - Parse and push `.data`, `.string`, `.mat` into the data image
 *  - Estimate instruction size (first word + extra words per operand)
 *  - Lookup opcodes / operand count
 *
 *  Notes:
 *  - Addressing mode numeric *codes* (for the first word) are defined in isa.h.
 *    Here we use an enum as a bitmask for legality/validation tables.
 *============================================================================*/

#ifndef ENCODING_H
#define ENCODING_H

#include "defaults.h"
#include <stddef.h>
#include "errors.h"
#include "codeimg.h"

/* Remove a ';' comment unless it's inside a quoted string (modifies in place). */
void strip_comment_inplace(char *s);

/* Addressing modes bitmask for legality checks (mode->allowed?). */
typedef enum {
    ADR_INVALID   = 0,
    ADR_IMMEDIATE = 1 << 0,  /* first-word code 0 (see isa.h) */
    ADR_DIRECT    = 1 << 1,  /* first-word code 1 */
    ADR_MATRIX    = 1 << 2,  /* first-word code 2 */
    ADR_REGISTER  = 1 << 3   /* first-word code 3 */
} AddrMode;

/* Pass 1 sizing info. 'words' includes first word. */
typedef struct { int words; int operands; } EncodedInstrSize;

/* ---------- Directive parsers (push into data image) ---------- */
/* Returns count of items pushed, or -1 on error (reported into errs). */
bool parse_symbol_name(const char *s, char *out, size_t outsz);
int  parse_and_push_data_operands(const char *s, CodeImg *data, Errors *errs, int lineno);
int  parse_and_push_string       (const char *s, CodeImg *data, Errors *errs, int lineno);
int  parse_and_push_mat          (const char *s, CodeImg *data, Errors *errs, int lineno);

/* ---------- Instruction sizing / opcode lookup ---------- */
/* Fills 'sz' with number of words & operand count. Returns false on error. */
bool encoding_estimate_size(const char *instr, EncodedInstrSize *sz, Errors *errs, int lineno);
/* Looks up mnemonic -> (opcode, argc). Returns false if unknown mnemonic. */
bool encoding_lookup_opcode(const char *mnemonic, int *out_opcode, int *out_argc);

#endif /* ENCODING_H */
