/*============================================================================
 *  hash_core.h
 *
 *  Low-level, generic chained hash table.
 *  Stores (key → value) where both are `void*`.
 *  Never included directly by user code; only by thin wrappers
 *  such as nameset.c or macro.c.
 *============================================================================*/

#ifndef HASH_CORE_H
#define HASH_CORE_H

#include <stddef.h>   /* size_t */


#define HASH_SIZE 113           /* must match buckets[] length   */

/*-----------  Node & table definitions  -----------------------------------*/
struct HCNode {
    char           *key;      /* heap-allocated copy of the key string */
    void           *val;      /* caller-supplied payload                */
    struct HCNode  *next;     /* next node in bucket chain              */
};

typedef struct {
    struct HCNode *buckets[HASH_SIZE]; /* prime bucket count gives better spread */
} HashCore;

/*-----------  API  --------------------------------------------------------*/
/* Initialise / destroy */
void  hc_init(HashCore *hc);
void  hc_free(HashCore *hc, void (*free_val)(void*));

/* Insert – return 1 on success, 0 if key already exists or OOM            */
int   hc_insert(HashCore *hc, const char *key, void *val);

/* Lookup – NULL if not found                                              */
void *hc_find  (const HashCore *hc, const char *key);

#endif /* HASH_CORE_H */
