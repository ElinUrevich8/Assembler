/*============================================================================
 *  assembler.c  —  Facade for the whole pipeline
 *
 *  Flow:
 *    1) preassemble(<base>.as * <base>.am)
 *    2) pass1_run(.am)  * symbol table, data image, ICF/DC
 *    3) pass2_run(.am)  * final code image, .ent / .ext collections
 *    4) output_write_*  * .ob (always if success), .ent/.ext if non-empty
 *
 *  Policy: on any fatal error, do not emit outputs.
 *============================================================================*/

#include "assembler.h"
#include "defaults.h"
#include "nameset.h"
#include "preassembler.h"
#include "pass1.h"
#include "pass2.h"
#include "output.h"
#include "errors.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global identifier set (macros + labels share one namespace). */
NameSet *g_used_names = NULL;

/* Build "<base><ext>" into 'out' (C90-safe, truncating if needed). */
static void make_path(char *out, size_t cap, const char *base, const char *ext) {
    size_t bl = strlen(base), el = strlen(ext);
    if (bl + el + 1 > cap) bl = (cap > el + 1) ? (cap - el - 1) : 0;
    memcpy(out, base, bl);
    memcpy(out + bl, ext, el + 1);
}

bool assemble_file(const char *base_path)
{
    char as_path[PATH_MAX], am_path[PATH_MAX];
    char path_ob[PATH_MAX], path_ent[PATH_MAX], path_ext[PATH_MAX];
    FILE *fp;

#ifndef EXT_OB
#define EXT_OB ".ob"
#endif

    DEBUG("Assembling: %s", base_path);

    make_path(as_path, sizeof as_path, base_path, EXT_AS);
    make_path(am_path, sizeof am_path, base_path, EXT_AM);
    make_path(path_ob, sizeof path_ob, base_path, EXT_OB);
    make_path(path_ent, sizeof path_ent, base_path, EXT_ENT);
    make_path(path_ext, sizeof path_ext, base_path, EXT_EXT);

    /* Shared name set: enforces macro/label uniqueness across stages. */
    g_used_names = (NameSet*)malloc(sizeof *g_used_names);
    if (!g_used_names) { perror("NameSet OOM"); return false; }
    ns_init(g_used_names);

    /* Stage 0: Macro expansion * .am */
    if (!preassemble(as_path, am_path)) {
        ns_free(g_used_names, NULL); free(g_used_names);
        return false;
    }

    /* Stage 1: symbols + sizing; Stage 2: final emission */
    {
        Pass1Result p1; Pass2Result p2; int ok;

        memset(&p1, 0, sizeof p1);
        if (!pass1_run(am_path, &p1) || !p1.ok) {
            errors_print(&p1.errors, as_path);
            pass1_free(&p1);
            ns_free(g_used_names, NULL); free(g_used_names);
            return false;
        }
        DEBUG("Pass-1 OK: IC=%d, DC=%d", p1.ic, p1.dc);

        memset(&p2, 0, sizeof p2);
        ok = pass2_run(am_path, &p1, &p2);
        if (!ok || !p2.ok || errors_count(p2.errors) > 0) {
            if (p2.errors) errors_print(p2.errors, as_path);
            pass2_free(&p2);
            pass1_free(&p1);
            ns_free(g_used_names, NULL); free(g_used_names);
            return false;
        }

        /* Always write .ob on success */
        if ((fp = fopen(path_ob, "w"))) { output_write_ob(fp, &p1, &p2); fclose(fp); }
        /* Write .ext/.ent only if they have content */
        if (p2.ext_len > 0 && (fp = fopen(path_ext, "w"))) { output_write_ext(fp, &p2); fclose(fp); }
        if (p2.ent_len > 0 && (fp = fopen(path_ent, "w"))) { output_write_ent(fp, &p2); fclose(fp); }

        pass2_free(&p2);
        pass1_free(&p1);
    }

    ns_free(g_used_names, NULL);
    free(g_used_names);
    return true;
}
