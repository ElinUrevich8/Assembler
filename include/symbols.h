/*============================================================================
 *  symbols.h
 *
 *  Symbol table (labels): define, query, mark entry/extern, relocate data.
 *
 *  Attributes:
 *    - SYM_CODE   : defined by a code label (value = IC at definition)
 *    - SYM_DATA   : defined by a data label (value = DC at definition; relocated later)
 *    - SYM_EXTERN : declared via .extern (no local definition allowed)
 *    - SYM_ENTRY  : marked via .entry (must end up locally defined & non-extern)
 *
 *  Lifecycle: symbols_new() * define/mark/lookup * relocate * foreach * symbols_free()
 *============================================================================*/
#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stdio.h>  /* FILE */
#include "defaults.h"
#include "errors.h"

/* Bit-flags describing a symbol's state. */
typedef enum {
    SYM_CODE   = 1u << 0,  /* defined as code (value = IC at definition) */
    SYM_DATA   = 1u << 1,  /* defined as data (value = DC at definition; relocated later) */
    SYM_EXTERN = 1u << 2,  /* declared by .extern (no local definition allowed) */
    SYM_ENTRY  = 1u << 3   /* marked by .entry (must be local & defined in the end) */
} SymAttr;

/* Readability helpers (macros = C90-friendly) */
#define SYMBOL_IS_DEFINED(symp)   (((symp)->attrs & (SYM_CODE | SYM_DATA)) != 0)
#define SYMBOL_IS_EXTERN(symp)    (((symp)->attrs & SYM_EXTERN) != 0)
#define SYMBOL_IS_ENTRY(symp)     (((symp)->attrs & SYM_ENTRY)  != 0)

/* Public view of a symbol.
 * name: owned by the Symbols table (allocated & freed inside symbols.c)
 * value: address (DATA symbols are relocated after pass 1)
 * attrs: bitmask of SymAttr
 * def_line: source line of definition (0 if not defined yet)
 */
typedef struct {
    const char *name;
    int         value;
    unsigned    attrs;
    int         def_line;
} Symbol;

/* Opaque table type (implemented in symbols.c) */
typedef struct Symbols Symbols;

/*--------------------------- Lifecycle ------------------------------------*/
Symbols *symbols_new(void);
void     symbols_free(Symbols *s);

/*--------------------------- Definitions / Flags ---------------------------*/
/* Define or re-define as CODE/DATA/EXTERN.
 * - Duplicate local definition -> error
 * - Defining after .entry is fine (ENTRY flag kept)
 * - Defining something declared EXTERN -> error
 */
bool symbols_define(Symbols *s, const char *name, int value,
                    unsigned attrs, int def_line, Errors *errs);

/* Mark as ENTRY (before or after definition). ENTRY on EXTERN is illegal. */
bool symbols_mark_entry(Symbols *s, const char *name, int line, Errors *errs);

/*--------------------------- Queries --------------------------------------*/
/* Lookup by name; returns true and copies found symbol to *out. */
bool symbols_lookup(const Symbols *s, const char *name, Symbol *out);
/* Convenience: returns true iff 'name' exists and is extern. */
bool symbols_is_external(const Symbols *s, const char *name);

/*--------------------------- Relocation / Iteration ------------------------*/
/* Relocate all DATA symbols by adding ic_final (run at end of pass 1). */
void symbols_relocate_data(Symbols *s, int ic_final);

/* Iterate all symbols in insertion order. */
typedef void (*SymbolsEachFn)(const Symbol *sym, void *user);
void symbols_foreach(const Symbols *s, SymbolsEachFn fn, void *user);

#endif /* SYMBOLS_H */
