/*============================================================================
 *  main.c
 *
 *  CLI driver: iterates argv, strips optional ".as", and calls assemble_file().
 *  Exit code is non-zero if any file fails to assemble.
 *============================================================================*/
#include "assembler.h"
#include "defaults.h"

#include <stdio.h>
#include <string.h>

/* Process a single command-line argument as a base path (with or without .as) */
static bool process_file(const char *arg)
{
    char base[MAX_SRC_PATH];
    const char *dot = strrchr(arg, '.');

    /* If ".../name.as" was passed, strip the ".as"; otherwise treat as base. */
    if (dot && strcmp(dot, EXT_AS) == 0) {
        size_t blen = (size_t)(dot - arg);
        if (blen >= sizeof base) blen = sizeof base - 1;
        memcpy(base, arg, blen);
        base[blen] = '\0';
    } else {
        strncpy(base, arg, sizeof base - 1);
        base[sizeof base - 1] = '\0';
    }

    printf(">>> Processing %s.as\n", base);
    if (!assemble_file(base)) {
        fprintf(stderr, "Assembly failed for %s.as\n", base);
        return false;
    }
    return true;
}

int main(int argc, char *argv[])
{
    bool all_ok = true;
    int  i;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1.as> [file2.as] ...\n", argv[0]);
        return ASM_FAILURE;
    }

    for (i = 1; i < argc; ++i) {
        if (!process_file(argv[i]))
            all_ok = false;
    }

    return all_ok ? ASM_SUCCESS : ASM_FAILURE;
}
