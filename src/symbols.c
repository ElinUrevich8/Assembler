#include "symbols.h"
#include "nameset.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* -------- internal map (open addressing, no deletes) ------------------ */

typedef struct {
    const char **keys;  /* interned names (NULL = empty) */
    int         *vals;  /* index into 'items' */
    size_t       cap;   /* power-of-two capacity */
    size_t       used;  /* number of occupied slots */
} PtrMap;

static size_t hash_ptr(const void *p) {
    /* 64/32-bit friendly pointer hash (xorshift). */
    uintptr_t x = (uintptr_t)p;
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (size_t)x;
}

static void ptrmap_init(PtrMap *m) { memset(m, 0, sizeof(*m)); }

static void ptrmap_free(PtrMap *m) {
    free(m->keys);
    free(m->vals);
    memset(m, 0, sizeof(*m));
}

static void ptrmap_grow(PtrMap *m) {
    size_t ncap = (m->cap == 0 ? 16 : m->cap * 2);
    const char **nkeys = (const char **)calloc(ncap, sizeof(const char*));
    int *nvals = (int *)malloc(ncap * sizeof(int));

    for (size_t i = 0; i < ncap; ++i) nvals[i] = -1;

    /* rehash */
    for (size_t i = 0; i < m->cap; ++i) {
        const char *k = m->keys[i];
        if (!k) continue;
        int v = m->vals[i];
        size_t h = hash_ptr(k);
        size_t mask = ncap - 1;
        size_t pos = h & mask;
        while (nkeys[pos]) pos = (pos + 1) & mask;
        nkeys[pos] = k;
        nvals[pos] = v;
    }

    free(m->keys);
    free(m->vals);
    m->keys = nkeys;
    m->vals = nvals;
    m->cap  = ncap;
    m->used = (m->used); /* unchanged */
}

static void ptrmap_put(PtrMap *m, const char *key, int val) {
    if ((m->used + 1) * 10 >= (m->cap ? m->cap : 1) * 7) {
        ptrmap_grow(m);
    }
    if (m->cap == 0) ptrmap_grow(m);

    size_t mask = m->cap - 1;
    size_t pos = hash_ptr(key) & mask;
    while (m->keys[pos]) {
        if (m->keys[pos] == key) { m->vals[pos] = val; return; }
        pos = (pos + 1) & mask;
    }
    m->keys[pos] = key;
    m->vals[pos] = val;
    m->used++;
}

static int ptrmap_get(const PtrMap *m, const char *key) {
    if (m->cap == 0) return -1;
    size_t mask = m->cap - 1;
    size_t pos = hash_ptr(key) & mask;
    while (m->keys[pos]) {
        if (m->keys[pos] == key) return m->vals[pos];
        pos = (pos + 1) & mask;
    }
    return -1;
}

/* -------- main structure ---------------------------------------------- */

struct Symbols {
    NameSet *intern;   /* shared interning for names */
    Symbol  *items;    /* dynamic array of symbols */
    size_t   len, cap; /* length/capacity of items */
    PtrMap   index;    /* name(ptr) -> items[index] */
};

static void ensure_items(Symbols *s, size_t need) {
    if (s->cap >= need) return;
    size_t ncap = s->cap ? s->cap * 2 : 16;
    while (ncap < need) ncap *= 2;
    s->items = (Symbol *)realloc(s->items, ncap * sizeof(Symbol));
    s->cap = ncap;
}

Symbols *symbols_new(void) {
    Symbols *s = (Symbols *)calloc(1, sizeof(Symbols));
    s->intern = nameset_new();
    ptrmap_init(&s->index);
    return s;
}

void symbols_free(Symbols *s) {
    if (!s) return;
    nameset_free(s->intern);
    free(s->items);
    ptrmap_free(&s->index);
    free(s);
}

/* Find existing symbol by name (interned). Returns index or -1. */
static int symbols_find_idx(const Symbols *s, const char *interned) {
    return ptrmap_get(&s->index, interned);
}

/* Insert a new symbol (assumes it doesn't exist yet). */
static int symbols_insert(Symbols *s, const char *interned,
                          int value, unsigned attrs, int def_line) {
    ensure_items(s, s->len + 1);
    int idx = (int)s->len++;
    s->items[idx].name = interned;
    s->items[idx].value = value;
    s->items[idx].attrs = attrs;
    s->items[idx].def_line = def_line;
    ptrmap_put(&s->index, interned, idx);
    return idx;
}

bool symbols_define(Symbols *s, const char *name, int value,
                    unsigned attrs, int def_line, Errors *errs) {
    /* We expect attrs to include one of CODE/DATA/EXTERN. */
    const unsigned kind_mask = (SYM_CODE | SYM_DATA | SYM_EXTERN);
    if ((attrs & kind_mask) == 0) {
        errors_addf(errs, def_line, "internal: symbols_define without kind");
        return false;
    }

    const char *interned = nameset_intern(s->intern, name);
    int idx = symbols_find_idx(s, interned);

    if (idx >= 0) {
        Symbol *sym = &s->items[idx];
        /* Cannot define a symbol that is extern. */
        if (sym->attrs & SYM_EXTERN) {
            errors_addf(errs, def_line,
                        "cannot define external symbol '%s' (declared extern at line %d)",
                        name, sym->def_line);
            return false;
        }
        /* Already defined locally as CODE/DATA? -> duplicate definition. */
        if (sym->attrs & (SYM_CODE | SYM_DATA)) {
            errors_addf(errs, def_line,
                        "duplicate label '%s' (previously defined at line %d)",
                        name, sym->def_line);
            return false;
        }
        /* Allowed case: previously only ENTRY; now define value+kind. */
        sym->value    = value;
        sym->attrs   |= (attrs & kind_mask);
        sym->def_line = def_line;
        return true;
    }

    /* New symbol. */
    symbols_insert(s, interned, value, attrs, def_line);
    return true;
}

bool symbols_mark_entry(Symbols *s, const char *name, int line, Errors *errs) {
    const char *interned = nameset_intern(s->intern, name);
    int idx = symbols_find_idx(s, interned);

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

    /* Not seen yet: create a placeholder with ENTRY; value will be set when defined. */
    symbols_insert(s, interned, 0, SYM_ENTRY, 0);
    return true;
}

bool symbols_lookup(const Symbols *s, const char *name, Symbol *out) {
    const char *interned = nameset_intern(s->intern, name);
    int idx = symbols_find_idx(s, interned);
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
    for (size_t i = 0; i < s->len; ++i) {
        if (s->items[i].attrs & SYM_DATA) {
            s->items[i].value += ic_final;
        }
    }
}
