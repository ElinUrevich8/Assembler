#ifndef SYMBOLS_H
#define SYMBOLS_H
#include <stdio.h>  /* for FILE */
#include "defaults.h"
#include "errors.h"

/* Bit-flags describing the symbol. */
typedef enum {
    SYM_CODE   = 1u << 0,  /* defined as code label (value = IC at definition) */
    SYM_DATA   = 1u << 1,  /* defined as data label (value = DC at definition) */
    SYM_EXTERN = 1u << 2,  /* declared by .extern (no value in this file) */
    SYM_ENTRY  = 1u << 3   /* marked by .entry (must be defined locally, not extern) */
} SymAttr;

typedef struct {
    const char *name;   /* interned string (owned by Nameset) */
    int         value;  /* address */
    unsigned    attrs;  /* bitmask of SymAttr */
    int         def_line; /* definition line (0 if not defined yet) */
} Symbol;

typedef struct Symbols Symbols;

/* Create / destroy the table. */
Symbols *symbols_new(void);
void     symbols_free(Symbols *s);

/* Define (or re-define) a symbol as CODE/DATA/EXTERN.
   - Duplicate local definition -> error.
   - Define after .entry is OK (entry flag kept).
   - Defining something previously declared EXTERN -> error. */
bool symbols_define(Symbols *s, const char *name, int value,
                    unsigned attrs, int def_line, Errors *errs);

/* Mark symbol as ENTRY (may appear before/after the actual definition).
   - ENTRY on EXTERN is illegal. */
bool symbols_mark_entry(Symbols *s, const char *name, int line, Errors *errs);

/* Lookup by name; returns true and copies to *out if found. */
bool symbols_lookup(const Symbols *s, const char *name, Symbol *out);

/* Convenience: does 'name' exist and is extern? */
bool symbols_is_external(const Symbols *s, const char *name);

/* Relocate all DATA symbols by adding ic_final to their value. */
void symbols_relocate_data(Symbols *s, int ic_final);

#endif /* SYMBOLS_H */
