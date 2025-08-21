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

# ------------ Final executable target -------------------------------------
# Links all object files into the final program called 'assembler'.
# - $(OBJS) : the list of object files to link
# - $^      : expands to all prerequisites (here, $(OBJS))
# - $@      : the target name ('assembler')
assembler: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# ------------ Pattern rule: compile .c -> build/%.o ------------------------
# This rule compiles source files from src/*.c into object files in build/*.o
# Step 1: Ensure the output directory exists
# Step 2: Compile the .c file into the corresponding .o file.
$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ------------ Convenience (phony) targets ---------------------------------
# These targets don’t create files — they’re just commands for convenience.
# - clean : remove build artifacts
# - test  : run the test suite
.PHONY: clean test

clean:
	rm -rf $(OBJDIR) assembler

# ------------ Test target -------------------------------------------------
# Runs the test suite using Python 3.5.2 (supported by the OpenU iso)
# Builds 'assembler' before running tests.
test: assembler
	python3 ./tests/run_test.py
