/*============================================================================
 *  preassembler.h
 *
 *  Stage 0 — expand macros and write <base>.am
 *
 *  Responsibilities:
 *    - Parse and store macro definitions (`mcro name` ... `mcroend`)
 *    - Expand macro calls inline
 *    - Strip comments/blank lines
 *
 *  Errors are reported internally; return value is true/false only.
 *============================================================================*/
#ifndef PREASSEMBLER_H
#define PREASSEMBLER_H

#include "defaults.h"

/* Expand macros from input .as (as_path) and write the resulting .am (am_path). */
bool preassemble(const char *as_path, const char *am_path);

#endif /* PREASSEMBLER_H */
