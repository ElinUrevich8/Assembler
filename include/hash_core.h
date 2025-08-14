/*============================================================================
 *  hash_core.h
 *
 *  Generic chained hash table: string key -> void* value.
 *  Internal helper used by higher-level modules (e.g., nameset, macro).
 *
 *  Notes:
 *   - Collisions are handled via forward-linked lists per bucket.
 *   - Caller owns the value memory; hc_free() accepts an optional
 *     destructor for values.
 *============================================================================*/

#ifndef HASH_CORE_H
#define HASH_CORE_H

#include <stddef.h>   /* size_t */

#define HASH_SIZE 113           /* Prime bucket count for better spread */

/* Node stored in a bucket’s forward list */
struct HCNode {
    char           *key;      /* heap copy of key string          */
    void           *val;      /* caller-managed payload           */
    struct HCNode  *next;     /* next node in chain               */
};

typedef struct {
    struct HCNode *buckets[HASH_SIZE];
} HashCore;

/* Lifecycle */
void  hc_init(HashCore *hc);
void  hc_free(HashCore *hc, void (*free_val)(void*));

/* Insert — returns 1 on success, 0 if key exists or OOM. */
int   hc_insert(HashCore *hc, const char *key, void *val);

/* Lookup — returns value pointer or NULL if not found. */
void *hc_find  (const HashCore *hc, const char *key);

#endif /* HASH_CORE_H */
