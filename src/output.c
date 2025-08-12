/* src/output.c — emit .ob, .ent, .ext in a/b/c/d base-4 format (C90) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "output.h"
#include "defaults.h"
#include "codeimg.h"

/* -------------------- Base-4 helpers (a/b/c/d) -------------------- */

static char base4_digit(unsigned d)
{
    static const char map[4] = { BASE4_CHAR_0, BASE4_CHAR_1, BASE4_CHAR_2, BASE4_CHAR_3 };
    return map[d & 3u];
}

/* Write trimmed base-4 value into out (NUL-terminated). */
static void b4_write_trim(unsigned int v, char *out, size_t cap)
{
    char tmp[64];
    size_t n = 0, i;

    if (cap == 0) return;
    if (v == 0u) { out[0] = base4_digit(0u); out[1 < cap ? 1 : 0] = '\0'; return; }

    while (v > 0u && n < sizeof tmp) {
        tmp[n++] = base4_digit(v & 3u);
        v >>= 2;
    }
    if (n + 1 > cap) n = cap - 1;           /* truncate if needed */
    for (i = 0; i < n; ++i) out[i] = tmp[n - 1 - i];
    out[n] = '\0';
}

/* Write padded base-4 value (fixed width) into out. */
static void b4_write_pad(unsigned int v, unsigned width, char pad_char,
                         char *out, size_t cap)
{
    char tmp[64];
    size_t n = 0, i, need, pad;

    if (cap == 0) return;
    if (width >= cap) width = (unsigned)cap - 1; /* keep room for NUL */

    if (v == 0u) {
        for (i = 0; i < width; ++i) out[i] = pad_char;
        out[width] = '\0';
        return;
    }

    while (v > 0u && n < sizeof tmp) {
        tmp[n++] = base4_digit(v & 3u);
        v >>= 2;
    }

    need = (width > 0u) ? width : n;
    if (need + 1 > cap) need = cap - 1;

    pad  = (need > n) ? (need - n) : 0u;
    for (i = 0; i < pad; ++i) out[i] = pad_char;
    for (i = 0; i < n && (pad + i) < need; ++i) out[pad + i] = tmp[n - 1 - i];
    out[need] = '\0';
}

/* 10-bit word → fixed 5 base-4 chars (a=0). */
static void b4_write_word10(unsigned int w10, char *out, size_t cap)
{
    b4_write_pad(w10 & 0x3FFu, BASE4_WORD_STRLEN, base4_digit(0u), out, cap);
}

/* Address: optional fixed width (OB_ADDR_PAD_WIDTH). */
static void b4_write_addr(unsigned int addr, char *out, size_t cap)
{
#if OB_ADDR_PAD_WIDTH > 0
    b4_write_pad(addr, OB_ADDR_PAD_WIDTH, base4_digit(0u), out, cap);
#else
    b4_write_trim(addr, out, cap);
#endif
}

/* --------------------------- Writers --------------------------- */

int output_write_ob(FILE *fp, const Pass1Result *p1, const Pass2Result *p2)
{
    unsigned int addr;
    size_t i, n;
    char s_code[32], s_data[32];

    if (!fp || !p1 || !p2) return 0;

    /* Header: <code_len> <data_len> in base-4 (use separate buffers) */
    b4_write_trim((unsigned)p2->code_len, s_code, sizeof s_code);
    b4_write_trim((unsigned)p2->data_len, s_data, sizeof s_data);
    fprintf(fp, "%s %s\n", s_code, s_data);

    /* Code image (final) from IC_INIT upward */
    addr = (unsigned)IC_INIT;
    n = codeimg_size(&p2->code_final);
    for (i = 0; i < n; ++i, ++addr) {
        unsigned int w = (unsigned)codeimg_at(&p2->code_final, i);
        char a[32], ws[32];
        b4_write_addr(addr, a, sizeof a);
        b4_write_word10(w, ws, sizeof ws);
        fprintf(fp, "%s %s\n", a, ws);
    }

    /* Data: last DC words of p1->code, starting at p1->ic */
    addr = (unsigned)p1->ic;
    {
        size_t total = codeimg_size(&p1->code);
        size_t dc    = (size_t)p1->dc;
        size_t start = total - dc;

        for (i = 0; i < dc; ++i, ++addr) {
            unsigned int w = (unsigned)codeimg_at(&p1->code, start + i);
            char a[32], ws[32];
            b4_write_addr(addr, a, sizeof a);
            b4_write_word10(w, ws, sizeof ws);
            fprintf(fp, "%s %s\n", a, ws);
        }
    }

    return 1;
}

int output_write_ent(FILE *fp, const Pass2Result *p2)
{
    size_t i;
    if (!fp || !p2) return 0;
    for (i = 0; i < p2->ent_len; ++i) {
        const EntryOut *e = &p2->ent[i];
        char a[32];
        b4_write_addr((unsigned)e->addr, a, sizeof a);
        fprintf(fp, "%s %s\n", e->name, a);
    }
    return 1;
}

int output_write_ext(FILE *fp, const Pass2Result *p2)
{
    size_t i;
    if (!fp || !p2) return 0;
    for (i = 0; i < p2->ext_len; ++i) {
        const ExtUse *u = &p2->ext[i];
        char a[32];
        b4_write_addr((unsigned)u->addr, a, sizeof a);
        fprintf(fp, "%s %s\n", u->name, a);
    }
    return 1;
}
