/*
Authors:
Guy Komari
Gil Mansharov
*/

#ifndef SYMBOLS_LIST_H
#define	SYMBOLS_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "errors.h"
#include "globalVariables.h"

typedef struct symbolsTableNode* symbolPtr;

typedef struct symbolsTableNode {
	char* name;
	uint address;
	bool isExternal;
	bool isCommand;
	bool isData;
	bool isEntry;
	symbolPtr next;
} symbolsTableNode;


/*
Description:
gets the head and tail of the symbols list, the symbol name, its destinated address, and  4 booleans that indicates whether
the symbol is an external, command, data, or entry.
*/
bool addToSymbolsList(symbolPtr*, symbolPtr*, char*, int, bool, bool, bool, bool);


/*
Description:
gets a node as a pointer to symbolPtr struct, the head and tail of the symbols list, and adds the node to the symbols list.
*/
void addNodeToSymbolList(symbolPtr*, symbolPtr*, symbolPtr*);

/*
Description:
frees the symbols list dynamically allocated memory.
*/
void freeSymbolsList(symbolPtr *head);

/*
Description:
gets the symbols list head, and the value of IC, and updates the addresses of the symbols that stored in the symbols list.
*/
void updateDataSymbols(symbolPtr*, int);

/*
Description:
gets the symbols list head, and prints all of its nodes.
*/
void printSymbolsList(symbolPtr *);

#endif
