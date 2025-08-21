/*============================================================================
 *  macro.c
 *
 *  Implementation of MacroTable:
 *    - Stores macro bodies by name (string -> body text).
 *    - Shares a single global identifier namespace with labels via NameSet,
 *      so duplicates across macros/labels are rejected early.
 *
 *  Depends on:
 *    * hash_core  — generic chained hash (key -> void*)
 *    * nameset    — global identifier set for uniqueness
 *============================================================================*/
#include "macro.h"
#include "hash_core.h"
#include "nameset.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* from assembler.c — single-namespace set (macros + labels) */
extern NameSet *g_used_names;

/*----------------------------------------------------------------------------
 * free_body(p): destructor for stored macro body strings.
 *----------------------------------------------------------------------------*/
static void free_body(void *p) { free(p); }

/*----------------------------------------------------------------------------
 * dup_cstr(s): small local strdup (C90-safe).
 *----------------------------------------------------------------------------*/
static char *dup_cstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = (char *)malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

/*----------------------------------------------------------------------------
 * macro_init(mt): initialize empty table.
 *----------------------------------------------------------------------------*/
void macro_init(MacroTable *mt)  { hc_init(&mt->core); }

/*----------------------------------------------------------------------------
 * macro_free(mt): free all entries and their owned bodies.
 *----------------------------------------------------------------------------*/
void macro_free(MacroTable *mt)  { hc_free(&mt->core, free_body); }

/*----------------------------------------------------------------------------
 * macro_define(mt, name, body, line_num):
 *   Define/add macro 'name' with text 'body'.
 *   Enforces global identifier uniqueness via g_used_names.
 *   Returns true on success; false on duplicate/OOM (with error printed).
 *----------------------------------------------------------------------------*/
bool macro_define(MacroTable *mt, const char *name, const char *body, int line_num)
{
    /* Enforce single global namespace (macros + labels) */
    if (!ns_add(g_used_names, name)) {
        fprintf(stderr, "Error(line %d): identifier '%s' already in use\n",
                line_num, name);
        return false;
    }

    /* Store a private copy of the body text (table owns it) */
    {
        char *dup = dup_cstr(body);
        if (!dup) {
            fprintf(stderr, "Error(line %d): out of memory for macro body\n", line_num);
            return false;
        }
        if (!hc_insert(&mt->core, name, dup)) {
            /* Duplicate name within MacroTable (should be rare) or OOM in hash. */
            free(dup);
            return false;
        }
    }
    return true;
}

/*----------------------------------------------------------------------------
 * macro_lookup(mt, name): return macro body string or NULL if not found.
 *----------------------------------------------------------------------------*/
const char *macro_lookup(const MacroTable *mt, const char *name)
{
    return (const char *)hc_find(&mt->core, name);
}
