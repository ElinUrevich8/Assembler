#ifndef CODEIMG_H
#define CODEIMG_H

#include <stddef.h>
#include "defaults.h"

typedef struct {
    int value;
    int src_line;
} CodeWord;

typedef struct CodeImg {
    CodeWord *words;
    size_t    len;
    size_t    cap;
} CodeImg;

/* API */
void   codeimg_init(CodeImg *img);
void   codeimg_free(CodeImg *img);
void   codeimg_push_word(CodeImg *img, int value, int lineno);
void   codeimg_relocate_data_after_code(CodeImg *code, CodeImg *data);
size_t codeimg_size_words(const CodeImg *img);

#endif
