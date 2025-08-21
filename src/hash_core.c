/*============================================================================
 *  hash_core.c
 *
 *  Generic chained hash table declared in hash_core.h.
 *  - Keys are NUL-terminated strings (owned by the table).
 *  - Values are opaque void* (owned by the caller unless a destructor is
 *    supplied to hc_free()).
 *  - Hash function: DJB2 (classic, simple, good enough for coursework).
 *
 *  Public API:
 *      void   hc_init(HashCore *hc);
 *      void   hc_free(HashCore *hc, void (*free_val)(void*));
 *      int    hc_insert(HashCore *hc, const char *key, void *val);   // 1=ok, 0=dup/OOM
 *      void*  hc_find(const HashCore *hc, const char *key);          // NULL if missing
 *============================================================================*/

#include "hash_core.h"
#include <stdlib.h>
#include <string.h>

/*----------------------------------------------------------------------------
 * my_strdup(s): minimal strdup replacement (C90-safe).
 *----------------------------------------------------------------------------*/
static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = (char *)malloc(len);
    if (d) {
        strcpy(d, s);
    }
    return d;
}

/*------------------------------------------------------------*/
/*   DJB2 hash, bucketized by HASH_SIZE                       */
/*------------------------------------------------------------*/
static unsigned int hash(const char *str) {
    unsigned int hash_val;
    int c;

    hash_val = 5381;
    while ((c = *str++)) {
        hash_val = ((hash_val << 5) + hash_val) + c; /* hash * 33 + c */
    }
    return hash_val % HASH_SIZE;
}

/*------------------------------------------------------------*/
/*  Public API                                                */
/*------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 * hc_init(hc): zero all buckets.
 *----------------------------------------------------------------------------*/
void hc_init(HashCore *hc)
{
    memset(hc->buckets, 0, sizeof hc->buckets);
}

/*----------------------------------------------------------------------------
 * hc_free(hc, free_val):
 *   Free all entries; if free_val!=NULL, call it on each stored value.
 *----------------------------------------------------------------------------*/
void hc_free(HashCore *hc, void (*free_val)(void*))
{
    size_t i;
    for ( i = 0; i < HASH_SIZE; ++i) {
        struct HCNode *n = hc->buckets[i], *tmp;
        while (n) {
            tmp = n->next;
            if (free_val) free_val(n->val);
            free(n->key);
            free(n);
            n = tmp;
        }
    }
}

/*----------------------------------------------------------------------------
 * hc_insert(hc, key, val):
 *   Insert <key,val>. Returns 1 on success, 0 on duplicate key or allocation failure.
 *----------------------------------------------------------------------------*/
int hc_insert(HashCore *hc, const char *key, void *val) {
    struct HCNode *n;
    unsigned idx = hash(key);
    struct HCNode *cur = hc->buckets[idx];

    /* Reject duplicates */
    while (cur) {
        if (strcmp(cur->key, key) == 0)
            return 0;
        cur = cur->next;
    }

    /* Allocate new node + key copy */
    n = (struct HCNode*)malloc(sizeof *n);
    if (!n) return 0;
    n->key  = my_strdup(key);
    if (!n->key) { free(n); return 0; }
    n->val  = val;
    n->next = hc->buckets[idx];
    hc->buckets[idx] = n;
    return 1;
}

/*----------------------------------------------------------------------------
 * hc_find(hc, key): return stored value pointer or NULL if not found.
 *----------------------------------------------------------------------------*/
void *hc_find(const HashCore *hc, const char *key)
{
    unsigned idx = hash(key);
    struct HCNode *cur = hc->buckets[idx];
    while (cur) {
        if (strcmp(cur->key, key) == 0)
            return cur->val;
        cur = cur->next;
    }
    return NULL;
}
