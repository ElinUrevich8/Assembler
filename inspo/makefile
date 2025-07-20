objects = main.o GlobalsFunctions.o assembler.o dataList.o errors.o firstPass.o globalVariables.o secondPass.o symbolsList.o
headers = main.h GlobalsFunctions.h assembler.h dataList.h errors.h firstPass.h globalVariables.h secondPass.h symbolsList.h

CFLAGS = -Wall -ansi -pedantic
CC = gcc

assembler: $(objects) $(headers)
	$(CC) -g $(CFLAGS) $(objects) -o assembler

main.o: main.c main.h assembler.h
	$(CC) -c $(CFLAGS) main.c -o main.o


GlobalsFunctions.o: GlobalsFunctions.c GlobalsFunctions.h dataList.h symbolsList.h globalVariables.h errors.h
	$(CC) -c $(CFLAGS) GlobalsFunctions.c -o GlobalsFunctions.o


assembler.o: assembler.c assembler.h firstPass.h GlobalsFunctions.h
	$(CC) -c $(CFLAGS) assembler.c -o assembler.o


dataList.o: dataList.c dataList.h globalVariables.h errors.h
	$(CC) -c $(CFLAGS) dataList.c -o dataList.o


errors.o: errors.c errors.h globalVariables.h
	$(CC) -c $(CFLAGS) errors.c -o errors.o
	

firstPass.o: firstPass.c firstPass.h GlobalsFunctions.h
	$(CC) -c $(CFLAGS) firstPass.c -o firstPass.o


globalVariables.o: globalVariables.c globalVariables.h
	$(CC) -c $(CFLAGS) globalVariables.c -o globalVariables.o


secondPass.o: secondPass.c secondPass.h globalVariables.h GlobalsFunctions.h
	$(CC) -c $(CFLAGS) secondPass.c -o secondPass.o


symbolsList.o: symbolsList.c globalVariables.h
	$(CC) -c $(CFLAGS) symbolsList.c -o symbolsList.o

.PHONY : clean
clean:
		-rm $(objects)
