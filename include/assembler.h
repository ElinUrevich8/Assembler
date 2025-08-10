/*============================================================================
 *  assembler.h
 *
 *  Public high-level API for the two-pass assembler.
 *  assemble_file() runs pre-assembler + Pass 1 + Pass 2
 *  and leaves final .ob / .ent / .ext outputs.
 *============================================================================*/

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "defaults.h"
#include "nameset.h"   /* for NameSet */

#ifdef __cplusplus
extern "C" {
#endif

bool assemble_file(const char *base_path);
/* shared global: macro/label single-namespace set */
extern NameSet *g_used_names;

#ifdef __cplusplus
}
#endif

#endif  /* ASSEMBLER_H */
