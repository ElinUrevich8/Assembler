/*============================================================================
 *  debug.c
 *
 *  Tiny printf-like logger guarded by g_debug_mode.
 *  Usage: if (g_debug_mode) DEBUG("IC=%d DC=%d\n", ic, dc);
 *============================================================================*/

#include "debug.h"
#include <stdarg.h>
#include <stdio.h>   /* fprintf, vfprintf */

/*----------------------------------------------------------------------------
 * g_debug_mode:
 *   Global on/off switch for debug logging. When zero, debug_log() is a no-op.
 *
 * Usage:
 *   if (g_debug_mode) DEBUG("value=%d\n", v);
 *----------------------------------------------------------------------------*/
int g_debug_mode = 0;

/*----------------------------------------------------------------------------
 * debug_log(fmt, ...):
 *   Print a debug line (prefixed with "[DEBUG] ") to stdout when enabled.
 *
 * Params:
 *   fmt - printf-style format string (must be non-NULL).
 *   ... - arguments for the format string.
 *
 * Returns:
 *   void - prints only if g_debug_mode != 0.
 *
 * Errors:
 *   I/O errors from fprintf/vfprintf are ignored (best-effort logging).
 *
 * Example:
 *   debug_log("IC=%d DC=%d\n", ic, dc);
 *----------------------------------------------------------------------------*/
void debug_log(const char *fmt, ...) {
    /* locals: */
    va_list args;        /* varargs list for formatting */

    if (!g_debug_mode) return;            /* no formatting cost when disabled */
    if (!fmt) return;                     /* defensive: nothing to print      */

    va_start(args, fmt);
    (void)fprintf(stdout, "[DEBUG] ");
    (void)vfprintf(stdout, fmt, args);
    va_end(args);
}
