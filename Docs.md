# MIGHF Microarchitecture + Assembler Documentation

> Version: Early Dev (Unix/MSYS Prototype)
> Author: OakyMac (aka r/OakyMac)
> License: MIT

---

## TL;DR

* **mighf** is a minimalist microarchitecture and assembly language combo.
* Built for learning, prototyping, and portability.
* Cross-platform, with MSYS2 recommended on Windows.

---

## 🧠 Microarchitecture Overview

### Registers

* Up to 65,536 general-purpose registers: `R0` to `R65535`
* 32-bit each
* Dynamic allocation (default: 256 registers)

### Memory

* 1024 bytes total
* Addressable via `LOAD` and `STORE`

### Flags

* `ZERO` flag (used for conditional jumps)

### PC (Program Counter)

* Automatically increments every cycle unless jumped manually

---

## 🧾 Instruction Set (ISA)

### Control

| Mnemonic     | Description              |
| ------------ | ------------------------ |
| `NOP`        | No operation             |
| `JMP addr`   | Jump to address          |
| `CMP r1, r2` | Set zero flag if equal   |
| `JE addr`    | Jump if zero flag is set |
| `HALT`       | Stop execution           |

### Data Movement

| Mnemonic        | Description                |
| --------------- | -------------------------- |
| `MOV r, imm`    | Set register to immediate  |
| `LOAD r, addr`  | Load mem\[addr] into reg   |
| `STORE r, addr` | Store reg into mem\[addr]  |
| `MOVZ r, imm`   | Move zero-extended imm     |
| `MOVN r, imm`   | Move NOT imm into register |

### Arithmetic / Logic

| Mnemonic      | Description              |
| ------------- | ------------------------ |
| `ADD r1, r2`  | Add r2 to r1             |
| `SUB r1, r2`  | Subtract r2 from r1      |
| `MUL r1, r2`  | Multiply r1 by r2        |
| `UDIV r1, r2` | Unsigned divide r1 by r2 |
| `NEG r`       | Negate register          |
| `AND r1, r2`  | Bitwise AND              |
| `ORR r1, r2`  | Bitwise OR               |
| `EOR r1, r2`  | Bitwise XOR              |
| `LSL r, imm`  | Logical shift left       |
| `LSR r, imm`  | Logical shift right      |

### I/O

| Mnemonic              | Description                         |
| --------------------- | ----------------------------------- |
| `PRINT REG r`         | Print value of register             |
| `PRINT MEM addr`      | Print value at memory address       |
| `TDRAW_CLEAR`         | ANSI terminal screen clear          |
| `TDRAW_PIXEL rX rY c` | Draw char `c` at coordinates rX, rY |

---

## 💬 Assembler Syntax

Each instruction is 1 line. Comments are not supported (yet). Example:

```asm
MOV R0 10
MOV R1 5
ADD R0 R1
PRINT REG 0
HALT
```

Supports high-numbered registers:

```asm
MOV R100 42
MOV R1000 100
ADD R100 R1000
PRINT REG 100
HALT
```

---

## 🖥️ CLI Shell Usage

When run without arguments:

```bash
$ ./mighf
Welcome to mighf-embedded micro-arch shell!
coreshell> help
```

### Commands

* `load <file>` — Load assembly file
* `run` — Execute loaded program
* `regs` — Show register state
* `reg <num>` — Show specific register
* `regcount` — Show register count info
* `resize <count>` — Change number of available registers
* `mem <addr>` — Show memory at address
* `exit` — Quit shell

---

## 📦 File-based Usage

When run with a file:

```bash
$ ./mighf demo.asm
```

* Loads and immediately executes the program.
* Use `.asm` for source files.

---

## 🛠️ Platform Support

* Designed for POSIX, works out-of-the-box on Linux/macOS
* Use **MSYS2** on Windows
* Not guaranteed to work on classic OSes without patching (e.g. Classic Mac, RISC OS)

---

## 💡 Dev Notes

* You can expand the ISA easily by adding cases to `execute_instruction` and `assemble`
* Currently no support for labels or comments — planned for future
* Very basic error handling, meant for learning & tinkering

---

## ✨ Credits

* Original dev: **OakyMac** (drop "u/OakyMac" if you share)
* License: MIT — free to use, remix, hack, etc.

---