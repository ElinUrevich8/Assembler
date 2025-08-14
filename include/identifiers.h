/*============================================================================
 *  identifiers.h
 *
 *  Identifier classification and validation helpers.
 *
 *  Rules (per spec):
 *   - Label: [A-Za-z][A-Za-z0-9]*, length <= MAX_LABEL_LEN, not reserved.
 *   - Macro: [A-Za-z][A-Za-z0-9_]*, length <= MAX_LABEL_LEN, not reserved.
 *   - Reserved identifiers include opcodes, directives, and registers.
 *============================================================================*/

#ifndef IDENTIFIERS_H
#define IDENTIFIERS_H

#include "defaults.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Non-zero iff 's' is a reserved identifier (mnemonic/directive/register). */
int is_reserved_identifier(const char *s);

/* Strict label rule per project spec (see header comment). */
int is_valid_label_name_strict(const char *s);

/* Macro naming rule used by pre-assembler (see header comment). */
int is_valid_macro_name(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* IDENTIFIERS_H */
