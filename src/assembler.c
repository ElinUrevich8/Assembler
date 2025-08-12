/*====================================================================
 *  assembler.c  –  facade layer: pre-assembler → Pass-1 → Pass-2
 *
 *  Responsibilities
 *  ------------------------------------------------------------------
 *  • Build full file names  <base>.as  <base>.am
 *  • Run the three stages in order:
 *        0) preassemble()   – macro expansion
 *        1) pass1_run()     – symbol table + partial code
 *        2) pass2_run()     – resolve symbols, write outputs
 *  • Maintain one global NameSet (g_used_names) so that macro names
 *    and label names share a single namespace.
 *  • On any failure:  free resources and delete the temporary .am file.
 *====================================================================*/

#include "assembler.h"     /* public assemble_file() prototype         */
#include "defaults.h"      /* bool, PATH_MAX, success / failure codes  */
#include "nameset.h"       /* generic hash-set for identifiers         */
#include "preassembler.h"  /* stage 0                                  */
#include "debug.h"
#include "pass1.h"


#include <stdio.h>
#include <string.h>        /* snprintf                                 */
#include <stdlib.h>        /* malloc/free                             */

/*--------------------------------------------------------------------
 *  Global identifier set – shared by all stages to enforce
 *  “macro names must not clash with label names”.
 *--------------------------------------------------------------------*/
NameSet *g_used_names = NULL;

/*--------------------------------------------------------------------
 *  assemble_file
 *--------------------------------------------------------------------
 *  base_path – file path without extension (e.g. “src/foo”)
 *  Returns    true  on full success, false on any error.
 *--------------------------------------------------------------------*/
bool assemble_file(const char *base_path)
{
    char as_path[PATH_MAX];    /* original source:  <base>.as */
    char am_path[PATH_MAX];    /* after macro expansion: <base>.am */

    DEBUG("Assembling file: %s", base_path);

    /* Build full file names */
    sprintf(as_path, "%s.as", base_path);
    sprintf(am_path, "%s.am", base_path);
    DEBUG("Source file: %s", as_path);
    DEBUG("Temporary file: %s", am_path);

    /* Initialise the shared identifier set */
    g_used_names = malloc(sizeof *g_used_names);
    if (!g_used_names) {
        fprintf(stderr, "Error: Out of memory while initializing NameSet.\n");
        goto error; 
    }

    ns_init(g_used_names);

    /*----------  Stage 0 : Pre-assembler  ----------*/
    if (!preassemble(as_path, am_path)) {
        goto error;
    }

     /*----------  Stage 1 : Pass-1  ----------*/
    {
        Pass1Result p1 = {0};
        if (!pass1_run(am_path, &p1) || !p1.ok) {
            DEBUG("Pass-1 failed for %s", as_path);
            errors_print(&p1.errors, as_path);   /* show collected errors */
            pass1_free(&p1);
            goto error;
        }
        DEBUG("Pass-1 OK: IC=%d, DC=%d", p1.ic, p1.dc);
        pass1_free(&p1);
    }

    /* ----------  Stage 2 : Pass-2  ---------- */
    Pass2Result p2;
    char path_ob[PATH_MAX], path_ent[PATH_MAX], path_ext[PATH_MAX];
    /* build <base>.ob/.ent/.ext into the buffers ... */

    if (!pass2_run(am_path, &p1, &p2) || !p2.ok || errors_count(p2.errors) > 0) {
        errors_print(p2.errors, source_name);  
        pass2_free(&p2);
        /* on failure, DO NOT emit any files */
        /* ... clean and return ... */
    }

    /* .ob is always written on success */
    if ((fp = fopen(path_ob, "w")) != NULL) {
        output_write_ob(fp, &p1, &p2);
        fclose(fp);
    }

    /* Only create .ext/.ent if there is content */
    if (p2.ext_len > 0 && (fp = fopen(path_ext, "w")) != NULL) {
        output_write_ext(fp, &p2);
        fclose(fp);
    }
    if (p2.ent_len > 0 && (fp = fopen(path_ent, "w")) != NULL) {
        output_write_ent(fp, &p2);
        fclose(fp);
    }

    pass2_free(&p2);
    goto success;

error:
    DEBUG("Pre-assembly failed for %s", as_path);
    ns_free(g_used_names, NULL);
    free(g_used_names);
    return false;

success:
    DEBUG("Pre-assembly successful, output: %s", am_path);
    /* remove(am_path); */
    ns_free(g_used_names, NULL);
    free(g_used_names);
    return true;
}
