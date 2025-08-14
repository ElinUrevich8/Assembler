/*============================================================================
 *  pass1.h
 *
 *  Pass 1 — build symbols and size images.
 *
 *  Responsibilities:
 *    - Scan the macro-expanded file (.am)
 *    - Parse directives: push values into data image
 *    - Parse instructions: validate shape and estimate size (IC)
 *    - Build symbol table with kinds (CODE/DATA/EXTERN/ENTRY)
 *    - Relocate DATA symbols by adding ICF at end of pass
 *
 *  Result:
 *    - Pass1Result holds the symbol table, code/data images, IC/DC, and errors.
 *============================================================================*/
#ifndef PASS1_H
#define PASS1_H

#include "defaults.h"  /* IC_INIT, bool */
#include "symbols.h"
#include "codeimg.h"   /* FULL definition of CodeImg */
#include "errors.h"

/* Aggregated output of Pass 1. */
typedef struct {
    Symbols *symbols;  /* symbol table (owned)                              */
    CodeImg  code;     /* pre-sized code image (may hold placeholders)      */
    CodeImg  data;     /* data image                                        */
    int      ic;       /* final IC (ICF)                                    */
    int      dc;       /* final DC                                          */
    Errors   errors;   /* collected errors                                  */
    bool     ok;       /* false on fatal problems                           */
} Pass1Result;

/* Run pass 1 over 'am_path' and fill 'out'. Returns true on success. */
bool pass1_run(const char *am_path, Pass1Result *out);
/* Free all allocations inside 'r'. Safe on zero-initialized structs. */
void pass1_free(Pass1Result *r);

#endif
