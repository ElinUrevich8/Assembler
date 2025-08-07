/*============================================================================
 *  hash_core.c
 *
 *  Implementation of the generic chained hash table declared in
 *  hash_core.h.  Uses the classic DJB2 hash function.
 *============================================================================*/

#include "hash_core.h"
#include <stdlib.h>
#include <string.h>


/* C90-compatible strdup */
static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = (char *)malloc(len);
    if (d) {
        strcpy(d, s);
    }
    return d;
}

/*------------------------------------------------------------*/
/*   Simple hash function                            */
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
void hc_init(HashCore *hc)
{
    memset(hc->buckets, 0, sizeof hc->buckets);
}

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

/* return 0 => duplicate key OR OOM */
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

    /* Allocate new node */
    n = malloc(sizeof *n);
    if (!n) return 0;
    n->key  = my_strdup(key);
    if (!n->key) { free(n); return 0; }
    n->val  = val;
    n->next = hc->buckets[idx];
    hc->buckets[idx] = n;
    return 1;
}

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
