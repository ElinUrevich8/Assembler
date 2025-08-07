/*============================================================================
 *  nameset.h
 *
 *  Generic chained hash-set for unique C-strings.
 *  Used throughout the project for:
 *    • macro names      (pre-assembler)
 *    • symbol / label names (Pass 1)
 *    • global “used identifiers” namespace
 *============================================================================*/

#ifndef NAMESET_H
#define NAMESET_H

#include "defaults.h"        /* bool, MAX_* limits */
#include "hash_core.h"


struct NSNode;                       /* forward node */

typedef struct NameSet {
    struct NSNode *buckets[HASH_SIZE];
} NameSet;


/* Initialise / free -------------------------------------------------------*/
void ns_init(NameSet *ns);
void ns_free(NameSet *ns, void (*free_val)(void*));   /* free_val may be NULL */

/* Operations --------------------------------------------------------------*/
bool ns_add      (NameSet *ns, const char *key);      /* false => already present */
bool ns_contains (const NameSet *ns, const char *key);

#endif  /* NAMESET_H */
