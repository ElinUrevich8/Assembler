/*============================================================================
 *  preassembler.h
 *
 *  Stage-0 driver: expands macros and writes <base>.am
 *  All error printing is done inside; returns true/false only.
 *============================================================================*/
#ifndef PREASSEMBLER_H
#define PREASSEMBLER_H
#include "defaults.h"


/* Function to preassemble an input file into an output file */
bool preassemble(const char *as_path, const char *am_path);

#endif /* PREASSEMBLER_H */