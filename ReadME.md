# Assembler Project Summary – Specification & Task Breakdown

## 📌 Project Name
**Custom Assembler for Educational CPU Model**  
**Course:** 20465 – Systems Programming Lab  
**Language:** C (ISO C90 / ANSI C standard)

---

## 🎯 Project Objective
Build a two-pass assembler in C that:
- Translates a custom assembly language into machine code for a fictional CPU.
- Supports macros, multiple addressing modes, label resolution, and memory layout.
- Produces various output files for code, entries, externals, and macro-expanded code.

---

## 📦 Deliverables
You must submit the following:
- ✅ All source files (`.c` and `.h`)
- ✅ Executable compiled with `gcc -ansi -Wall -pedantic`
- ✅ `Makefile`
- ✅ `.as` test files (3 valid, 3 with errors)
- ✅ Output files: `.am`, `.ob`, `.ent`, `.ext`
- ✅ Screenshots showing terminal outputs (errors + valid cases)

---

## 🏗 Project Structure (Suggested Files)
| File             | Responsibility                                  |
|------------------|--------------------------------------------------|
| `main.c`         | File handling, CLI args, invokes all stages      |
| `preassembler.c` | Macro expansion, writes `.am`                    |
| `first_pass.c`   | Parses `.am`, builds symbol table, partial code  |
| `second_pass.c`  | Final encoding using symbol table                |
| `symbol_table.c` | Linked list / hash table for labels              |
| `parser.c`       | Lexical and syntax analysis per line             |
| `encoder.c`      | Encoding to binary/base-4 format                 |
| `utils.c`        | Common helpers (string, file ops, etc.)          |
| `file_writer.c`  | Writes `.ob`, `.ent`, `.ext`                     |
| `types.h`        | Definitions: enums, structs, constants           |

---

## 🧱 Core Tasks Breakdown

### 1. Pre-Assembler Stage
- Expand all `mcro` definitions
- Replace macro calls inline
- Create: `.am` file (expanded code)
- Validate macro names (not same as opcode)
- Ensure each `mcro` has a matching `mcroend`

---

### 2. First Pass (on `.am`)
- Parse lines and recognize instructions, directives, labels
- Build the symbol table
- Encode:
  - First word of instructions
  - Entire data section
- Track instruction counter (IC) and data counter (DC)

---

### 3. Second Pass
- Complete encoding with label addresses
- Finalize binary encoding and set A/R/E bits
- Generate output: `.ob`, `.ent`, `.ext`

---

### 4. Code Requirements
- Support 16 opcodes:
  - `mov`, `cmp`, `add`, `sub`, `lea`, `clr`, `not`, `inc`, `dec`,
    `jmp`, `bne`, `red`, `prn`, `jsr`, `rts`, `stop`

- Addressing Modes:
  | Mode | Description         | Example           |
  |------|---------------------|-------------------|
  | 0    | Immediate           | `#5`              |
  | 1    | Direct              | `LABEL`           |
  | 2    | Matrix              | `LABEL[r2][r5]`   |
  | 3    | Register direct     | `r7`              |

---

## 📤 Output Files

| Extension | Description                               |
|-----------|-------------------------------------------|
| `.am`     | Source with expanded macros               |
| `.ob`     | Final machine code (base-4)               |
| `.ent`    | Entry-point labels and resolved addresses |
| `.ext`    | External symbol references in use         |

---

## ❗ Error Handling
Detect and report:
- Invalid or duplicate labels
- Unknown opcode or directive
- Wrong operand count/type
- Undefined symbols
- Invalid macro definitions
- Improper use of `.entry` or `.extern`

---

## 🧪 Testing Guidelines
Create `.as` test files:
- ✅ Include all instructions and addressing modes
- ❌ Examples with:
  - Too many operands
  - Undefined symbols
  - Duplicate labels

---

## 🧩 Additional Implementation Notes

### 1. Instruction Encoding
- Each instruction starts with a **10-bit word**, encoded in a custom base-4 format.
- The **first word** includes:
  - Opcode (4 bits)
  - Addressing mode for source and destination (2 bits each)
  - A/R/E flags (2 bits)

### 2. Addressing Modes Details
- **0 – Immediate**: `#5` → value encoded in next word.
- **1 – Direct**: label → address resolved from symbol table.
- **2 – Matrix Access**: `LABEL[rX][rY]` → 2 extra words encoded with register indices and base address.
- **3 – Register Direct**: `r0` to `r7` → encoded directly.

### 3. A/R/E Bits
- Each code word includes two bits indicating:
  - `00` – Absolute (A)
  - `10` – Relocatable (R)
  - `01` – External (E)

### 4. Macro Expansion Rules
- Macros start with `mcro` and end with `mcroend`.
- No nested macros.
- Macro names must not clash with:
  - Opcodes (e.g. `mov`)
  - Directives (e.g. `.data`, `.string`)
- Macro expansion must be done before the first assembler pass.

### 5. Symbol Table Management
- Use a linked list or dynamic structure to manage labels.
- Label properties:
  - Name
  - Address
  - Type: `code`, `data`, `entry`, `external`

### 6. Memory Layout
- Memory starts at address **100 (decimal)**.
- All code is placed first, followed by data.
- Final `.ob` file includes:
  - Instruction section
  - Then data section
  - All in base-4 format with addresses

### 7. Error Formatting & Detection
- Must report line number for each error.
- Report **multiple errors per file**, not just the first.
- Error types include:
  - Illegal label name (e.g. starting with number)
  - Duplicate label definitions
  - Undefined symbols
  - Misused opcodes or directives
  - Incorrect operand count/type
- If errors exist, **no output files should be generated**.

### 8. Output Files Overview
- `.am` – Macro-expanded input source
- `.ob` – Machine code in base-4, code followed by data
- `.ent` – All `.entry` labels with final addresses
- `.ext` – All references to `.extern` symbols with code locations

### 9. Assumptions (Allowed Simplifications)
- Each macro is defined and used in the same file.
- Macro definitions are always syntactically correct.
- Labels appear at the start of a line and are properly formatted.
- Each line is ≤ 80 characters (excluding `\\n`).

---


## ⚙️ Compilation & Usage

```bash
make
./assembler file1 file2 file3
