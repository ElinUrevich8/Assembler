/*============================================================================
 *  pass1.c
 *
 *  First pass:
 *    - Parse labels, directives, and instructions from the .am file.
 *    - Build the symbol table, generate the data image, and reserve code size
 *      by pushing placeholder words into the code image.
 *    - Validate names and enforce “single namespace with macros”.
 *    - Collect all diagnostics (do not stop on first error).
 *
 *  Output:
 *    Pass1Result with:
 *      symbols  — table of CODE/DATA/EXTERN/ENTRY with addresses
 *      code     — code image with placeholder words
 *      data     — data image (raw data words)
 *      ic, dc   — final counters
 *============================================================================*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pass1.h"
#include "encoding.h"   /* .data/.string/.mat parsers + size estimator */
#include "assembler.h"  /* for g_used_names */
#include "nameset.h"
#include "identifiers.h"

/* --- small local helpers -------------------------------- */

/*----------------------------------------------------------------------------
 * has_prefix(s, prefix): return non-zero if s starts with prefix.
 *----------------------------------------------------------------------------*/
static int has_prefix(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s++ != *prefix++) return 0;
    }
    return 1;
}

/*----------------------------------------------------------------------------
 * ltrim(p): skip leading whitespace.
 *----------------------------------------------------------------------------*/
static const char* ltrim(const char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/*----------------------------------------------------------------------------
 * empty_or_comment(p): true if line is empty or a pure comment (';').
 *----------------------------------------------------------------------------*/
static bool empty_or_comment(const char *p) {
    p = ltrim(p);
    return (*p == '\0' || *p == ';');
}

/*----------------------------------------------------------------------------
 * read_optional_label(p, out, cap, errs, lineno):
 *   Parse an optional "LABEL:" at the start of the line.
 *   Rules (per spec):
 *     - First char must be a letter [A-Za-z]
 *     - Following chars: letters or digits only (NO underscore)
 *     - Max length: MAX_LABEL_LEN
 *     - Reserved words are not allowed
 *   Returns pointer just after the colon if a label is present, the original
 *   'p' if none, or NULL on invalid label syntax (error recorded).
 *----------------------------------------------------------------------------*/
static const char* read_optional_label(const char *p, char *out, size_t cap,
                                       Errors *errs, int lineno) {
    const char *s = ltrim(p);
    const char *colon = strchr(s, ':');
    const char *q;
    size_t n;

    if (!colon) return p; /* no label candidate */

    /* First char must be a letter */
    if (!isalpha((unsigned char)*s)) {
        errors_addf(errs, lineno, "illegal label name '%.*s'", (int)(colon - s), s);
        return NULL;
    }

    /* Allow [A-Za-z][A-Za-z0-9]* only (no underscore) */
    q = s + 1;
    while (q < colon) {
        if (!isalnum((unsigned char)*q)) {
            errors_addf(errs, lineno, "illegal label name '%.*s'", (int)(colon - s), s);
            return NULL;
        }
        ++q;
    }

    /* Length limit */
    n = (size_t)(colon - s);
    if (n > MAX_LABEL_LEN) {
        errors_addf(errs, lineno, "illegal label name '%.*s' (too long)", (int)(colon - s), s);
        return NULL;
    }

    /* Copy to out now that we know it's syntactically fine */
    if (n >= cap) n = cap - 1; /* safety against tiny buffers */
    memcpy(out, s, n);
    out[n] = '\0';

    /* Reserved words not allowed */
    if (is_reserved_identifier(out)) {
        errors_addf(errs, lineno, "illegal label name '%s' (reserved)", out);
        return NULL;
    }

    return colon + 1;
}

/*----------------------------------------------------------------------------
 * push_placeholders(code, n, lineno):
 *   Append 'n' zero words into 'code' (to be filled during Pass 2).
 *----------------------------------------------------------------------------*/
static void push_placeholders(CodeImg *code, int n, int lineno) {
    int i;
    for (i = 0; i < n; ++i) {
        codeimg_push_word(code, 0, lineno);
    }
}

/*----------------------------------------------------------------------------
 * init_result(r): initialize a fresh Pass1Result structure.
 *----------------------------------------------------------------------------*/
static void init_result(Pass1Result *r) {
    memset(r, 0, sizeof(*r));
    r->symbols = symbols_new();
    codeimg_init(&r->code);
    codeimg_init(&r->data);
    r->ic = IC_INIT;  /* usually 100 */
    r->dc = 0;
    r->errors = errors_new();
    r->ok = true;
}

/* --- line handling ------------------------------------------------------ */

/*----------------------------------------------------------------------------
 * handle_directive(p, label, r, lineno):
 *   Process a directive line (.data/.string/.mat/.extern/.entry).
 *   For data-bearing directives, define 'label' (if present) as DATA.
 *----------------------------------------------------------------------------*/
static void handle_directive(const char *p, const char *label,
                             Pass1Result *r, int lineno) {
    /* .data */
    if (has_prefix(p, ".data")) {
        int pushed;
        if (label && *label) {
            /* enforce single namespace with macros */
            if (g_used_names && ns_contains(g_used_names, label)) {
                errors_addf(&r->errors, lineno,
                            "label '%s' conflicts with macro name", label);
                r->ok = false;
                return;
            }

            if (!symbols_define(r->symbols, label, r->dc, SYM_DATA, lineno, &r->errors))
                r->ok = false;
        }
        pushed = parse_and_push_data_operands(p + 5, &r->data, &r->errors, lineno);
        if (pushed < 0) { r->ok = false; return; }
        r->dc += pushed;
        return;
    }

    /* .string */
    if (has_prefix(p, ".string")) {
        int pushed;
        if (label && *label) {
            if (g_used_names && ns_contains(g_used_names, label)) {
                errors_addf(&r->errors, lineno,
                            "label '%s' conflicts with macro name", label);
                r->ok = false;
                return;
            }

            if (!symbols_define(r->symbols, label, r->dc, SYM_DATA, lineno, &r->errors))
                r->ok = false;
        }
        pushed = parse_and_push_string(p + 7, &r->data, &r->errors, lineno);
        if (pushed < 0) { r->ok = false; return; }
        r->dc += pushed;
        return;
    }

    /* .mat */
    if (has_prefix(p, ".mat")) {
        int pushed;
        if (label && *label) {
            if (g_used_names && ns_contains(g_used_names, label)) {
                errors_addf(&r->errors, lineno,
                            "label '%s' conflicts with macro name", label);
                r->ok = false;
                return;
            }

            if (!symbols_define(r->symbols, label, r->dc, SYM_DATA, lineno, &r->errors))
                r->ok = false;
        }
        /* Expected: .mat [rows][cols] <optional comma-separated initializers> */
        pushed = parse_and_push_mat(p + 4, &r->data, &r->errors, lineno);
        if (pushed < 0) { r->ok = false; return; }
        r->dc += pushed; /* rows*cols returned by parser */
        return;
    }

    /* .extern */
    if (has_prefix(p, ".extern")) {
        char name[128];

        /* Spec: a label before .extern is meaningless -> ignore (no error) */
        (void)label;

        if (!parse_symbol_name(p + 7, name, sizeof name)) {
            errors_addf(&r->errors, lineno, "expected symbol after .extern");
            r->ok = false;
            return;
        }
        if (!symbols_define(r->symbols, name, 0, SYM_EXTERN, lineno, &r->errors))
            r->ok = false;
        return;
    }

    /* .entry */
    if (has_prefix(p, ".entry")) {
        char name[128];

        /* Spec: a label before .entry is meaningless -> ignore (no error) */
        (void)label;

        if (!parse_symbol_name(p + 6, name, sizeof name)) {
            errors_addf(&r->errors, lineno, "expected symbol after .entry");
            r->ok = false;
            return;
        }
        if (!symbols_mark_entry(r->symbols, name, lineno, &r->errors))
            r->ok = false;
        return;
    }

    errors_addf(&r->errors, lineno, "unknown directive");
    r->ok = false;
}

/*----------------------------------------------------------------------------
 * handle_instruction(p, label, r, lineno):
 *   Parse an instruction line, define label (if present) as CODE, estimate
 *   its size, and push placeholder words into the code image.
 *----------------------------------------------------------------------------*/
static void handle_instruction(const char *p, const char *label,
                               Pass1Result *r, int lineno) {
    EncodedInstrSize sz;

    if (label && *label) {
        /* enforce single namespace with macros */
        if (g_used_names && ns_contains(g_used_names, label)) {
            errors_addf(&r->errors, lineno,
                        "label '%s' conflicts with macro name", label);
            r->ok = false;
            return;
        }
        if (!symbols_define(r->symbols, label, r->ic, SYM_CODE, lineno, &r->errors)) {
            r->ok = false;
            return;
        }
    }

    if (!encoding_estimate_size(p, &sz, &r->errors, lineno)) {
        r->ok = false;
        return;
    }

    /* Reserve words in code image (placeholders to be filled in pass 2). */
    push_placeholders(&r->code, sz.words, lineno);
    r->ic += sz.words;
}

/*----------------------------------------------------------------------------
 * handle_line(line, r, lineno):
 *   Single-line handler: optional label, then directive or instruction.
 *----------------------------------------------------------------------------*/
static void handle_line(char *line, Pass1Result *r, int lineno) {
    const char *p;
    char label[128];
    const char *after_label;

    p = line;
    if (empty_or_comment(p)) return;

    label[0] = '\0';
    after_label = read_optional_label(p, label, sizeof(label), &r->errors, lineno);
    if (!after_label) { r->ok = false; return; }

    p = ltrim(after_label);

    if (*p == '\0' || *p == ';') {
        /* A naked label with nothing after it is not allowed in this spec. */
        if (label[0]) {
            errors_addf(&r->errors, lineno, "label without statement");
            r->ok = false;
        }
        return;
    }

    if (*p == '.') {
        handle_directive(p, label[0] ? label : NULL, r, lineno);
    } else {
        handle_instruction(p, label[0] ? label : NULL, r, lineno);
    }
}

/* --- public API --------------------------------------------------------- */

/*----------------------------------------------------------------------------
 * pass1_run(am_path, out):
 *   Run the first pass: build symbols, data image, and reserve code size.
 *   Returns true if the file was processed (even if r->ok==false due to errors).
 *----------------------------------------------------------------------------*/
bool pass1_run(const char *am_path, Pass1Result *out) {
    FILE *fp;
    char buf[4096];
    int lineno;

    init_result(out);

    fp = fopen(am_path, "r");
    if (!fp) {
        errors_addf(&out->errors, 0, "cannot open %s", am_path);
        out->ok = false;
        return false;
    }

    lineno = 0;
    while (fgets(buf, sizeof buf, fp)) {
        lineno++;
        strip_comment_inplace(buf);
        handle_line(buf, out, lineno);
    }
    fclose(fp);

    /* Finalize: relocate DATA symbols and (optionally) append data after code. */
    symbols_relocate_data(out->symbols, out->ic);
    codeimg_relocate_data_after_code(&out->code, &out->data);

    return true;
}

/*----------------------------------------------------------------------------
 * pass1_free(r): release memory owned by Pass1Result.
 *----------------------------------------------------------------------------*/
void pass1_free(Pass1Result *r) {
    if (!r) return;
    symbols_free(r->symbols);
    codeimg_free(&r->code);
    codeimg_free(&r->data);
    errors_free(&r->errors);
    memset(r, 0, sizeof(*r));
}
