/* src/codeimg.c — minimal C90 implementation of the CodeImg runtime */
#include "codeimg.h"
#include <stdlib.h>
#include <string.h>

/* Ensure capacity for at least 'need' words */
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

void codeimg_push_word(CodeImg *img, int value, int lineno) {
    ensure_cap(img, img->len + 1);
    img->words[img->len].value = value;
    img->words[img->len].src_line = lineno;
    img->len++;
}

void codeimg_relocate_data_after_code(CodeImg *code, CodeImg *data) {
    size_t i;
    /* Append data image to the end of code image */
    for (i = 0; i < data->len; ++i) {
        codeimg_push_word(code, data->words[i].value, data->words[i].src_line);
    }
    /* Optional: clear data image to avoid double use */
    codeimg_free(data);
    codeimg_init(data);
}

size_t codeimg_size_words(const CodeImg *img) {
    return img->len;
}

size_t codeimg_size(const CodeImg *img) {
    return codeimg_size_words(img);
}

int codeimg_at(const CodeImg *img, size_t index) {
    /* No bounds check for speed; callers are trusted in this project */
    return img->words[index].value;
}