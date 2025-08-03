#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "preassembler.h"

#define MAX_LINE_LENGTH 80
#define MAX_MACRO_NAME 31
#define HASH_TABLE_SIZE 101 /* Prime number for better hash distribution */

/* Macro structure */
typedef struct Macro {
    char name[MAX_MACRO_NAME + 1];
    char *content;
    struct Macro *next;
} Macro;

/* Hash table structure */
typedef struct {
    Macro *buckets[HASH_TABLE_SIZE];
} MacroTable;

/* Reserved words */
static const char *reserved_words[] = {
    "mov", "cmp", "add", "sub", "not", "clr", "lea", "inc", "dec",
    "jmp", "bne", "red", "prn", "jsr", "rts", "stop",
    ".data", ".string", ".entry", ".extern", NULL
};

/* Function prototypes */
static unsigned int hash(const char *str);
static int is_reserved_word(const char *name);
static int is_valid_macro_name(const char *name);
static void init_macro_table(MacroTable *table);
static int add_macro(MacroTable *table, const char *name, char *content);
static char *get_macro_content(MacroTable *table, const char *name);
static void free_macro_table(MacroTable *table);
static int expand_macro_content(MacroTable *table, const char *content, FILE *output, int line_number, int *error);

/* Simple hash function */
static unsigned int hash(const char *str) {
    unsigned int hash_val;
    int c;

    hash_val = 5381; 
    while ((c = *str++)) {
        hash_val = ((hash_val << 5) + hash_val) + c; /* hash * 33 + c */
    }
    return hash_val % HASH_TABLE_SIZE;
}

/* Check if name is a reserved word */
static int is_reserved_word(const char *name) {
    int i;

    i = 0;
    while (reserved_words[i]) {
        if (strcmp(name, reserved_words[i]) == 0) {
            return 1;
        }
        i++;
    }
    return 0;
}

/* Validate macro name */
static int is_valid_macro_name(const char *name) {
    int i;

    if (!isalpha(name[0])) return 0;
    if (strlen(name) > MAX_MACRO_NAME - 1) return 0; /* Check macro name length */
    i = 0;
    while (name[i]) {
        if (!isalnum(name[i]) && name[i] != '_') return 0;
        i++;
    }
    return 1;
}

/* Initialize macro table */
static void init_macro_table(MacroTable *table) {
    int i;

    i = 0;
    while (i < HASH_TABLE_SIZE) {
        table->buckets[i] = NULL;
        i++;
    }
}

/* Add macro to hash table */
static int add_macro(MacroTable *table, const char *name, char *content) {
    unsigned int index;
    Macro *current;
    Macro *new_macro;

    index = hash(name);
    current = table->buckets[index];
    /* Check for duplicate macro name */
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return 0;
        }
        current = current->next;
    }
    
    new_macro = malloc(sizeof(Macro));
    if (!new_macro) return 0;
    
    strncpy(new_macro->name, name, MAX_MACRO_NAME);
    new_macro->name[MAX_MACRO_NAME] = '\0';
    new_macro->content = malloc(strlen(content) + 1);
    if (!new_macro->content) {
        free(new_macro);
        return 0;
    }
    strcpy(new_macro->content, content);
    new_macro->next = table->buckets[index];
    table->buckets[index] = new_macro;
    return 1;
}

/* Get macro content by name */
static char *get_macro_content(MacroTable *table, const char *name) {
    unsigned int index;
    Macro *current;

    index = hash(name);
    current = table->buckets[index];
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current->content;
        }
        current = current->next;
    }
    return NULL;
}

/* Free macro table */
static void free_macro_table(MacroTable *table) {
    int i;
    Macro *current;
    Macro *temp;

    i = 0;
    while (i < HASH_TABLE_SIZE) {
        current = table->buckets[i];
        while (current) {
            temp = current;
            current = current->next;
            free(temp->content);
            free(temp);
        }
        i++;
    }
}

/* Helper function to recursively expand macro content in nested macros */
static int expand_macro_content(MacroTable *table, const char *content, FILE *output, int line_number, int *error) {
    char *content_copy = strdup(content); /* Copy content to avoid modifying original */
    char *line = content_copy;
    char *next_line;
    char *trimmed;
    char *end;
    char *macro_content;

    if (!content_copy) {
        fprintf(stderr, "Error(line %d): Memory allocation failed for macro content copy\n", line_number);
        *error = 1;
        return 0;
    }

    while (line && *line) {
        /* Find the next newline */
        next_line = strchr(line, '\n');
        if (next_line) {
            *next_line = '\0';
            next_line++;
        }

        /* Trim leading and trailing whitespace */
        trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        end = trimmed + strlen(trimmed) - 1;
        while (end > trimmed && isspace(*end)) *end-- = '\0';

        if (strlen(trimmed) > 0) {
            /* Check if trimmed line is a macro */
            macro_content = get_macro_content(table, trimmed);
            if (macro_content) {
                fprintf(stderr, "Debug: Expanding nested macro '%s' at line %d\n", trimmed, line_number);
                /* Recursively expand nested macro */
                if (!expand_macro_content(table, macro_content, output, line_number, error)) {
                    free(content_copy);
                    return 0;
                }
            } else {
                /* Write the original line (including leading spaces) */
                fprintf(stderr, "Debug: Writing macro line '%s' at line %d\n", line, line_number);
                if (fputs(line, output) == EOF) {
                    fprintf(stderr, "Error(line %d): Failed to write macro line to output file\n", line_number);
                    *error = 1;
                    free(content_copy);
                    return 0;
                }
                /* Add newline if it was present */
                if (next_line || line[strlen(line)] == '\n') {
                    if (fputc('\n', output) == EOF) {
                        fprintf(stderr, "Error(line %d): Failed to write newline to output file\n", line_number);
                        *error = 1;
                        free(content_copy);
                        return 0;
                    }
                }
            }
        } else if (next_line || line[strlen(line)] == '\n') {
            /* Write empty line */
            fprintf(stderr, "Debug: Writing empty macro line at line %d\n", line_number);
            if (fputc('\n', output) == EOF) {
                fprintf(stderr, "Error(line %d): Failed to write empty macro line to output file\n", line_number);
                *error = 1;
                free(content_copy);
                return 0;
            }
        }

        line = next_line;
    }

    free(content_copy);
    return 1;
}


/* Main preassembler function */
int preassemble(const char *input_filename, const char *output_filename) {
    /* Input file pointer for reading the source .as file */
    FILE *input;
    /* Output file pointer for writing the preassembled .am file */
    FILE *output;
    /* Hash table to store macro definitions */
    MacroTable macro_table;
    /* Buffer to store a single line from the input file */
    char line[MAX_LINE_LENGTH + 1];
    /* Pointer to store the content of a macro during definition */
    char *macro_content;
    /* Buffer to store the name of a macro being defined */
    char macro_name[MAX_MACRO_NAME + 1];
    /* Tracks the current line number for error reporting */
    int line_number;
    /* Flag to indicate if currently inside a macro definition */
    int in_macro;
    /* Flag to indicate if an error occurred during processing */
    int error;
    /* Current allocated size of the macro_content buffer */
    size_t macro_content_size;
    /* Current length of content stored in macro_content */
    size_t macro_content_len;
    /* Pointer to the trimmed start of a line, skipping leading whitespace */
    char *trimmed;
    /* Pointer to the end of a trimmed line, for removing trailing whitespace */
    char *end;
    /* Pointer to the start of a macro name in a line */
    char *name;
    /* Pointer to the end of a macro name in a line */
    char *name_end;
    /* Temporary pointer for reallocating macro_content */
    char *new_content;
    long pos;
    long pos_after;

    /* Validate input filename extension */
    size_t len = strlen(input_filename);
    if (len < 3 || strcmp(input_filename + len - 3, ".as") != 0) {
        fprintf(stderr, "Error: Input filename '%s' does not have .as extension\n", input_filename);
        return 0;
    }

    fprintf(stderr, "Debug: Attempting to open input file '%s'\n", input_filename);
    input = fopen(input_filename, "r");
    if (!input) {
        fprintf(stderr, "Error: Failed to open input file '%s'\n", input_filename);
        return 0;
    }

    fprintf(stderr, "Debug: Attempting to open output file '%s'\n", output_filename);
    output = fopen(output_filename, "w");
    if (!output) {
        fprintf(stderr, "Error: Failed to open output file '%s'\n", output_filename);
        fclose(input);
        return 0;
    }

    init_macro_table(&macro_table);
    macro_content = NULL;
    line_number = 0;
    in_macro = 0;
    error = 0;
    macro_content_size = MAX_LINE_LENGTH * 100;
    macro_content_len = 0;

    while (1) {
        pos = ftell(input);
        fprintf(stderr, "Debug: Before fgets at line %d, file pos: %ld, in_macro=%d\n", line_number + 1, pos, in_macro);
        if (!fgets(line, MAX_LINE_LENGTH + 1, input)) {
            if (feof(input)) {
                fprintf(stderr, "Debug: EOF reached after line %d\n", line_number);
            }
            if (ferror(input)) {
                fprintf(stderr, "Error(line %d): File read error\n", line_number + 1);
                error = 1;
            }
            fprintf(stderr, "Debug: fgets failed at line %d, in_macro=%d\n", line_number + 1, in_macro);
            break;
        }
        len = strlen(line);
        pos = ftell(input);
        fprintf(stderr, "Debug: Read line %d: '%s' (length: %lu, file pos: %ld)\n", line_number + 1, line, (unsigned long)len, pos);
        /* Strip \r for Windows-style newlines */
        if (len > 0 && line[len - 1] == '\r') {
            line[len - 1] = '\0';
            len--;
        }
        line_number++;
        fprintf(stderr, "Debug: After processing line %d, continuing loop\n", line_number);

        /* Check if line exceeds maximum length */
        if (strlen(line) >= MAX_LINE_LENGTH && line[MAX_LINE_LENGTH - 1] != '\n') {
            fprintf(stderr, "Error(line %d): Line exceeds %d characters\n", line_number, MAX_LINE_LENGTH);
            error = 1;
            break;
        }
        
        trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        
        end = trimmed + strlen(trimmed) - 1;
        while (end > trimmed && isspace(*end)) *end-- = '\0';
        
        if (strlen(trimmed) == 0) {
            fprintf(stderr, "Debug: Writing empty line %d: %s\n", line_number, line);
            if (fputs(line, output) == EOF) {
                fprintf(stderr, "Error(line %d): Failed to write to output file\n", line_number);
                error = 1;
                break;
            }
            if (fflush(output) != 0) {
                fprintf(stderr, "Error(line %d): Failed to flush output file\n", line_number);
                error = 1;
                break;
            }
            continue;
        }

        fprintf(stderr, "Debug: Processing line %d: %s\n", line_number, trimmed);

        if (strncmp(trimmed, "mcro ", 5) == 0) {

            
            char name_buffer[MAX_MACRO_NAME + 1];
            name = trimmed + 5;
            while (isspace(*name)) name++;
            if (*name == '\0') {
                fprintf(stderr, "Error(line %d): Missing macro name\n", line_number);
                error = 1;
                break;
            }
            name_end = name;
            while (*name_end && !isspace(*name_end) && (name_end - name) < MAX_MACRO_NAME) name_end++;
            if (name_end - name >= MAX_MACRO_NAME) {
                fprintf(stderr, "Error(line %d): Macro name too long\n", line_number);
                error = 1;
                break;
            }
            strncpy(name_buffer, name, name_end - name);
            name_buffer[name_end - name] = '\0';
            name = name_buffer;
            
            if (is_reserved_word(name)) {
                fprintf(stderr, "Error(line %d): Macro name '%s' is a reserved word\n", line_number, name);
                error = 1;
                break;
            }
            
            if (!is_valid_macro_name(name)) {
                fprintf(stderr, "Error(line %d): Invalid macro name '%s'\n", line_number, name);
                error = 1;
                break;
            }
            
            fprintf(stderr, "Debug: Defining macro '%s' at line %d\n", name, line_number);
            strncpy(macro_name, name, MAX_MACRO_NAME);
            macro_name[MAX_MACRO_NAME] = '\0';
            in_macro = 1;
            macro_content = malloc(macro_content_size);
            if (!macro_content) {
                fprintf(stderr, "Error(line %d): Memory allocation failed for macro content\n", line_number);
                error = 1;
                break;
            }
            macro_content[0] = '\0';
            macro_content_len = 0;
        
        
            pos_after = ftell(input);
            fprintf(stderr, "Debug: File position after processing line %d: %ld\n", line_number, pos_after);
            continue; /* Skip to next line after starting macro definition */

        }

        else if (strcmp(trimmed, "mcroend") == 0) {
            if (!in_macro) {
                fprintf(stderr, "Debug: Ignoring 'mcroend' at line %d (not in macro)\n", line_number);
                continue; /* Skip to next line if mcroend appears outside macro definition */
            }
            
            if (!add_macro(&macro_table, macro_name, macro_content)) {
                fprintf(stderr, "Error(line %d): Duplicate macro '%s'\n", line_number, macro_name);
                error = 1;
                free(macro_content);
                break;
            }
            fprintf(stderr, "Debug: Macro '%s' defined at line %d\n", macro_name, line_number);
            in_macro = 0;
            free(macro_content);
            macro_content = NULL;
            continue; /* Skip to next line after ending macro definition */
        }
        else if (in_macro) {
            size_t line_len;
            line_len = strlen(trimmed);
            fprintf(stderr, "Debug: Before realloc: macro_content_size=%lu, macro_content_len=%lu, line_len=%lu\n", 
                    (unsigned long)macro_content_size, (unsigned long)macro_content_len, (unsigned long)line_len);
            if (macro_content_len + line_len + 2 > macro_content_size) {
                macro_content_size *= 2;
                new_content = realloc(macro_content, macro_content_size);
                if (!new_content) {
                    fprintf(stderr, "Error(line %d): Memory reallocation failed for macro content\n", line_number);
                    free(macro_content);
                    error = 1;
                    break;
                }
                macro_content = new_content;
                fprintf(stderr, "Debug: Realloc succeeded, new macro_content_size=%lu\n", (unsigned long)macro_content_size);
            }
            strcat(macro_content, trimmed);
            strcat(macro_content, "\n");
            macro_content_len += line_len + 1;
            fprintf(stderr, "Debug: Added line to macro '%s' at line %d, new macro_content_len=%lu\n", 
                macro_name, line_number, (unsigned long)macro_content_len);
            continue; /* Skip to next line after adding to macro */ 
        }

        else {
            char *content;
            fprintf(stderr, "Debug: Entering else block for line %d\n", line_number);
            content = get_macro_content(&macro_table, trimmed);
            fprintf(stderr, "Debug: get_macro_content returned for '%s' at line %d\n", trimmed, line_number);
            if (content) {
                fprintf(stderr, "Debug: Expanding macro '%s' at line %d\n", trimmed, line_number);
                if (!expand_macro_content(&macro_table, content, output, line_number, &error)) {
                    fprintf(stderr, "Error(line %d): Failed to expand macro content\n", line_number);
                    error = 1;
                    break;
                }
            } else {
                fprintf(stderr, "Debug: Writing non-macro line %d: %s\n", line_number, line);
                if (fputs(line, output) == EOF) {
                    fprintf(stderr, "Error(line %d): Failed to write non-macro line to output file\n", line_number);
                    error = 1;
                    break;
                }
                if (line[strlen(line) - 1] != '\n') {
                    if (fputc('\n', output) == EOF) {
                        fprintf(stderr, "Error(line %d): Failed to write newline to output file\n", line_number);
                        error = 1;
                        break;
                    }
                    fprintf(stderr, "Debug: Added newline after non-macro line %d\n", line_number);
                }
                fprintf(stderr, "Debug: Wrote non-macro line: %s\n", line);
            }
            fprintf(stderr, "Debug: Before fflush for line %d\n", line_number);
            if (fflush(output) != 0) {
                fprintf(stderr, "Error(line %d): Failed to flush output file\n", line_number);
                error = 1;
                break;
            }
            fprintf(stderr, "Debug: After fflush for line %d, continuing loop\n", line_number);
            }
    }    

    if (in_macro) {
        fprintf(stderr, "Error(line %d): Unclosed macro definition\n", line_number);
        error = 1;
        free(macro_content);
        macro_content = NULL;
        in_macro = 0; /* Reset in_macro to prevent double-free */
    }
    
    if (error) {
        free_macro_table(&macro_table);
        fclose(input);
        if (fflush(output) != 0) {
            fprintf(stderr, "Error: Failed to flush output file during cleanup\n");
        }
        fclose(output);
        fprintf(stderr, "Debug: Exiting due to errors\n");
        remove(output_filename);
        return 0;
    }
    
    free_macro_table(&macro_table);
    fclose(input);
    if (fflush(output) != 0) {
        fprintf(stderr, "Error: Failed to flush output file during cleanup\n");
    }
    fclose(output);
    fprintf(stderr, "Debug: Preassembly completed successfully\n");
    return 1;
 }
