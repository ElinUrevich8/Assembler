#ifndef PASS1_H
#define PASS1_H

#include "defaults.h"  /* IC_INIT, bool */
#include "symbols.h"
#include "codeimg.h"   /* FULL definition of CodeImg */
#include "errors.h"

typedef struct {
    Symbols *symbols;
    CodeImg  code;
    CodeImg  data;
    int      ic;
    int      dc;
    Errors   errors;
    bool     ok;
} Pass1Result;

bool pass1_run(const char *am_path, Pass1Result *out);
void pass1_free(Pass1Result *r);

#endif
