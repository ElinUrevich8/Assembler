/*============================================================================
 *  defaults.h
 *
 *  Centralised project-wide constants and small compatibility helpers.
 *  Target platform: standard Ubuntu / GCC (but keep ANSI-C90 fallback).
 *============================================================================*/

#ifndef DEFAULTS_H
#define DEFAULTS_H

/*-------------------------------------------------------------------------*/
/*  System limits                                                           */
/*-------------------------------------------------------------------------*/
#include <limits.h>            /* Brings PATH_MAX, NAME_MAX, etc.          */

#ifndef PATH_MAX               /* Fallback for exotic tool-chains          */
#define PATH_MAX 4096
#endif

/*-------------------------------------------------------------------------*/
/*  Source-file syntax limits (per spec)                                    */
/*-------------------------------------------------------------------------*/
/* Maximum characters in a single source line, excluding the final '\n'   */
#define MAX_LINE_LEN      80
/* Maximum characters in a label or macro name ,including terminating NUL  */
#define MAX_LABEL_LEN     31  


/*-------------------------------------------------------------------------*/
/*  Assembler / machine parameters                                          */
/*-------------------------------------------------------------------------*/
#define WORD_SIZE_BITS    10   /* Our imaginary CPU uses 10-bit words       */
#define MEMORY_CAPACITY  256   /* Address space: 0–255 inclusive            */
#define IC_START         100   /* Program code starts from address 100      */
#define MAX_OPERANDS       2   /* ISA never has more than two operands      */

/* Back-compat: some modules expect IC_INIT */
#ifndef IC_INIT
#define IC_INIT IC_START
#endif

/*-------------------------------------------------------------------------*/
/*  Derived helper sizes                                                    */
/*-------------------------------------------------------------------------*/
#define MAX_SRC_PATH  (PATH_MAX)               /* Buffer for input file path */
#define MAX_AM_PATH  (PATH_MAX + 4)  /* room for ".am" + NUL */
#define MAX_OB_PATH  (PATH_MAX + 4)  /* room for ".ob" + NUL */

/*-------------------------------------------------------------------------*/
/*  Boolean type fallback for strict C90                                    */
/*-------------------------------------------------------------------------*/
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


/*--------------------------------------------------*/
/*  File-extension constants                        */
/*--------------------------------------------------*/
#define EXT_AS   ".as"
#define EXT_AM   ".am"
#define EXT_OB   ".ob"
#define EXT_ENT  ".ent"
#define EXT_EXT  ".ext"


/*-------------------------------------------------------------------------*/
/*  Misc. return codes                                                      */
/*-------------------------------------------------------------------------*/
#define ASM_SUCCESS     0
#define ASM_FAILURE     1


/* ---------- Base-4 formatting for .ob ---------- */
#define BASE4_CHAR_0 'a'
#define BASE4_CHAR_1 'b'
#define BASE4_CHAR_2 'c'
#define BASE4_CHAR_3 'd'

/* 10-bit word -> 5 base-4 digits */
#define BASE4_WORD_STRLEN 5

/* Address column padding in .ob:
   0  = trim (no fixed width)
   4+ = fixed width (use 4 if you want address like "bbcc", etc.) */
#ifndef OB_ADDR_PAD_WIDTH
#define OB_ADDR_PAD_WIDTH 0
#endif


#endif /* DEFAULTS_H */
