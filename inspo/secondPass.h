/*
Authors:
Guy Komari
Gil Mansharov
*/

#ifndef SECOND_PASS_H
#define SECOND_PASS_H

#include "globalVariables.h"
#include "GlobalsFunctions.h"

/*
functions used for the second pass module
*/

bool secondPass(char*);

void createFile(char*, FILE*, char*, char*);

void printToEntryFile(char*);

int getOpcodeGroup(char*);

int getAddressingType(int*);

bool encodingCommand(char*);

bool makeBinaryCode(int, char*, char*, char *);

void makeSingleOperandBinary(char *, int *);

void printToExternFile(char *);

void printIcAndDCWeird();

void printBinaryWord(int *);

void setOpcodeBinaryCode(char *, int *);

void copyBinaryOpcode(int, int *);

bool makeOperandsWeird(int, int *, int *, char *, char *);

void printRegisterWeird(char *, char *, bool, bool);

void copyBinaryRegister(int *, int, char *);

bool printStructWithDotWeird(char *);

void printNumberWeird(char*, char *);

bool printDestOperand(char *, char *, int);

bool printWeirdDataOperand(char *);

void printDataWeird();

bool LabelDeclaredButNotDefined(char *);

void resetFilesName(char *, char *, char *);

void resetSingleLine(char *);

#endif
