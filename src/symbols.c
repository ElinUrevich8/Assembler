/*============================================================================
 *  symbols.c
 *
 *  Simple array-backed symbol table.
 *  - Names are duplicated on insert and freed by symbols_free().
 *  - Lookups are linear (adequate for course-scale inputs).
 *  - Iteration order is insertion order.
 *
 *  Symbol flags:
 *      SYM_CODE, SYM_DATA, SYM_EXTERN, SYM_ENTRY
 *============================================================================*/

#include "symbols.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>   /* FILE, fprintf */

/*----------------------------------------------------------------------------
 * dup_c90(s):
 *   strdup replacement (C90-friendly): allocates and copies 's'.
 *
 * Params:
 *   s - NUL-terminated source string.
 *
 * Returns:
 *   char* / NULL - newly allocated copy; NULL on OOM.
 *
 * Errors:
 *   OOM: returns NULL (callers must check).
 *----------------------------------------------------------------------------*/
static char *dup_c90(const char *s) {
    size_t n = strlen(s) + 1;        /* bytes incl. NUL           */
    char *p = (char*)malloc(n);      /* new buffer                */
    if (p) memcpy(p, s, n);
    return p;
}

/* -------- main storage ------------------------------------------------ */

/*----------------------------------------------------------------------------
 * Symbols:
 *   Main table backing store. Dynamic array grows geometrically.
 *----------------------------------------------------------------------------*/
struct Symbols {
    Symbol  *items;    /* dynamic array of symbols */
    size_t   len;      /* number of valid entries */
    size_t   cap;      /* allocated capacity */
};

/*----------------------------------------------------------------------------
 * ensure_items(s, need):
 *   Grow 's->items' to at least 'need' elements (geometric growth).
 *
 * Params:
 *   s    - table to grow.
 *   need - required capacity (number of Symbol entries).
 *
 * Returns:
 *   1/0 - 1 on success; 0 on OOM (no change to the table on failure).
 *
 * Errors:
 *   OOM: returns 0 (callers must treat as fatal and propagate).
 *----------------------------------------------------------------------------*/
static int ensure_items(Symbols *s, size_t need) {
    size_t ncap;              /* target capacity */
    Symbol *nptr;             /* new allocation  */

    if (s->cap >= need) return 1;
    ncap = s->cap ? s->cap * 2 : 16;
    while (ncap < need) ncap *= 2;

    nptr = (Symbol *)realloc(s->items, ncap * sizeof(Symbol));
    if (!nptr) return 0;      /* OOM */
    s->items = nptr;
    s->cap = ncap;
    return 1;
}

/*----------------------------------------------------------------------------
 * symbols_new():
 *   Allocate a zero-initialized symbol table.
 *
 * Params:
 *   (none)
 *
 * Returns:
 *   Symbols* / NULL - new table or NULL on OOM.
 *
 * Errors:
 *   OOM: NULL return; caller must check.
 *----------------------------------------------------------------------------*/
Symbols *symbols_new(void) {
    /* zero-initialized table */
    return (Symbols *)calloc(1, sizeof(Symbols));
}

/*----------------------------------------------------------------------------
 * symbols_free(s):
 *   Free the table and all owned names. Safe to call with NULL.
 *
 * Params:
 *   s - table to free (may be NULL).
 *
 * Returns:
 *   void.
 *
 * Errors:
 *   None.
 *----------------------------------------------------------------------------*/
void symbols_free(Symbols *s) {
    size_t i;                  /* index over items */
    if (!s) return;
    for (i = 0; i < s->len; ++i) {
        /* names are dup_c90'ed below */
        free((void*)s->items[i].name);
    }
    free(s->items);
    free(s);
}

/*----------------------------------------------------------------------------
 * symbols_find_idx(s, name):
 *   Linear search by name (keeps code simple for small inputs).
 *
 * Params:
 *   s    - table to search.
 *   name - symbol name to find.
 *
 * Returns:
 *   >=0 index on success; -1 if not found.
 *
 * Errors:
 *   None.
 *----------------------------------------------------------------------------*/
static int symbols_find_idx(const Symbols *s, const char *name) {
    size_t i;                           /* scan cursor */
    for (i = 0; i < s->len; ++i) {
        if (strcmp(s->items[i].name, name) == 0) return (int)i;
    }
    return -1;
}

/*----------------------------------------------------------------------------
 * symbols_insert(s, name, value, attrs, def_line):
 *   Insert a new symbol record (assumes name not present by design).
 *
 * Params:
 *   s        - table.
 *   name     - symbol name (duplicated).
 *   value    - numeric value.
 *   attrs    - flags (SYM_CODE/SYM_DATA/SYM_EXTERN/SYM_ENTRY).
 *   def_line - definition line (or 0 for placeholders).
 *
 * Returns:
 *   index/-1 - index of the inserted symbol; -1 on OOM.
 *
 * Errors:
 *   OOM: ensure_items or dup_c90 failure yields -1.
 *----------------------------------------------------------------------------*/
static int symbols_insert(Symbols *s, const char *name,
                          int value, unsigned attrs, int def_line) {
    int idx;                 /* index for the new entry */

    if (!ensure_items(s, s->len + 1)) return -1;

    idx = (int)s->len++;
    s->items[idx].name = dup_c90(name);   /* owned by table */
    if (!s->items[idx].name) {
        /* Roll back len on failure so table stays consistent. */
        s->len--;
        return -1;
    }
    s->items[idx].value = value;
    s->items[idx].attrs = attrs;
    s->items[idx].def_line = def_line;
    return idx;
}

/*----------------------------------------------------------------------------
 * symbols_define(s, name, value, attrs, def_line, errs):
 *   Define/declare a symbol. Updates existing placeholders or inserts new.
 *
 * Params:
 *   s        - table.
 *   name     - symbol name.
 *   value    - numeric value (relocated for DATA later by relocate_data()).
 *   attrs    - must include exactly one of {CODE, DATA, EXTERN}. ENTRY may be OR'ed.
 *   def_line - line number for diagnostics.
 *   errs     - error aggregator.
 *
 * Returns:
 *   true/false - true on success; false on invalid usage, duplicates, or OOM.
 *
 * Errors:
 *   - Invalid attrs (internal misuse) -> false (no user error printed).
 *   - Duplicate/extern conflicts -> user-facing diagnostics added.
 *   - OOM while inserting -> false (no partial insert).
 *----------------------------------------------------------------------------*/
bool symbols_define(Symbols *s, const char *name, int value,
                    unsigned attrs, int def_line, Errors *errs) {
    int idx;                          /* found index or -1 */
    const unsigned kind_mask = (SYM_CODE | SYM_DATA | SYM_EXTERN);

    if ((attrs & kind_mask) == 0) {
        /* Internal misuse; don't emit user-facing error. */
        return false;
    }

    idx = symbols_find_idx(s, name);
    if (idx >= 0) {
        /* Update or reject based on current flags. */
        Symbol *sym = &s->items[idx];  /* existing record */
        if (sym->attrs & SYM_EXTERN) {
            errors_addf(errs, def_line,
                        "cannot define external symbol '%s' (declared extern at line %d)",
                        name, sym->def_line);
            return false;
        }
        if (sym->attrs & (SYM_CODE | SYM_DATA)) {
            errors_addf(errs, def_line,
                        "duplicate label '%s' (previously defined at line %d)",
                        name, sym->def_line);
            return false;
        }
        sym->value    = value;
        sym->attrs   |= (attrs & kind_mask);
        sym->def_line = def_line;
        return true;
    }

    /* First time we see this name: insert a record. */
    if (symbols_insert(s, name, value, attrs, def_line) < 0) {
        /* OOM */
        return false;
    }
    return true;
}

/*----------------------------------------------------------------------------
 * symbols_mark_entry(s, name, line, errs):
 *   Mark symbol as .entry. If unseen, create a placeholder carrying ENTRY.
 *
 * Params:
 *   s    - table.
 *   name - symbol name.
 *   line - line number for diagnostics.
 *   errs - error aggregator.
 *
 * Returns:
 *   true/false - true on success; false on extern conflict or OOM.
 *
 * Errors:
 *   - .entry + .extern conflict -> diagnostic added.
 *   - OOM while inserting placeholder -> false.
 *----------------------------------------------------------------------------*/
bool symbols_mark_entry(Symbols *s, const char *name, int line, Errors *errs) {
    int idx = symbols_find_idx(s, name); /* existing index or -1 */
    if (idx >= 0) {
        Symbol *sym = &s->items[idx];
        if (sym->attrs & SYM_EXTERN) {
            errors_addf(errs, line,
                        "symbol '%s' marked .entry but also declared .extern", name);
            return false;
        }
        sym->attrs |= SYM_ENTRY;
        return true;
    }
    /* Not seen yet: create a placeholder carrying ENTRY (value set later). */
    if (symbols_insert(s, name, 0, SYM_ENTRY, 0) < 0) {
        return false;  /* OOM */
    }
    return true;
}

/*----------------------------------------------------------------------------
 * symbols_lookup(s, name, out):
 *   Fetch a copy of the symbol record by name.
 *
 * Params:
 *   s    - table.
 *   name - symbol name.
 *   out  - optional output (may be NULL to just check existence).
 *
 * Returns:
 *   true/false - true if found (and *out filled); false if not found.
 *
 * Errors:
 *   None.
 *----------------------------------------------------------------------------*/
bool symbols_lookup(const Symbols *s, const char *name, Symbol *out) {
    int idx = symbols_find_idx(s, name);
    if (idx < 0) return false;
    if (out) *out = s->items[idx];
    return true;
}

/*----------------------------------------------------------------------------
 * symbols_is_external(s, name):
 *   Convenience: report whether 'name' is declared .extern.
 *
 * Params:
 *   s    - table.
 *   name - symbol name.
 *
 * Returns:
 *   true/false - true if found and has SYM_EXTERN; false otherwise.
 *
 * Errors:
 *   None.
 *----------------------------------------------------------------------------*/
bool symbols_is_external(const Symbols *s, const char *name) {
    Symbol tmp; /* scratch */
    if (!symbols_lookup(s, name, &tmp)) return false;
    return (tmp.attrs & SYM_EXTERN) != 0;
}

/*----------------------------------------------------------------------------
 * symbols_relocate_data(s, ic_final):
 *   Add 'ic_final' to values of all DATA symbols (final relocation).
 *
 * Params:
 *   s        - table.
 *   ic_final - final code size (ICF) to offset data labels by.
 *
 * Returns:
 *   void.
 *
 * Errors:
 *   None.
 *----------------------------------------------------------------------------*/
void symbols_relocate_data(Symbols *s, int ic_final) {
    size_t i;                     /* index over items */
    for (i = 0; i < s->len; ++i) {
        if (s->items[i].attrs & SYM_DATA) {
            s->items[i].value += ic_final;
        }
    }
}

/*----------------------------------------------------------------------------
 * symbols_dump(fp, s):
 *   Debug helper: dump as lines "<name> <value> FLAG|FLAG|...".
 *
 * Params:
 *   fp - output stream (e.g., stdout).
 *   s  - table to dump.
 *
 * Returns:
 *   void.
 *
 * Errors:
 *   I/O failures ignored (best-effort debug output).
 *----------------------------------------------------------------------------*/
void symbols_dump(FILE *fp, const Symbols *s) {
    size_t i;                         /* index over items */
    for (i = 0; i < s->len; ++i) {
        const Symbol *sym = &s->items[i];
        char flags[64]; flags[0]='\0';  /* assembled flag string */

        if (sym->attrs & SYM_CODE)   strcat(flags, flags[0]?"|CODE":"CODE");
        if (sym->attrs & SYM_DATA)   strcat(flags, flags[0]?"|DATA":"DATA");
        if (sym->attrs & SYM_EXTERN) strcat(flags, flags[0]?"|EXTERN":"EXTERN");
        if (sym->attrs & SYM_ENTRY)  strcat(flags, flags[0]?"|ENTRY":"ENTRY");
        if (!flags[0]) strcpy(flags, "NONE");

        fprintf(fp, "%s %d %s\n", sym->name, sym->value, flags);
    }
}

/*----------------------------------------------------------------------------
 * symbols_foreach(s, fn, user):
 *   Apply 'fn' to each symbol in insertion order.
 *
 * Params:
 *   s    - table (may be NULL -> no-op).
 *   fn   - callback invoked for each entry.
 *   user - user data pointer passed to callback.
 *
 * Returns:
 *   void.
 *
 * Errors:
 *   None (defensive NULL checks).
 *----------------------------------------------------------------------------*/
void symbols_foreach(const Symbols *s, SymbolsEachFn fn, void *user) {
    size_t i;                   /* index over items */
    if (!s || !fn) return;
    for (i = 0; i < s->len; ++i) {
        fn(&s->items[i], user);
    }
}

/*----------------------------------------------------------------------------
 * symbols_count(s):
 *   Optional convenience for tests/tools.
 *
 * Params:
 *   s - table (may be NULL).
 *
 * Returns:
 *   size_t - number of items (0 if s==NULL).
 *----------------------------------------------------------------------------*/
size_t symbols_count(const Symbols *s) { return s ? s->len : 0; }

/*----------------------------------------------------------------------------
 * symbols_at(s, i, out):
 *   Fetch the i'th symbol (in insertion order).
 *
 * Params:
 *   s   - table.
 *   i   - zero-based index.
 *   out - output record (must be non-NULL on success).
 *
 * Returns:
 *   true/false - true if i is in range and *out filled; false otherwise.
 *
 * Errors:
 *   None (defensive bounds checks).
 *----------------------------------------------------------------------------*/
bool symbols_at(const Symbols *s, size_t i, Symbol *out) {
    if (!s || i >= s->len || !out) return false;
    *out = s->items[i];
    return true;
}
