/*====================================================================
 *  assembler.c  –  facade layer: pre-assembler -> Pass-1 -> Pass-2
 *====================================================================*/
#include "assembler.h"
#include "defaults.h"
#include "nameset.h"
#include "preassembler.h"
#include "pass1.h"
#include "pass2.h"       /* Pass-2 types + API */
#include "output.h"      /* .ob/.ent/.ext writers */
#include "errors.h"      /* errors_count / errors_print */
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global identifier set: enforces "macro names must not clash with label names". */
NameSet *g_used_names = NULL;

/* Build “<base><ext>” safely into 'out'. */
static void make_path(char *out, size_t cap, const char *base, const char *ext) {
    /* Simple, C90-safe snprintf replacement */
    size_t bl = strlen(base), el = strlen(ext);
    if (bl + el + 1 > cap) { /* truncate conservatively */
        bl = (cap > el + 1) ? (cap - el - 1) : 0;
    }
    memcpy(out, base, bl);
    memcpy(out + bl, ext, el + 1);
}

bool assemble_file(const char *base_path)
/*----------------------------------------------------------------------*/
{
    char as_path[PATH_MAX];           /* <base>.as */
    char am_path[PATH_MAX];           /* <base>.am */
    char path_ob[PATH_MAX];           /* <base>.ob */
    char path_ent[PATH_MAX];          /* <base>.ent */
    char path_ext[PATH_MAX];          /* <base>.ext */
    FILE *fp;

    /* Some trees define EXT_O by mistake; fall back to .ob if EXT_OB is missing */
#ifndef EXT_OB
#define EXT_OB ".ob"
#endif

    DEBUG("Assembling file: %s", base_path);

    make_path(as_path, sizeof as_path, base_path, EXT_AS);
    make_path(am_path, sizeof am_path, base_path, EXT_AM);
    make_path(path_ob, sizeof path_ob, base_path, EXT_OB);
    make_path(path_ent, sizeof path_ent, base_path, EXT_ENT);
    make_path(path_ext, sizeof path_ext, base_path, EXT_EXT);

    /* Init the shared identifier set */
    g_used_names = (NameSet*)malloc(sizeof *g_used_names);
    if (!g_used_names) { perror("NameSet OOM"); return false; }
    ns_init(g_used_names);

    /* ---------- Stage 0: pre-assembler ---------- */
    if (!preassemble(as_path, am_path)) {
        ns_free(g_used_names, NULL); free(g_used_names);
        return false;
    }

    /* ---------- Stage 1 ---------- */
    {
        Pass1Result p1;
        Pass2Result p2;
        int ok;

        memset(&p1, 0, sizeof p1);
        if (!pass1_run(am_path, &p1) || !p1.ok) {
            errors_print(&p1.errors, as_path);
            pass1_free(&p1);
            ns_free(g_used_names, NULL); free(g_used_names);
            return false;
        }
        DEBUG("Pass-1 OK: IC=%d, DC=%d", p1.ic, p1.dc);

        /* ---------- Stage 2 ---------- */
        memset(&p2, 0, sizeof p2);
        ok = pass2_run(am_path, &p1, &p2);
        if (!ok || !p2.ok || errors_count(p2.errors) > 0) {
            if (p2.errors) errors_print(p2.errors, as_path);
            pass2_free(&p2);
            pass1_free(&p1);
            ns_free(g_used_names, NULL); free(g_used_names);
            return false;      /* do not emit any files on failure */
        }

        /* .ob is always written on full success */
        fp = fopen(path_ob, "w");
        if (fp) {
            output_write_ob(fp, &p1, &p2);
            fclose(fp);
        }

        /* Only create .ext/.ent if they have content */
        if (p2.ext_len > 0) {
            fp = fopen(path_ext, "w");
            if (fp) { output_write_ext(fp, &p2); fclose(fp); }
        }
        if (p2.ent_len > 0) {
            fp = fopen(path_ent, "w");
            if (fp) { output_write_ent(fp, &p2); fclose(fp); }
        }

        pass2_free(&p2);
        pass1_free(&p1);
    }

    /* Optional: remove(am_path); */
    ns_free(g_used_names, NULL);
    free(g_used_names);
    return true;
}
