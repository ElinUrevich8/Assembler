#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pass1.h"
#include "encoding.h"   /* parse .data/.string + estimate instruction size */

/* --- small local helpers ------------------------------------------------ */

/* Trim leading whitespace; return pointer to first non-space. */
static const char* ltrim(const char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/* Return true if line is empty or a pure comment (';'). */
static bool empty_or_comment(const char *p) {
    p = ltrim(p);
    return (*p == '\0' || *p == ';');
}

/* Read optional "LABEL:" at start of line into out (if present).
   Returns pointer after the label and colon, or original p if none.
   On invalid label syntax, writes error and returns NULL. */
static const char* read_optional_label(const char *p, char *out, size_t cap,
                                       Errors *errs, int lineno) {
    const char *s = ltrim(p);
    const char *colon = strchr(s, ':');
    if (!colon) return p; /* no label */

    /* Make sure chars before ':' form a valid symbol. */
    char name[128];
    const char *after_id;
    if (!isalpha((unsigned char)*s) && *s != '_') return p; /* not a label */
    after_id = s + 1;
    while (isalnum((unsigned char)*after_id) || *after_id == '_') after_id++;

    /* If the ':' we saw isn’t right after the identifier, then it’s not a label. */
    if (after_id != colon) return p;

    size_t n = (size_t)(colon - s);
    if (n >= cap) n = cap - 1;
    memcpy(out, s, n);
    out[n] = '\0';
    return colon + 1;
}

/* Push N placeholder words into code image (for size reservation). */
static void push_placeholders(CodeImg *code, int n, int lineno) {
    for (int i = 0; i < n; ++i) {
        codeimg_push_word(code, 0, lineno);
    }
}

/* Initialize Pass1Result with fresh containers and counters. */
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

static void handle_directive(const char *p, const char *label,
                             Pass1Result *r, int lineno) {
    /* .data */
    if (starts_with(p, ".data")) {
        if (label && *label) {
            if (!symbols_define(r->symbols, label, r->dc, SYM_DATA, lineno, &r->errors))
                r->ok = false;
        }
        int pushed = parse_and_push_data_operands(p + 5, &r->data, &r->errors, lineno);
        if (pushed < 0) { r->ok = false; return; }
        r->dc += pushed;
        return;
    }

    /* .string */
    if (starts_with(p, ".string")) {
        if (label && *label) {
            if (!symbols_define(r->symbols, label, r->dc, SYM_DATA, lineno, &r->errors))
                r->ok = false;
        }
        int pushed = parse_and_push_string(p + 7, &r->data, &r->errors, lineno);
        if (pushed < 0) { r->ok = false; return; }
        r->dc += pushed;
        return;
    }

    /* .extern */
    if (starts_with(p, ".extern")) {
        if (label && *label) {
            errors_addf(&r->errors, lineno, "label before .extern is illegal");
            r->ok = false;
        }
        char name[128];
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
    if (starts_with(p, ".entry")) {
        if (label && *label) {
            errors_addf(&r->errors, lineno, "label before .entry is illegal");
            r->ok = false;
        }
        char name[128];
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

/* Parse and account for an instruction (no symbol resolution here). */
static void handle_instruction(const char *p, const char *label,
                               Pass1Result *r, int lineno) {
    if (label && *label) {
        if (!symbols_define(r->symbols, label, r->ic, SYM_CODE, lineno, &r->errors)) {
            r->ok = false;
            return;
        }
    }

    EncodedInstrSize sz;
    if (!encoding_estimate_size(p, &sz, &r->errors, lineno)) {
        r->ok = false;
        return;
    }

    /* Reserve words in code image (placeholders to be filled in pass 2). */
    push_placeholders(&r->code, sz.words, lineno);
    r->ic += sz.words;
}

/* Process one source line from the .am file. */
static void handle_line(char *line, Pass1Result *r, int lineno) {
    const char *p = line;
    if (empty_or_comment(p)) return;

    char label[128] = {0};
    const char *after_label = read_optional_label(p, label, sizeof(label), &r->errors, lineno);
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

/* Run the first pass: build symbols, data image, and reserve code size. */
bool pass1_run(const char *am_path, Pass1Result *out) {
    init_result(out);

    FILE *fp = fopen(am_path, "r");
    if (!fp) {
        errors_addf(&out->errors, 0, "cannot open %s", am_path);
        out->ok = false;
        return false;
    }

    char buf[4096];
    int lineno = 0;
    while (fgets(buf, sizeof buf, fp)) {
        lineno++;
        handle_line(buf, out, lineno);
    }
    fclose(fp);

    /* Finalize: relocate DATA symbols and (optionally) append data after code. */
    symbols_relocate_data(out->symbols, out->ic);
    codeimg_relocate_data_after_code(&out->code, &out->data);

    return true;
}

/* Release memory owned by Pass1Result. */
void pass1_free(Pass1Result *r) {
    if (!r) return;
    symbols_free(r->symbols);
    codeimg_free(&r->code);
    codeimg_free(&r->data);
    errors_free(&r->errors);
    memset(r, 0, sizeof(*r));
}
