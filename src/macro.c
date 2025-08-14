/*============================================================================
 *  macro.c
 *
 *  Implementation of MacroTable.
 *  Relies on:
 *    * hash_core  — generic chained hash (key,void*)
 *    * nameset    — global identifier set for uniqueness (shared namespace)
 *============================================================================*/
#include "macro.h"
#include "hash_core.h"
#include "nameset.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* from assembler.c — single-namespace set (macros + labels) */
extern NameSet *g_used_names;

/* Free function for stored body strings */
static void free_body(void *p) { free(p); }

/* Small local strdup */
static char *dup_cstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *d = (char *)malloc(n);
    if (d) memcpy(d, s, n);
    return d;
}

void macro_init(MacroTable *mt)  { hc_init(&mt->core); }
void macro_free(MacroTable *mt)  { hc_free(&mt->core, free_body); }

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
            free(dup);                     /* duplicate name or OOM in hash */
            return false;
        }
    }
    return true;
}

const char *macro_lookup(const MacroTable *mt, const char *name)
{
    return (const char *)hc_find(&mt->core, name);
}
