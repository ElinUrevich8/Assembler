/*============================================================================
 *  main.c
 *
 *  CLI driver: iterates arg_values, strips optional ".as", and calls assemble_file().
 *  Exit code is non-zero if any file fails to assemble.
 *============================================================================*/
#include "assembler.h"
#include "defaults.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>

/* Process a single command-line argument as a base path (with or without .as) */
static bool process_file(const char *path)
{
    char base[MAX_SRC_PATH];
    const char *dot = strrchr(path, '.');

    /* If ".../name.as" was passed, strip the ".as"; otherwise treat as base. */
    if (dot && strcmp(dot, EXT_AS) == 0) {
        size_t blen = (size_t)(dot - path);
        if (blen >= sizeof base) blen = sizeof base - 1;
        memcpy(base, path, blen);
        base[blen] = '\0';
    } else {
        strncpy(base, path, sizeof base - 1);
        base[sizeof base - 1] = '\0';
    }

    DEBUG(">>> Processing %s.as\n", base);
    if (!assemble_file(base)) {
        fprintf(stderr, "Assembly failed for %s.as\n", base);
        return false;
    }
    return true;
}

/* Check number of arguments, since running the program as ./assembler <file1.as> [file2.as] ...
treats './assembler' as first argument the program requests at least 2 arguments */
int main(int arg_count, char *arg_values[])
{
    bool all_ok = true;
    int  i;

    if (arg_count < 2) {
        fprintf(stderr, "Usage: %s <file1.as> [file2.as] ...\n", arg_values[0]);
        return ASM_FAILURE;
    }

    for (i = 1; i < arg_count; ++i) {
        if (!process_file(arg_values[i]))
            all_ok = false;
    }

    return all_ok ? ASM_SUCCESS : ASM_FAILURE;
}
