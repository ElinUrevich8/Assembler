/*
Authors:
Guy Komari
Gil Mansharov
*/

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "firstPass.h"
#include "GlobalsFunctions.h"

/*Description: start the first pass, returns true if didnt find errors*/
bool firstpass(char*);

/*Description: start the first pass, returns true if didnt find errors*/
bool secondPass(char*);

void runAssembler(char*);

#endif
