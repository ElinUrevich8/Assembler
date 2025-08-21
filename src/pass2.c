/*============================================================================
 *  pass2.c
 *
 *  Second pass:
 *    - Re-scan the .am file for instruction lines.
 *    - Parse each instruction (encoding_parse_instruction).
 *    - Finish encoding: emit extra operand words in the required order
 *      and with correct ARE bits (A/R/E), recording .ext uses as we go.
 *    - Collect .entry outputs from the symbol table.
 *    - Produce the final instruction image in out->code_final.
 *
 *  Note on external-use addresses:
 *    To keep .ext addresses exact, the IC increment for each emitted
 *    operand word happens inside the helper that pushes that word. This
 *    keeps the word-stream and the IC in lock-step.
 *============================================================================*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pass2.h"
#include "encoding.h"
#include "encoding_parse.h"
#include "symbols.h"
#include "isa.h"

/*---------------------------- small utilities ---------------------------*/

/*----------------------------------------------------------------------------
 * dup_c90(s):
 *   strdup replacement: allocate a new buffer and copy string 's'.
 *
 * Params:
 *   s - NUL-terminated string to duplicate.
 *
 * Returns:
 *   char* / NULL - newly allocated copy on success; NULL on OOM.
 *
 * Errors:
 *   OOM: returns NULL (callers must check).
 *----------------------------------------------------------------------------*/
static char *dup_c90(const char *s)
{
    size_t n = strlen(s) + 1;   /* bytes including NUL */
    char *p = (char*)malloc(n); /* new buffer          */
    if (p) memcpy(p, s, n);
    return p;
}

/*----------------------------------------------------------------------------
 * SymInfo:
 *   Normalized symbol info required by Pass 2.
 *----------------------------------------------------------------------------*/
typedef struct {
    int is_extern;  /* 1 if symbol is declared .extern */
    int defined;    /* 1 if symbol has CODE or DATA address */
    int value;      /* final relocated address (IC for CODE, ICF+offset for DATA) */
} SymInfo;

/*----------------------------------------------------------------------------
 * sym_info_lookup(p1, name, out):
 *   Look up 'name' in p1->symbols and normalize the interesting bits.
 *
 * Params:
 *   p1   - pass1 result (provides final symbols and relocation).
 *   name - symbol name to look up.
 *   out  - output struct to receive normalized info.
 *
 * Returns:
 *   1/0 - 1 if found and *out filled; 0 if not found.
 *
 * Errors:
 *   None (pure query). 'out' must be non-NULL if return is 1.
 *----------------------------------------------------------------------------*/
static int sym_info_lookup(const Pass1Result *p1, const char *name, SymInfo *out)
{
    Symbol s;                       /* found symbol snapshot */
    if (!symbols_lookup(p1->symbols, name, &s)) return 0;
    out->is_extern = (s.attrs & SYM_EXTERN) != 0;
    out->defined   = (s.attrs & (SYM_CODE | SYM_DATA)) != 0;
    out->value     = s.value;
    return 1;
}

/*-------------------- dynamic arrays (.ext / .ent) ----------------------*/

/*----------------------------------------------------------------------------
 * ext_ensure(out, need):
 *   Ensure capacity for 'need' ExtUse rows in out->ext (grows geometrically).
 *
 * Params:
 *   out  - pass2 result holder (owns ext[]).
 *   need - required number of slots.
 *
 * Returns:
 *   void - on OOM, sets out->ok=0 and records an error.
 *
 * Errors:
 *   OOM: adds "out of memory (ext)" to error list and leaves capacity unchanged.
 *----------------------------------------------------------------------------*/
static void ext_ensure(Pass2Result *out, size_t need)
{
    size_t cap = out->ext_cap ? out->ext_cap : 8;
    ExtUse *nptr;                   /* new allocation pointer */

    if (out->ext_cap >= need) return;
    while (cap < need) cap *= 2;

    nptr = (ExtUse*)realloc(out->ext, cap * sizeof(*out->ext));
    if (!nptr) {
        out->ok = 0;
        errors_addf(out->errors, 0, "out of memory while growing .ext table");
        return;
    }
    out->ext = nptr;
    out->ext_cap = cap;
}

/*----------------------------------------------------------------------------
 * ent_ensure(out, need):
 *   Ensure capacity for 'need' EntryOut rows in out->ent (grows geometrically).
 *
 * Params:
 *   out  - pass2 result holder (owns ent[]).
 *   need - required number of slots.
 *
 * Returns:
 *   void - on OOM, sets out->ok=0 and records an error.
 *
 * Errors:
 *   OOM: adds "out of memory (ent)" to error list and leaves capacity unchanged.
 *----------------------------------------------------------------------------*/
static void ent_ensure(Pass2Result *out, size_t need)
{
    size_t cap = out->ent_cap ? out->ent_cap : 8;
    EntryOut *nptr;                 /* new allocation pointer */

    if (out->ent_cap >= need) return;
    while (cap < need) cap *= 2;

    nptr = (EntryOut*)realloc(out->ent, cap * sizeof(*out->ent));
    if (!nptr) {
        out->ok = 0;
        errors_addf(out->errors, 0, "out of memory while growing .ent table");
        return;
    }
    out->ent = nptr;
    out->ent_cap = cap;
}

/*----------------------------------------------------------------------------
 * ext_push(out, name, addr):
 *   Append one external use row (symbol name + use-site address).
 *
 * Params:
 *   out  - pass2 result holder.
 *   name - external symbol name.
 *   addr - IC address of the *operand* word that referenced the extern.
 *
 * Returns:
 *   void - on OOM sets out->ok=0 and records an error; partial append is skipped.
 *
 * Errors:
 *   OOM on ext_ensure or dup_c90: error is recorded; no row appended.
 *----------------------------------------------------------------------------*/
static void ext_push(Pass2Result *out, const char *name, int addr)
{
    char *dup;                      /* owned copy of 'name' */
    ext_ensure(out, out->ext_len + 1);
    if (!out->ok) return;

    dup = dup_c90(name);
    if (!dup) {
        out->ok = 0;
        errors_addf(out->errors, 0, "out of memory duplicating extern name");
        return;
    }

    out->ext[out->ext_len].name = dup;
    out->ext[out->ext_len].addr = addr;
    out->ext_len++;
}

/*----------------------------------------------------------------------------
 * ent_push(out, name, addr):
 *   Append one .entry row (symbol name + final relocated address).
 *
 * Params:
 *   out  - pass2 result holder.
 *   name - entry symbol name.
 *   addr - final relocated address.
 *
 * Returns:
 *   void - on OOM sets out->ok=0 and records an error; partial append is skipped.
 *
 * Errors:
 *   OOM on ent_ensure or dup_c90: error is recorded; no row appended.
 *----------------------------------------------------------------------------*/
static void ent_push(Pass2Result *out, const char *name, int addr)
{
    char *dup;                      /* owned copy of 'name' */
    ent_ensure(out, out->ent_len + 1);
    if (!out->ok) return;

    dup = dup_c90(name);
    if (!dup) {
        out->ok = 0;
        errors_addf(out->errors, 0, "out of memory duplicating entry name");
        return;
    }

    out->ent[out->ent_len].name = dup;
    out->ent[out->ent_len].addr = addr;
    out->ent_len++;
}

/*---------------------- input line pre-filtering ------------------------*/

/*----------------------------------------------------------------------------
 * line_should_encode(pline):
 *   Decide whether the current line contains an instruction to encode.
 *   Skips blank/comment lines, optional leading "label:", and directives.
 *
 * Params:
 *   pline - in/out pointer to C-string; advanced to opcode on return.
 *
 * Returns:
 *   1/0 - 1 if line should be parsed/encoded; 0 otherwise.
 *
 * Errors:
 *   None. Tolerant to underscores in labels here (Pass 1 did validation).
 *----------------------------------------------------------------------------*/
static int line_should_encode(const char **pline)
{
    const char *p = *pline; /* scanning cursor */

    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0' || *p == ';') return 0;

    /* Optional leading label. */
    if (isalpha((unsigned char)*p) || *p == '_') {
        const char *q = p;
        while (isalnum((unsigned char)*q) || *q == '_') q++;
        if (*q == ':') {
            p = q + 1;
            while (*p && isspace((unsigned char)*p)) p++;
        }
    }

    /* Not an instruction if it starts with a directive or is empty. */
    if (*p == '.' || *p == '\0' || *p == ';') return 0;

    *pline = p;
    return 1;
}

/*----------------------------------------------------------------------------
 * check_fit8(errs, lineno, v, what):
 *   Warn if an 8-bit payload is out of the nominal range. Encoders mask anyway.
 *
 * Params:
 *   errs  - error aggregator (adds a note when out of range).
 *   lineno- 1-based input line number.
 *   v     - value to check.
 *   what  - text describing the value ("immediate", "address").
 *
 * Returns:
 *   void.
 *
 * Errors:
 *   None (adds a warning-like error entry).
 *----------------------------------------------------------------------------*/
static void check_fit8(Errors *errs, int lineno, long v, const char *what)
{
    if (v < -128 || v > 255) {
        errors_addf(errs, lineno, "%s value out of 8-bit range: %ld (masked)", what, v);
    }
}

/*-------------------- operand: symbol extra-word emitter -----------------*/

/*----------------------------------------------------------------------------
 * emit_symbol_operand(name, io_ic, lineno, p1, out):
 *   Emit one extra operand word for a symbol (DIRECT or MATRIX base label).
 *   Increments *io_ic exactly when the word is emitted to keep .ext addresses exact.
 *
 * Params:
 *   name   - symbol name.
 *   io_ic  - in/out instruction counter (absolute IC).
 *   lineno - input line for diagnostics.
 *   p1     - pass1 result (for symbol table).
 *   out    - pass2 result (code image + .ext collection + errors).
 *
 * Returns:
 *   void - on undefined symbol, emits E-word + error; on extern, records .ext then E-word;
 *          on local, emits R-word with relocated address.
 *
 * Errors:
 *   Undefined symbol: records error, emits E-word.
 *   OOM while recording .ext: sets out->ok=0; still emits encoded word to keep stream stable.
 *----------------------------------------------------------------------------*/
static void emit_symbol_operand(const char *name,
                                int *io_ic,
                                int lineno,
                                const Pass1Result *p1,
                                Pass2Result *out)
{
    SymInfo s;   /* normalized symbol info */

    if (!sym_info_lookup(p1, name, &s)) {
        errors_addf(out->errors, lineno, "undefined symbol '%s'", name);
        codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
        (*io_ic)++;
        return;
    }

    if (s.is_extern) {
        /* Record address of the operand word we are about to emit. */
        ext_push(out, name, *io_ic);
        codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
        (*io_ic)++;
        return;
    }

    if (s.defined) {
        check_fit8(out->errors, lineno, (long)s.value, "address");
        codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
        (*io_ic)++;
        return;
    }

    /* Defensive: unreachable in normal flows. */
    errors_addf(out->errors, lineno, "undefined symbol '%s'", name);
    codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
    (*io_ic)++;
}

/*------------------------------- encoder --------------------------------*/

/*----------------------------------------------------------------------------
 * encode_instruction_into(line, lineno, io_ic, p1, out):
 *   Parse+encode exactly one input line; append words to out->code_final.
 *
 * Params:
 *   line   - raw input line (mutable for comment stripping by caller).
 *   lineno - 1-based input line number for diagnostics.
 *   io_ic  - in/out absolute IC (first word = 100).
 *   p1     - pass1 result (symbols + dc).
 *   out    - pass2 result (code image, .ext/.ent arrays, errors, ok flag).
 *
 * Returns:
 *   1/0 - 1 on success (even if line is not an instruction); 0 on parse/encode error.
 *
 * Errors:
 *   Parser failures recorded in out->errors; out->ok set to 0. Keeps scanning.
 *----------------------------------------------------------------------------*/
static int encode_instruction_into(const char *line,
                                   int lineno,
                                   int *io_ic,
                                   const Pass1Result *p1,
                                   Pass2Result *out)
{
    ParsedInstr pi;        /* parsed instruction fields */
    const char *p = line;  /* scanning cursor (maybe jumps past label) */

    if (!line_should_encode(&p)) return 1;

    if (!encoding_parse_instruction(p, &pi, out->errors, lineno)) {
        out->ok = 0;
        return 0;
    }

    /* First word: opcode + addressing-mode fields. ARE=A (absolute). */
    codeimg_push_word(&out->code_final,
                      (int)isa_first_word(pi.opcode,
                                          (pi.argc == 2 ? (int)pi.src_mode : -1),
                                          (pi.argc >= 1 ? (int)pi.dst_mode : -1)),
                      lineno);
    (*io_ic)++;

    /* ---------- two operands ---------- */
    if (pi.argc == 2) {
        /* register-register pair packs into ONE extra A-word. */
        if (pi.src_mode == ADR_REGISTER && pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.src_reg, pi.dst_reg),
                              lineno);
            (*io_ic)++;
            return 1;
        }

        /* source operand */
        if (pi.src_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.src_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.src_imm), lineno);
            (*io_ic)++;
        } else if (pi.src_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_src(pi.src_reg), lineno);
            (*io_ic)++;
        } else if (pi.src_mode == ADR_DIRECT) {
            emit_symbol_operand(pi.src_sym, io_ic, lineno, p1, out);
        } else if (pi.src_mode == ADR_MATRIX) {
            /* base label word (E/R) */
            emit_symbol_operand(pi.src_sym, io_ic, lineno, p1, out);
            /* row/col registers word (A) */
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.src_mat_row, pi.src_mat_col),
                              lineno);
            (*io_ic)++;
        }

        /* destination operand */
        if (pi.dst_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.dst_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.dst_imm), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_dst(pi.dst_reg), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_DIRECT) {
            emit_symbol_operand(pi.dst_sym, io_ic, lineno, p1, out);
        } else if (pi.dst_mode == ADR_MATRIX) {
            emit_symbol_operand(pi.dst_sym, io_ic, lineno, p1, out);
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.dst_mat_row, pi.dst_mat_col),
                              lineno);
            (*io_ic)++;
        }

        return 1;
    }

    /* ---------- one operand ---------- */
    if (pi.argc == 1) {
        if (pi.dst_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.dst_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.dst_imm), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_dst(pi.dst_reg), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_DIRECT) {
            emit_symbol_operand(pi.dst_sym, io_ic, lineno, p1, out);
        } else if (pi.dst_mode == ADR_MATRIX) {
            emit_symbol_operand(pi.dst_sym, io_ic, lineno, p1, out);
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.dst_mat_row, pi.dst_mat_col),
                              lineno);
            (*io_ic)++;
        }
        return 1;
    }

    /* ---------- zero operands ---------- */
    return 1;
}

/*-------------------------- .ent collection logic -----------------------*/

/*----------------------------------------------------------------------------
 * ent_collect_cb(sym, ud):
 *   symbols_foreach callback: if 'sym' is a valid .entry, append to out->ent.
 *   Rejects extern+entry and undefined+entry combinations.
 *
 * Params:
 *   sym - current symbol (read-only).
 *   ud  - Pass2Result* to receive rows/errors.
 *
 * Returns:
 *   void.
 *
 * Errors:
 *   Inconsistent entries recorded with line numbers from sym->def_line.
 *----------------------------------------------------------------------------*/
static void ent_collect_cb(const Symbol *sym, void *ud)
{
    Pass2Result *out = (Pass2Result*)ud;  /* output collector */

    if (!SYMBOL_IS_ENTRY(sym)) return;

    if (SYMBOL_IS_EXTERN(sym)) {
        errors_addf(out->errors, sym->def_line,
                    "entry symbol '%s' declared extern", sym->name);
        return;
    }
    if (!SYMBOL_IS_DEFINED(sym)) {
        errors_addf(out->errors, sym->def_line,
                    "entry symbol '%s' is undefined", sym->name);
        return;
    }
    ent_push(out, sym->name, sym->value);
}

/*----------------------------------------------------------------------------
 * collect_entries(p1, out):
 *   Visit all symbols and collect .ent rows into 'out'.
 *
 * Params:
 *   p1  - pass1 result (provides final symbols + relocation).
 *   out - pass2 result to receive entry rows.
 *
 * Returns:
 *   1 - always succeeds (individual errors added per bad symbol).
 *
 * Errors:
 *   None beyond individual symbol diagnostics.
 *----------------------------------------------------------------------------*/
static int collect_entries(const Pass1Result *p1, Pass2Result *out)
{
    symbols_foreach(p1->symbols, ent_collect_cb, out);
    return 1;
}

/*--------------------------------- API ----------------------------------*/

/*----------------------------------------------------------------------------
 * pass2_run(am_path, p1, out):
 *   Run Pass 2 over <am_path>, producing:
 *     - out->code_final (finished instruction image),
 *     - out->ext[] (use-sites for external symbols),
 *     - out->ent[] (entries),
 *     - out->code_len / out->data_len (for writers).
 *
 * Params:
 *   am_path - path to the preassembled .am input file.
 *   p1      - pass1 result (symbols, final DC, shared error aggregator).
 *   out     - result holder (initialized by this function).
 *
 * Returns:
 *   1/0 - 1 on overall success; 0 if any errors were recorded or I/O failed.
 *
 * Errors:
 *   I/O: fopen/fgets/fclose failures recorded; out->ok=0.
 *   Encode/parse: recorded by helpers; out->ok=0 but scanning continues.
 *   Memory: .ext/.ent OOM recorded and out->ok=0.
 *----------------------------------------------------------------------------*/
int pass2_run(const char *am_path, const Pass1Result *p1, Pass2Result *out)
{
    FILE *fp = NULL;                           /* input .am stream           */
    char line[MAX_LINE_LEN + 2];               /* room for trailing '\n'+NUL */
    int lineno = 0;                            /* 1-based line counter       */
    int ic = IC_INIT;                          /* absolute IC (first word=100) */

    memset(out, 0, sizeof(*out));
    out->ok = 1;
    out->errors = (Errors*)&p1->errors;        /* borrow Pass 1 aggregator   */
    codeimg_init(&out->code_final);

    fp = fopen(am_path, "r");
    if (!fp) {
        errors_addf(out->errors, 0, "cannot open %s", am_path);
        out->ok = 0;
        return 0;
    }

    while (fgets(line, sizeof line, fp)) {
        lineno++;
        /* Remove comments (but keep ';' inside strings). */
        strip_comment_inplace(line);

        if (!encode_instruction_into(line, lineno, &ic, p1, out)) {
            out->ok = 0; /* keep scanning to collect all errors */
        }
    }

    if (ferror(fp)) {
        errors_addf(out->errors, lineno, "I/O error while reading %s", am_path);
        out->ok = 0;
    }

    if (fclose(fp) != 0) {
        errors_addf(out->errors, 0, "failed to close %s", am_path);
        out->ok = 0;
    }

    /* Build .ent */
    (void)collect_entries(p1, out);

    /* Final sizes for output writers. */
    out->code_len = codeimg_size(&out->code_final);
    out->data_len = (size_t)p1->dc;

    return out->ok;
}

/*----------------------------------------------------------------------------
 * pass2_free(r):
 *   Release memory owned by Pass 2 result (except shared errors).
 *
 * Params:
 *   r - Pass2Result to free/reset (safe to call with NULL).
 *
 * Returns:
 *   void.
 *
 * Errors:
 *   None. After return, *r fields are zeroed/NULLed where applicable.
 *----------------------------------------------------------------------------*/
void pass2_free(Pass2Result *r)
{
    size_t i;  /* loop index over ext/ent arrays */
    if (!r) return;

    for (i = 0; i < r->ext_len; ++i) free(r->ext[i].name);
    free(r->ext);  r->ext = NULL; r->ext_len = 0; r->ext_cap = 0;

    for (i = 0; i < r->ent_len; ++i) free(r->ent[i].name);
    free(r->ent);  r->ent = NULL; r->ent_len = 0; r->ent_cap = 0;

    /* Do NOT free r->errors: it's borrowed from Pass 1. */
    r->errors = NULL;
    r->ok = 0;
}
