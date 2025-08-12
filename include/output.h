/* output.h
 * Object/side-file writers.
 * Only create .ext/.ent when there is something to write.
 */

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
 * Returns non-zero on success.
 */
int output_write_ob(FILE *fp, const Pass1Result *p1, const Pass2Result *p2);

/* Write the entries file (.ent) iff p2->ent_len > 0.
 * One line per entry: <name> <address-in-base-4>
 * Returns non-zero on success (if there is nothing to write, returns 1 and does nothing).
 */
int output_write_ent(FILE *fp, const Pass2Result *p2);

/* Write the externs file (.ext) iff p2->ext_len > 0.
 * One line per use-site: <name> <use-address-in-base-4>
 * Returns non-zero on success (if there is nothing to write, returns 1 and does nothing).
 */
int output_write_ext(FILE *fp, const Pass2Result *p2);

#endif /* OUTPUT_H */
