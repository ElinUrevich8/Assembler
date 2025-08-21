# Assembler Project

A two-pass assembler with a macro prepass, written in C90.  
It processes `.as` source files, expands macros, parses and encodes instructions, resolves symbols, and produces:

- **`.ob`** — object file in a custom base-4 alphabet (`a=0, b=1, c=2, d=3`)
- **`.ent`** — entry symbols and their addresses
- **`.ext`** — extern symbols’ usage sites

---

##  High-Level Pipeline

1. **Preassembler (`.as` → `.am`)**  
   - Expands `mcro … mcroend` macros.  
   - Strips comments/blank lines.  
   - Validates macro names.  
   - Outputs a macro-expanded `.am` file.

2. **Pass 1**  
   - Builds **symbol table** with label kinds (CODE/DATA/EXTERN/ENTRY) and addresses.  
   - Parses `.data`, `.string`, `.mat` → pushes values into **data image**.  
   - Estimates instruction sizes for IC (instruction counter).  
   - Relocates `DATA` labels by adding the final code size (**ICF**).

3. **Pass 2**  
   - Re-parses `.am` to **emit final words** into the code image.  
   - Encodes **first word** (opcode + addressing modes + ARE flag).  
   - Emits operand words:  
     - `#imm` → absolute A-word.  
     - `rX` → absolute A-word (two regs may be packed).  
     - `LABEL` → relocatable R-word (or E-word for externs).  
     - `LABEL[rA][rB]` → label word (R/E) + row/col reg-pair word (A).  
   - Records extern use-sites for `.ext` and `.entry` addresses for `.ent`.

4. **Writers**  
   - `.ob` — first line: `<code_len> <data_len>` in base-4 a/b/c/d; then `<address> <word>` lines (addresses start at 100).  
   - `.ent` — `<symbol> <address>` for each entry.  
   - `.ext` — `<symbol> <use_site_address>` for each extern use.

---

##  Module main Responsibilities

- **assembler.c / main.c** — Orchestrate stages; CLI handling.  
- **preassembler.c / macro.c** — Expand macros; write `.am`.  
- **identifiers.c** — Reserved words and identifier validation.  
- **pass1.c** — Build symbol table; track IC/DC; push data; relocate data symbols.  
- **encoding.c** — Parse directives and instructions, estimate size, strip comments.  
- **nameset.c** — String set for tracking used names (e.g., macros); prevent name collisions.  
- **hash_core.c** — Generic chained hash table with string keys and `void*` values.  
- **symbols.c** — Define/lookup/mark entry/extern; iterate symbols.  
- **pass2.c** — Emit final words; resolve symbols; log externs; collect entries.  
- **codeimg.c** — Manage code/data images; push/get/relocate words.  
- **isa.c** — Pack/unpack 10-bit words; encode first word, registers, ARE bits.  
- **output.c** — Base-4 printing (`a=0,b=1,c=2,d=3`), write `.ob`, `.ent`, `.ext`.  
- **errors.c** — Collect and print assembler errors.  
---


##  Project Structure
```
├── doc
│ └── project-structure
├── include
│ ├── assembler.h
│ ├── codeimg.h
│ ├── debug.h
│ ├── defaults.h
│ ├── encoding.h
│ ├── encoding_parse.h
│ ├── errors.h
│ ├── hash_core.h
│ ├── identifiers.h
│ ├── isa.h
│ ├── macro.h
│ ├── nameset.h
│ ├── output.h
│ ├── pass1.h
│ ├── pass2.h
│ ├── preassembler.h
│ └── symbols.h
├── Makefile
├── src
│ ├── assembler.c
│ ├── codeimg.c
│ ├── debug.c
│ ├── encoding.c
│ ├── errors.c
│ ├── hash_core.c
│ ├── identifiers.c
│ ├── isa.c
│ ├── macro.c
│ ├── main.c
│ ├── nameset.c
│ ├── output.c
│ ├── pass1.c
│ ├── pass2.c
│ ├── preassembler.c
│ └── symbols.c
└── tests
├── cases
│ ├── pass1/...
│ ├── pass2/...
│ └── preassembler/...
└── run_test.py
```

## 🔧 Build & Run

### Build

Note that no prefix "*.as" is needed while running the programm but the files should have *.as 
```
make
./assembler <file> <file2>
```

## Clean
```
make clean
```

## Run Tests
```
make test
```