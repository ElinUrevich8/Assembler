#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static void ensure_cap(Errors *e, size_t need) {
    if (e->cap >= need) return;
    size_t ncap = e->cap ? e->cap * 2 : 8;
    while (ncap < need) ncap *= 2;
    e->items = (ErrorItem *)realloc(e->items, ncap * sizeof(ErrorItem));
    e->cap = ncap;
}

Errors errors_new(void) {
    Errors e = {0};
    return e;
}

void errors_free(Errors *e) {
    if (!e) return;
    for (size_t i = 0; i < e->len; ++i) {
        free(e->items[i].msg);
    }
    free(e->items);
    e->items = NULL;
    e->len = e->cap = 0;
}

void errors_addf(Errors *e, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    va_list ap2;
    va_copy(ap2, ap);

    int need = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    if (need < 0) { va_end(ap); return; }

    char *buf = (char *)malloc((size_t)need + 1);
    vsnprintf(buf, (size_t)need + 1, fmt, ap);
    va_end(ap);

    ensure_cap(e, e->len + 1);
    e->items[e->len].line = line;
    e->items[e->len].msg  = buf;
    e->len++;
}

void errors_merge(Errors *dst, const Errors *src) {
    if (!src || src->len == 0) return;
    ensure_cap(dst, dst->len + src->len);
    for (size_t i = 0; i < src->len; ++i) {
        /* duplicate the message string so both remain independent */
        size_t n = strlen(src->items[i].msg);
        char *copy = (char *)malloc(n + 1);
        memcpy(copy, src->items[i].msg, n + 1);
        dst->items[dst->len].line = src->items[i].line;
        dst->items[dst->len].msg  = copy;
        dst->len++;
    }
}

void errors_print(const Errors *e, const char *filename) {
    if (!e) return;
    for (size_t i = 0; i < e->len; ++i) {
        if (filename && filename[0])
            fprintf(stderr, "%s:%d: %s\n", filename, e->items[i].line, e->items[i].msg);
        else
            fprintf(stderr, "%d: %s\n", e->items[i].line, e->items[i].msg);
    }
}
