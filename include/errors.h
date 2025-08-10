#ifndef ERRORS_H
#define ERRORS_H

#include <stddef.h>

/* Single collected error message. */
typedef struct {
    int   line;   /* source line (0 if unknown) */
    char *msg;    /* heap-allocated message */
} ErrorItem;

/* Growable list of errors. */
typedef struct {
    ErrorItem *items;
    size_t     len;
    size_t     cap;
} Errors;

/* Create an empty error list. */
Errors errors_new(void);

/* Free all memory inside the list (messages + array). */
void errors_free(Errors *e);

/* Append a formatted error (like printf). */
void errors_addf(Errors *e, int line, const char *fmt, ...);

/* Append all errors from 'src' into 'dst' (src remains valid). */
void errors_merge(Errors *dst, const Errors *src);

/* Print errors to stderr as: "filename:line: message". */
void errors_print(const Errors *e, const char *filename);

#endif /* ERRORS_H */
