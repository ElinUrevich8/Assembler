#include "firstPass.h"

/*variables declaration*/
symbolPtr symbolListHead;
symbolPtr symbolListTail;
dataPtr dataListHead;
dataPtr dataListTail;
long IC;
long DC;

/*
Description: the main function of the first pass.
The first pass purpose is to:
1. Notify of compilation errors.
2. Parses from the input file the commands and their sizes and updates IC.
3. Parses from the input file the labels, entries, and externals, and updates DC.
*/
bool firstpass(char* filename)
{
	bool is_label, is_data_command, endFile, is_entry, is_extern, symbolFlag, noErrorsFlag, defined_label, correctLabel;
	FILE* sourceFileHandle;

	char line[MAX_LINE_LENGTH + 1] = { 0 };
	char labelName[MAX_LINE_LENGTH + 1] = { 0 };
	char data[MAX_LINE_LENGTH + 1] = { 0 };

	symbolListHead = NULL;
	symbolListTail = NULL;
	dataListHead = NULL;
	dataListTail = NULL;
	IC = IC_START; /* = 100 */
	DC = DC_START;/* = 0 */

	is_label = is_data_command = endFile = is_entry = is_extern = symbolFlag = defined_label = correctLabel = FALSE;
	noErrorsFlag = TRUE;
	lineCounter = 0;

	/*open the input file*/
	sourceFileHandle = fopen(filename, "r");
	if (sourceFileHandle == NULL)
	{
		printError(OPEN_FILE_ERROR);
		return FALSE;
	}
	/*parse every line in the file*/
	while ((endFile = readLine(sourceFileHandle, line)))
	{
		is_label = is_extern = is_entry = is_data_command = FALSE;
		if (isComment(line) || isEmptySentence(line))/*case - comment or empty sentence*/
		{
			resetLine(line, labelName, data);
			continue;
		}
		if ((is_entry = (isEntry(line))))/*case - entry sentence*/
		{
			is_label = isLabel(line, labelName);
			noErrorsFlag&= is_label;
			resetLine(line, labelName, data);
			continue;
		}

		is_extern = isExtern(line);/*is extern sentence*/
		is_label = isLabel(line, labelName);/*checks if there is a valid label definition*/
		if (is_label && (isKeyword(labelName)|| (isRegister(labelName))))/*checks if the label is not a keyword or a register*/
		{
			if(isKeyword(labelName))
				printError(LABEL_NAME_EQUALS_KEYWORD_NAME);
			else if(isRegister(labelName))
				printError(LABEL_NAME_EQUALS_REGISTER_NAME);
			resetLine(line, labelName, data);
			continue;
		}
		if (!is_extern && (is_data_command = getSymbol(line, data, is_label)))/*case - a data definition*/
		{
			if (is_label)/*case - the data definition are inside a label-> LABEL: .string "abcd" /... */
			{
				defined_label = isLabelDefined(&symbolListHead, labelName);
				noErrorsFlag &= !defined_label; /*if label already defined then there is an error*/
				if (!defined_label)
				{
					/*addToSymbolsList (head, tail, label name, address, isExternal, isCommand, isData, isEntry)*/
					noErrorsFlag &= addToSymbolsList(&symbolListHead, &symbolListTail, labelName, DC, FALSE, FALSE, TRUE, FALSE);
				}
			}
			if (!is_label & !StartWithDataCommand(line))/*case - if not a valid data definition */
			{
				printError(INVALID_LABEL_OR_OPCODE_DECLARATION);
				noErrorsFlag = FALSE;
			}
			else/*parse the data command*/
			{
				noErrorsFlag &= ParseData(&dataListHead, &dataListTail, line, is_label, labelName);
			}
		}
		else if (is_extern)/*case - there is an extern declaration*/
		{
			if(is_label)/*with a label*/
				noErrorsFlag &= AddExternLabelToSymbolsList(labelName);
			else/* without a label */
			{
				resetLine(line, labelName, data);
				continue;
			}
		}
		else if(!is_data_command)/* case - command declaration */
		{
			if (is_label)/* case - command inside a label*/
			{
				defined_label = isLabelDefined(&symbolListHead, labelName);
				noErrorsFlag &= !defined_label; /*if label already defined then there is an error*/
				noErrorsFlag &= parseCommand(line, is_label, labelName);
			}
			else if (!is_label & !StartWithCommand(line))/*case - if not a valid command definition */
			{
				printError(INVALID_LABEL_OR_OPCODE_DECLARATION);
				noErrorsFlag = FALSE;
			}
			else/*parse the command*/
			{
				noErrorsFlag &= parseCommand(line, is_label, labelName);/*counter lines for the code(IC) and finds errors
																		does not encoding the commands, we do it in the
																		second pass*/
			}
		}
		resetLine(line, labelName, data);
	}

	if (!noErrorsFlag)
	{
		freeSymbolsList(&symbolListHead);
		freeDataList(&dataListHead);
		return FALSE;
	}
	updateDataSymbols(&symbolListHead, IC);
	fclose(sourceFileHandle);
	return TRUE;
}


/*
Description: parses the data and inserts it to the data list, and updates DC.
Returns true if no errors were found. 
*/
bool ParseData(dataPtr *dataListHead, dataPtr *dataListTail, char *line, bool is_label, char *labelName)
{
	bool first, hasError;
	int i, dataLength, strLength, numRequiredBytes, count;
	char *token, *temp, *dataTempBuffer;
	char stringBuffer[MAX_LINE_LENGTH + 1] = { 0 };
	char symbolType[MAX_LINE_LENGTH + 1] = { 0 };
	char *stringSymbol = DataCommand[0], *dataSymbol = DataCommand[1], *structSymbol = DataCommand[2];
	const int stringSymbolLength = DataCommandLength[0], dataSymbolLength = DataCommandLength[1], structSymbolLength = DataCommandLength[2];
	i = dataLength = strLength = numRequiredBytes = count = 0;
	dataLength = strlen(line);
	dataTempBuffer = (char*)malloc(dataLength + 1);
	first = TRUE;
	hasError = FALSE;
	if (!dataTempBuffer)
	{
		printError(ALLOCATE_MEMORY_ERROR);
		return FALSE;
	}
	strncpy(dataTempBuffer, line, dataLength + 1);
	if (getSymbol(dataTempBuffer, symbolType, is_label) == FALSE)/*get data type*/
	{
		free(dataTempBuffer);
		return FALSE;
	}
	temp = dataTempBuffer;
	if(is_label)
		temp += strlen(labelName) + 1;

	temp = trimLeftStr(temp);
	if (strncmp(symbolType, stringSymbol, stringSymbolLength) == 0)/* .string */
	{
		temp += stringSymbolLength;
		temp = trimStr(temp);
		if (*temp != '\0' && !isString(temp))
		{
			printError(INVALID_CHARACTERS);
			free(dataTempBuffer);
			return FALSE;
		}
		strcpy(dataTempBuffer, temp);
		token = strtok(dataTempBuffer, "\"");
		if (token == NULL)
		{
			printError(MISSING_ARGUMENTS_IN_DIRECTIVE);
			hasError = TRUE;
		}
		else
		{
			strcpy(stringBuffer, token);
			DC = addStringToData(dataListHead, dataListTail, stringBuffer, DC);
			free(dataTempBuffer);
			return TRUE;
		}
	}
	else if (strncmp(symbolType, dataSymbol, dataSymbolLength) == 0)/* .data */
	{
		temp += dataSymbolLength;
		temp = trimLeftStr(temp);
		token = strtok(temp, ",");
		if (token == NULL)
		{
			printError(MISSING_ARGUMENTS_IN_DIRECTIVE);
			hasError = TRUE;
		}
		else
		{
			while (token != NULL)
			{
				if (!(isNumber(token)))
				{
					printError(NOT_NUMBER_IN_DATA);
					hasError = TRUE;
				}
				else if (!hasError)
				{
					i = atoi(trimStr(token));
					if (i != 0 || (strcmp(token, "0") == 0))
					{
						if (!addNumberToDataList(dataListHead, dataListTail, DC, i))
						{
							free(dataTempBuffer);
							return FALSE;
						}
						DC++;
					}
					else
					{
						printError(INVALID_NUMBER_IN_DATA_LABEL);
					}
				}
				token = strtok(NULL, ",");
			}
		}

	}
	else if (strncmp(symbolType, structSymbol, structSymbolLength) == 0)/* .struct */
	{
		count = 0;
		temp += structSymbolLength;
		temp = trimLeftStr(temp);
		token = strtok(temp, ",");
		if (token == NULL)
		{
			printError(MISSING_ARGUMENTS_IN_DIRECTIVE);
			hasError = TRUE;
		}
		else
		{
			while (token != NULL)
			{
				count++;
				if (count > 2)
					break;
				if (first)
				{
					if (!(isNumber(token)))
					{
						printError(SORCE_OPERAND_NOT_NUMBER_IN_STRUCT);
						hasError = TRUE;
					}
					first = FALSE;
				}
				else
				{
					if (!isString(token))
					{
						printError(DESTINATION_OPERAND_NOT_STRING_IN_STRUCT);
						hasError = TRUE;
					}
				}
				if (!hasError)
				{
					token = trimLeftStr(token);
					i = atoi(token);
					if (i != 0 || (strcmp(token, "0") == 0))
					{
						if (!addNumberToDataList(dataListHead, dataListTail, DC, i))
						{
							free(dataTempBuffer);
							return FALSE;
						}
						DC++;
					}
					else
					{
						strLength = strlen(token);
						if (strLength > 2 && token[0] == '"' && token[strLength - 1] == '"')
						{
							strncpy(stringBuffer, token + 1, strLength - 2);
							addStringToData(dataListHead, dataListTail, stringBuffer, DC);
							DC += strLength - 1;
						}
						else if (strLength == 3 && token[0] == '\'' && token[2] == '\'')
						{
							addToDataList(dataListHead, dataListTail, DC, character, (int)(token[1]));
							DC++;
						}
						else
						{
							printError(STRUCT_DEFINITION_ERROR);
						}
					}
				}
				token = strtok(NULL, ",");
			}
			if (count < NUM_OF_STRUCT_ARGUMENTS)
			{
				printError(MISSING_ARGUMENTS_IN_DIRECTIVE);
				hasError = TRUE;
			}
			else if (count > NUM_OF_STRUCT_ARGUMENTS)
			{
				printError(TOO_MUCH_WORDS_IN_STRUCT);
				hasError = TRUE;
			}

		}
	}
	else
	{
		free(dataTempBuffer);
		return FALSE;
	}
	free(dataTempBuffer);
	if (hasError)
		return FALSE;
	return TRUE;
}


/*
Description: gets a line that contains an extern label (.extern) and adds it to the symbols list.
Returns false if the label already defined.
*/
bool AddExternLabelToSymbolsList(char *label)
{
	char* temp = label;
	/*search label*/
	if (isLabelDefined(&symbolListHead, label))
	{
		return FALSE;
	}
	return addToSymbolsList(&symbolListHead, &symbolListTail, temp, 0, TRUE, FALSE, FALSE, FALSE);
}


/*
gets a line with a command (for example "mov r1, r2"), and if it has a label, adds it to the symbols list.
afterwards, calculates the size of the command in memory, and increments IC with the command size.
Returns true if no errors were found. 
*/
bool parseCommand(char *line, bool label, char* labelName)/*with or without label*/
{
	int sizeOfCommand = 0;
	char *temp = line;
	temp = trimStr(line);
	if (label)
	{
		temp += strlen(labelName) + 1;
		addToSymbolsList(&symbolListHead, &symbolListTail, labelName, IC, FALSE, TRUE, FALSE, FALSE);
	}
	temp = trimStr(temp);
	sizeOfCommand = getCommandSize(temp);
	IC += sizeOfCommand;
	return (sizeOfCommand != 0) ? TRUE : FALSE;
}
