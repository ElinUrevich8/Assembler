#ifndef ENCODING_H
#define ENCODING_H

#include <stdbool.h>
#include <stddef.h>
#include "errors.h"
#include "codeimg.h"

/* Addressing modes recognized by the parser/estimator. */
typedef enum {
    ADR_INVALID  = 0,
    ADR_IMMEDIATE= 1 << 0,   /* #number */
    ADR_DIRECT   = 1 << 1,   /* label */
    ADR_REGISTER = 1 << 2    /* r0..r7 */
} AddrMode;

/* Result for instruction size estimation (pass 1). */
typedef struct {
    int words;     /* total machine words the instruction will occupy */
    int operands;  /* number of parsed operands (0..2) */
} EncodedInstrSize;

/* Parse a legal symbol name from s into out.
   Skips leading spaces. Returns true on success and advances nothing (caller
   gets the extracted name; use it to continue parsing). */
bool parse_symbol_name(const char *s, char *out, size_t outsz);

/* Parse `.data` operand list and push each integer as a data word.
   Returns number of words pushed, or -1 on error (and reports via errs). */
int parse_and_push_data_operands(const char *s, CodeImg *data,
                                 Errors *errs, int lineno);

/* Parse `.string "..."` and push characters as words plus a terminating 0.
   Returns number of words pushed (len+1), or -1 on error. */
int parse_and_push_string(const char *s, CodeImg *data,
                          Errors *errs, int lineno);

/* Estimate the size (in words) of an instruction line.
   Parses the mnemonic and operands, validates addressing modes,
   and applies the "two registers share one word" optimization.
   Returns true on success; false on syntax or addressing errors. */
bool encoding_estimate_size(const char *instr, EncodedInstrSize *sz,
                            Errors *errs, int lineno);

/* Tiny helper used in pass1 for directives. */
static inline bool starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) return false;
    }
    return true;
}

/* Look up opcode number (0..15) and arg count by mnemonic.
   Returns false if mnemonic is unknown. */
bool encoding_lookup_opcode(const char *mnemonic, int *out_opcode, int *out_argc);


#endif /* ENCODING_H */
