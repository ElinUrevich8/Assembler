/* src/encoding.c  – C90-safe helpers for directives & instruction sizing
 *
 * Implements the API in include/encoding.h and include/encoding_parse.h:
 *  - parse_symbol_name()
 *  - parse_and_push_data_operands()
 *  - parse_and_push_string()
 *  - parse_and_push_mat()
 *  - encoding_estimate_size()
 *  - encoding_lookup_opcode()
 *  - encoding_parse_instruction()   (shared with Pass 2)
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "encoding.h"         /* defaults.h, errors.h, codeimg.h, AddrMode */
#include "encoding_parse.h"   /* ParsedInstr + public parse for Pass 2 */

/* ------------------------- local helpers ------------------------------ */

/* Skip ASCII whitespace. */
static const char* skip_ws(const char *p) {
    while (p && *p && isspace((unsigned char)*p)) p++;
    return p;
}

/* Read identifier [A-Za-z_][A-Za-z0-9_]*; returns start, sets *out_end. */
static const char* read_word(const char *p, const char **out_end) {
    const char *s = p;
    if (!isalpha((unsigned char)*s) && *s != '_') return NULL;
    s++;
    while (isalnum((unsigned char)*s) || *s == '_') s++;
    *out_end = s;
    return p;
}

/* Parse optional sign + decimal integer; returns 1 on success, 0 on failure. */
int parse_int10(const char *p, const char **end, long *out) {
    char *e;
    long v;
    e = NULL;
    v = strtol(p, &e, 10);
    if (e == p) return 0;
    *out = v;
    *end = e;
    return 1;
}

/* Try parse register r0..r7; if yes set *reg (0..7) and *end. */
int parse_register(const char *p, const char **end, int *reg) {
    if (p[0] == 'r' && isdigit((unsigned char)p[1]) && !isalnum((unsigned char)p[2])) {
        int r = p[1] - '0';
        if (r >= 0 && r <= 7) {
            *reg = r;
            *end = p + 2;
            return 1;
        }
    }
    return 0;
}

/* Parse a single operand and classify addressing mode.
   For ADR_DIRECT, copy symbol into sym_out if provided. */
AddrMode parse_operand(const char *p, const char **end,
                       char *sym_out, size_t sym_cap) {
    const char *e;
    const char *w;
    long v;
    int r;

    p = skip_ws(p);
    if (*p == '#') { /* immediate */
        if (!parse_int10(p + 1, &e, &v)) return ADR_INVALID;
        *end = e;
        return ADR_IMMEDIATE;
    }
    if (parse_register(p, &e, &r)) { *end = e; return ADR_REGISTER; }

    w = read_word(p, &e);
    if (w) {
        size_t n = (size_t)(e - w);
        if (sym_out && sym_cap) {
            if (n >= sym_cap) n = sym_cap - 1;
            memcpy(sym_out, w, n);
            sym_out[n] = '\0';
        }
        *end = e;
        return ADR_DIRECT; /* may be "upgraded" to ADR_MATRIX if followed by [rX][rY] */
    }
    return ADR_INVALID;
}

/* After parsing: only spaces and/or a comment are allowed. */
int rest_is_comment_or_ws(const char *p) {
    p = skip_ws(p);
    return (*p == '\0' || *p == ';');
}

/* Read mnemonic into buf; returns pointer just after mnemonic or NULL. */
const char* read_mnemonic(const char *p, char *buf, size_t cap) {
    const char *end;
    const char *w;
    size_t n;

    p = skip_ws(p);
    w = read_word(p, &end);
    if (!w) return NULL;

    n = (size_t)(end - w);
    if (n >= cap) n = cap - 1;
    memcpy(buf, w, n);
    buf[n] = '\0';
    return end;
}

/* Consume a comma between operands (after skipping spaces). */
static int parse_comma(const char **p) {
    const char *q = skip_ws(*p);
    if (*q != ',') return 0;
    *p = q + 1;
    return 1;
}

/* Parse a suffix of the form: [rX][rY]; returns 1 on success and sets row/col/end. */
static int parse_matrix_suffix(const char *p, const char **end, int *row, int *col) {
    const char *q;

    q = skip_ws(p);
    if (*q != '[') return 0;
    q++;
    q = skip_ws(q);
    if (!parse_register(q, &q, row)) return 0;
    q = skip_ws(q);
    if (*q != ']') return 0;
    q++;

    q = skip_ws(q);
    if (*q != '[') return 0;
    q++;
    q = skip_ws(q);
    if (!parse_register(q, &q, col)) return 0;
    q = skip_ws(q);
    if (*q != ']') return 0;
    q++;

    *end = q;
    return 1;
}

/* --------------------------- opcode spec ------------------------------ */

typedef struct {
    const char *name;
    int         opcode;   /* 0..15 per spec */
    int         argc;     /* number of operands (0,1,2) */
    unsigned    src_ok;   /* allowed addressing modes for src (2-operand ops) */
    unsigned    dst_ok;   /* allowed addressing modes for dst (or single-operand) */
} OpSpec;

/* Addressing masks for convenience — treat MATRIX as "direct-like". */
#define AM_ALL      (ADR_IMMEDIATE | ADR_DIRECT | ADR_REGISTER | ADR_MATRIX)
#define AM_NOIMM    (ADR_DIRECT | ADR_REGISTER | ADR_MATRIX)
#define AM_ONLYDIR  (ADR_DIRECT | ADR_MATRIX)

/* 16 ops aligned with your spec */
static const OpSpec OPS[] = {
    /* name  op argc  src-ok     dst-ok */
    {"mov",   0, 2, AM_ALL,     AM_NOIMM},
    {"cmp",   1, 2, AM_ALL,     AM_ALL},
    {"add",   2, 2, AM_ALL,     AM_NOIMM},
    {"sub",   3, 2, AM_ALL,     AM_NOIMM},
    {"not",   4, 1, 0,          AM_NOIMM},
    {"clr",   5, 1, 0,          AM_NOIMM},
    {"lea",   6, 2, AM_ONLYDIR, AM_NOIMM}, /* source must be "direct-like" */
    {"inc",   7, 1, 0,          AM_NOIMM},
    {"dec",   8, 1, 0,          AM_NOIMM},
    {"jmp",   9, 1, 0,          AM_ONLYDIR},
    {"bne",  10, 1, 0,          AM_ONLYDIR},
    {"red",  11, 1, 0,          AM_NOIMM},
    {"prn",  12, 1, 0,          AM_ALL},   /* prn allows immediate */
    {"jsr",  13, 1, 0,          AM_ONLYDIR},
    {"rts",  14, 0, 0,          0},
    {"stop", 15, 0, 0,          0}
};
static const size_t OPS_LEN = sizeof(OPS) / sizeof(OPS[0]);

/* Linear lookup is fine for 16 ops. */
static const OpSpec* find_op(const char *mnemonic) {
    size_t i;
    for (i = 0; i < OPS_LEN; ++i) {
        if (strcmp(mnemonic, OPS[i].name) == 0) return &OPS[i];
    }
    return NULL;
}

/* --------------------------- public API ------------------------------- */

/* Parse a legal symbol name (skips leading spaces). */
bool parse_symbol_name(const char *s, char *out, size_t outsz) {
    const char *end;
    const char *w;
    size_t n;

    s = skip_ws(s);
    w = read_word(s, &end);
    if (!w) return false;

    n = (size_t)(end - w);
    if (n >= outsz) n = outsz - 1;
    memcpy(out, w, n);
    out[n] = '\0';
    return true;
}

/* Parse `.data` list and push each integer into the data image.
   Returns number of words pushed, or -1 on error. */
int parse_and_push_data_operands(const char *s, CodeImg *data,
                                 Errors *errs, int lineno) {
    int count = 0;
    const char *p = s;
    int expect_number = 1;

    p = skip_ws(p);
    if (*p == ';' || *p == '\0') {
        errors_addf(errs, lineno, "malformed .data list");
        return -1;
    }

    for (;;) {
        const char *e;
        long v;

        p = skip_ws(p);

        if (expect_number) {
            if (*p == ',' || *p == ';' || *p == '\0') {
                errors_addf(errs, lineno, "malformed .data list");
                return -1;
            }
            if (!parse_int10(p, &e, &v)) {
                errors_addf(errs, lineno, "malformed .data list");
                return -1;
            }
            codeimg_push_word(data, (int)v, lineno);
            count++;
            p = skip_ws(e);
            expect_number = 0;
            continue;
        }

        if (*p == ',') {
            p++;
            p = skip_ws(p);
            if (*p == ';' || *p == '\0') {
                errors_addf(errs, lineno, "malformed .data list");
                return -1;
            }
            expect_number = 1;
            continue;
        }
        if (*p == ';' || *p == '\0') {
            break;
        }
        errors_addf(errs, lineno, "malformed .data list");
        return -1;
    }

    if (count <= 0) {
        errors_addf(errs, lineno, "malformed .data list");
        return -1;
    }
    return count;
}

/* Parse `.string "..."`, push chars as words + terminating 0.
   Supports \" and \\ escapes minimally. */
int parse_and_push_string(const char *s, CodeImg *data,
                          Errors *errs, int lineno) {
    const char *p;
    int pushed;

    p = skip_ws(s);
    if (*p != '\"') {
        errors_addf(errs, lineno, ".string expects a quoted literal");
        return -1;
    }
    p++; /* after opening quote */

    pushed = 0;
    while (*p && *p != '\"') {
        char c = *p++;
        if (c == '\\' && *p) { /* simple escapes */
            char esc = *p++;
            if (esc == '\"') c = '\"';
            else if (esc == '\\') c = '\\';
            else c = esc; /* pass-through others */
        }
        codeimg_push_word(data, (unsigned char)c, lineno);
        pushed++;
    }
    if (*p != '\"') {
        errors_addf(errs, lineno, "missing closing quote in .string");
        return -1;
    }
    p++; /* after closing quote */

    codeimg_push_word(data, 0, lineno); /* NUL terminator */
    pushed++;

    if (!rest_is_comment_or_ws(p)) {
        errors_addf(errs, lineno, "unexpected text after .string");
        return -1;
    }
    return pushed;
}

/* Parse .mat [rows][cols] with optional initializer list, push cell words. */
int parse_and_push_mat(const char *s, CodeImg *data, Errors *errs, int lineno) {
    const char *p = s;
    long rows = 0, cols = 0;
    const char *e;
    int total, filled, expect_number;

    p = skip_ws(p);
    if (*p != '[') { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
    p++;
    if (!parse_int10(p, &e, &rows)) { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
    p = skip_ws(e);
    if (*p != ']') { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
    p++;

    p = skip_ws(p);
    if (*p != '[') { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
    p++;
    if (!parse_int10(p, &e, &cols)) { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
    p = skip_ws(e);
    if (*p != ']') { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
    p++;

    if (rows <= 0 || cols <= 0) {
        errors_addf(errs, lineno, "malformed .mat definition");
        return -1;
    }

    total = (int)(rows * cols);
    if (total <= 0) {
        errors_addf(errs, lineno, "malformed .mat definition");
        return -1;
    }

    p = skip_ws(p);
    filled = 0;

    if (*p == '\0' || *p == ';') {
        while (filled < total) { codeimg_push_word(data, 0, lineno); filled++; }
        return total;
    }

    expect_number = 1;
    for (;;) {
        long v;

        p = skip_ws(p);

        if (expect_number) {
            if (*p == ',' || *p == '\0' || *p == ';') { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
            if (!parse_int10(p, &e, &v)) { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
            if (filled >= total) { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
            codeimg_push_word(data, (int)v, lineno);
            filled++;
            p = skip_ws(e);
            expect_number = 0;
            continue;
        }

        if (*p == ',') {
            p++;
            p = skip_ws(p);
            if (*p == '\0' || *p == ';') { errors_addf(errs, lineno, "malformed .mat definition"); return -1; }
            expect_number = 1;
            continue;
        }

        if (*p == '\0' || *p == ';') break;

        errors_addf(errs, lineno, "malformed .mat definition");
        return -1;
    }

    while (filled < total) { codeimg_push_word(data, 0, lineno); filled++; }
    return total;
}

/* Extra words per addressing mode (register/immediate/direct=1, matrix=2). */
static int words_for_mode(AddrMode m) {
    if (m == ADR_REGISTER)  return 1;
    if (m == ADR_IMMEDIATE) return 1;
    if (m == ADR_DIRECT)    return 1;
    if (m == ADR_MATRIX)    return 2; /* label + reg-pair word */
    return 0;
}

/* Estimate the number of words an instruction will occupy (pass 1).
 * Uses the same tokenization as the runtime parser and "upgrades"
 * ADR_DIRECT to ADR_MATRIX if a label is followed by [rX][rY].
 */
bool encoding_estimate_size(const char *instr, EncodedInstrSize *sz,
                            Errors *errs, int lineno) {
    char mnem[32];
    const char *p;
    const OpSpec *spec;
    AddrMode src, dst;
    char tmp_sym[64];
    const char *e, *e2, *peek;
    int add;

    sz->words = 0;
    sz->operands = 0;

    p = read_mnemonic(instr, mnem, sizeof mnem);
    if (!p) { errors_addf(errs, lineno, "expected instruction mnemonic"); return false; }
    spec = find_op(mnem);
    if (!spec) { errors_addf(errs, lineno, "unknown mnemonic '%s'", mnem); return false; }

    src = ADR_INVALID;
    dst = ADR_INVALID;
    e = p;

    if (spec->argc == 2) {
        peek = e; while (*peek && isspace((unsigned char)*peek)) peek++;
        if (*peek == ',' || *peek == '\0' || *peek == ';') {
            errors_addf(errs, lineno, "missing source operand");
            return false;
        }

        /* src */
        src = parse_operand(e, &e, tmp_sym, sizeof tmp_sym);
        if (src == ADR_INVALID) { errors_addf(errs, lineno, "invalid source operand"); return false; }

        /* if label followed by [rX][rY] → matrix */
        if (src == ADR_DIRECT) {
            int row, col;
            if (parse_matrix_suffix(e, &e2, &row, &col)) {
                src = ADR_MATRIX;
                e = e2;
            }
        }

        if (!parse_comma(&e)) { errors_addf(errs, lineno, "expected comma between operands"); return false; }

        peek = e; while (*peek && isspace((unsigned char)*peek)) peek++;
        if (*peek == '\0' || *peek == ';') { errors_addf(errs, lineno, "missing destination operand"); return false; }

        /* dst */
        dst = parse_operand(e, &e, tmp_sym, sizeof tmp_sym);
        if (dst == ADR_INVALID) { errors_addf(errs, lineno, "invalid destination operand"); return false; }

        if (dst == ADR_DIRECT) {
            int row2, col2;
            if (parse_matrix_suffix(e, &e2, &row2, &col2)) {
                dst = ADR_MATRIX;
                e = e2;
            }
        }

        if (!(spec->src_ok & src)) { errors_addf(errs, lineno, "addressing mode not allowed for source"); return false; }
        if (!(spec->dst_ok & dst)) { errors_addf(errs, lineno, "addressing mode not allowed for destination"); return false; }
        if (!rest_is_comment_or_ws(e)) { errors_addf(errs, lineno, "unexpected text after instruction"); return false; }

        /* sizing: first word + extras (with reg-reg packing fast path) */
        sz->operands = 2;
        if (src == ADR_REGISTER && dst == ADR_REGISTER) {
            sz->words = 1 + 1; /* first + packed reg-pair */
        } else {
            add = words_for_mode(src) + words_for_mode(dst);
            sz->words = 1 + add;
        }
        return true;
    }

    if (spec->argc == 1) {
        dst = parse_operand(e, &e, tmp_sym, sizeof tmp_sym);
        if (dst == ADR_INVALID) { errors_addf(errs, lineno, "invalid operand"); return false; }

        if (dst == ADR_DIRECT) {
            int row3, col3;
            if (parse_matrix_suffix(e, &e2, &row3, &col3)) {
                dst = ADR_MATRIX;
                e = e2;
            }
        }

        if (!(spec->dst_ok & dst)) { errors_addf(errs, lineno, "addressing mode not allowed"); return false; }
        if (!rest_is_comment_or_ws(e)) { errors_addf(errs, lineno, "unexpected text after instruction"); return false; }

        sz->operands = 1;
        sz->words = 1 + words_for_mode(dst);
        return true;
    }

    /* argc == 0 */
    if (!rest_is_comment_or_ws(e)) {
        errors_addf(errs, lineno, "unexpected text after zero-operand instruction");
        return false;
    }
    sz->operands = 0;
    sz->words = 1;
    return true;
}

/* Lookup numeric opcode and argc by mnemonic. */
bool encoding_lookup_opcode(const char *mnemonic, int *out_opcode, int *out_argc) {
    const OpSpec *s = find_op(mnemonic);
    if (!s) return false;
    if (out_opcode) *out_opcode = s->opcode;
    if (out_argc)   *out_argc   = s->argc;
    return true;
}

/* ----------------- public parser for Pass 2 (full line) ---------------- */

bool encoding_parse_instruction(const char *line, ParsedInstr *out,
                                Errors *errs, int lineno)
{
    char mnem[32];
    const char *p, *e, *peek, *e2;
    const OpSpec *spec;
    AddrMode src, dst;
    long v;
    int r;

    memset(out, 0, sizeof(*out));
    out->src_mode = ADR_INVALID;
    out->dst_mode = ADR_INVALID;

    /* mnemonic */
    p = read_mnemonic(line, mnem, sizeof mnem);
    if (!p) { errors_addf(errs, lineno, "expected instruction mnemonic"); return false; }
    spec = find_op(mnem);
    if (!spec) { errors_addf(errs, lineno, "unknown mnemonic '%s'", mnem); return false; }
    out->opcode = spec->opcode;

    /* 2-operand case */
    if (spec->argc == 2) {
        peek = p; while (*peek && isspace((unsigned char)*peek)) peek++;
        if (*peek == ',' || *peek == '\0' || *peek == ';') { errors_addf(errs, lineno, "missing source operand"); return false; }

        /* src */
        src = parse_operand(p, &e, out->src_sym, sizeof out->src_sym);
        if (src == ADR_INVALID) { errors_addf(errs, lineno, "invalid source operand"); return false; }

        /* immed/reg payload (if needed) */
        if (src == ADR_IMMEDIATE) {
            if (!parse_int10(skip_ws(p + 1), &p, &v)) { errors_addf(errs, lineno, "invalid immediate"); return false; }
            out->src_imm = v;
        } else if (src == ADR_REGISTER) {
            if (!parse_register(skip_ws(p), &p, &r)) { errors_addf(errs, lineno, "invalid register"); return false; }
            out->src_reg = r;
        } else if (src == ADR_DIRECT) {
            int row, col;
            if (parse_matrix_suffix(e, &e2, &row, &col)) {
                src = ADR_MATRIX;
                out->src_mat_row = row;
                out->src_mat_col = col;
                e = e2;
            }
        }
        p = e;

        if (!parse_comma(&p)) { errors_addf(errs, lineno, "expected comma between operands"); return false; }

        peek = p; while (*peek && isspace((unsigned char)*peek)) peek++;
        if (*peek == '\0' || *peek == ';') { errors_addf(errs, lineno, "missing destination operand"); return false; }

        /* dst */
        dst = parse_operand(p, &e, out->dst_sym, sizeof out->dst_sym);
        if (dst == ADR_INVALID) { errors_addf(errs, lineno, "invalid destination operand"); return false; }

        if (dst == ADR_IMMEDIATE) {
            if (*skip_ws(p) != '#') { errors_addf(errs, lineno, "invalid immediate"); return false; }
            if (!parse_int10(skip_ws(p) + 1, &p, &v)) { errors_addf(errs, lineno, "invalid immediate"); return false; }
            out->dst_imm = v;
        } else if (dst == ADR_REGISTER) {
            if (!parse_register(skip_ws(p), &p, &r)) { errors_addf(errs, lineno, "invalid register"); return false; }
            out->dst_reg = r;
        } else if (dst == ADR_DIRECT) {
            int row2, col2;
            if (parse_matrix_suffix(e, &e2, &row2, &col2)) {
                dst = ADR_MATRIX;
                out->dst_mat_row = row2;
                out->dst_mat_col = col2;
                e = e2;
            }
        }
        p = e;

        /* legality + trailing-junk check */
        if (!(spec->src_ok & src)) { errors_addf(errs, lineno, "addressing mode not allowed for source"); return false; }
        if (!(spec->dst_ok & dst)) { errors_addf(errs, lineno, "addressing mode not allowed for destination"); return false; }
        if (!rest_is_comment_or_ws(p)) { errors_addf(errs, lineno, "unexpected text after instruction"); return false; }

        out->argc = 2;
        out->src_mode = src;
        out->dst_mode = dst;
        return true;
    }

    /* 1-operand */
    if (spec->argc == 1) {
        dst = parse_operand(p, &e, out->dst_sym, sizeof out->dst_sym);
        if (dst == ADR_INVALID) { errors_addf(errs, lineno, "invalid operand"); return false; }

        if (dst == ADR_IMMEDIATE) {
            if (*skip_ws(p) != '#') { errors_addf(errs, lineno, "invalid immediate"); return false; }
            if (!parse_int10(skip_ws(p) + 1, &p, &v)) { errors_addf(errs, lineno, "invalid immediate"); return false; }
            out->dst_imm = v;
        } else if (dst == ADR_REGISTER) {
            if (!parse_register(skip_ws(p), &p, &r)) { errors_addf(errs, lineno, "invalid register"); return false; }
            out->dst_reg = r;
        } else if (dst == ADR_DIRECT) {
            int row3, col3;
            if (parse_matrix_suffix(e, &e2, &row3, &col3)) {
                dst = ADR_MATRIX;
                out->dst_mat_row = row3;
                out->dst_mat_col = col3;
                e = e2;
            }
        }

        if (!(spec->dst_ok & dst)) { errors_addf(errs, lineno, "addressing mode not allowed"); return false; }
        if (!rest_is_comment_or_ws(e)) { errors_addf(errs, lineno, "unexpected text after instruction"); return false; }

        out->argc = 1;
        out->dst_mode = dst;
        return true;
    }

    /* 0-operand */
    if (!rest_is_comment_or_ws(p)) {
        errors_addf(errs, lineno, "unexpected text after zero-operand instruction");
        return false;
    }
    out->argc = 0;
    return true;
}
