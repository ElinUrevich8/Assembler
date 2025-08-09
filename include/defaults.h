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
/* Maximum characters in a single source line, including the final '\n'   */
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

/*-------------------------------------------------------------------------*/
/*  Derived helper sizes                                                    */
/*-------------------------------------------------------------------------*/
#define MAX_SRC_PATH  (PATH_MAX)               /* Buffer for input file path */
#define MAX_AM_PATH   (PATH_MAX + 3)           /* room for ".am" + NUL       */
#define MAX_O_PATH    (PATH_MAX + 2)           /* room for ".o" + NUL       */

/*-------------------------------------------------------------------------*/
/*  Boolean type fallback for strict C90                                    */
/*-------------------------------------------------------------------------*/
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
  #include <stdbool.h>
#else
  typedef int bool;
  #define true  1
  #define false 0
#endif

/*-------------------------------------------------------------------------*/
/*  Unique base-4 printing helpers                                          */
/*-------------------------------------------------------------------------*/
#define BASE4_CHAR_0  'a'
#define BASE4_CHAR_1  'b'
#define BASE4_CHAR_2  'c'
#define BASE4_CHAR_3  'd'
#define BASE4_WORD_STRLEN 5     /* Each 10-bit word prints as 5 characters  */

/*--------------------------------------------------*/
/*  File-extension constants                        */
/*--------------------------------------------------*/
#define EXT_AS   ".as"
#define EXT_AM   ".am"
#define EXT_O    ".o"
#define EXT_ENT  ".ent"
#define EXT_EXT  ".ext"


/*-------------------------------------------------------------------------*/
/*  Misc. return codes                                                      */
/*-------------------------------------------------------------------------*/
#define ASM_SUCCESS     0
#define ASM_FAILURE     1

#endif /* DEFAULTS_H */
