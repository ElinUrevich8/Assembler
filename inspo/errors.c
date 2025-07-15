#include "errors.h"

/*
Description: Prints an error while opening or creating a file
error - error description
fileName - file name
*/
void printFileError(char* error, char* fileName)
{
	fprintf(stderr, "[%s] : %s %s - \"%s\"\n", AssemblyfileName, "[Error] -> ", error, fileName);
}

/*
Description: Prints an error that is detected during the first pass (compilation error)
error - error description
*/
void printError(char* error)
{
	fprintf(stderr, "[%s] : [IN LINE - %d] %s %s\n", AssemblyfileName, lineCounter, "[Error] -> ", error);
}

/*
Description: Prints an error that is detected during the first pass (compilation error)
error - error description
ref - a reference that caused the error
*/
void printExtendedError(char* error, char* ref)
{
	fprintf(stderr, "[%s] : [IN LINE - %d] %s %s - \"%s\"\n", AssemblyfileName, lineCounter, "[Error] -> ", error, ref);
}

/*
Description: Prints a warning that is detected during the second pass
error - error description
ref - a reference that caused the error
*/
void printWarning(char* warn, char* ref)
{
	fprintf(stderr, "[%s] : [IN LINE - %d] %s %s - \"%s\"\n", AssemblyfileName, lineCounter, "[Warning]-> ", warn, ref);
}

/*
Description: Prints an error that is detected during the second pass (runtime error)
error - error description
ref - a reference that caused the error
*/
void printRuntimeError(char* error, char* ref)
{
	fprintf(stderr, "[%s] : [IN LINE - %d] %s %s - \"%s\"\n", AssemblyfileName, lineCounter, "[RUNTIME ERROR] -> ", error, ref);
}
