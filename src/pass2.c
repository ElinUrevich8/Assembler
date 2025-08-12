#include <ctype.h>
#include <string.h>
#include "pass2.h"
#include "encoding.h"
#include "encoding_parse.h"
#include "symbols.h"
#include "isa.h"

/* Local adapter: normalize symbol info needed for A/R/E decision.
 * NOTE: Pass 1 already relocates DATA symbols; 'value' here is final. */
typedef struct {
    int is_extern;  /* true if SYM_EXTERN */
    int defined;    /* true if CODE or DATA */
    int value;      /* final address (IC/relocated DC) */
} SymInfo;

static int sym_info_lookup(const Pass1Result *p1, const char *name, SymInfo *out) {
    Symbol s;
    if (!symbols_lookup(p1->symbols, name, &s)) return 0;
    out->is_extern = (s.attrs & SYM_EXTERN) != 0;
    out->defined   = (s.attrs & (SYM_CODE | SYM_DATA)) != 0;
    out->value     = s.value;
    return 1;
}

/* Decide whether a line should be encoded:
 * - skip empty/comment lines
 * - strip optional "LABEL:" prefix
 * - skip directives (handled in pass 1)
 */
static int line_should_encode(const char **pline) {
    const char *p = *pline;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0' || *p == ';') return 0;

    /* Optional leading label */
    if (isalpha((unsigned char)*p) || *p == '_') {
        const char *q = p;
        while (isalnum((unsigned char)*q) || *q == '_') q++;
        if (*q == ':') { p = q + 1; while (*p && isspace((unsigned char)*p)) p++; }
    }

    /* Directives (e.g., .data/.string/.mat) produce words in pass 1 only */
    if (*p == '.' || *p == '\0' || *p == ';') return 0;

    *pline = p;
    return 1;
}

/* Utility: emit an error if a value won't fit in 8-bit payload (signed or unsigned),
 * but still encode the masked low 8 bits to keep addresses aligned. */
static void check_fit8(Errors *errs, int lineno, long v, const char *what) {
    /* Accept -128..255 if your spec allows signed immediates; otherwise tighten to 0..255. */
    if (v < -128 || v > 255) {
        errors_addf(errs, lineno, "%s value out of 8-bit range: %ld (masked)", what, v);
    }
}

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

    /* first word */
    codeimg_push_word(&out->code_final,
                      (int)isa_first_word(pi.opcode,
                                          (pi.argc==2 ? (int)pi.src_mode : -1),
                                          (pi.argc>=1 ? (int)pi.dst_mode : -1)),
                      lineno);
    (*io_ic)++;

    /* 2 operands */
    if (pi.argc == 2) {
        if (pi.src_mode == ADR_REGISTER && pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_regs_pair(pi.src_reg, pi.dst_reg), lineno);
            (*io_ic)++;
            return 1;
        }

        /* ---- src ---- */
        if (pi.src_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.src_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.src_imm), lineno);
            (*io_ic)++;
        } else if (pi.src_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_src(pi.src_reg), lineno);
            (*io_ic)++;
        } else if (pi.src_mode == ADR_DIRECT) {
            SymInfo s; int use_addr = *io_ic;
            if (!sym_info_lookup(p1, pi.src_sym, &s) || !s.defined) {
                errors_addf(out->errors, lineno, "undefined symbol '%s'", pi.src_sym);
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
            } else if (s.is_extern) {
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
                ext_push(out, pi.src_sym, use_addr);
            } else {
                check_fit8(out->errors, lineno, s.value, "address");
                codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
            }
            (*io_ic)++;
        } else if (pi.src_mode == ADR_MATRIX) {
            /* word #1: base label (R/E) */
            SymInfo s; int use_addr = *io_ic;
            if (!sym_info_lookup(p1, pi.src_sym, &s) || !s.defined) {
                errors_addf(out->errors, lineno, "undefined symbol '%s'", pi.src_sym);
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
            } else if (s.is_extern) {
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
                ext_push(out, pi.src_sym, use_addr);
            } else {
                check_fit8(out->errors, lineno, s.value, "address");
                codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
            }
            (*io_ic)++;

            /* word #2: row/col regs (ARE=A) — row in [9:6], col in [5:2] */
            codeimg_push_word(&out->code_final, (int)isa_word_regs_pair(pi.src_mat_row, pi.src_mat_col), lineno);
            (*io_ic)++;
        }

        /* ---- dst ---- */
        if (pi.dst_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.dst_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.dst_imm), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_dst(pi.dst_reg), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_DIRECT) {
            SymInfo s; int use_addr = *io_ic;
            if (!sym_info_lookup(p1, pi.dst_sym, &s) || !s.defined) {
                errors_addf(out->errors, lineno, "undefined symbol '%s'", pi.dst_sym);
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
            } else if (s.is_extern) {
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
                ext_push(out, pi.dst_sym, use_addr);
            } else {
                check_fit8(out->errors, lineno, s.value, "address");
                codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
            }
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_MATRIX) {
            SymInfo s; int use_addr = *io_ic;
            if (!sym_info_lookup(p1, pi.dst_sym, &s) || !s.defined) {
                errors_addf(out->errors, lineno, "undefined symbol '%s'", pi.dst_sym);
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
            } else if (s.is_extern) {
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
                ext_push(out, pi.dst_sym, use_addr);
            } else {
                check_fit8(out->errors, lineno, s.value, "address");
                codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
            }
            (*io_ic)++;

            codeimg_push_word(&out->code_final, (int)isa_word_regs_pair(pi.dst_mat_row, pi.dst_mat_col), lineno);
            (*io_ic)++;
        }

        return 1;
    }

    /* 1 operand */
    if (pi.argc == 1) {
        if (pi.dst_mode == ADR_IMMEDIATE) {
            check_fit8(out->errors, lineno, pi.dst_imm, "immediate");
            codeimg_push_word(&out->code_final, (int)isa_word_imm(pi.dst_imm), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_REGISTER) {
            codeimg_push_word(&out->code_final, (int)isa_word_reg_dst(pi.dst_reg), lineno);
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_DIRECT) {
            SymInfo s; int use_addr = *io_ic;
            if (!sym_info_lookup(p1, pi.dst_sym, &s) || !s.defined) {
                errors_addf(out->errors, lineno, "undefined symbol '%s'", pi.dst_sym);
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
            } else if (s.is_extern) {
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
                ext_push(out, pi.dst_sym, use_addr);
            } else {
                check_fit8(out->errors, lineno, s.value, "address");
                codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
            }
            (*io_ic)++;
        } else if (pi.dst_mode == ADR_MATRIX) {
            SymInfo s; int use_addr = *io_ic;
            if (!sym_info_lookup(p1, pi.dst_sym, &s) || !s.defined) {
                errors_addf(out->errors, lineno, "undefined symbol '%s'", pi.dst_sym);
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
            } else if (s.is_extern) {
                codeimg_push_word(&out->code_final, (int)isa_word_extern(), lineno);
                ext_push(out, pi.dst_sym, use_addr);
            } else {
                check_fit8(out->errors, lineno, s.value, "address");
                codeimg_push_word(&out->code_final, (int)isa_word_reloc(s.value), lineno);
            }
            (*io_ic)++;

            codeimg_push_word(&out->code_final, (int)isa_word_regs_pair(pi.dst_mat_row, pi.dst_mat_col), lineno);
            (*io_ic)++;
        }
        return 1;
    }

    /* 0 operands: nothing more */
    return 1;
}


/* Collect .ent rows: only defined, non-extern symbols marked ENTRY.
 * Errors are recorded for invalid cases; empty result => no .ent file.
 */
static void ent_collect_cb(const Symbol *sym, void *ud) {
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

static int collect_entries(const Pass1Result *p1, Pass2Result *out) {
    symbols_foreach(p1->symbols, ent_collect_cb, out);
    return 1;
}
