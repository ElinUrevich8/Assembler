/* src/encoding.c
 *
 * Parsing helpers for directives and instructions + opcode table.
 * - parse_symbol_name()
 * - parse_and_push_data_operands()
 * - parse_and_push_string()
 * - encoding_estimate_size()
 * - encoding_lookup_opcode()
 *
 * Relies on:
 *   - codeimg_push_word(CodeImg*, int value, int lineno)
 *   - errors_addf(Errors*, int lineno, const char *fmt, ...)
 */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "encoding.h"

/* ---------- local helpers ---------- */

/* Skip ASCII whitespace. */
static const char* skip_ws(const char *p) {
    while (p && *p && isspace((unsigned char)*p)) p++;
    return p;
}

/* Is a reasonable separator/end after a token? */
static int is_sep(int ch) {
    return ch == 0 || ch == ',' || isspace(ch) || ch == ';';
}

/* Read an identifier [A-Za-z_][A-Za-z0-9_]*; returns start, sets *out_end. */
static const char* read_word(const char *p, const char **out_end) {
    const char *s = p;
    if (!isalpha((unsigned char)*s) && *s != '_') return NULL;
    s++;
    while (isalnum((unsigned char)*s) || *s == '_') s++;
    *out_end = s;
    return p;
}

/* Parse optional sign + decimal integer; returns true on success. */
static bool parse_int10(const char *p, const char **end, long *out) {
    char *e = NULL;
    long v = strtol(p, &e, 10);
    if (e == p) return false;
    *out = v;
    *end = e;
    return true;
}

/* Try parse register r0..r7; if yes set *reg (0..7) and *end. */
static bool parse_register(const char *p, const char **end, int *reg) {
    if (p[0] == 'r' && isdigit((unsigned char)p[1]) && !isalnum((unsigned char)p[2])) {
        int r = p[1] - '0';
        if (r >= 0 && r <= 7) {
            *reg = r;
            *end = p + 2;
            return true;
        }
    }
    return false;
}

/* Parse a single operand and classify addressing mode.
   For ADR_DIRECT, copy symbol into sym_out if provided. */
static AddrMode parse_operand(const char *p, const char **end,
                              char *sym_out, size_t sym_cap) {
    p = skip_ws(p);
    if (*p == '#') { /* immediate */
        long v;
        const char *e;
        if (!parse_int10(p + 1, &e, &v)) return ADR_INVALID;
        *end = e;
        return ADR_IMMEDIATE;
    }
    /* register? */
    {
        int r; const char *e;
        if (parse_register(p, &e, &r)) { *end = e; return ADR_REGISTER; }
    }
    /* label (direct) */
    {
        const char *e;
        const char *w = read_word(p, &e);
        if (w) {
            size_t n = (size_t)(e - w);
            if (sym_out && sym_cap) {
                if (n >= sym_cap) n = sym_cap - 1;
                memcpy(sym_out, w, n);
                sym_out[n] = '\0';
            }
            *end = e;
            return ADR_DIRECT;
        }
    }
    return ADR_INVALID;
}

/* After parsing an instruction: allow only spaces and optional comment. */
static bool rest_is_comment_or_ws(const char *p) {
    p = skip_ws(p);
    return (*p == '\0' || *p == ';');
}

/* Read mnemonic into buf; returns pointer just after mnemonic or NULL. */
static const char* read_mnemonic(const char *p, char *buf, size_t cap) {
    p = skip_ws(p);
    const char *end;
    const char *w = read_word(p, &end);
    if (!w) return NULL;
    size_t n = (size_t)(end - w);
    if (n >= cap) n = cap - 1;
    memcpy(buf, w, n);
    buf[n] = '\0';
    return end;
}

/* Consume a comma between operands (after skipping spaces). */
static bool parse_comma(const char **p) {
    const char *q = skip_ws(*p);
    if (*q != ',') return false;
    *p = q + 1;
    return true;
}

/* ---------- opcode spec (exactly per your table 0..15) ---------- */

typedef struct {
    const char *name;
    int         opcode;   /* 0..15 per spec */
    int         argc;     /* number of operands (0,1,2) */
    unsigned    src_ok;   /* allowed addressing modes for src (2-operand ops) */
    unsigned    dst_ok;   /* allowed addressing modes for dst (or single-operand) */
} OpSpec;

/* Addressing masks for convenience. */
#define AM_ALL      (ADR_IMMEDIATE | ADR_DIRECT | ADR_REGISTER)
#define AM_NOIMM    (ADR_DIRECT | ADR_REGISTER)
#define AM_ONLYDIR  (ADR_DIRECT)

/* Ops table aligned with the project */
static const OpSpec OPS[] = {
    /* opcode, name, argc, masks */
    {"mov",   0, 2, AM_ALL,     AM_NOIMM},
    {"cmp",   1, 2, AM_ALL,     AM_ALL},
    {"add",   2, 2, AM_ALL,     AM_NOIMM},
    {"sub",   3, 2, AM_ALL,     AM_NOIMM},
    {"not",   4, 1, 0,          AM_NOIMM},
    {"clr",   5, 1, 0,          AM_NOIMM},
    {"lea",   6, 2, AM_ONLYDIR, AM_NOIMM},
    {"inc",   7, 1, 0,          AM_NOIMM},
    {"dec",   8, 1, 0,          AM_NOIMM},
    {"jmp",   9, 1, 0,          AM_NOIMM},
    {"bne",  10, 1, 0,          AM_NOIMM},
    {"red",  11, 1, 0,          AM_NOIMM},
    {"prn",  12, 1, 0,          AM_ALL},    /* prn allows immediate */
    {"jsr",  13, 1, 0,          AM_NOIMM},
    {"rts",  14, 0, 0,          0},
    {"stop", 15, 0, 0,          0}
};
static const size_t OPS_LEN = sizeof(OPS) / sizeof(OPS[0]);

/* Linear lookup is fine for 16 ops; replace with a small hash if you like. */
static const OpSpec* find_op(const char *mnemonic) {
    for (size_t i = 0; i < OPS_LEN; ++i) {
        if (strcmp(mnemonic, OPS[i].name) == 0) return &OPS[i];
    }
    return NULL;
}

/* ---------- public API ---------- */

/* Parse a legal symbol name from a string (skips leading spaces). */
bool parse_symbol_name(const char *s, char *out, size_t outsz) {
    s = skip_ws(s);
    const char *end;
    const char *w = read_word(s, &end);
    if (!w) return false;
    size_t n = (size_t)(end - w);
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
    p = skip_ws(p);

    /* We require at least one number. */
    bool need_number = true;
    while (*p) {
        p = skip_ws(p);
        if (*p == ';' || *p == '\0') break;

        if (!need_number) {
            errors_addf(errs, lineno, "expected comma between .data numbers");
            return -1;
        }

        long v;
        const char *e;
        if (!parse_int10(p, &e, &v)) {
            errors_addf(errs, lineno, "invalid number in .data");
            return -1;
        }
        codeimg_push_word(data, (int)v, lineno);
        count++;

        p = skip_ws(e);
        if (*p == ',') {
            p++;
            need_number = true;
            continue;
        }

        /* No comma => should be end-of-line/comment/space. */
        need_number = false;
        p = skip_ws(p);
        if (*p == ';' || *p == '\0') break;
        if (!is_sep((unsigned char)*p)) {
            errors_addf(errs, lineno, "unexpected text after .data list");
            return -1;
        }
        break;
    }

    if (count == 0) {
        errors_addf(errs, lineno, ".data requires at least one number");
        return -1;
    }
    return count;
}

/* Parse `.string "..."`, push chars as words plus a terminating 0.
   Supports \" and \\ escapes minimally. Returns words pushed or -1 on error. */
int parse_and_push_string(const char *s, CodeImg *data,
                          Errors *errs, int lineno) {
    const char *p = skip_ws(s);
    if (*p != '\"') {
        errors_addf(errs, lineno, ".string expects a quoted literal");
        return -1;
    }
    p++; /* after opening quote */

    int pushed = 0;
    while (*p && *p != '\"') {
        char c = *p++;
        if (c == '\\' && *p) { /* simple escape handling */
            char esc = *p++;
            if (esc == '\"') c = '\"';
            else if (esc == '\\') c = '\\';
            else c = esc; /* pass-through for other escapes */
        }
        codeimg_push_word(data, (unsigned char)c, lineno);
        pushed++;
    }
    if (*p != '\"') {
        errors_addf(errs, lineno, "missing closing quote in .string");
        return -1;
    }
    p++; /* after closing quote */

    /* NUL terminator */
    codeimg_push_word(data, 0, lineno);
    pushed++;

    if (!rest_is_comment_or_ws(p)) {
        errors_addf(errs, lineno, "unexpected text after .string");
        return -1;
    }
    return pushed;
}

/* Estimate number of words for an instruction line (pass 1).
   Validates addressing modes and applies the "two registers share one word"
   optimization. On success fills sz and returns true. */
bool encoding_estimate_size(const char *instr, EncodedInstrSize *sz,
                            Errors *errs, int lineno) {
    sz->words = 0;
    sz->operands = 0;

    char mnem[32];
    const char *p = read_mnemonic(instr, mnem, sizeof mnem);
    if (!p) {
        errors_addf(errs, lineno, "expected instruction mnemonic");
        return false;
    }
    const OpSpec *spec = find_op(mnem);
    if (!spec) {
        errors_addf(errs, lineno, "unknown mnemonic '%s'", mnem);
        return false;
    }

    AddrMode src = ADR_INVALID, dst = ADR_INVALID;
    char tmp_sym[64];
    const char *e = p;

    if (spec->argc == 2) {
        /* parse src */
        src = parse_operand(e, &e, tmp_sym, sizeof tmp_sym);
        if (src == ADR_INVALID) {
            errors_addf(errs, lineno, "invalid source operand");
            return false;
        }
        if (!parse_comma(&e)) {
            errors_addf(errs, lineno, "expected comma between operands");
            return false;
        }
        /* parse dst */
        dst = parse_operand(e, &e, tmp_sym, sizeof tmp_sym);
        if (dst == ADR_INVALID) {
            errors_addf(errs, lineno, "invalid destination operand");
            return false;
        }
        if (!(spec->src_ok & src)) {
            errors_addf(errs, lineno, "addressing mode not allowed for source");
            return false;
        }
        if (!(spec->dst_ok & dst)) {
            errors_addf(errs, lineno, "addressing mode not allowed for destination");
            return false;
        }
        if (!rest_is_comment_or_ws(e)) {
            errors_addf(errs, lineno, "unexpected text after instruction");
            return false;
        }
        sz->operands = 2;

        /* size: 1 base + operand words (two registers share one word) */
        int add = 0;
        if (src == ADR_REGISTER && dst == ADR_REGISTER) {
            add = 1; /* both encoded together */
        } else {
            if (src != ADR_INVALID) add += 1;
            if (dst != ADR_INVALID) add += 1;
        }
        sz->words = 1 + add;
        return true;
    }

    if (spec->argc == 1) {
        dst = parse_operand(e, &e, tmp_sym, sizeof tmp_sym);
        if (dst == ADR_INVALID) {
            errors_addf(errs, lineno, "invalid operand");
            return false;
        }
        if (!(spec->dst_ok & dst)) {
            errors_addf(errs, lineno, "addressing mode not allowed");
            return false;
        }
        if (!rest_is_comment_or_ws(e)) {
            errors_addf(errs, lineno, "unexpected text after instruction");
            return false;
        }
        sz->operands = 1;
        sz->words = 1 + 1; /* base + one operand word */
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

/* Lookup the numeric opcode (0..15) and argc for a mnemonic. */
bool encoding_lookup_opcode(const char *mnemonic, int *out_opcode, int *out_argc) {
    const OpSpec *s = find_op(mnemonic);
    if (!s) return false;
    if (out_opcode) *out_opcode = s->opcode;
    if (out_argc)   *out_argc   = s->argc;
    return true;
}
