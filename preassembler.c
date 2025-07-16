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
    /* Check for duplicate macro name*/
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

/* Main preassembler function */
int preassemble(const char *input_filename, const char *output_filename) {
    FILE *input;
    FILE *output;
    MacroTable macro_table;
    char line[MAX_LINE_LENGTH + 1];
    char *macro_content;
    char macro_name[MAX_MACRO_NAME + 1];
    int line_number;
    int in_macro;
    int error;
    size_t macro_content_size;
    size_t macro_content_len;
    char *trimmed;
    char *end;
    char *name;
    char *name_end;
    char *new_content;

    input = fopen(input_filename, "r");
    output = fopen(output_filename, "w");
    if (!input || !output) {
        if (input) fclose(input);
        if (output) fclose(output);
        return 0;
    }

    init_macro_table(&macro_table);
    macro_content = NULL;
    line_number = 0;
    in_macro = 0;
    error = 0;
    macro_content_size = MAX_LINE_LENGTH * 100;
    macro_content_len = 0;

    while (fgets(line, MAX_LINE_LENGTH + 1, input)) {
        line_number++;
        trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        
        end = trimmed + strlen(trimmed) - 1;
        while (end > trimmed && isspace(*end)) *end-- = '\0';
        
        if (strlen(trimmed) == 0) continue;

        if (strncmp(trimmed, "mcro ", 5) == 0) {
            if (in_macro) {
                fprintf(stderr, "Error(line %d): Nested macro definition\n", line_number);
                error = 1;
                break;
            }
            
            name = trimmed + 5;
            while (isspace(*name)) name++;
            name_end = name;
            while (*name_end && !isspace(*name_end)) name_end++;
            *name_end = '\0';
            
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
            
            strncpy(macro_name, name, MAX_MACRO_NAME);
            macro_name[MAX_MACRO_NAME] = '\0';
            in_macro = 1;
            macro_content = malloc(macro_content_size);
            if (!macro_content) {
                error = 1;
                break;
            }
            macro_content[0] = '\0';
            macro_content_len = 0;
        }
        else if (strcmp(trimmed, "mcroend") == 0) {
            if (!in_macro) {
                continue;
            }
            
            if (!add_macro(&macro_table, macro_name, macro_content)) {
                fprintf(stderr, "Error(line %d): Duplicate macro '%s'\n", line_number, macro_name);
                error = 1;
                free(macro_content);
                break;
            }
            in_macro = 0;
            free(macro_content);
            macro_content = NULL;
        }
        else if (in_macro) {
            size_t line_len;

            line_len = strlen(trimmed);
            if (macro_content_len + line_len + 2 > macro_content_size) {
                macro_content_size *= 2;
                new_content = realloc(macro_content, macro_content_size);
                if (!new_content) {
                    free(macro_content);
                    error = 1;
                    break;
                }
                macro_content = new_content;
            }
            strcat(macro_content, trimmed);
            strcat(macro_content, "\n");
            macro_content_len += line_len + 1;
        }
        else {
            char *content;

            content = get_macro_content(&macro_table, trimmed);
            if (content) {
                fputs(content, output);
            } else {
                fputs(line, output);
            }
        }
    }

    if (in_macro) {
        fprintf(stderr, "Error(line %d): Unclosed macro definition\n", line_number);
        error = 1;
        free(macro_content);
    }

    free_macro_table(&macro_table);
    fclose(input);
    fclose(output);
    
    if (error) {
        remove(output_filename);
        return 0;
    }
    
    return 1;
}