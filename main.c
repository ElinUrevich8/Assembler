
#include <stdio.h>
#include <string.h>
#include "preassembler.h"

#define MAX_FILENAME_LENGTH 256

/* Function to process a single input file */
static int process_file(const char *input_file) {
    char base_filename[MAX_FILENAME_LENGTH];
    char am_file[MAX_FILENAME_LENGTH];

    /* Extract base filename (remove .as extension) */
    strncpy(base_filename, input_file, MAX_FILENAME_LENGTH - 1);
    base_filename[MAX_FILENAME_LENGTH - 1] = '\0';
    if (strlen(base_filename) > 3 && strcmp(base_filename + strlen(base_filename) - 3, ".as") == 0) {
        base_filename[strlen(base_filename) - 3] = '\0';
    }

    /* Construct output filename */
    sprintf(am_file, "%s.am", base_filename);

    /* Step 1: Run preassembler */
    if (!preassemble(input_file, am_file)) {
        fprintf(stderr, "Preassembler failed for %s\n", input_file);
        remove(am_file); /* Ensure no output file remains on error */
        return 0;
    }

    return 1;
}

/* Validates files are provided and processes them */
int main(int argc, char *argv[]) {
    int i;
    int success = 1;
    char input_file[MAX_FILENAME_LENGTH];

    if (argc < 2) {
        fprintf(stderr, "Error: No files were provided.\n Usage: %s <file1> [file2] ...\n", argv[0]);
        return 1;
    }

    for (i = 1; i < argc; i++) {
        /* Construct input filename */
        sprintf(input_file, "%s.as", argv[i]);
        printf("Processing file: %s\n", input_file);

        if (!process_file(input_file)) {
            success = 0;
        }
    }

    return success ? 0 : 1;
}