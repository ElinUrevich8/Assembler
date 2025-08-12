/* output.c
 * Emits .ob (always when assembly succeeds), and conditionally .ent/.ext.
 * This module is deliberately dumb: it trusts Pass 1/2 to have validated data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "output.h"
#include "defaults.h"
#include "codeimg.h"
#include "debug.h"

/* ---------- local helpers: unique base-4 formatting ---------- */

/* Convert a non-negative integer 'v' to the course's "unique base-4" string.
 * Adjust implementation to your exact alphabet/spec if needed.
 * This returns a static buffer (thread-unsafe; fine for this project).
 */
static const char *fmt_base4(unsigned int v) {
    /* Example alphabet: 0 1 2 3
     * If your spec uses a custom alphabet (e.g., '.,', etc.), swap here.
     */
    static char buf[64];
    char tmp[64];
    size_t n = 0, i;
    if (v == 0) {
        buf[0] = '0'; buf[1] = '\0';
        return buf;
    }
    while (v > 0 && n + 1 < sizeof(tmp)) {
        unsigned int d = v & 3u; /* v % 4 */
        tmp[n++] = (char)('0' + d);
        v >>= 2;                 /* v /= 4 */
    }
    /* reverse into buf */
    for (i = 0; i < n; ++i) buf[i] = tmp[n - 1 - i];
    buf[n] = '\0';
    return buf;
}

/* Convert a signed word to base-4 per your object format.
 * If you store words as unsigned internally, adapt as necessary.
 */
static const char *fmt_word_base4(unsigned int w) {
    /* If your word is 10-bit, mask here so printing never leaks upper bits. */
    return fmt_base4(w & 0x3FFu);
}

/* ---------- writers ---------- */

int output_write_ob(FILE *fp, const Pass1Result *p1, const Pass2Result *p2) {
    unsigned int addr;
    size_t i, n;

    if (!fp || !p1 || !p2) return 0;

    /* Header: code_len data_len (base-4) */
    fprintf(fp, "%s %s\n",
            fmt_base4((unsigned int)p2->code_len),
            fmt_base4((unsigned int)p2->data_len));

    /* Code region: final code from pass2 */
    addr = (unsigned int)IC_INIT;
    n = codeimg_size(&p2->code_final);
    for (i = 0; i < n; ++i, ++addr) {
        unsigned int w = (unsigned int)codeimg_at(&p2->code_final, i);
        fprintf(fp, "%s %s\n", fmt_base4(addr), fmt_word_base4(w));
    }

    /* Data region: appended after code per pass1 relocation.
     * Pass 1 already concatenated data after code inside its image.
     * You can either:
     *   - read the tail DC words out of p1->code image,
     *   - or keep a separate data image in pass1 (if you have one).
     * Below assumes p1->code holds [code placeholders] + [data], as in your pass1.c.
     */

    addr = (unsigned int)p1->ICF; /* first data address */
    /* Pull last DC words from the pass1 image. */
    {
        size_t total = codeimg_size(&p1->code);
        size_t dc    = p1->data_len;
        size_t start = total - dc;
        for (i = 0; i < dc; ++i, ++addr) {
            unsigned int w = (unsigned int)codeimg_at(&p1->code, start + i);
            fprintf(fp, "%s %s\n", fmt_base4(addr), fmt_word_base4(w));
        }
    }

    return 1;
}

int output_write_ent(FILE *fp, const Pass2Result *p2) {
    size_t i;
    if (!fp || !p2) return 0;
    if (p2->ent_len == 0) return 1; /* nothing to write; DO NOT create file upstream */

    for (i = 0; i < p2->ent_len; ++i) {
        const EntryOut *e = &p2->ent[i];
        fprintf(fp, "%s %s\n", e->name, fmt_base4((unsigned int)e->addr));
    }
    return 1;
}

int output_write_ext(FILE *fp, const Pass2Result *p2) {
    size_t i;
    if (!fp || !p2) return 0;
    if (p2->ext_len == 0) return 1; /* nothing to write; DO NOT create file upstream */

    for (i = 0; i < p2->ext_len; ++i) {
        const ExtUse *u = &p2->ext[i];
        fprintf(fp, "%s %s\n", u->name, fmt_base4((unsigned int)u->addr));
    }
    return 1;
}
