#include "globalVariables.h"

int lineCounter;

ERAStructure ERATypes[ERA_TYPES_SIZE] = {
{ 0	,	Absolute	,	"00" },
{ 1	,	External	,	"01" },
{ 2	,	Relocatable	,	"10" }
};

char* WIERD_32_BASE[BASE_LENGTH] = {
	"!","@","#","$","%","^","&","*",
	"<",">","a","b","c","d","e","f",
	"g","h","i","j","k","l","m","n",
	"o","p","q","r","s","t","u","v" };



char* Keywords[NUM_OF_KEYWORDS] = { "mov","cmp","add","sub","not","clr","inc", "lea","dec","jmp","bne","red","prn","jsr","rts","stop",
".data", ".string", ".extern", ".entry", ".struct" };



char* Registers[NUM_OF_REGISTERS] = { "r0","r1","r2","r3","r4","r5","r6","r7" };


RegisterStructure registersBinary[NUM_OF_REGISTERS] = {
{ "r0"	,{ 0,0,0,0 } },
{ "r1"	,{ 0,0,0,1 } },
{ "r2"	,{ 0,0,1,0 } },
{ "r3"	,{ 0,0,1,1 } },
{ "r4"	,{ 0,1,0,0 } },
{ "r5"	,{ 0,1,0,1 } },
{ "r6"	,{ 0,1,1,0 } },
{ "r7"	,{ 0,1,1,1 } }
};

opcodeStructure opcodes[NUM_OF_OPCODES] = {
{ 0		,	"mov"	,{ 0,0,0,0 }	,	FIRST_GROUP },
{ 1		,	"cmp"	,{ 0,0,0,1 }	,	FIRST_GROUP },
{ 2		,	"add"	,{ 0,0,1,0 }	,	FIRST_GROUP },
{ 3		,	"sub"	,{ 0,0,1,1 }	,	FIRST_GROUP },
{ 4		,	"not"	,{ 0,1,0,0 }	,	SECOND_GROUP },
{ 5		,	"clr"	,{ 0,1,0,1 }	,	SECOND_GROUP },
{ 6		,	"lea"	,{ 0,1,1,0 }	,	FIRST_GROUP },
{ 7		,	"inc"	,{ 0,1,1,1 }	,	SECOND_GROUP },
{ 8		,	"dec"	,{ 1,0,0,0 }	,	SECOND_GROUP },
{ 9		,	"jmp"	,{ 1,0,0,1 }	,	SECOND_GROUP },
{ 10	,	"bne"	,{ 1,0,1,0 }	,	SECOND_GROUP },
{ 11	,	"red"	,{ 1,0,1,1 }	,	SECOND_GROUP },
{ 12	,	"prn"	,{ 1,1,0,0 }	,	SECOND_GROUP },
{ 13	,	"jsr"	,{ 1,1,0,1 }	,	SECOND_GROUP },
{ 14	,	"rts"	,{ 1,1,1,0 }	,	THIRD_GROUP },
{ 15	,	"stop"	,{ 1,1,1,1 }	,	THIRD_GROUP }
};


char *OpcodesFirstGroup[FIRST_GROUP_SIZE] = { "mov","cmp","add","sub","lea" };
char *OpcodesSecondGroup[SECOND_GROUP_SIZE] = { "not", "clr","inc","dec","jmp","bne","red","prn","jsr" };
char *OpcodesThirdGroup[THIRD_GROUP_SIZE] = { "rts", "stop" };

char *DataCommand[NUM_OF_DATA_TYPES] = {".string", ".data", ".struct" };
int DataCommandLength[NUM_OF_DATA_TYPES] = { 7, 5, 7 };

char* AssemblyfileName;
