/*============================================================================
 *  preassembler.c
 *
 *  Stage-0 driver: expands macros and writes <base>.am
 *  All diagnostics are printed here; returns true on success, false on error.
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
#include "encoding.h"
#include "assembler.h"    /* brings: extern NameSet *g_used_names */
#include "identifiers.h"  /* is_valid_macro_name / is_reserved_identifier */


bool preassemble(const char *in_path, const char *out_path)
/*----------------------------------------------------------------------------*/
{
    FILE       *in  = NULL;
    FILE       *out = NULL;
    MacroTable  macros;

    char  line[MAX_LINE_LEN + 2];       /* +2 for '\n' + NUL            */
    char *macro_buf   = NULL;           /* accumulates macro body       */
    size_t buf_cap    = 0, buf_len = 0;
    char   macro_name[MAX_LABEL_LEN + 1];

    bool   in_macro   = false;          /* <<< was int, now bool */
    bool   error      = false;          /* <<< was int, now bool */
    int    line_no    = 0;

    /*------------------------------------------------------------------*/
    in = fopen(in_path, "r");
    if (!in) { perror(in_path); return false; }

    out = fopen(out_path, "w");
    if (!out) { perror(out_path); fclose(in); return false; }

    macro_init(&macros);

    /*------------------------------------------------------------------*/
    while (fgets(line, sizeof line, in)) {
        ++line_no;
        strip_comment_inplace(line);

        /* check line length (visible chars > MAX_LINE_LEN) */
        if (strlen(line) > MAX_LINE_LEN &&
            line[MAX_LINE_LEN] != '\n') {
            fprintf(stderr,
                    "Error(line %d): line exceeds %d characters\n",
                    line_no, MAX_LINE_LEN);
            error = true; break;
        }

        /* trim leading / trailing whitespace */
        {
            char *trim = line;
            while (isspace((unsigned char)*trim)) ++trim;
            {
                char *end = trim + strlen(trim);
                while (end > trim && isspace((unsigned char)end[-1])) --end;
                *end = '\0';
            }

            /* empty or comment line -> copy as-is */
            if (*trim == '\0' || *trim == ';') {
                fputs(line, out);
                continue;
            }

            /*----------------------------------------------------------*/
            /* Start of macro definition                                */
            /*----------------------------------------------------------*/
            if (strncmp(trim, "mcro", 4) == 0 && isspace((unsigned char)trim[4])) {
                if (in_macro) {
                    fprintf(stderr,
                            "Error(line %d): nested macro not allowed\n",
                            line_no);
                    error = true; break;
                }

                {
                    const char *p = trim + 4;
                    while (isspace((unsigned char)*p)) ++p;
                    if (*p == '\0') {
                        fprintf(stderr,
                                "Error(line %d): missing macro name\n", line_no);
                        error = true; break;
                    }

                    /* extract name */
                    {
                        size_t nlen = 0;
                        while (p[nlen] && !isspace((unsigned char)p[nlen])) ++nlen;
                        if (nlen > MAX_LABEL_LEN) {
                            fprintf(stderr,
                                    "Error(line %d): macro name too long\n",
                                    line_no);
                            error = true; break;
                        }
                        strncpy(macro_name, p, nlen);
                        macro_name[nlen] = '\0';
                    }
                }

                if (error) break;

                if (!is_valid_macro_name(macro_name)) {
                    fprintf(stderr, "Error(line %d): illegal macro name '%s'\n", line_no, macro_name);
                    error = true; 
                    break;
                }

                /* prepare buffer for body */
                buf_cap = 128;
                macro_buf = malloc(buf_cap);
                if (!macro_buf) { error = true; break; }
                macro_buf[0] = '\0';
                buf_len   = 0;
                in_macro  = true;
                continue;
            }

            /*----------------------------------------------------------*/
            /* End of macro definition                                  */
            /*----------------------------------------------------------*/
            if (strcmp(trim, "mcroend") == 0) {
                if (!in_macro) {
                    fprintf(stderr,
                            "Error(line %d): 'mcroend' without 'mcro'\n", line_no);
                    error = true; break;
                }
                if (!macro_define(&macros, macro_name, macro_buf, line_no)) {
                    error = true; break;   /* duplicate / OOM handled inside */
                }

                /* Share macro name with later passes to enforce single namespace */
                if (g_used_names) {
                    (void)ns_add(g_used_names, macro_name); /* ignore dup result; macro_define already checked */
                }

                free(macro_buf); macro_buf = NULL;
                in_macro = false;
                continue;
            }

            /*----------------------------------------------------------*/
            /* Inside macro: accumulate                                 */
            /*----------------------------------------------------------*/
            if (in_macro) {
                size_t needed = buf_len + strlen(trim) + 2;
                if (needed > buf_cap) {
                    buf_cap *= 2;
                    {
                        char *tmp = realloc(macro_buf, buf_cap);
                        if (!tmp) { error = true; break; }
                        macro_buf = tmp;
                    }
                }
                strcat(macro_buf, trim);
                strcat(macro_buf, "\n");
                buf_len += strlen(trim) + 1;
                continue;
            }

            /*----------------------------------------------------------*/
            /* Normal line: maybe macro call                            */
            /*----------------------------------------------------------*/
            {
                const char *body = macro_lookup(&macros, trim);
                if (body) {
                    fputs(body, out);
                } else {
                    fputs(line, out);
                    if (line[strlen(line) - 1] != '\n')
                        fputc('\n', out);
                }
            }
        } /* end trim-processing block */
    }     /* end while */

    /* EOF reached */
    if (in_macro && !error) {
        fprintf(stderr, "Error: unclosed macro '%s'\n", macro_name);
        error = true;
    }

    /*------------------------------------------------------------------*/
    macro_free(&macros);
    if (macro_buf) free(macro_buf);
    fclose(in);
    fclose(out);

    if (error) {
        remove(out_path);
        return false;
    }
    return true;
}
