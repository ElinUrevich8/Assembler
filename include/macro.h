/*============================================================================
 *  macro.h — store macro definitions (name → body string)
 *============================================================================*/
#ifndef MACRO_H
#define MACRO_H

#include "defaults.h"
#include "hash_core.h"               /* for HashCore */

#ifdef __cplusplus
extern "C" {
#endif

/* Full definition — now the compiler knows sizeof(MacroTable) */
typedef struct MacroTable {
    HashCore core;                   /* buckets from hash_core */
} MacroTable;

/* Life-cycle */
void  macro_init  (MacroTable *mt);
void  macro_free  (MacroTable *mt);

/* Define new macro — false on duplicate / OOM */
bool  macro_define (MacroTable *mt,
                    const char *name,
                    const char *body,
                    int line_num);

/* Lookup — NULL if not found */
const char *macro_lookup (const MacroTable *mt, const char *name);

#ifdef __cplusplus
}
#endif
#endif /* MACRO_H */
