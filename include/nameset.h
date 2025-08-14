/*============================================================================
 *  nameset.h
 *
 *  NameSet: a tiny chained hash-set for unique NUL-terminated strings.
 *
 *  Used for:
 *    * Macro names (pre-assembler)
 *    * Symbol/label names (pass 1)
 *    * Global “used identifiers” namespace (catch duplicates across scopes)
 *
 *  Notes:
 *    - Only uniqueness (presence) is tracked; no payload per key.
 *    - Caller owns key storage; keys are copied internally.
 *============================================================================*/
#ifndef NAMESET_H
#define NAMESET_H

#include "defaults.h"        /* bool, MAX_* limits */
#include "hash_core.h"

struct NSNode;                       /* forward declaration */

/* Public container (concrete so it can live on the stack). */
typedef struct NameSet {
    struct NSNode *buckets[HASH_SIZE];
} NameSet;

/*--------------------------- Lifecycle ------------------------------------*/
void ns_init(NameSet *ns);
void ns_free(NameSet *ns, void (*free_val)(void*));   /* free_val may be NULL */

/*--------------------------- Operations -----------------------------------*/
/* Insert a key; returns false if already present. */
bool ns_add      (NameSet *ns, const char *key);
/* Query presence; returns true iff key exists. */
bool ns_contains (const NameSet *ns, const char *key);

#endif  /* NAMESET_H */
