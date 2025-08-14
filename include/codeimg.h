/*============================================================================
 *  codeimg.h
 *
 *  Code/Data image container used by both passes and the writers.
 *
 *  Concepts:
 *  - A CodeImg is an append-only vector of 10-bit "words" (stored as int),
 *    tagged with the source line for debug/error reporting.
 *  - Pass 1 appends DATA words; Pass 2 appends CODE words.
 *  - After Pass 1, data is relocated to follow code (ICF) via
 *    codeimg_relocate_data_after_code().
 *
 *  Read-only accessors are used by output.c to serialize .ob.
 *============================================================================*/

#ifndef CODEIMG_H
#define CODEIMG_H

#include <stddef.h>
#include "defaults.h"

typedef struct {
    int value;     /* 10-bit payload + A/R/E (stored as int)              */
    int src_line;  /* Source line number where this word originated        */
} CodeWord;

typedef struct CodeImg {
    CodeWord *words; /* dynamic array of words                             */
    size_t    len;   /* number of used elements                             */
    size_t    cap;   /* capacity of the array                               */
} CodeImg;

/* Lifecycle */
void   codeimg_init(CodeImg *img);                             /* zeroed vector */
void   codeimg_free(CodeImg *img);                             /* free storage  */

/* Mutation */
void   codeimg_push_word(CodeImg *img, int value, int lineno); /* append word   */
/* Relocate all DATA words so they begin right after CODE words (ICF). */
void   codeimg_relocate_data_after_code(CodeImg *code, CodeImg *data);

/* Introspection */
size_t codeimg_size_words(const CodeImg *img);                 /* total words   */

/* Read-only helpers (used by output.c) */
size_t codeimg_size(const CodeImg *img);                       /* == len        */
int    codeimg_at(const CodeImg *img, size_t index);           /* word value    */

#endif
