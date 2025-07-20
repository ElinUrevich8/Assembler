/*
Authors:
Guy Komari
Gil Mansharov
*/

#ifndef DATA_LIST_H
#define DATA_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globalVariables.h"
#include "errors.h"

typedef enum {
	positiveNumber,
	negativeNumber,
	character,
	eof
} dataType;

typedef struct dataTableNode* dataPtr;

typedef struct dataTableNode {
	int dataCounter;
	dataType type;
	int asciiCode;
	dataPtr next;
}dataTableNode;


/*
Description:
Gets the data list head and tail, a string, and the value of DC, and adds the string to the data list (as a null terminated string).
*/
int addStringToData(dataPtr*, dataPtr*, char*, int);

/*
Description:
Gets the data list head and tail, the value of DC, the type of the data, and the value to add, as a number (if the type is a char,
it should be casted to char according to its value in the ascii table).
*/
bool addToDataList(dataPtr *head, dataPtr *tail, int dc, int dType, int ascii);

/*
Description:
Gets a node as a pointer to a symbolPtr struct, and the head and tail of the data list, and adds the node to the data list.
*/
void addNodeToDataList(dataPtr*, dataPtr*, dataPtr*);

/*
Description:
Gets the head of the data list, and Frees the dynamically allocated memory that has been stored in the data list.
*/
void freeDataList(dataPtr*);

/*
Description:
Gets the head of the data list, and prints all of its nodes.
*/
void printDataList(dataPtr*);

#endif
