#ifndef ENCODING_H
#define ENCODING_H

#include "defaults.h"
#include <stddef.h>
#include "errors.h"
#include "codeimg.h"

typedef enum {
    ADR_INVALID   = 0,
    ADR_IMMEDIATE = 1 << 0,
    ADR_DIRECT    = 1 << 1,
    ADR_REGISTER  = 1 << 2
} AddrMode;

typedef struct {
    int words;
    int operands;
} EncodedInstrSize;

bool parse_symbol_name(const char *s, char *out, size_t outsz);

int  parse_and_push_data_operands(const char *s, CodeImg *data,
                                  Errors *errs, int lineno);

int  parse_and_push_string(const char *s, CodeImg *data,
                           Errors *errs, int lineno);

bool encoding_estimate_size(const char *instr, EncodedInstrSize *sz,
                            Errors *errs, int lineno);

bool encoding_lookup_opcode(const char *mnemonic, int *out_opcode, int *out_argc);

#endif
