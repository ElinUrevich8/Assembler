/* pass2.h
 * Second pass interface: resolves symbol references and finalizes instruction words.
 * Also collects info for side files:
 *   - .ext: one record per external use-site (symbol + address of the operand word)
 *   - .ent: one record per valid entry symbol (local, defined)
 *
 * NOTE: Pass 2 does not write files. It only produces data; the output module
 * decides whether to create .ob/.ent/.ext based on list sizes.
 */

#ifndef PASS2_H
#define PASS2_H

#include "defaults.h"   /* size limits, C90 bool fallback */
#include "codeimg.h"    /* CodeImg container */
#include "symbols.h"    /* symbol table built by pass 1 */
#include "errors.h"     /* error aggregation/printing */
#include "pass1.h"      /* Pass1Result: ICF/DC/symbols/concatenated image */

/* One external use-site:
 * 'addr' is the absolute address of the operand word that references 'name'.
 * (Addresses are the same ones printed to .ext)
 */
typedef struct ExtUse {
    char *name;  /* owned heap string; freed by pass2_free() */
    int   addr;  /* IC-based absolute address */
} ExtUse;

/* One entry output row:
 * 'addr' is the final (already-relocated) address as will be printed to .ent.
 */
typedef struct EntryOut {
    char *name;  /* owned heap string; freed by pass2_free() */
    int   addr;  /* absolute address (after pass1 relocation) */
} EntryOut;

/* Aggregate result of pass 2.
 * Invariants:
 *  - code_len == ICF - IC_INIT (on success)
 *  - data_len == DC from pass 1
 *  - code_final holds only instruction words (data stays in pass1 image)
 */
typedef struct Pass2Result {
    CodeImg  code_final;      /* finalized instruction words (code only) */
    size_t   code_len;        /* number of code words */
    size_t   data_len;        /* number of data words (copied from pass1) */

    /* .ext content: one element per use-site of an external symbol */
    ExtUse   *ext;
    size_t    ext_len;
    size_t    ext_cap;

    /* .ent content: one element per valid entry symbol */
    EntryOut *ent;
    size_t    ent_len;
    size_t    ent_cap;

    /* Errors collected during pass 2 (parsing/encoding/entries) */
    Errors   *errors;

    /* Non-zero if pass 2 finished without fatal problems.
     * (There may still be warnings/errors recorded in 'errors'.) */
    int       ok;
} Pass2Result;

/* Runs the second pass over the macro-expanded file (.am).
 * - am_path: path to the .am input
 * - p1: pass-1 results (symbols + ICF/DC + concatenated image)
 * - out: receives results; must be cleaned with pass2_free()
 *
 * Returns non-zero on success. On fatal failure, returns 0 and sets out->ok=0.
 */
int pass2_run(const char *am_path, const Pass1Result *p1, Pass2Result *out);

/* Frees all allocations inside 'r' (safe to call on zero-initialized structs). */
void pass2_free(Pass2Result *r);

#endif /* PASS2_H */
