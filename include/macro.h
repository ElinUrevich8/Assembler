/*============================================================================
 *  macro.h
 *
 *  Macro table: store and retrieve macro definitions (name -> body).
 *
 *  Context:
 *    - Used exclusively by the pre-assembler to expand `mcro ... mcroend`.
 *    - Names must respect the project's identifier rules.
 *
 *  Design notes:
 *    - Internally backed by a small chained hash (hash_core).
 *    - API owns the body string; stored as-is and returned as const char*.
 *============================================================================*/
#ifndef MACRO_H
#define MACRO_H

#include "defaults.h"
#include "hash_core.h"               /* for HashCore */

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque-like public struct: concrete here so callers can stack-allocate. */
typedef struct MacroTable {
    HashCore core;                   /* buckets from hash_core */
} MacroTable;

/*--------------------------- Lifecycle ------------------------------------*/
/* Initialise an empty macro table. */
void  macro_init  (MacroTable *mt);
/* Free all memory held by the table (definitions and buckets). */
void  macro_free  (MacroTable *mt);

/*--------------------------- Operations -----------------------------------*/
/* Define a new macro.
 * Returns false on duplicate definition or allocation failure.
 * 'line_num' is recorded in diagnostic messages (if any).
 */
bool  macro_define (MacroTable *mt,
                    const char *name,
                    const char *body,
                    int line_num);

/* Lookup a macro body by name; returns NULL if not found. */
const char *macro_lookup (const MacroTable *mt, const char *name);

#ifdef __cplusplus
}
#endif
#endif /* MACRO_H */
