#include "assembler.h"

/*
Description: This function gets a file name and assembles a single file
*/
void runAssembler(char* fileName)
{
	bool flag;
	flag = TRUE;
	if (isFileExists(fileName) == FALSE)
	{
		printFileError(OPEN_INPUT_FILE_ERROR, fileName);
		return;
	}
	flag = firstpass(fileName);
	if (flag)/*no errors found in the first pass*/
	{
		flag = secondPass(fileName);
		if (!flag)/*error found in the second pass*/
		{
			printFileError(ERRORS_IN_SECOND_PASS, fileName);
			return;
		}
	}
	else/*errors found in the first pass*/
	{
		printFileError(ERRORS_IN_FIRST_PASS, fileName);
		return;
	}
}
