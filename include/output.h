/*============================================================================
 *  output.h
 *
 *  Writers for object and side files.
 *
 *  Files:
 *    - .ob  : object words (code then data), printed in base-4 a/b/c/d
 *    - .ent : entries (name + final relocated address)
 *    - .ext : extern use-sites (name + address of the operand word)
 *
 *  Policy:
 *    - .ent/.ext are written ONLY if they would be non-empty.
 *    - All functions return non-zero on success.
 *============================================================================*/
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdio.h>
#include "pass1.h"
#include "pass2.h"

/* Write the object file (.ob).
 * Layout:
 *   <code_len> <data_len>                ; header, both in unique base-4
 *   <addr> <word>                        ; code words, IC_INIT..ICF-1
 *   <addr> <word>                        ; data words, ICF..ICF+DC-1
 */
int output_write_ob(FILE *fp, const Pass1Result *p1, const Pass2Result *p2);

/* Write .ent iff there is at least one entry.
 * One line per entry: <name> <address-in-base-4>
 */
int output_write_ent(FILE *fp, const Pass2Result *p2);

/* Write .ext iff there is at least one extern use-site.
 * One line per use-site: <name> <use-address-in-base-4>
 */
int output_write_ext(FILE *fp, const Pass2Result *p2);

#endif /* OUTPUT_H */
