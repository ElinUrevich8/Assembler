#ifndef IDENTIFIERS_H
#define IDENTIFIERS_H

#include "defaults.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns non-zero iff s is a reserved identifier (opcode/directive[/register]). */
int is_reserved_identifier(const char *s);

/* Strict label rule (project spec): [A-Za-z][A-Za-z0-9]*, length <= MAX_LABEL_LEN,
   and must NOT be a reserved identifier. */
int is_valid_label_name_strict(const char *s);

/* Macro rule used by pre-assembler: starts with a letter, then [A-Za-z0-9_]*,
   length <= MAX_LABEL_LEN, and must NOT be a reserved identifier. */
int is_valid_macro_name(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* IDENTIFIERS_H */
