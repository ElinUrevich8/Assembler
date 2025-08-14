/*============================================================================
 *  defaults.h
 *
 *  Centralised constants and cross-platform fallbacks.
 *
 *  Highlights:
 *  - Source constraints (MAX_LINE_LEN, MAX_LABEL_LEN)
 *  - Machine model (10-bit WORD_SIZE_BITS, IC_START, base-4 alphabet)
 *  - Paths and extensions (.as/.am/.ob/.ent/.ext)
 *============================================================================*/

#ifndef DEFAULTS_H
#define DEFAULTS_H

#include <limits.h>            /* PATH_MAX, NAME_MAX, … */

/* Fallbacks for non-POSIX toolchains */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* ---------- Source syntax limits (per spec) ---------- */
#define MAX_LINE_LEN      80    /* excluding trailing '\n'                   */
#define MAX_LABEL_LEN     31    /* includes terminating NUL                  */

/* ---------- Assembler / machine parameters ---------- */
#define WORD_SIZE_BITS    10    /* 10-bit machine word                       */
#define MEMORY_CAPACITY  256    /* address space 0..255                      */
#define IC_START         100    /* program code base address                  */
#define MAX_OPERANDS       2

#ifndef IC_INIT
#define IC_INIT IC_START
#endif

/* ---------- Derived buffer sizes ---------- */
#define MAX_SRC_PATH  (PATH_MAX)
#define MAX_AM_PATH  (PATH_MAX + 4)  /* ".am"  */
#define MAX_OB_PATH  (PATH_MAX + 4)  /* ".ob"  */

/* ---------- C90 boolean fallback ---------- */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
  #include <stdbool.h>
#else
  #ifndef __bool_true_false_are_defined
    typedef int bool;
    #define true  1
    #define false 0
    #define __bool_true_false_are_defined 1
  #endif
#endif

/* ---------- File extensions ---------- */
#define EXT_AS   ".as"
#define EXT_AM   ".am"
#define EXT_OB   ".ob"
#define EXT_ENT  ".ent"
#define EXT_EXT  ".ext"

/* ---------- Return codes ---------- */
#define ASM_SUCCESS     0
#define ASM_FAILURE     1

/* ---------- Base-4 printing (a/b/c/d) ---------- */
#define BASE4_CHAR_0 'a'
#define BASE4_CHAR_1 'b'
#define BASE4_CHAR_2 'c'
#define BASE4_CHAR_3 'd'
#define BASE4_WORD_STRLEN 5     /* 10 bits -> 5 base-4 digits */

/* Address column left padding (0 = trim; >=4 = fixed width) */
#ifndef OB_ADDR_PAD_WIDTH
#define OB_ADDR_PAD_WIDTH 0
#endif

#endif /* DEFAULTS_H */
