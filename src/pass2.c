/* src/pass2.c
 * Pass 2: resolve symbols, finalize code words, collect .ext/.ent.
 * - Uses encoding_parse_instruction() (shared with Pass 1) for perfect alignment.
 * - Emits A/R/E per ISA (see isa.h).
 * - Records extern use-sites (symbol + address of the operand word).
 * - Collects valid ENTRY symbols for .ent via symbols_foreach().
 * C90 compliant.
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pass2.h"
#include "encoding.h"
#include "encoding_parse.h"
#include "symbols.h"
#include "isa.h"

/* --------------------------- small helpers ---------------------------- */

static char *dup_c90(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* Normalized symbol info needed by Pass 2. */
typedef struct {
    int is_extern;  /* SYM_EXTERN ? */
    int defined;    /* SYM_CODE|SYM_DATA ? */
    int value;      /* final address (IC or relocated DC) */
} SymInfo;

static int sym_info_lookup(const Pass1Result *p1, const char *name, SymInfo *out)
{
    Symbol s;
    if (!symbols_lookup(p1->symbols, name, &s)) return 0;
    out->is_extern = (s.attrs & SYM_EXTERN) != 0;
    out->defined   = (s.attrs & (SYM_CODE | SYM_DATA)) != 0;
    out->value     = s.value;
    return 1;
}

/* -------------------- containers growth + pushers --------------------- */

static void ext_ensure(Pass2Result *out, size_t need)
{
    size_t cap = out->ext_cap ? out->ext_cap : 8;
    if (out->ext_cap >= need) return;
    while (cap < need) cap *= 2;
    out->ext = (ExtUse*)realloc(out->ext, cap * sizeof(*out->ext));
    out->ext_cap = cap;
}

static void ent_ensure(Pass2Result *out, size_t need)
{
    size_t cap = out->ent_cap ? out->ent_cap : 8;
    if (out->ent_cap >= need) return;
    while (cap < need) cap *= 2;
    out->ent = (EntryOut*)realloc(out->ent, cap * sizeof(*out->ent));
    out->ent_cap = cap;
}

/* Record an external use: 'addr' is the address of the operand word just pushed. */
static void ext_push(Pass2Result *out, const char *name, int addr)
{
    ext_ensure(out, out->ext_len + 1);
    out->ext[out->ext_len].name = dup_c90(name);
    out->ext[out->ext_len].addr = addr;
    out->ext_len++;
}

/* Record an entry row (name + final address). */
static void ent_push(Pass2Result *out, const char *name, int addr)
{
    ent_ensure(out, out->ent_len + 1);
    out->ent[out->ent_len].name = dup_c90(name);
    out->ent[out->ent_len].addr = addr;
    out->ent_len++;
}

/* ------------------------ input line filtering ------------------------ */
/* Skip empty/comment lines; strip optional leading label; ignore directives. */
static int line_should_encode(const char **pline)
{
    const char *p = *pline;

    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0' || *p == ';') return 0;

    /* optional leading label */
    if (isalpha((unsigned char)*p) || *p == '_') {
        const char *q = p;
        while (isalnum((unsigned char)*q) || *q == '_') q++;
        if (*q == ':') {
            p = q + 1;
            while (*p && isspace((unsigned char)*p)) p++;
        }
    }

    /* directives (.data/.string/.mat/.entry/.extern) are not encoded in pass2 */
    if (*p == '.' || *p == '\0' || *p == ';') return 0;

    *pline = p;
    return 1;
}

/* Warn if extra-word payload doesn't fit 8 bits; still masked to 8b by isa_word_* */
static void check_fit8(Errors *errs, int lineno, long v, const char *what)
{
    if (v < -128 || v > 255) {
        errors_addf(errs, lineno, "%s value out of 8-bit range: %ld (masked)", what, v);
    }
}

/* Emit the extra word for a symbol operand (for DIRECT or MATRIX base label).
   Order of checks:
   1) not found     → error + E word
   2) extern        → E word + record use-site
   3) local defined → R word with address
   4) otherwise     → error + E word (defensive) */
static void emit_symbol_operand(const char *name,
                                int use_addr,
                                int lineno,
                                const Pass1Result *p1,
                                Pass2Result *out)
{
    SymInfo s;
    if (!sym_info_lookup(p1, name, &s)) {
        errors_addf(out->errors, lineno, "undefined symbol '%s'", name);
        codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
        return;
    }

    if (s.is_extern) {
        codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
        ext_push(out, name, use_addr);
        return;
    }

    if (s.defined) {
        check_fit8(out->errors, lineno, s.value, "address");
        codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
        return;
    }

    errors_addf(out->errors, lineno, "undefined symbol '%s'", name);
    codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
}

/* ------------------------------ encoder ------------------------------- */
/* Parse and encode a single line into out->code_final; advances *io_ic. */
static int encode_instruction_into(const char *line,
                                   int lineno,
                                   int *io_ic,
                                   const Pass1Result *p1,
                                   Pass2Result *out)
{
    ParsedInstr pi;
    const char *p = line;

    if (!line_should_encode(&p)) return 1;

    if (!encoding_parse_instruction(p, &pi, out->errors, lineno)) {
        out->ok = 0;
        return 0;
    }

    /* First word: opcode + mode fields; ARE=A */
    codeimg_push_word(&out->code_final,
                      (int)isa_first_word(pi.opcode,
                                          (pi.argc == 2 ? (int)pi.src_mode : -1),
                                          (pi.argc >= 1 ? (int)pi.dst_mode : -1)),
                      lineno);
    (*io_ic)++;

    /* === two operands === */
    if (pi.argc == 2) {
        /* register-register: packed into one extra word */
        if (pi.src_mode == ADR_REGISTER && pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.src_reg, pi.dst_reg),
                              lineno);
            (*io_ic)++;
            return 1;
        }

        /* source */
        if (pi.src_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.src_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.src_imm), lineno);
            (*io_ic)++;
        } else if (pi.src_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_src(pi.src_reg), lineno);
            (*io_ic)++;
        } else if (pi.src_mode == ADR_DIRECT) {
            emit_symbol_operand(pi.src_sym, *io_ic, lineno, p1, out);
            (*io_ic)++;
        } else if (pi.src_mode == ADR_MATRIX) {
            /* base label word (E/R) */
            emit_symbol_operand(pi.src_sym, *io_ic, lineno, p1, out);
            (*io_ic)++;
            /* row/col registers word (A) */
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.src_mat_row, pi.src_mat_col),
                              lineno);
            (*io_ic)++;
        }

        /* destination */
        if (pi.dst_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.dst_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.dst_imm), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_dst(pi.dst_reg), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_DIRECT) {
            emit_symbol_operand(pi.dst_sym, *io_ic, lineno, p1, out);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_MATRIX) {
            emit_symbol_operand(pi.dst_sym, *io_ic, lineno, p1, out);
            (*io_ic)++;
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.dst_mat_row, pi.dst_mat_col),
                              lineno);
            (*io_ic)++;
        }

        return 1;
    }

    /* === one operand === */
    if (pi.argc == 1) {
        if (pi.dst_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.dst_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.dst_imm), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_dst(pi.dst_reg), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_DIRECT) {
            emit_symbol_operand(pi.dst_sym, *io_ic, lineno, p1, out);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_MATRIX) {
            emit_symbol_operand(pi.dst_sym, *io_ic, lineno, p1, out);
            (*io_ic)++;
            codeimg_push_word(&out->code_final,
                              (int)isa_word_regs_pair(pi.dst_mat_row, pi.dst_mat_col),
                              lineno);
            (*io_ic)++;
        }
        return 1;
    }

    /* === zero operands === */
    return 1;
}

/* ------------------------ .ent collection logic ----------------------- */

static void ent_collect_cb(const Symbol *sym, void *ud)
{
    Pass2Result *out = (Pass2Result*)ud;

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

static int collect_entries(const Pass1Result *p1, Pass2Result *out)
{
    symbols_foreach(p1->symbols, ent_collect_cb, out);
    return 1;
}

/* ------------------------------ public API ---------------------------- */

int pass2_run(const char *am_path, const Pass1Result *p1, Pass2Result *out)
{
    FILE *fp;
    char line[MAX_LINE_LEN + 2]; /* room for '\n' and '\0' */
    int lineno;
    int ic;

    memset(out, 0, sizeof(*out));
    out->ok = 1;
    out->errors = (Errors*)&p1->errors; /* borrow Pass 1 aggregator */
    codeimg_init(&out->code_final);

    fp = fopen(am_path, "r");
    if (!fp) {
        errors_addf(out->errors, 0, "cannot open %s", am_path);
        out->ok = 0;
        return 0;
    }

    ic = IC_INIT;
    lineno = 0;

    while (fgets(line, sizeof line, fp)) {
        lineno++;
        /* make comments invisible (but keep ';' inside strings) */
        strip_comment_inplace(line);

        if (!encode_instruction_into(line, lineno, &ic, p1, out)) {
            out->ok = 0; /* keep scanning to collect all errors */
        }
    }

    fclose(fp);

    /* entries for .ent */
    collect_entries(p1, out);

    /* final counts for writers */
    out->code_len = codeimg_size(&out->code_final);
    out->data_len = (size_t)p1->dc;

    return out->ok;
}

void pass2_free(Pass2Result *r)
{
    size_t i;
    if (!r) return;

    for (i = 0; i < r->ext_len; ++i) free(r->ext[i].name);
    free(r->ext);
    r->ext = NULL;
    r->ext_len = 0;
    r->ext_cap = 0;

    for (i = 0; i < r->ent_len; ++i) free(r->ent[i].name);
    free(r->ent);
    r->ent = NULL;
    r->ent_len = 0;
    r->ent_cap = 0;

    /* Do NOT free r->errors: it's borrowed from Pass 1. */
    r->errors = NULL;
    r->ok = 0;
}
