/*============================================================================
 *  codeimg.c
 *
 *  Append-only container for 10-bit words with source-line tags.
 *  Used by:
 *    - Pass 1: building the data image and reserving code placeholders
 *    - Pass 2: emitting the final code image
 *    - output.c: writing .ob/.ent/.ext
 *
 *  Notes:
 *    - This module intentionally stays minimal: no dynamic "remove" operations.
 *    - OOM handling: ensure_cap() fails fast (prints to stderr and aborts),
 *      because callers assume pushes succeed when the process has memory.
 *============================================================================*/

#include "codeimg.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>  /* fprintf, stderr */

/*----------------------------------------------------------------------------
 * ensure_cap(img, need):
 *   Make sure 'img->words' can hold at least 'need' CodeWord items.
 *   Grows capacity geometrically (x2) to amortize realloc cost.
 *
 * Params:
 *   img  - target image to grow
 *   need - required number of elements (len <= need <= cap after call)
 *
 * Errors:
 *   If allocation fails, prints an OOM message and aborts the process.
 *----------------------------------------------------------------------------*/
static void ensure_cap(CodeImg *img, size_t need) {
    size_t ncap;
    CodeWord *nw;

    if (img->cap >= need) return;

    ncap = (img->cap ? img->cap * 2 : 32);
    while (ncap < need) ncap *= 2;

    nw = (CodeWord *)realloc(img->words, ncap * sizeof(CodeWord));
    if (!nw) {
        fprintf(stderr, "codeimg: out of memory (realloc %lu items)\n",
                (unsigned long)ncap);
        abort();
    }
    img->words = nw;
    img->cap = ncap;
}

/*----------------------------------------------------------------------------
 * codeimg_init(img):
 *   Initialize an empty image (no allocation yet).
 *----------------------------------------------------------------------------*/
void codeimg_init(CodeImg *img) {
    img->words = NULL;
    img->len   = 0;
    img->cap   = 0;
}

/*----------------------------------------------------------------------------
 * codeimg_free(img):
 *   Release all memory owned by 'img' and reset to empty state.
 *----------------------------------------------------------------------------*/
void codeimg_free(CodeImg *img) {
    free(img->words);
    img->words = NULL;
    img->len   = 0;
    img->cap   = 0;
}

/*----------------------------------------------------------------------------
 * codeimg_push_word(img, value, lineno):
 *   Append one word with its source line number (for diagnostics).
 *
 * Params:
 *   value  - encoded 10-bit word (callers may store int; writers mask)
 *   lineno - original source line where this word originated
 *----------------------------------------------------------------------------*/
void codeimg_push_word(CodeImg *img, int value, int lineno) {
    ensure_cap(img, img->len + 1);
    img->words[img->len].value    = value;
    img->words[img->len].src_line = lineno;
    img->len++;
}

/*----------------------------------------------------------------------------
 * codeimg_relocate_data_after_code(code, data):
 *   Append all 'data' words after the end of 'code', then reset 'data' to
 *   an empty image. Used at the end of Pass 1.
 *----------------------------------------------------------------------------*/
void codeimg_relocate_data_after_code(CodeImg *code, CodeImg *data) {
    size_t i;
    for (i = 0; i < data->len; ++i) {
        codeimg_push_word(code, data->words[i].value, data->words[i].src_line);
    }
    codeimg_free(data);
    codeimg_init(data);
}

/*----------------------------------------------------------------------------
 * Read-only helpers
 *----------------------------------------------------------------------------*/
size_t codeimg_size_words(const CodeImg *img) { return img->len; }
size_t codeimg_size(const CodeImg *img)       { return codeimg_size_words(img); }

/*----------------------------------------------------------------------------
 * codeimg_at(img, index):
 *   Return the stored word at 'index'. Callers ensure bounds.
 *----------------------------------------------------------------------------*/
int codeimg_at(const CodeImg *img, size_t index) { return img->words[index].value; }
