/*============================================================================
 *  assembler.h
 *
 *  High-level API for the two-pass assembler.
 *
 *  Orchestrates the full pipeline:
 *    1) Preassembler: macro expansion (.as -> .am)
 *    2) Pass 1: build symbol table, size code/data (.am scan)
 *    3) Pass 2: resolve symbols, emit final words (code image)
 *    4) Writers: produce .ob / .ent / .ext outputs
 *
 *  Notes:
 *  - assemble_file() is fail-fast: it aggregates errors per stage and
 *    returns false if any fatal error occurred; partial outputs are not written.
 *  - g_used_names is a shared set that ensures macro/label namespaces
 *    don’t collide.
 *============================================================================*/

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "defaults.h"
#include "nameset.h"   /* for NameSet */

#ifdef __cplusplus
extern "C" {
#endif

/* Assemble a single source file given its base path (without extension).
 * Returns true on success, false if any errors were reported.
 * Side effects:
 *  - Writes base.am / base.ob / base.ent (if any entries) / base.ext (if any extern uses).
 */
bool assemble_file(const char *base_path);

/* Shared global: single-namespace set for macro names and labels. */
extern NameSet *g_used_names;

#ifdef __cplusplus
}
#endif

#endif  /* ASSEMBLER_H */
