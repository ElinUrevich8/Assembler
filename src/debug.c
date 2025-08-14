/*============================================================================
 *  debug.c
 *
 *  Tiny printf-like logger guarded by g_debug_mode.
 *  Usage: if (g_debug_mode) DEBUG("IC=%d DC=%d\n", ic, dc);
 *============================================================================*/
#include "debug.h"
#include <stdarg.h>

int g_debug_mode = 0;

void debug_log(const char *fmt, ...) {
    if (g_debug_mode) {                     /* no formatting cost when disabled */
        va_list args;
        va_start(args, fmt);
        fprintf(stdout, "[DEBUG] ");
        vfprintf(stdout, fmt, args);
        va_end(args);
    }
}
