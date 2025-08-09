/*============================================================================
 *  macro.c
 *
 *  Implementation of MacroTable.
 *  Relies on:
 *    • hash_core  — generic chained hash (key,void*)    (internal)
 *    • nameset    — global identifier set for uniqueness
 *============================================================================*/

#include "macro.h"
#include "hash_core.h"
#include "nameset.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>     /* fprintf */

/*--------------------------------------------------------------------*/
/*  Extern: global identifier set (defined in assembler.c)            */
/*--------------------------------------------------------------------*/
extern NameSet g_used_names;

/* Helper to free stored body strings */
static void free_body(void *p) { free(p); }

/*--------------------------------------------------------------------*/
static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = (char *)malloc(len);
    if (d) {
        strcpy(d, s);
    }
    return d;
}

/*--------------------------------------------------------------------*/
void macro_init(MacroTable *mt)
{
    hc_init(&mt->core);
}

/*--------------------------------------------------------------------*/
void macro_free(MacroTable *mt)
{
    hc_free(&mt->core, free_body);
}

/*--------------------------------------------------------------------*/
bool macro_define(MacroTable *mt,
                  const char *name,
                  const char *body,
                  int line_num)
{
    /* First: ensure global uniqueness (macros + labels) */
    if (!ns_add(&g_used_names, name)) {
        fprintf(stderr,
                "Error(line %d): identifier '%s' already in use\n",
                line_num, name);
        return false;
    }

    /* Duplicate the body text */
    {
        char *dup = my_strdup(body);
        if (!dup) {
            fprintf(stderr,
                    "Error(line %d): out of memory for macro body\n",
                    line_num);
            return false;
        }

        /* Insert into the local hash (name → dup) */
        if (!hc_insert(&mt->core, name, dup)) {
            /* Should not happen (uniqueness already enforced) */
            free(dup);
            return false;
        }
    }
    return true;
}

/*--------------------------------------------------------------------*/
const char *macro_lookup(const MacroTable *mt, const char *name)
{
    return (const char *)hc_find(&mt->core, name);
}
