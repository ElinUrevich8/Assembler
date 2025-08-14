/*============================================================================
 *  errors.h
 *
 *  Error aggregation for multi-stage assembly.
 *
 *  Usage:
 *    Errors e = errors_new();
 *    errors_addf(&e, lineno, "bad thing: %s", detail);
 *    if (errors_count(&e)) errors_print(&e, filename);
 *    errors_free(&e);
 *============================================================================*/

#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>

typedef struct {
    int   line;   /* Source line (0 if unknown) */
    char *msg;    /* Heap-allocated error text   */
} ErrorItem;

typedef struct {
    ErrorItem *items;
    size_t     len;
    size_t     cap;
} Errors;

/* Lifecycle */
Errors errors_new(void);
void   errors_free(Errors *e);

/* Mutation */
void   errors_addf(Errors *e, int line, const char *fmt, ...);
void   errors_merge(Errors *dst, const Errors *src);

/* Reporting */
void   errors_print(const Errors *e, const char *filename);

/* Introspection */
int    errors_count(const Errors *errs);

#endif /* ERRORS_H */
