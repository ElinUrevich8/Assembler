/*============================================================================
 *  errors.c
 *
 *  Growable list of { line, message } used across stages to collect and print
 *  diagnostics without aborting early (per project spec).
 *
 *  Usage:
 *    Errors e = errors_new();
 *    errors_addf(&e, lineno, "bad thing: %s", detail);
 *    if (errors_count(&e)) errors_print(&e, filename);
 *    errors_free(&e);
 *============================================================================*/

#include "errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
    /* va_copy fallback for strict C90 compilers */
    #ifndef va_copy
        #define va_copy(dest, src) memcpy(&(dest), &(src), sizeof(va_list))
    #endif
    int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif

/*----------------------------------------------------------------------------
 * ensure_cap(e, need):
 *   Make sure the internal array can hold at least 'need' items.
 *----------------------------------------------------------------------------*/
static void ensure_cap(Errors *e, size_t need) {
    size_t ncap;
    if (e->cap >= need) return;
    ncap = e->cap ? e->cap * 2 : 8;
    while (ncap < need) ncap *= 2;
    e->items = (ErrorItem *)realloc(e->items, ncap * sizeof(ErrorItem));
    if (!e->items) {
        /* If OOM, print and abort: this is a debugging aid; normal runs are small. */
        fprintf(stderr, "errors: out of memory\n");
        abort();
    }
    e->cap = ncap;
}

/*----------------------------------------------------------------------------
 * errors_new():
 *   Return a zero-initialized Errors aggregator.
 *----------------------------------------------------------------------------*/
Errors errors_new(void) {
    Errors e;
    memset(&e, 0, sizeof e);
    return e;
}

/*----------------------------------------------------------------------------
 * errors_free(e):
 *   Free all messages and reset to empty.
 *----------------------------------------------------------------------------*/
void errors_free(Errors *e) {
    size_t i;
    if (!e) return;
    for (i = 0; i < e->len; ++i) {
        free(e->items[i].msg);
    }
    free(e->items);
    e->items = NULL;
    e->len = e->cap = 0;
}

/*----------------------------------------------------------------------------
 * errors_addf(e, line, fmt, ...):
 *   printf-like append; stores a heap copy of the formatted message.
 *----------------------------------------------------------------------------*/
void errors_addf(Errors *e, int line, const char *fmt, ...) {
    va_list ap, ap2;
    int need;
    char *buf;

    va_start(ap, fmt);
    va_copy(ap2, ap);
    need = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);
    if (need < 0) { va_end(ap); return; }

    buf = (char *)malloc((size_t)need + 1);
    if (!buf) { va_end(ap); return; }
    (void)vsnprintf(buf, (size_t)need + 1, fmt, ap);
    va_end(ap);

    ensure_cap(e, e->len + 1);
    e->items[e->len].line = line;
    e->items[e->len].msg  = buf;
    e->len++;
}

/*----------------------------------------------------------------------------
 * errors_merge(dst, src):
 *   Append items from 'src' to 'dst' (deep-copy messages).
 *----------------------------------------------------------------------------*/
void errors_merge(Errors *dst, const Errors *src) {
    size_t i, n;
    char *copy;
    if (!src || src->len == 0) return;
    ensure_cap(dst, dst->len + src->len);
    for (i = 0; i < src->len; ++i) {
        n = strlen(src->items[i].msg);
        copy = (char *)malloc(n + 1);
        if (!copy) continue;
        memcpy(copy, src->items[i].msg, n + 1);
        dst->items[dst->len].line = src->items[i].line;
        dst->items[dst->len].msg  = copy;
        dst->len++;
    }
}

/*----------------------------------------------------------------------------
 * errors_print(e, filename):
 *   Print all collected diagnostics to stderr.
 *----------------------------------------------------------------------------*/
void errors_print(const Errors *e, const char *filename) {
    size_t i;
    if (!e) return;
    for (i = 0; i < e->len; ++i) {
        if (filename && filename[0])
            fprintf(stderr, "%s:%d: %s\n", filename, e->items[i].line, e->items[i].msg);
        else
            fprintf(stderr, "%d: %s\n", e->items[i].line, e->items[i].msg);
    }
}

/*----------------------------------------------------------------------------
 * errors_count(e):
 *   Return the number of accumulated messages.
 *----------------------------------------------------------------------------*/
int errors_count(const Errors *e) {
    return e ? (int)e->len : 0;
}
