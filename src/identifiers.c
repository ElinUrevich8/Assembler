/*============================================================================
 *  identifiers.c
 *
 *  Small helpers for validating identifier names used by the assembler:
 *    - Reserved words (opcodes + directives)
 *    - Label names (strict: letters/digits only, start with letter)
 *    - Macro names  (allow underscore, start with letter)
 *
 *  Keep the reserved list centralized here.
 *============================================================================*/

#include <string.h>
#include <ctype.h>
#include "identifiers.h"

/* Keep this list in ONE place. Add/remove items here only. */
static const char *const k_reserved_words[] = {
    /* opcodes */
    "mov","cmp","add","sub","not","clr","lea","inc","dec",
    "jmp","bne","red","prn","jsr","rts","stop",
    /* directives */
    ".data",".string",".entry",".extern",".mat", 0
};

int is_reserved_identifier(const char *s)
{
    int i;
    for (i = 0; k_reserved_words[i]; ++i)
        if (strcmp(s, k_reserved_words[i]) == 0) return 1;
    return 0;
}

/* Labels (for symbols in code/data):
 *  - Must start with a letter
 *  - Then letters or digits only (no underscore)
 *  - Max length: MAX_LABEL_LEN
 *  - Must NOT be a reserved word
 */
int is_valid_label_name_strict(const char *s)
{
    const unsigned char *p = (const unsigned char*)s;
    int len = 0;

    if (!p || !isalpha(*p)) return 0;         /* must start with a letter */
    ++p; ++len;

    while (*p) {
        if (!isalnum(*p)) return 0;           /* no '_' for labels */
        ++len;
        if (len > MAX_LABEL_LEN) return 0;    /* length limit */
        ++p;
    }

    if (is_reserved_identifier(s)) return 0;  /* cannot be reserved */
    return 1;
}

/* Macro names are similar, but allow underscores after the first letter. */
int is_valid_macro_name(const char *s)
{
    const unsigned char *p = (const unsigned char*)s;
    int len = 0;

    if (!p || !isalpha(*p)) return 0;         /* must start with a letter */
    ++p; ++len;

    while (*p) {
        if (!isalnum(*p) && *p != '_') return 0;  /* macros allow '_' */
        ++len;
        if (len > MAX_LABEL_LEN) return 0;
        ++p;
    }

    if (is_reserved_identifier(s)) return 0;  /* cannot be reserved */
    return 1;
}
