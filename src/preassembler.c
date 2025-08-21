/*============================================================================
 *  preassembler.c
 *
 *  Stage 0 (Preassembler): expand macros and write <base>.am
 *
 *  Responsibilities:
 *    - Parse mcro/mcroend blocks, validate macro names (not reserved, proper
 *      identifier), and collect the macro body text.
 *    - Expand macro invocations by inlining their stored body.
 *    - Preserve non-macro lines (after stripping comments & trimming spaces).
 *    - Share macro names with the global NameSet so later passes can optionally
 *      enforce a unified identifier namespace (macros vs. labels).
 *    - Emit diagnostics with line numbers and fail-fast on fatal errors.
 *
 *  Notes:  
 *    - On any error at this stage, the output .am is removed and 'false' is returned.
 *============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "defaults.h"     /* MAX_LINE_LEN / MAX_LABEL_LEN / bool   */
#include "nameset.h"      /* global identifier set                 */
#include "macro.h"        /* MacroTable API                        */
#include "preassembler.h" /* prototype                             */
#include "debug.h"
#include "encoding.h"     /* strip_comment_inplace                 */
#include "assembler.h"    /* brings: extern NameSet *g_used_names  */
#include "identifiers.h"  /* is_valid_macro_name / is_reserved_identifier */

/*----------------------------------------------------------------------------
 * preassemble(in_path, out_path):
 *   Expand macros from 'in_path' (the .as file) and write the expanded source
 *   to 'out_path' (the .am file).
 *
 *   Returns:
 *     true  on success (output written),
 *     false on failure (diagnostics printed; output file is removed).
 *----------------------------------------------------------------------------*/
bool preassemble(const char *in_path, const char *out_path)
{
    FILE       *in  = NULL;  /* input .as stream  */
    FILE       *out = NULL;  /* output .am stream */
    MacroTable  macros;      /* symbol table for macros during this run */

    /* Line buffer for reading source; +2 for newline + NUL terminator */
    char  line[MAX_LINE_LEN + 2];

    /* Macro construction state */
    char  *macro_buf = NULL;               /* grows with realloc while recording a macro body */
    size_t buf_cap   = 0;                  /* capacity of macro_buf */
    size_t buf_len   = 0;                  /* current length of accumulated body text */
    char   macro_name[MAX_LABEL_LEN + 1];  /* holds name of macro being defined */

    /* State flags / counters */
    bool   in_macro = false;   /* true while recording inside mcro..mcroend */
    bool   error    = false;   /* sticky error flag → triggers cleanup */
    int    line_no  = 0;       /* current input line number (for diagnostics) */

    /* Try to open input and output files */
    in = fopen(in_path, "r");
    if (!in) { perror(in_path); return false; }

    out = fopen(out_path, "w");
    if (!out) { perror(out_path); fclose(in); return false; }

    macro_init(&macros);

    /* === Main line-reading loop === */
    while (fgets(line, sizeof line, in)) {
        /* Decls first (C90): */
        char *trim;                 /* working pointer to trimmed line */
        const char *body = NULL;    /* potential macro body for expansion */

        ++line_no;
        strip_comment_inplace(line);

        /* Reject physical lines longer than MAX_LINE_LEN */
        if (strlen(line) > MAX_LINE_LEN && line[MAX_LINE_LEN] != '\n') {
            fprintf(stderr, "Error(line %d): line exceeds %d characters\n",
                    line_no, MAX_LINE_LEN);
            error = true; break;
        }

        /* Trim leading/trailing whitespace into 'trim' */
        trim = line;
        while (isspace((unsigned char)*trim)) ++trim;
        {
            char *end; 
            end = trim + strlen(trim);
            while (end > trim && isspace((unsigned char)end[-1])) --end;
            *end = '\0';
        }

        /* Empty or pure comment → preserve original line */
        if (*trim == '\0' || *trim == ';') {
            fputs(line, out);
            continue;
        }

        /* -- Macro definition start ("mcro <name>") -- */
        if (strncmp(trim, "mcro", 4) == 0 && isspace((unsigned char)trim[4])) {
            const char *p;
            size_t nlen;

            /* Extract macro name */
            p = trim + 4;
            while (isspace((unsigned char)*p)) ++p;
            if (*p == '\0') {
                fprintf(stderr, "Error(line %d): missing macro name\n", line_no);
                error = true; break;
            }

            nlen = 0;
            while (p[nlen] && !isspace((unsigned char)p[nlen])) ++nlen;
            if (nlen > MAX_LABEL_LEN) {
                fprintf(stderr, "Error(line %d): macro name too long\n", line_no);
                error = true; break;
            }
            strncpy(macro_name, p, nlen);
            macro_name[nlen] = '\0';

            if (!is_valid_macro_name(macro_name)) {
                fprintf(stderr, "Error(line %d): illegal macro name '%s'\n",
                        line_no, macro_name);
                error = true; break;
            }

            /* Prepare body buffer for accumulation */
            buf_cap   = 128;
            buf_len   = 0;
            macro_buf = (char*)malloc(buf_cap);
            if (!macro_buf) { perror("malloc"); error = true; break; }
            macro_buf[0] = '\0';
            in_macro  = true;
            continue;
        }

        /* -- Macro definition end ("mcroend") -- */
        if (strcmp(trim, "mcroend") == 0) {
            if (!in_macro) {
                fprintf(stderr, "Error(line %d): 'mcroend' without 'mcro'\n", line_no);
                error = true; break;
            }
            if (!macro_define(&macros, macro_name, macro_buf, line_no)) {
                /* macro_define already printed error (duplicate/OOM) */
                error = true; break;
            }

            /* Best-effort: record macro name in the global NameSet (non-fatal).
             * Rationale:
             *   - Macros allow '_' while labels do not; some NameSet policies may
             *     reject valid macro names.
             *   - The NameSet can persist across files/tests; duplicates there
             *     must not invalidate a successful macro definition.
             *   - True resource failures (OOM) would already be flagged elsewhere.
             */
            if (g_used_names) {
                (void)ns_add(g_used_names, macro_name); /* ignore boolean result */
            }

            free(macro_buf); macro_buf = NULL;
            in_macro = false;
            continue;
        }

        /* -- Inside macro body: accumulate line into buffer -- */
        if (in_macro) {
            size_t need;     /* required capacity for next append */
            size_t new_cap;  /* candidate new capacity */
            char *tmp;       /* realloc scratch */

            need = buf_len + strlen(trim) + 2; /* space for '\n' + NUL */
            if (need > buf_cap) {
                new_cap = buf_cap * 2;
                if (new_cap < need) new_cap = need;
                tmp = (char*)realloc(macro_buf, new_cap);
                if (!tmp) { perror("realloc"); error = true; break; }
                macro_buf = tmp;
                buf_cap   = new_cap;
            }
            strcat(macro_buf, trim);
            strcat(macro_buf, "\n");
            buf_len += strlen(trim) + 1;
            continue;
        }

        /* -- Normal line: expand macro if it matches a known name -- */
        body = macro_lookup(&macros, trim);
        if (body) {
            fputs(body, out);
        } else {
            fputs(line, out);
            if (line[strlen(line) - 1] != '\n')
                fputc('\n', out);
        }
    } /* end while loop */

    /* Error: reached EOF while still in a macro */
    if (in_macro && !error) {
        fprintf(stderr, "Error: unclosed macro '%s'\n", macro_name);
        error = true;
    }

    /* Cleanup */
    macro_free(&macros);
    if (macro_buf) free(macro_buf);
    fclose(in);
    fclose(out);

    if (error) {
        remove(out_path); /* Rmove a partial .am file */
        return false;
    }
    return true;
}
