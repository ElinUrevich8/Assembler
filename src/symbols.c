/* src/symbols.c */
#include "symbols.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>   /* for FILE, fprintf */

/* C90-safe strdup replacement */
static char *dup_c90(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* -------- main structure ---------------------------------------------- */

struct Symbols {
    Symbol  *items;    /* dynamic array of symbols */
    size_t   len;
    size_t   cap;
};

static void ensure_items(Symbols *s, size_t need) {
    size_t ncap;
    if (s->cap >= need) return;
    ncap = s->cap ? s->cap * 2 : 16;
    while (ncap < need) ncap *= 2;
    s->items = (Symbol *)realloc(s->items, ncap * sizeof(Symbol));
    s->cap = ncap;
}

Symbols *symbols_new(void) {
    Symbols *s = (Symbols *)calloc(1, sizeof(Symbols));
    return s;
}

void symbols_free(Symbols *s) {
    size_t i;
    if (!s) return;
    for (i = 0; i < s->len; ++i) {
        /* names are dup_c90'ed below */
        free((void*)s->items[i].name);
    }
    free(s->items);
    free(s);
}

/* Find existing symbol by name (linear; small and simple). */
static int symbols_find_idx(const Symbols *s, const char *name) {
    size_t i;
    for (i = 0; i < s->len; ++i) {
        if (strcmp(s->items[i].name, name) == 0) return (int)i;
    }
    return -1;
}

/* Insert a new symbol (assumes it doesn't exist yet). */
static int symbols_insert(Symbols *s, const char *name,
                          int value, unsigned attrs, int def_line) {
    int idx;
    ensure_items(s, s->len + 1);
    idx = (int)s->len++;
    s->items[idx].name = dup_c90(name);   /* C90-safe strdup */
    s->items[idx].value = value;
    s->items[idx].attrs = attrs;
    s->items[idx].def_line = def_line;
    return idx;
}

bool symbols_define(Symbols *s, const char *name, int value,
                    unsigned attrs, int def_line, Errors *errs) {
    int idx;
    const unsigned kind_mask = (SYM_CODE | SYM_DATA | SYM_EXTERN);

    if ((attrs & kind_mask) == 0) {
        /* internal misuse—don’t emit user error here */
        return false;
    }

    idx = symbols_find_idx(s, name);
    if (idx >= 0) {
        Symbol *sym = &s->items[idx];
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

    symbols_insert(s, name, value, attrs, def_line);
    return true;
}

bool symbols_mark_entry(Symbols *s, const char *name, int line, Errors *errs) {
    int idx = symbols_find_idx(s, name);
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
    /* not seen yet: create placeholder with ENTRY flag */
    symbols_insert(s, name, 0, SYM_ENTRY, 0);
    return true;
}

bool symbols_lookup(const Symbols *s, const char *name, Symbol *out) {
    int idx = symbols_find_idx(s, name);
    if (idx < 0) return false;
    if (out) *out = s->items[idx];
    return true;
}

bool symbols_is_external(const Symbols *s, const char *name) {
    Symbol tmp;
    if (!symbols_lookup(s, name, &tmp)) return false;
    return (tmp.attrs & SYM_EXTERN) != 0;
}

void symbols_relocate_data(Symbols *s, int ic_final) {
    size_t i;
    for (i = 0; i < s->len; ++i) {
        if (s->items[i].attrs & SYM_DATA) {
            s->items[i].value += ic_final;
        }
    }
}

/* Optional test helper */
void symbols_dump(FILE *fp, const Symbols *s) {
    size_t i;
    for (i = 0; i < s->len; ++i) {
        const Symbol *sym = &s->items[i];
        char flags[64]; flags[0]='\0';
        if (sym->attrs & SYM_CODE)   strcat(flags, flags[0]?"|CODE":"CODE");
        if (sym->attrs & SYM_DATA)   strcat(flags, flags[0]?"|DATA":"DATA");
        if (sym->attrs & SYM_EXTERN) strcat(flags, flags[0]?"|EXTERN":"EXTERN");
        if (sym->attrs & SYM_ENTRY)  strcat(flags, flags[0]?"|ENTRY":"ENTRY");
        if (!flags[0]) strcpy(flags, "NONE");
        fprintf(fp, "%s %d %s\n", sym->name, sym->value, flags);
    }
}
