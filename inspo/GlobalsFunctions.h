/*
Authors:
Guy Komari
Gil Mansharov
*/
#ifndef GLOBAL_FUNCTIONS_H
#define GLOBAL_FUNCTIONS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "dataList.h"
#include "symbolsList.h"
#include "globalVariables.h"
#include "errors.h"

/*Description: is a valid label */
bool checkLabel(char *, char*);

/*Description: is a valid register*/
bool isRegister(char*);

/*Description: is a valid opcode*/
bool isOpcode(char*);

/*
Description: gets a token and returns TRUE if it's a valid label. otherwise, returns FALSE.
*/
bool isValidLabel(char*);

/*
Description: gets an operand and returns TRUE if it's a number operand. otherwise, returns FALSE.
*/
bool isNumOperand(char*);

/*
Description: gets an operand and returns TRUE if it's a struct with a dot, and afterwards the number 1 or 2.
otherwise, returns FALSE.
*/
bool isStructWithDotOperand(char*);

/*
Description: gets an operand and returns it's addressing mode. if the operand is invalid, returns -1.
*/
AddressingMode getOperandAddressing(char*);

/*
Description: Gets a line to check if it has a label definition,
if true - returns TRUE and insert the label to the destination string, otherwise, returns FALSE.
*/
bool isLabel(char*, char*);

/*
Description: gets a valid label and checks if this label already defined
*/
bool isLabelDefined(symbolPtr*, char*);

/*
Description: gets an line and returns TRUE if it's an entry line declaration. otherwise, returns FALSE.
*/
bool isEntry(char*);

/*
Description: gets an line and returns TRUE if it's an extern line declaration. otherwise, returns FALSE.
*/
bool isExtern(char*);

/*
Description: is an empty sentence
*/
bool isEmptySentence(char*);

/*
Description: is a comment sentence - begins with ';'
*/
bool isComment(char*);

/*
Description: gets a token and returns TRUE if its a keyword. otherwise, returns FALSE.
*/
bool isKeyword(char*);

/*
Description: gets a file path and returns TRUE if the file exists and can be opened for reading. otherwise, returns FALSE.
*/
bool isFileExists(char*);

/*
Description: gets the head and tail of the data list, the current data counter, and a number, and adds it to the data list.
*/
bool addNumberToDataList(dataPtr*, dataPtr*, int, int);

/*
Description: gets a line of data and a destination string and copies the symbol (.struct, .string or .data)
inside the data line to the destination string.
returns TRUE if .struct, .string or .data symbol is exist (only one of them), if not, returns FALSE.
*/
bool getSymbol(char*, char*, bool);

/*
Description: trim whitespace from a string
*/
char *trimStr(char*);

/*
Description: trim whitespace on right end of a string
*/
char *trimRightStr(char*);

/*
trim whitespaces on left end of a string
*/
char *trimLeftStr(char*);

/*
Description: gets a parsed command (i.e "mov r1, r3") without any spaces or unnecessary characters.
returns the number that IC should be incremented with.
returns 0 when failed.
*/
int getCommandSize(char*);

/*
Description: convert decimal code to binary code
*/
void decimalToBinary(int, int *, int);

/*
Description: convert binary code to "wierd 32 base"
*/
void binaryToWierd(int* binary, char* res);

/*
Description: convert 10 word length represents in binary code to "wierd 32 base"
*/
void decimalToWierd(int num, char* res);

/*
Description: convert a binary code to decimal number
and returns the number
*/
int binaryToDecimal(int *);

/*
Description: checks if a line is a data command (declaration of .string, .data or .struct).
returns TRUE if the line is a data command, otherwise, returns FALSE.
*/
bool isDataCommand(char*);

/*
Description: gets a number operand and returns it as an integer.
*/
int getNumber(char*);

/*
Description: gets a FILE* handle and a destination string, and reads the next line from the file to the string.
*/
bool readLine(FILE*, char*);

/*
Description: reset the variables used in the first pass
*/
void resetLine(char *, char* , char *);

/*
Description: is a valid number
*/
bool isNumber(char *);

/*
Description: is a valid string
*/
bool isString(char *);

/*
Description: is a sentence start with command
*/
bool StartWithCommand(char *);

/*
Description: is a sentence start with data command
*/
bool StartWithDataCommand(char *);


#endif
