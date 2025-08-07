#include "nameset.h"
#include <stdlib.h>
#include <string.h>
#include "hash_core.h"

/* Define NSNode struct */
typedef struct NSNode {
    char *key;
    struct NSNode *next;
} NSNode;

/* C90-compatible strdup */
static char *my_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = (char *)malloc(len);
    if (d) strcpy(d, s);
    return d;
}

static unsigned hash(const char *s) {
    unsigned h = 5381;
    int c;
    while ((c = *s++)) h = ((h << 5) + h) + c; /* h * 33 + c */
    return h % HASH_SIZE;
}

void ns_init(NameSet *ns) {
    memset(ns, 0, sizeof *ns);
}

static NSNode *find(NSNode *n, const char *k) {
    while (n && strcmp(n->key, k)) n = n->next;
    return n;
}

bool ns_add(NameSet *ns, const char *k) {
    unsigned idx = hash(k);
    NSNode *n;
    if (find(ns->buckets[idx], k)) return false;           /* dup */
    n = (NSNode *)malloc(sizeof *n);
    if (!n) return false;                                  /* OOM */
    n->key = my_strdup(k);
    n->next = ns->buckets[idx];
    ns->buckets[idx] = n;
    return true;
}

bool ns_contains(const NameSet *ns, const char *k) {
    return find(ns->buckets[hash(k)], k) != NULL;
}

void ns_free(NameSet *ns, void (*free_val)(void*)) {
    size_t i;
    for (i = 0; i < HASH_SIZE; i++) {
        NSNode *n = ns->buckets[i], *tmp;
        while (n) {
            tmp = n->next;
            if (free_val) free_val(NULL); /* no value */
            free(n->key);
            free(n);
            n = tmp;
        }
    }
}
