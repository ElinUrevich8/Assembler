/*
Authors:
Guy Komari
Gil Mansharov
*/

#ifndef FIRST_PASS_H
#define FIRST_PASS_H
#include "GlobalsFunctions.h"

/*
scanning line and sending it to the proper function in the "parser" module.
*/

bool firstpass(char*);

bool ParseData(dataPtr*, dataPtr*, char *, bool, char*);

bool AddExternLabelToSymbolsList(char *line);

bool parseCommand(char *line, bool, char *);


#endif
