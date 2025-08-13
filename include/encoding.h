#ifndef ENCODING_H
#define ENCODING_H

#include "defaults.h"
#include <stddef.h>
#include "errors.h"
#include "codeimg.h"

/* Remove a ';' comment unless it's inside a quoted string.
 * Modifies the line in-place.
 */
void strip_comment_inplace(char *s);

/*  Addressing modes (bitmask for legality tables) 
 * Note: the 2-bit FIELD CODES in the machine word are defined in isa.h.
 * These values are only for "allowed modes" masks and APIs.
 */
typedef enum {
    ADR_INVALID   = 0,
    ADR_IMMEDIATE = 1 << 0,  /* code 0 in first word */
    ADR_DIRECT    = 1 << 1,  /* code 1 in first word */
    ADR_MATRIX    = 1 << 2,  /* code 2 in first word */
    ADR_REGISTER  = 1 << 3   /* code 3 in first word */
} AddrMode;

/*  Pass 1 sizing info  */
typedef struct { int words; int operands; } EncodedInstrSize;

/*  Directives parsers (data and strings)  */
bool parse_symbol_name(const char *s, char *out, size_t outsz);
int  parse_and_push_data_operands(const char *s, CodeImg *data, Errors *errs, int lineno);
int  parse_and_push_string(const char *s, CodeImg *data, Errors *errs, int lineno);
int  parse_and_push_mat(const char *s, CodeImg *data, Errors *errs, int lineno);

/*  Instruction sizing / opcode lookup  */
bool encoding_estimate_size(const char *instr, EncodedInstrSize *sz, Errors *errs, int lineno);
bool encoding_lookup_opcode(const char *mnemonic, int *out_opcode, int *out_argc);

#endif /* ENCODING_H */
