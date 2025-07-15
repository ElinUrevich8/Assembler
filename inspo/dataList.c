#include "dataList.h"

bool addToDataList(dataPtr *head, dataPtr *tail, int dc, int dType, int ascii)
{
	dataPtr temp;
	if (ascii >= 512 || ascii <= -512)
		return FALSE;
	temp = (dataPtr)(malloc(sizeof(dataTableNode)));
	if (!temp)
	{
		printError(ALLOCATE_MEMORY_ERROR);
		return FALSE;
	}
	temp->dataCounter = dc;
	temp->type = dType;
	temp->asciiCode = ascii;
	temp->next = NULL;
	addNodeToDataList(&temp, head, tail);
	return TRUE;
}


int addStringToData(dataPtr *dataListHead, dataPtr *dataListTail, char *str, int dc)
{
	int i, length;
	length = strlen(str);
	for (i = 0; i < length; i++)
	{
		addToDataList(dataListHead, dataListTail, dc, character, (int)(str[i]));
		dc++;
	}
	addToDataList(dataListHead, dataListTail, dc, character, '\0');
	dc++;
	return dc;
}

void addNodeToDataList(dataPtr *temp, dataPtr *head, dataPtr *tail)
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


void freeDataList(dataPtr *head)
{
	dataPtr temp;
	while (*head)
	{
		temp = *head;
		*head = (*head)->next;
		free(temp);
	}
}

void printDataList(dataPtr *head)
{
	dataPtr temp = *head;
	printf("%s\n", "Data List:");
	while (temp != NULL)
	{
		if (temp->type == character && temp->asciiCode >= 0 && temp->asciiCode < 256)
		{
			if (temp->asciiCode != 0)
				printf(" ('%c' ,%d) --> ", temp->asciiCode, temp->dataCounter);
			else
				printf(" ('\\0' ,%d) --> ", temp->dataCounter);
		}
		else if (temp->type == positiveNumber || temp->type == negativeNumber)
		{
			printf(" (%d ,%d) --> ", temp->asciiCode, temp->dataCounter);
		}
		else
		{
			printf(" NULL ");
		}
		temp = temp->next;
	}
	puts("");
}
