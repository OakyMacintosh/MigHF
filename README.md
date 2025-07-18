# mighf

> “Who said I’m into building chips? I’m just a creative programmer.”
>
> <cite>OakyMacintosh</cite>


## Overview

**mighf** is a minimal, educational micro-architecture and emulator project.  
It lets you experiment with a simple CPU, custom instruction set, and assembly-like programming—all in software.

## Features

- Simple 8-register, 1KB memory micro-architecture
- Assembler and Runtime made in pure `C`
- In-terminal drawing with ASCII graphics
- Interactive shell for loading and running programs
- Easily extensible instruction set

## Getting Started

### Build

```sh
gcc source/mighf-unix-msys.c -o mighf.bin
```
>[!NOTE]
>The `mingw-w64` toolkit can also be used to compile `mighf`
>It was tested by me.

### Run

```sh
./mighf.bin
```

### Example

Load and run an assembly program:

```sh
coreshell> load rom/rom.s
coreshell> run
```

## Assembly Example

```asm
# TDRAW_CLEAR <-- Removed that instruction in commit `e60aecc`
MOV R1, 10
MOV R2, 5
# TDRAW_PIXEL R1, R2, '*' <-- Removed that instruction in commit `e60aecc`
```

> [!TIP]
> To learn about the `mighf` assembly language read the [Docs](./Docs.md)

## Project Structure

- `source/` — Runtime and assembler source code
- `rom/` — MicroComputing ROM (also the Mini BIOS)
- `README.md` — This file

## Contributing

Pull requests and suggestions are welcome!  
Feel free to open issues for bugs or feature requests.

## License

MIT License

---

Made with ❤️ by [OakyMacintosh](https://www.youtube.com/@OakyMacintosh).

