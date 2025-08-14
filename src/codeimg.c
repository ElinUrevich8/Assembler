/*============================================================================
 *  codeimg.c
 *
 *  Append-only container for 10-bit words with source-line tags.
 *  Used by Pass 1 (data image), Pass 2 (final code), and output.c (.ob).
 *============================================================================*/
#include "codeimg.h"
#include <stdlib.h>
#include <string.h>

/* Geometric growth: double capacity until it's >= need */
static void ensure_cap(CodeImg *img, size_t need) {
    size_t ncap;
    if (img->cap >= need) return;
    ncap = (img->cap ? img->cap * 2 : 32);
    while (ncap < need) ncap *= 2;
    img->words = (CodeWord *)realloc(img->words, ncap * sizeof(CodeWord));
    img->cap = ncap;
}

void codeimg_init(CodeImg *img) {
    img->words = NULL;
    img->len = 0;
    img->cap = 0;
}

void codeimg_free(CodeImg *img) {
    free(img->words);
    img->words = NULL;
    img->len = 0;
    img->cap = 0;
}

/* Append a word + remember source line for diagnostics */
void codeimg_push_word(CodeImg *img, int value, int lineno) {
    ensure_cap(img, img->len + 1);
    img->words[img->len].value = value;
    img->words[img->len].src_line = lineno;
    img->len++;
}

/* Append data after code (ICF), then reset 'data' to empty */
void codeimg_relocate_data_after_code(CodeImg *code, CodeImg *data) {
    size_t i;
    for (i = 0; i < data->len; ++i) {
        codeimg_push_word(code, data->words[i].value, data->words[i].src_line);
    }
    codeimg_free(data);
    codeimg_init(data);
}

/* Read-only helpers */
size_t codeimg_size_words(const CodeImg *img) { return img->len; }
size_t codeimg_size(const CodeImg *img)       { return codeimg_size_words(img); }

/* Note: callers ensure bounds in this project */
int codeimg_at(const CodeImg *img, size_t index) { return img->words[index].value; }
