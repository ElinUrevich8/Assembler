/*==========================================================================
 *  Pass 2
 *  -------
 *  - Reads the preassembled & scanned file again (.am).
 *  - Uses encoding_parse_instruction() to re-parse each instruction line.
 *  - Finishes encoding by emitting the extra operand words:
 *      • immediates (A)
 *      • registers (A)
 *      • direct symbols (R or E + record to .ext)
 *      • matrix base label (R/E) + row/col registers (A)
 *  - Collects .entry output (name + final relocated address).
 *  - Produces the final code image (instructions only) into out->code_final.
 *  - Shares the Pass 1 Errors aggregator (prints all issues together).
 *
 *  IMPORTANT FIX:
 *  --------------
 *  External symbol use-sites must be recorded at the exact address of the
 *  extra operand word that we push for them. To avoid off-by-one errors,
 *  we now increment the instruction counter (IC) *inside* the function that
 *  pushes the word. That way, the IC and the emitted words are always in
 *  lock-step, and .ext entries point to the correct addresses in the .ob.
 *
 *  C90 compliant.
 *==========================================================================*/

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

/* C90-safe strdup:
 * Allocates a new buffer and copies the zero-terminated string into it. */
static char *dup_c90(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = (char*)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* Normalized symbol info required by Pass 2. */
typedef struct {
    int is_extern;  /* 1 if symbol is declared .extern */
    int defined;    /* 1 if symbol has CODE or DATA address */
    int value;      /* final relocated address (IC for CODE, ICF+offset for DATA) */
} SymInfo;

/* Look up a symbol and normalize the interesting bits for easy branching.
 * Returns 0 if not found, 1 if found (and fills *out). */
static int sym_info_lookup(const Pass1Result *p1, const char *name, SymInfo *out)
{
    Symbol s;
    if (!symbols_lookup(p1->symbols, name, &s)) return 0;
    out->is_extern = (s.attrs & SYM_EXTERN) != 0;
    out->defined   = (s.attrs & (SYM_CODE | SYM_DATA)) != 0;
    out->value     = s.value;
    return 1;
}

/*-------------------- dynamic arrays (.ext / .ent) ----------------------*/

/* Ensure capacity for 'ext' rows (external use sites). */
static void ext_ensure(Pass2Result *out, size_t need)
{
    size_t cap = out->ext_cap ? out->ext_cap : 8;
    if (out->ext_cap >= need) return;
    while (cap < need) cap *= 2;
    out->ext = (ExtUse*)realloc(out->ext, cap * sizeof(*out->ext));
    out->ext_cap = cap;
}

/* Ensure capacity for 'ent' rows (entry output). */
static void ent_ensure(Pass2Result *out, size_t need)
{
    size_t cap = out->ent_cap ? out->ent_cap : 8;
    if (out->ent_cap >= need) return;
    while (cap < need) cap *= 2;
    out->ent = (EntryOut*)realloc(out->ent, cap * sizeof(*out->ent));
    out->ent_cap = cap;
}

/* Append one external use (symbol + address of the *operand* word). */
static void ext_push(Pass2Result *out, const char *name, int addr)
{
    ext_ensure(out, out->ext_len + 1);
    out->ext[out->ext_len].name = dup_c90(name);
    out->ext[out->ext_len].addr = addr;
    out->ext_len++;
}

/* Append one entry output row (name + final relocated address). */
static void ent_push(Pass2Result *out, const char *name, int addr)
{
    ent_ensure(out, out->ent_len + 1);
    out->ent[out->ent_len].name = dup_c90(name);
    out->ent[out->ent_len].addr = addr;
    out->ent_len++;
}

/*---------------------- input line pre-filtering ------------------------*/

/* Decide whether a given line should be parsed/encoded in Pass 2.
 * - Skips blank/comment lines.
 * - Skips optional leading "label:" (Pass 1 already validated names).
 * - Skips directives (.data/.string/.mat/.entry/.extern).
 * If the line contains an instruction, *pline is advanced to the opcode. */
static int line_should_encode(const char **pline)
{
    const char *p = *pline;

    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0' || *p == ';') return 0;

    /* Optional leading label; Pass 1 forbids underscores, but we only need
     * to skip "label:" quickly here—accepting '_' is harmless at this point. */
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

/* If an 8-bit payload is out of range, warn (the ISA encoders mask it anyway).
 * This helps catch suspicious immediates/addresses early. */
static void check_fit8(Errors *errs, int lineno, long v, const char *what)
{
    if (v < -128 || v > 255) {
        errors_addf(errs, lineno, "%s value out of 8-bit range: %ld (masked)", what, v);
    }
}

/*-------------------- operand: symbol extra-word emitter -----------------*/
/* Emit one extra operand word for a symbol (DIRECT or MATRIX base label).
 *
 *  - If the symbol is undefined  → emit E-word + error
 *  - If the symbol is external   → record use-site (.ext) at current IC,
 *                                  then emit E-word and advance IC
 *  - If the symbol is local      → emit R-word with the relocated address
 *                                  and advance IC
 *
 *  IMPORTANT:
 *  ----------
 *  The instruction counter (*io_ic) is incremented INSIDE this function
 *  exactly when the extra word is pushed. This guarantees that:
 *    • the address recorded in .ext matches the actual word address, and
 *    • callers do NOT add an extra increment afterward (no off-by-one).
 */
static void emit_symbol_operand(const char *name,
                                int *io_ic,
                                int lineno,
                                const Pass1Result *p1,
                                Pass2Result *out)
{
    SymInfo s;

    if (!sym_info_lookup(p1, name, &s)) {
        errors_addf(out->errors, lineno, "undefined symbol '%s'", name);
        codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
        (*io_ic)++;
        return;
    }

    if (s.is_extern) {
        /* Record the address of the operand word we are about to emit.
         * (IC points to that word BEFORE we push it.) */
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

    /* Defensive: placeholders for truly weird states. */
    errors_addf(out->errors, lineno, "undefined symbol '%s'", name);
    codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
    (*io_ic)++;
}

/*------------------------------- encoder --------------------------------*/
/* Parse+encode exactly one input line; append words to out->code_final.
 * On success returns 1 (even if the line was not an instruction).
 * On parse/encode error returns 0 but continues to scan the file. */
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
/* Callback for symbols_foreach(): push one row to .ent if valid.
 * Rejects impossible combinations (extern+entry, undefined+entry). */
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

/* Visit all symbols and collect .ent rows. */
static int collect_entries(const Pass1Result *p1, Pass2Result *out)
{
    symbols_foreach(p1->symbols, ent_collect_cb, out);
    return 1;
}

/*--------------------------------- API ----------------------------------*/
/* Run Pass 2 over <am_path>, producing:
 *  - out->code_final (finished instruction image),
 *  - out->ext[] (use-sites for external symbols),
 *  - out->ent[] (entries),
 *  - out->code_len / out->data_len (for writers). */
int pass2_run(const char *am_path, const Pass1Result *p1, Pass2Result *out)
{
    FILE *fp;
    char line[MAX_LINE_LEN + 2]; /* room for trailing '\n' + NUL */
    int lineno;
    int ic;

    memset(out, 0, sizeof(*out));
    out->ok = 1;
    out->errors = (Errors*)&p1->errors;    /* borrow Pass 1 aggregator */
    codeimg_init(&out->code_final);

    fp = fopen(am_path, "r");
    if (!fp) {
        errors_addf(out->errors, 0, "cannot open %s", am_path);
        out->ok = 0;
        return 0;
    }

    ic = IC_INIT;  /* IC points to absolute code addresses (first word=100) */
    lineno = 0;

    while (fgets(line, sizeof line, fp)) {
        lineno++;
        /* Remove comments (but keep ';' inside strings). */
        strip_comment_inplace(line);

        if (!encode_instruction_into(line, lineno, &ic, p1, out)) {
            out->ok = 0; /* keep scanning to collect all errors */
        }
    }

    fclose(fp);

    /* Build .ent */
    collect_entries(p1, out);

    /* Final sizes for output writers. */
    out->code_len = codeimg_size(&out->code_final);
    out->data_len = (size_t)p1->dc;

    return out->ok;
}

/* Release memory owned by Pass 2 result (except shared errors). */
void pass2_free(Pass2Result *r)
{
    size_t i;
    if (!r) return;

    for (i = 0; i < r->ext_len; ++i) free(r->ext[i].name);
    free(r->ext);  r->ext = NULL; r->ext_len = 0; r->ext_cap = 0;

    for (i = 0; i < r->ent_len; ++i) free(r->ent[i].name);
    free(r->ent);  r->ent = NULL; r->ent_len = 0; r->ent_cap = 0;

    /* Do NOT free r->errors: it's borrowed from Pass 1. */
    r->errors = NULL;
    r->ok = 0;
}
