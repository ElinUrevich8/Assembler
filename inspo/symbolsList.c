#include "symbolsList.h"

bool addToSymbolsList(symbolPtr *head, symbolPtr *tail, char* symbol, int addr, bool isEx, bool isC, bool isD, bool isEn)
{
	int i;
	symbolPtr temp;
	symbolPtr* pTemp;
	char* labelName = (char*)(malloc(MAX_LABEL_SIZE + 1));
	if (!labelName)
	{
		printError("Memory allocation failed in symbols list addition");
		return FALSE;
	}
	for (i = 0; i < MAX_LABEL_SIZE + 1; i++)
	{
		labelName[i] = 0;
	}
	strncpy(labelName, symbol, MAX_LABEL_SIZE);
	temp = (symbolPtr)((symbolsTableNode*)(malloc(sizeof(symbolsTableNode))));

	if (!temp)
	{
		fprintf(stderr, "cannot allocate memory in line %d for label %s \n", addr, symbol);
		free(labelName);
		return FALSE;
	}
	temp->name = labelName;
	temp->address = addr;
	temp->isExternal = isEx;
	temp->isCommand = isC;
	temp->isData = isD;
	temp->isEntry = isEn;
	temp->next = NULL;
	pTemp = &temp;
	addNodeToSymbolList(pTemp, head, tail);
	return TRUE;
}


void addNodeToSymbolList(symbolPtr *temp, symbolPtr *head, symbolPtr *tail)
{
	if (*head == NULL)/*case- empty list*/
	{
		*head = *temp;
		*tail = *temp;
		(*temp)->next = NULL;
		return;
	}
	else
	{
		(*tail)->next = *temp;
		*tail = *temp;
		(*temp)->next = NULL;
		return;
	}
}

void updateDataSymbols(symbolPtr *head, int ic)
{
	symbolPtr temp = *head;
	while (temp)
	{
		if ((temp)->isData)
			(temp)->address += ic;
		temp = temp->next;
	}
}

void freeSymbolsList(symbolPtr *head)
{
	symbolPtr temp;
	while (*head)
	{
		temp = *head;
		*head = (*head)->next;
		free(temp->name);
		free(temp);
	}
}

void printSymbolsList(symbolPtr *head)
{
	symbolPtr temp = *head;
	printf("%s\n", "Symbols list:");
	while (temp != NULL)
	{
		printf(" (%s ,%d) --> ", temp->name, temp->address);
		temp = temp->next;
	}
	printf(" NULL \n");
	puts("");
}

