/*============================================================================
 *  pass2.h
 *
 *  Pass 2 — resolve symbols and finalize code words.
 *
 *  Responsibilities:
 *    - Re-parse .am, using Pass 1 results (ICF/DC + symbols)
 *    - Emit first word + extra operand words per instruction
 *    - For direct labels: emit R-words (or E-words for externs) and
 *      record extern use-sites (for .ext)
 *    - Collect entries (for .ent)
 *
 *  Note: Pass 2 does NOT write files. output.c decides what to write.
 *============================================================================*/
#ifndef PASS2_H
#define PASS2_H

#include "defaults.h"   /* size limits, C90 bool fallback */
#include "codeimg.h"    /* CodeImg container */
#include "symbols.h"    /* symbol table built by pass 1 */
#include "errors.h"     /* error aggregation/printing */
#include "pass1.h"      /* Pass1Result: ICF/DC/symbols/concatenated image */

/* One external use-site:
 * 'addr' is the absolute address of the operand word that references 'name'.
 */
typedef struct ExtUse {
    char *name;  /* owned heap string; freed by pass2_free() */
    int   addr;  /* IC-based absolute address */
} ExtUse;

/* One entry output row:
 * 'addr' is the final (already-relocated) address printed to .ent.
 */
typedef struct EntryOut {
    char *name;  /* owned heap string; freed by pass2_free() */
    int   addr;  /* absolute address (after pass1 relocation) */
} EntryOut;

/* Aggregate result of pass 2. */
typedef struct Pass2Result {
    CodeImg  code_final;      /* finalized instruction words (code only) */
    size_t   code_len;        /* number of code words */
    size_t   data_len;        /* number of data words (copied from pass1) */

    /* .ext content: one element per use-site of an external symbol */
    ExtUse   *ext;  size_t ext_len;  size_t ext_cap;

    /* .ent content: one element per valid entry symbol */
    EntryOut *ent;  size_t ent_len;  size_t ent_cap;

    Errors   *errors;     /* errors collected during pass 2 */

    int       ok;         /* non-zero on success (fatal-free run) */
} Pass2Result;

/* Run pass 2 using 'am_path' and Pass 1 output; fills 'out'. Returns non-zero on success. */
int  pass2_run(const char *am_path, const Pass1Result *p1, Pass2Result *out);
/* Free all allocations in 'r'. Safe on zero-initialized structs. */
void pass2_free(Pass2Result *r);

#endif /* PASS2_H */
