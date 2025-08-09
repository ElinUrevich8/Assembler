<<<<<<< HEAD
#==================================================
# File: Makefile
# Author: Elin Urevich
# Purpose: Makefile for Assembler Program.
#==================================================

# ------------ compiler flags ---------------------------------------------
CC      = gcc
CFLAGS  = -Wall -ansi -pedantic -Iinclude

# ------------ folders & file lists ---------------------------------------
OBJDIR  = build
SRCS    := $(wildcard src/*.c)
OBJS    := $(SRCS:src/%.c=$(OBJDIR)/%.o)

# ------------ final executable -------------------------------------------
assembler: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# ------------ pattern rule: .c → build/%.o -------------------------------
# • single implicit rule (no second rule for 'build')
# • first command makes sure the directory exists
$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ------------ convenience targets ----------------------------------------
.PHONY: clean test

clean:
	rm -rf $(OBJDIR) assembler

test: assembler
	python3 ./tests/run_test.py
=======
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
	rm -f *.o assembler
>>>>>>> origin/main
