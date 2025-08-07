#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#include <stdarg.h>

extern int g_debug_mode;

void debug_log(const char *fmt, ...);

#define DEBUG debug_log

#endif