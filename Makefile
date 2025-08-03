#==================================================
# File: Makefile
# Author: Elin Urevich
# Purpose: Makefile for Assembler Program.
#==================================================
CC = gcc
CFLAGS = -Wall -ansi -pedantic

assembler: main.c preassembler.c
	$(CC) $(CFLAGS) -o assembler main.c preassembler.c

clean:
	rm -f *.o assembler filesamples/*.o filesamples/*.am
