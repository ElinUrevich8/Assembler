#include "GlobalsFunctions.h"

bool isRegister(char *token)
{
	int i;

	for (i = 0; i < NUM_OF_REGISTERS; i++)
	{
		if (strcmp(token, Registers[i]) == 0)
			return TRUE;
	}
	return FALSE;
}

bool isOpcode(char *token)
{
	int i = 0;
	for (i = 0; i < NUM_OF_OPCODES; i++)
	{
		if (!(strcmp(token, opcodes[i].opcodeName)))
			return TRUE;
	}
	return FALSE;

}

bool isLabel(char* src, char* dest)
{
	bool res, flag;
	char *temp, *start;
	int len;
	res = TRUE;
	flag = FALSE;
	temp = trimStr(src);
	start = temp;
	len = 0;
	if (strstr(temp, EXTERN_DECLERATION))
	{
		temp = strstr(temp, EXTERN_DECLERATION);
		temp = (char *)(temp + EXTERN_LENGTH);
		temp = trimLeftStr(temp);
		if (checkLabel(src, EXTERN))
		{
			if(dest!='\0')
				strcpy(dest, temp);
			return TRUE;
		}
		return FALSE;
	}
	else if (strstr(temp, ENTRY_DECLERATION))
	{
		temp = strstr(temp, ENTRY_DECLERATION);
		temp = (char *)(temp + ENTRY_LENGTH);
		temp = trimLeftStr(temp);
		if (checkLabel(src, ENTRY))
		{
			if (dest != '\0')
				strcpy(dest, temp);
			return TRUE;
		}
		return FALSE;
	}

	if (temp[0] == '.')/*case - data : .data/.string ...*/
		return FALSE;
	/*else - sentence begins with a label*/
	if (!(isalpha(*temp)))/*label doesnt start with a letter*/
	{
		printError(LABEL_NOT_START_WITH_ALPHA);
		res = FALSE;
	}
	while (*temp && !isspace(*temp) && strncmp((char*)(temp), ":", 1) != 0)
	{
		if (!flag && *temp && !isalpha(*temp) && !isdigit(*temp))
		{
			printError(LABEL_CONTAINS_NON_ALPHA_OR_DIGIT);
			res = FALSE;
			flag = TRUE;
		}
		temp++;
		len++;
	}
	if (strncmp((char*)(temp++), ":", 1) != 0)
	{
		return FALSE;
	}
	if (len > MAX_LABEL_SIZE)
	{
		printError(LABEL_LENGTH_ERROR);
		return FALSE;
	}
	if (dest != NULL)
		strncpy(dest, start, len);
	return res;
}


bool isLabelDefined(symbolPtr *headOfSymbolsList, char* token)
{
	symbolPtr temp = *headOfSymbolsList;
	while (temp)
	{
		if (!(strcmp(token, (temp)->name)))
		{
			printError(LABEL_IS_ALREADY_DEFINED);
			return TRUE;
		}
		temp = (temp)->next;
	}
	return FALSE;
}


bool isEntry(char* line)
{
	char *temp = trimStr(line);
	if (strstr(temp, ENTRY_DECLERATION))
		return TRUE;
	return FALSE;
}


bool isExtern(char* line)
{
	char *temp = trimStr(line);
	if (strstr(temp, EXTERN_DECLERATION))
		return TRUE;
	return FALSE;
}


bool isValidLabel(char* token)
{
	int i = 0, length;
	char* trimmed;
	trimmed = trimStr(token);
	length = strlen(trimmed);
	if (length == 0 || length > 30)
		return FALSE;
	if (trimmed[i] < 0 || trimmed[i] > 255)
		return FALSE;
	if (!isalpha(trimmed[i++]))
		return FALSE;
	for (; i < length; i++)
	{
		if (trimmed[i] < 0 || trimmed[i] > 255 || trimmed[i] == '.')
			return FALSE;
		if (!(isalpha(trimmed[i]) || isdigit(trimmed[i])))
			return FALSE;
	}
	return TRUE;
}



bool checkLabel(char *line , char *type)
{
	int i;
	char* temp;
	bool res = TRUE;
	if (strcmp(type, EXTERN) == 0 || strcmp(type, ENTRY) == 0)
	{
		if (strcmp(type, EXTERN) == 0)
		{
			temp = strstr(line,EXTERN_DECLERATION);
			temp += EXTERN_LENGTH;
			temp = trimStr(temp);
		}
		else/*entry*/
		{
			temp = strstr(line, ENTRY_DECLERATION);
			temp += ENTRY_LENGTH;
			temp = trimStr(temp);
		}
	}
	temp = trimStr(temp);
	if (strlen(temp) == 0)
	{
		printError(NO_LABEL_DECLARED_ERROR);
		return FALSE;
	}
	if (strlen(temp) > MAX_LABEL_SIZE)
	{
		printError(LABEL_LENGTH_ERROR);
		return FALSE;
	}
	if (!isalpha(temp[0]))
	{
		printError(LABEL_NOT_START_WITH_ALPHA);
		res = FALSE;
	}
	for (i = 1; i < strlen(temp); i++)
	{
		if (!(isalpha(temp[i]) || isdigit(temp[i])))
		{
			if (isspace(temp[i]))
			{
				printError(MORE_THEN_ONE_LABEL);
			}
			else
			{
				printError(LABEL_CONTAINS_NON_ALPHA_OR_DIGIT);
			}
			return FALSE;
		}
	}
	return res;
}

bool isNumOperand(char* token)
{
	int  i = 0, num;
	char* trimmed;
	trimmed = trimStr(token);
	if (trimmed[i++] != '#')
		return FALSE;
	num = atoi(trimmed + i);
	if (num == 0)
	{
		if (trimmed[i] == '+' || trimmed[i] == '-')
			i++;
		if (strcmp(trimmed + 1, "0") != 0)
			return FALSE;
	}
	trimmed++;
	if (num > MAX_NUMBER || num < MIN_NUMBER)
	{
		if(num > MAX_NUMBER)
			printExtendedError(OVERFLOW_COMMAND_POSITIVE_NUMBER, trimmed);
		else 
			printExtendedError(OVERFLOW_COMMAND_NEGATIVE_NUMBER, trimmed);
		return FALSE;
	}
	else
		return TRUE;
}


bool isEmptySentence(char* token)
{
	while (*token != '\0')
	{
		if (!isspace((unsigned char)*token))
			return FALSE;
		token++;
	}
	return TRUE;
}

bool isComment(char* token)
{
	char* temp = trimLeftStr(token);
	if (temp[0] == ';')
		return TRUE;
	return FALSE;
}


char *trimLeftStr(char *str)
{
	if (str == NULL)
		return NULL;
	while (*str >= 0 && *str <= 255 && isspace(*str))
		str++;
	return str;
}

char *trimRightStr(char *str)
{
	int length;
	char *back;
	if (str == NULL)
		return NULL;
	length = strlen(str);
	if (length == 0)
		return str;
	back = str + length;
	if (*back == '\0')
	{
		do
		{
			back--;
		} while (*back >= 0 && *back <= 255 && isspace(*back));
	}
	*(back + 1) = '\0';
	return str;
}

char* trimStr(char* str)
{
	char* res = trimRightStr(trimLeftStr(str));
	return res;
}


bool isFileExists(char* fileName)
{
	FILE* fHandle;
	fHandle = fopen(fileName, "r");
	if (fHandle == NULL)
		return FALSE;
	else
		fclose(fHandle);
	return TRUE;
}


bool getSymbol(char* data, char* dest, bool is_label)
{
	bool afterLabel = FALSE, detectedSymbol = FALSE;
	int i, j, length, correctSymbol = -1 ;
	afterLabel = !is_label;
	length = strlen(data);
	for (i = 0; i < length; i++)
	{
		if (afterLabel)
		{
			if (!isspace(data[i]))
			{
				for (j = 0; j < NUM_OF_DATA_TYPES; j++)
				{
					if (strncmp(data + i, DataCommand[j], strlen(DataCommand[j])) == 0)
					{
						if (detectedSymbol)
							return FALSE;
						else
						{
							detectedSymbol = TRUE;
							correctSymbol = j;
							i += strlen(DataCommand[j]) - 1;
							continue;
						}
					}
				}
			}
		}
		else
		{
			if (data[i] == ':')
			{
				if (i != 0)
					afterLabel = TRUE;
				else
					return FALSE;
			}
			else if (!isalpha(data[i]))
			{
				if (i == 0 && !isdigit(data[i]))
					return FALSE;
			}

			else
				continue;
		}
	}

	if (correctSymbol != -1)
	{
		if (dest != NULL)
			strncpy(dest, DataCommand[correctSymbol], strlen(DataCommand[correctSymbol]));
		return TRUE;
	}
	return FALSE;
}

bool addNumberToDataList(dataPtr *head, dataPtr *tail, int dc, int num)
{
	if (num >= 0)
	{
		if (num > MAX_DATA_NUMBER)
		{
			printError(OVERFLOW_DATA_POSITIVE_NUMBER);
			return FALSE;
		}
		addToDataList(head, tail, dc, positiveNumber, num);
	}
	else
	{
		if (num < MIN_DATA_NUMBER)
		{
			printError(OVERFLOW_DATA_NEGATIVE_NUMBER);
			return FALSE;
		}
		addToDataList(head, tail, dc, negativeNumber, num);
	}
	return TRUE;
}



AddressingMode getOperandAddressing(char* token)
{
	if (isNumOperand(token))
		return IMMEDIATE;
	else if (isRegister(token))
		return DIRECT_REGISTER;
	else if (isStructWithDotOperand(token))
		return STRUCT_ACCESS;
	else if (isValidLabel(token))
		return DIRECT_MEMORY;
	else
		return -1;
}


int getCommandSize(char* command)
{
	int i, sizeOfCommand = 0;
	AddressingMode srcOperandAddressing, destOperandAddressing;
	opcodeStructure *opcode;
	char* token, *srcOperand, *destOperand, *tooMuchWordsFlag, *delim = " \t,";
	token = strtok(command, delim);
	for (i = 0; i < NUM_OF_OPCODES; i++)
	{
		if (token == NULL)
			continue;
		if (strcmp(token, opcodes[i].opcodeName) == 0)
		{
			opcode = &(opcodes[i]);

			switch (opcode->group)
			{
				/* 2 Operands Opcode */
			case FIRST_GROUP:
			{
				srcOperand = strtok(NULL, delim);
				destOperand = strtok(NULL, delim);
				tooMuchWordsFlag = strtok(NULL, delim);

				if (srcOperand == NULL)
				{
					printError(TOO_FEW_OPERANDS_IN_COMMAND);
					return FALSE;
				}

				if (isKeyword(srcOperand))
				{
					printError(OPERAND_NAME_EQUALS_KEYWORD_NAME);
					return FALSE;
				}


				if ((srcOperandAddressing = getOperandAddressing(srcOperand)) == -1)
				{
					printError(INVALID_SRC_OPERAND);
					return 0;
				}


				if (destOperand == NULL)
				{
					printError(TOO_FEW_OPERANDS_IN_COMMAND);
					return FALSE;
				}

				if (isKeyword(destOperand))
				{
					printError(OPERAND_NAME_EQUALS_KEYWORD_NAME);
					return FALSE;
				}



				if ((destOperandAddressing = getOperandAddressing(destOperand)) == -1)
				{
					printError(INVALID_DEST_OPERAND);
					return 0;
				}

				if (strcmp(opcode->opcodeName, "lea") == 0 && (srcOperandAddressing == IMMEDIATE || srcOperandAddressing == DIRECT_REGISTER))
				{
					printError(LEA_COMMAND_ADDRESSING_MODE_ERROR);
					return 0;
				}

				if (destOperandAddressing == IMMEDIATE)
				{
					if (strcmp(opcode->opcodeName, "cmp") != 0)
					{
						printError(CMP_COMMAND_ADDRESSING_MODE_ERROR);
						return 0;
					}
				}

				if (tooMuchWordsFlag != NULL)
				{
					printError(TOO_MUCH_WORDS_IN_COMMAND);
					return 0;
				}

				sizeOfCommand++; /*opcode instruction size*/

								 /* For example: "sub r1,r4" */
				if (srcOperandAddressing == DIRECT_REGISTER && destOperandAddressing == DIRECT_REGISTER)
					sizeOfCommand++;

				/* For example: "mov S1.1,S2.2" - requires the address of S1 + the number at the first field in S1 + the address of S2 + the number at the second field in S2 (4 instructions in total)*/
				else if (srcOperandAddressing == STRUCT_ACCESS && destOperandAddressing == STRUCT_ACCESS)
					sizeOfCommand += 4;

				/* For example: "cmp S1.1,#2" - requires the address of S1 + the number at the first field in S1 + the number 2 (3 instructions in total) */
				else if (srcOperandAddressing == STRUCT_ACCESS || destOperandAddressing == STRUCT_ACCESS)
					sizeOfCommand += 3;

				/* All the other cases, for example: "lea STR,r1" - requires the address of STR + the code of the register r1 (2 instructions total) */
				else
					sizeOfCommand += 2;
				return sizeOfCommand;
			}
			break;

			/* 1 Operand Opcode*/
			case SECOND_GROUP:
			{
				destOperand = strtok(NULL, delim);
				tooMuchWordsFlag = strtok(NULL, delim);

				if (destOperand == NULL)
				{
					printError(TOO_FEW_OPERANDS_IN_COMMAND);
					return FALSE;
				}
				if (isKeyword(destOperand))
				{
					printError(OPERAND_NAME_EQUALS_KEYWORD_NAME);
					return FALSE;
				}

				if ((destOperandAddressing = getOperandAddressing(destOperand)) == -1)
				{
					printError(INVALID_DEST_OPERAND);
					return 0;
				}

				if (destOperandAddressing == IMMEDIATE)
				{
					if (strcmp(opcode->opcodeName, "prn") != 0)
					{
						printError(PRN_COMMAND_ADDRESSING_MODE_ERROR);
						return 0;
					}
				}

				if (tooMuchWordsFlag != NULL)
				{
					printError(TOO_MUCH_WORDS_IN_COMMAND);
					return 0;
				}

				sizeOfCommand++;
				if (destOperandAddressing == STRUCT_ACCESS)
					sizeOfCommand += 2;
				else
					sizeOfCommand++;
				return sizeOfCommand;
			}
			break;

			/* No Operands Opcode */
			case THIRD_GROUP:
			{
				tooMuchWordsFlag = strtok(NULL, delim);
				if (tooMuchWordsFlag != NULL)
				{
					printError(TOO_MUCH_WORDS_IN_COMMAND);
					return 0;
				}
				sizeOfCommand = 1;
				return sizeOfCommand;
			}
			break;

			default:
				return 0;
			}
			return 0;
		}
	}
	printError(INVALID_OPCODE_ERROR);
	return 0;
}


void decimalToWierd(int num, char* res)
{
	char *a;
	char *b;
	if (num<0)/*case - num < 0*/
	{
		num = MAX10BITS - (num*(-1));
	}
	a = WIERD_32_BASE[num % BASE_LENGTH];
	num = num / BASE_LENGTH;
	b = WIERD_32_BASE[num % BASE_LENGTH];
	strcat(res, b);
	strcat(res, a);
}

void binaryToWierd(int *binary, char* res)
{
	int num = 0;
	num = binaryToDecimal(binary);
	decimalToWierd(num, res);
}

void decimalToBinary(int num, int *binaryNum, int arrSize)
{
	int i;
	if (num<0)/*case - num < 0*/
	{
		num = MAX10BITS - num * (-1);
	}
	for (i = arrSize - 1; i >= 0; i--)
	{
		binaryNum[i] = num % 2;
		num = num / 2;
	}
}

int binaryToDecimal(int *binaryNum)
{
	int i;
	int num = 0;
	int mult = 1;
	for (i = WORD_SIZE - 1; i >= 0; i--)
	{
		num = num + binaryNum[i] * mult;
		mult = mult * 2;
	}
	return num;
}


bool isDataCommand(char* token)
{
	char * temp;
	temp = trimStr(token);
	while (!isspace(*temp))
	{
		temp++;
	}
	while (isspace(*temp))
	{
		temp++;
	}
	if (!strncmp(temp, DataCommand[0], DataCommandLength[0]) || !strncmp(temp, DataCommand[1], DataCommandLength[1]) || !strncmp(temp, DataCommand[2], DataCommandLength[2]))
		return TRUE;
	return FALSE;
}

int getNumber(char* token)
{
	char* temp;
	char * num;
	int end = 0;
	temp = trimStr(token);
	if (strncmp(temp, "#", 1) == 0)
	{
		temp++;
	}
	num = temp;

	while (*temp >= 0 && *temp < MAX8BITS && !isspace(*temp))
	{
		end++;
		temp++;
	}
	return atoi(num);
}


bool isKeyword(char* token)
{
	int i = 0;
	char *tmp = trimStr(token);
	for (i = 0; i < NUM_OF_KEYWORDS; i++)
	{
		if (strcmp(Keywords[i], tmp) == 0)
			return TRUE;
	}
	return FALSE;
}

bool isStructWithDotOperand(char* operand)
{
	int length;
	char* temp;
	char* token;
	int num;
	length = strlen(operand);
	temp = (char*)malloc(length + 1);
	if (!temp)
	{
		printError(ALLOCATE_MEMORY_ERROR);
		return FALSE;
	}
	strncpy(temp, operand, length + 1);
	token = strtok(temp, ".");
	trimLeftStr(token);
	if (!isValidLabel(token))
	{
		free(temp);
		return FALSE;
	}
	token = strtok(NULL, ".");
	if (token == NULL)
	{
		free(temp);
		return FALSE;
	}
	token = trimRightStr(token);
	num = atoi(token);
	if (num != 1 && num != 2)
	{
		free(temp);
		return FALSE;
	}
	token = strtok(NULL, ".");
	free(temp);
	return (token == NULL) ? TRUE : FALSE;

}

bool readLine(FILE* fp, char* line)
{
	bool flag = FALSE;
	lineCounter++;
	if (line == NULL)
		return FALSE;
	if (feof(fp))
		return FALSE;
	flag = (fgets(line, MAX_LINE_LENGTH, fp) != NULL) ? TRUE : FALSE;
	return flag;
}

void resetLine(char *line, char* labelName, char *data)
{
	int i;
	for (i = 0; i < MAX_LINE_LENGTH + 1; i++)
	{
		line[i] = 0;
		labelName[i] = 0;
		data[i] = 0;
	}
}


bool isNumber(char *token)
{
	int i = 0;
	token = trimStr(token);
	if (token[i] == '+' || token[i] == '-')
		i++;
	for (; i < strlen(token); i++)
	{
		if (!isdigit(token[i]))
			return FALSE;
	}
	return TRUE;
}

bool isString(char *token)
{
	int i = 0;
	token = trimStr(token);
	if (token != NULL && token[i++] == '"' && token[strlen(token) - 1] == '"')
	{
		for (; i < strlen(token) - 1; i++)
		{
			if (isspace(token[i]))
				return FALSE;
		}
	}
	else
	{
		return FALSE;
	}
	return TRUE;
}


bool StartWithCommand(char * line)
{
	int i = 0, len = 0;
	char *token = trimStr(line);
	char *temp = (char *)calloc(MAX_LINE_LENGTH, sizeof(char));
	for (i = 0; i < sizeof(temp); i++)
	{
		if (isspace(token[i]))
			break;
		len++;
	}
	strncpy(temp, token, len);
	for (i = 0; i < NUM_OF_OPCODES; i++)
	{
		if (!(strcmp(temp, opcodes[i].opcodeName)))
		{
			free(temp);
			return TRUE;
		}
	}
	free(temp);
	return FALSE;
}

bool StartWithDataCommand(char * line)
{
	int i = 0;
	char *token = trimStr(line);
	char temp[MAX_LINE_LENGTH + 1] = { 0 };
	for (i = 0; i < sizeof(temp); i++)
	{
		if ((*token && (isspace(token[0]))) || *token == '\0')
			break;
		temp[i] = token[0];
		token++;
	}
	for (i = 0; i < NUM_OF_DATA_TYPES; i++)
	{
		if (!(strcmp(temp, DataCommand[i])))
		{
			return TRUE;
		}
	}
	return FALSE;
}
