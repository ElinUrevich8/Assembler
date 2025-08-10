/*====================================================================
 *  main.c  –  command-line driver for the two-pass assembler
 *====================================================================*/
#include "assembler.h"    /* high-level assemble_file() */
#include "defaults.h"     /* brings boolean type, PATH_MAX, etc. */

#include <stdio.h>
#include <string.h>

/*--------------------------------------------------------------*/
/*  Process a single .as file                                   */
/*--------------------------------------------------------------*/
static bool process_file(const char *as_path)
{
    char base[MAX_SRC_PATH];     /* path without the ".as" suffix */
    char *dot;

    /* copy input path into a mutable buffer */
    strncpy(base, as_path, sizeof base - 1);
    base[sizeof base - 1] = '\0';

    /* verify that the extension is ".as" and strip it */
    dot = strrchr(base, '.');
    if (!dot || strcmp(dot, EXT_AS) != 0) {          /* EXT_AS from defaults.h */
        fprintf(stderr, "Error: '%s' must have %s extension\n",
                as_path, EXT_AS);
        return false;
    }
    *dot = '\0';                                     /* now 'base' holds <path> */

    printf(">>> Processing %s\n", as_path);

    if (!assemble_file(base)) {
        fprintf(stderr, "Assembly failed for %s\n", as_path);
        return false;
    }
    return true;
}

/*--------------------------------------------------------------*/
/*  main – iterate over all command-line files                  */
/*--------------------------------------------------------------*/
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
