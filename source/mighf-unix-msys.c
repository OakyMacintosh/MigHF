/*
    Created and designed by:

 ________  ________  ___  __        ___    ___ _____ ______   ________  ________     
|\   __  \|\   __  \|\  \|\  \     |\  \  /  /|\   _ \  _   \|\   __  \|\   ____\    
\ \  \|\  \ \  \|\  \ \  \/  /|_   \ \  \/  / | \  \\\__\ \  \ \  \|\  \ \  \___|    
 \ \  \\\  \ \   __  \ \   ___  \   \ \    / / \ \  \\|__| \  \ \   __  \ \  \       
  \ \  \\\  \ \  \ \  \ \  \\ \  \   \/  /  /   \ \  \    \ \  \ \  \ \  \ \  \____  
   \ \_______\ \__\ \__\ \__\\ \__\__/  / /      \ \__\    \ \__\ \__\ \__\ \_______\
    \|_______|\|__|\|__|\|__| \|__|\___/ /        \|__|     \|__|\|__|\|__|\|_______|
                                  \|___|/                                            
                                                                                     
                                                                                     
    DEVELOPER NOTES:
        This program was coded thinking of POSIX-compliant systems, non POSIX OSes will need a compatibility layer.
        On Windows, I recommend using MSYS2 to compile it.
        On systems you may need to patch the code, like on Classic MacOS or on RiscOS.
        This code was from my original Gist.
        On case of using this code, please credit me (OakyMac, or just put "u/OakyMac") as the original author.
    LICENSE:
        This code is licensed under the MIT License.

    
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <unistd.h>
#endif

#define VDISP_WIDTH 320
#define VDISP_HEIGHT 240

#define MEM_SIZE 1024
#define REG_COUNT 8
#define MAX_LINE 128

// Registers: R0-R7
uint32_t regs[REG_COUNT];
uint8_t memory[MEM_SIZE];
uint32_t pc = 0; // Program counter
uint8_t running = 1;

// Function prototypes
void vdisp_init();
void vdisp_quit();
void wait_for_window_close();
void tdraw_clear();
void tdraw_pixel(int x, int y, char c);

// Simple instruction set
typedef enum {
    NOP,    // 0
    MOV,    // 1: MOV reg, imm
    ADD,    // 2: ADD reg1, reg2
    SUB,    // 3: SUB reg1, reg2
    LOAD,   // 4: LOAD reg, addr
    STORE,  // 5: STORE reg, addr
    JMP,    // 6: JMP addr
    CMP,    // 7: CMP reg1, reg2
    JE,     // 8: JE addr
    HALT,   // 9
    AND,    // 10
    ORR,    // 11
    EOR,    // 12
    LSL,    // 13
    LSR,    // 14
    MUL,    // 15
    UDIV,   // 16
    NEG,    // 17
    MOVZ,   // 18
    MOVN,   // 19
    PRINT,   // 20
    TDRAW_CLEAR,
    TDRAW_PIXEL
} Instr;

typedef struct {
    Instr opcode;
    uint8_t op1;
    uint8_t op2;
    uint32_t imm;
} Instruction;

// Simple program memory
Instruction program[MEM_SIZE];

// Flags
uint8_t flag_zero = 0;

// Micro-architecture: fetch-decode-execute
void execute_instruction(Instruction *inst) {
    switch (inst->opcode) {
        case NOP:
            break;
        case MOV:
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = inst->imm;
            break;
        case ADD:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] += regs[inst->op2];
            break;
        case SUB:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] -= regs[inst->op2];
            break;
        case LOAD:
            if (inst->op1 < REG_COUNT && inst->imm < MEM_SIZE)
                regs[inst->op1] = memory[inst->imm];
            break;
        case STORE:
            if (inst->op1 < REG_COUNT && inst->imm < MEM_SIZE)
                memory[inst->imm] = regs[inst->op1] & 0xFF;
            break;
        case JMP:
            if (inst->imm < MEM_SIZE)
                pc = inst->imm - 1;
            break;
        case CMP:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                flag_zero = (regs[inst->op1] == regs[inst->op2]);
            break;
        case JE:
            if (flag_zero && inst->imm < MEM_SIZE)
                pc = inst->imm - 1;
            break;
        case HALT:
            running = 0;
            break;
        case AND:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] &= regs[inst->op2];
            break;
        case ORR:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] |= regs[inst->op2];
            break;
        case EOR:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] ^= regs[inst->op2];
            break;
        case LSL:
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] <<= inst->imm;
            break;
        case LSR:
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] >>= inst->imm;
            break;
        case MUL:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT)
                regs[inst->op1] *= regs[inst->op2];
            break;
        case UDIV:
            if (inst->op1 < REG_COUNT && inst->op2 < REG_COUNT && regs[inst->op2] != 0)
                regs[inst->op1] /= regs[inst->op2];
            break;
        case NEG:
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = -regs[inst->op1];
            break;
        case MOVZ:
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = (uint16_t)inst->imm;
            break;
        case MOVN:
            if (inst->op1 < REG_COUNT)
                regs[inst->op1] = ~(inst->imm);
            break;
        case PRINT:
            // PRINT REG idx  or PRINT MEM idx
            if (inst->op1 == 0 && inst->imm < REG_COUNT) // REG
                printf("R%u = %u\n", inst->imm, regs[inst->imm]);
            else if (inst->op1 == 1 && inst->imm < MEM_SIZE) // MEM
                printf("MEM[%u] = %u\n", inst->imm, memory[inst->imm]);
            break;
        case TDRAW_CLEAR:
            tdraw_clear();
            break;
        case TDRAW_PIXEL:
            tdraw_pixel(regs[inst->imm & 0xFF], regs[(inst->imm >> 8) & 0xFF], (char)((inst->imm >> 16) & 0xFF));
            break;
        default:
            break;
    }
}

// Simple assembler for demo
int assemble(const char *line, Instruction *inst) {
    char op[32], arg1[32], arg2[32], arg3[32];
    int n = sscanf(line, "%31s %31s %31s %31s", op, arg1, arg2, arg3);
    if (n < 1) return 0;

    // Convert op to uppercase for case-insensitive matching
    for (int i = 0; op[i]; i++) op[i] = toupper((unsigned char)op[i]);

    if (strcmp(op, "NOP") == 0) { inst->opcode = NOP; }
    else if (strcmp(op, "MOV") == 0 && n == 3) {
        inst->opcode = MOV;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "ADD") == 0 && n == 3) {
        inst->opcode = ADD;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "SUB") == 0 && n == 3) {
        inst->opcode = SUB;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "LOAD") == 0 && n == 3) {
        inst->opcode = LOAD;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "STORE") == 0 && n == 3) {
        inst->opcode = STORE;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "JMP") == 0 && n == 2) {
        inst->opcode = JMP;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "CMP") == 0 && n == 3) {
        inst->opcode = CMP;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "JE") == 0 && n == 2) {
        inst->opcode = JE;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "HALT") == 0) { inst->opcode = HALT; }
    else if (strcmp(op, "AND") == 0 && n == 3) {
        inst->opcode = AND;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "ORR") == 0 && n == 3) {
        inst->opcode = ORR;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "EOR") == 0 && n == 3) {
        inst->opcode = EOR;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "LSL") == 0 && n == 3) {
        inst->opcode = LSL;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "LSR") == 0 && n == 3) {
        inst->opcode = LSR;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "MUL") == 0 && n == 3) {
        inst->opcode = MUL;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "UDIV") == 0 && n == 3) {
        inst->opcode = UDIV;
        inst->op1 = arg1[1] - '0';
        inst->op2 = arg2[1] - '0';
    }
    else if (strcmp(op, "NEG") == 0 && n == 2) {
        inst->opcode = NEG;
        inst->op1 = arg1[1] - '0';
    }
    else if (strcmp(op, "MOVZ") == 0 && n == 3) {
        inst->opcode = MOVZ;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "MOVN") == 0 && n == 3) {
        inst->opcode = MOVN;
        inst->op1 = arg1[1] - '0';
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "PRINT") == 0 && n == 3) {
        inst->opcode = PRINT;
        // Convert arg1 to uppercase for case-insensitive matching
        for (int i = 0; arg1[i]; i++) arg1[i] = toupper((unsigned char)arg1[i]);
        if (strcmp(arg1, "REG") == 0) {
            inst->op1 = 0;
            inst->imm = atoi(arg2);
        } else if (strcmp(arg1, "MEM") == 0) {
            inst->op1 = 1;
            inst->imm = atoi(arg2);
        } else {
            return 0;
        }
    }
    else if (strcmp(op, "TDRAW_CLEAR") == 0) {
        inst->opcode = TDRAW_CLEAR;
    } else if (strcmp(op, "TDRAW_PIXEL") == 0 && n == 4) {
        inst->opcode = TDRAW_PIXEL;
        int rx = arg1[1] - '0';
        int ry = arg2[1] - '0';
        int ch = arg3[0];
        inst->imm = rx | (ry << 8) | (ch << 16);
    }
    else return 0;
    return 1;
}

// Print system information
void print_system_info() {
    // Print OS
#if defined(_WIN32)
    printf("Windows\n");
#elif defined(__linux__)
    printf("Linux\n");
#elif defined(__APPLE__)
    printf("macOS\n");
#elif defined(__FreeBSD__)
    printf("FreeBSD\n");
#else
    printf("Unknown OS\n");
#endif

    // Print architecture
#if defined(_WIN64) || defined(__x86_64__) || defined(__amd64__)
    printf("Architecture: x86_64\n");
#elif defined(_WIN32) || defined(__i386__)
    printf("Architecture: x86 (32-bit)\n");
#elif defined(__aarch64__)
    printf("Architecture: ARM64\n");
#elif defined(__arm__)
    printf("Architecture: ARM\n");
#elif defined(__powerpc64__)
    printf("Architecture: PowerPC64\n");
#elif defined(__powerpc__)
    printf("Architecture: PowerPC\n");
#else
    printf("Architecture: Unknown\n");
#endif

    // Print number of processors
#if defined(_WIN32)
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    printf("Processors: %u\n", sysinfo.dwNumberOfProcessors);
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    if (nprocs > 0)
        printf("Processors: %ld\n", nprocs);
    else
        printf("Processors: Unknown\n");
#else
    printf("Processors: Unknown\n");
#endif
}

// UEFI Shell-like shell
void shell() {
    char line[MAX_LINE];
    printf("OpenBoot v1:: mighf-\n");
    print_system_info();
    printf("Type 'help' for commands.\n");
    while (1) {
        printf("coreshell> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strncmp(line, "exit", 4) == 0) break;
        else if (strncmp(line, "help", 4) == 0) {
            printf("Commands:\n");
            printf("  load <file>   - Load program\n");
            printf("  run           - Run program\n");
            printf("  regs          - Show registers\n");
            printf("  mem <addr>    - Show memory at addr\n");
            printf("  exit          - Exit shell\n");
        }
        else if (strncmp(line, "regs", 4) == 0) {
            for (int i = 0; i < REG_COUNT; i++)
                printf("R%d: %u\n", i, regs[i]);
        }
        else if (strncmp(line, "mem", 3) == 0) {
            int addr = atoi(line + 4);
            if (addr >= 0 && addr < MEM_SIZE)
                printf("MEM[%d]: %u\n", addr, memory[addr]);
            else
                printf("Invalid address\n");
        }
        else if (strncmp(line, "load", 4) == 0) {
            char fname[64];
            if (sscanf(line, "load %63s", fname) == 1) {
                FILE *f = fopen(fname, "r");
                if (!f) { printf("Cannot open file\n"); continue; }
                char pline[MAX_LINE];
                int idx = 0;
                while (fgets(pline, sizeof(pline), f) && idx < MEM_SIZE) {
                    if (assemble(pline, &program[idx]))
                        idx++;
                }
                fclose(f);
                printf("Loaded %d instructions\n", idx);
            } else {
                printf("Usage: load <file>\n");
            }
        }
        else if (strncmp(line, "run", 3) == 0) {
            pc = 0;
            running = 1;
            while (running && pc < MEM_SIZE) {
                execute_instruction(&program[pc]);
                pc++;
            }
            printf("Program finished.\n");
       //     wait_for_window_close(); // <-- Only after running the program
        }
        else {
            printf("Unknown command. Type 'help'.\n");
        }
    }
}

// CLI mode: load and run file
void run_file(const char *fname) {
    FILE *f = fopen(fname, "r");
    if (!f) {
        printf("Cannot open file: %s\n", fname);
        return;
    }
    char pline[MAX_LINE];
    int idx = 0;
    while (fgets(pline, sizeof(pline), f) && idx < MEM_SIZE) {
        if (assemble(pline, &program[idx]))
            idx++;
    }
    fclose(f);
    printf("Loaded %d instructions from %s\n", idx, fname);
    pc = 0;
    running = 1;
    while (running && pc < MEM_SIZE) {
        execute_instruction(&program[pc]);
        pc++;
    }
    printf("Program finished.\n");
}

void tdraw_clear() {
    printf("\033[2J\033[H"); // ANSI clear screen and move cursor to home
    fflush(stdout);
}

void tdraw_pixel(int x, int y, char c) {
    printf("\033[%d;%dH%c", y + 1, x + 1, c); // ANSI move cursor and print char
    fflush(stdout);
}



int main(int argc, char **argv) {
    memset(regs, 0, sizeof(regs));
    memset(memory, 0, sizeof(memory));
    memset(program, 0, sizeof(program));
    if (argc > 1) {
        run_file(argv[1]);
    } else {
        shell();
    }
    return 0;
}