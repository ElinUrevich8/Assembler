/*============================================================================
 *  debug.h
 *
 *  Minimal debug logging helper.
 *  Usage:
 *     if (g_debug_mode) DEBUG("format %d\n", x);
 *  The DEBUG macro routes to debug_log(), which is printf-like.
 *============================================================================*/

#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>

extern int g_debug_mode;

/* printf-like debug logger; writes to stderr. */
void debug_log(const char *fmt, ...);

#define DEBUG debug_log

#endif
