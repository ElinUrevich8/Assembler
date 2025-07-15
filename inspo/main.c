#include "main.h"

int main(int argc, char** argv)
{
	int i = 0, fileNameLength;
	if (argc < 2)
	{
		printError(MISSING_INPUT_FILE);
	}
	for (i = 1; i < argc; i++)
	{
		fileNameLength = strlen(argv[i]);
		AssemblyfileName = (char*)malloc(fileNameLength + 1);
		if (AssemblyfileName == NULL)
		{
			printError(FILE_NAME_MEM_ALLOCATION_FAILURE);
			continue;
		}
		strncpy(AssemblyfileName, argv[i], fileNameLength);
		AssemblyfileName[fileNameLength] = '\0';
		runAssembler(AssemblyfileName);
		free(AssemblyfileName);
	}
	return EXIT_SUCCESS;
}
