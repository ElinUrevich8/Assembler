#ifndef PASS1_H
#define PASS1_H

#include <stdbool.h>
#include "symbols.h"
#include "codeimg.h"
#include "errors.h"
#include "defaults.h"   /* e.g., IC_INIT = 100 */

typedef struct {
    Symbols *symbols;   /* final symbol table after relocation of DATA */
    CodeImg  code;      /* code image with placeholders for Pass 2 */
    CodeImg  data;      /* data image (will be appended after code) */
    int      ic;        /* instruction counter (final at end of pass) */
    int      dc;        /* data counter (final at end of pass) */
    Errors   errors;    /* collected errors */
    bool     ok;        /* false if any fatal errors occurred */
} Pass1Result;

/* Run the first pass on a preassembled .am file (no macros left). */
bool pass1_run(const char *am_path, Pass1Result *out);

/* Free all resources allocated inside Pass1Result. */
void pass1_free(Pass1Result *r);

#endif /* PASS1_H */
