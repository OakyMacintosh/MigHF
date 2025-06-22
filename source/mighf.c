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

#include "assembler.h"
#include "program_counter.h"

// #define VDISP_WIDTH 320
// #define VDISP_HEIGHT 240

#define MEM_SIZE 2024
#define MAX_REGISTERS 65536  // Maximum for 16-bit addressing
#define DEFAULT_REG_COUNT 256
#define MAX_PROGRAM_SIZE 1024
#define MAX_LINE 128

uint32_t *regs = NULL;
uint32_t current_reg_count = DEFAULT_REG_COUNT;
uint8_t memory[MEM_SIZE];
uint32_t pc = 0; // Program counter
uint8_t running = 1;

// Add these globals for stack support
#define STACK_SIZE 1024
uint32_t stack[STACK_SIZE];
uint32_t sp = 0; // Stack pointer

// Add this for signed comparison
int last_cmp = 0;

// Function prototypes
void init_registers(uint32_t count);
void cleanup_registers();
void vdisp_init();
void vdisp_quit();
void wait_for_window_close();
void tdraw_clear();
void tdraw_pixel(int x, int y, char c);

// Simple program memory
Instruction program[MEM_SIZE];

// Flags
uint8_t flag_zero = 0;

// Initialize register array with specified count
void init_registers(uint32_t count) {
    if (regs) free(regs);
    current_reg_count = (count > MAX_REGISTERS) ? MAX_REGISTERS : count;
    regs = calloc(current_reg_count, sizeof(uint32_t));
    if (!regs) {
        printf("Error: Could not allocate memory for %u registers\n", current_reg_count);
        exit(1);
    }
}

// Cleanup register memory
void cleanup_registers() {
    if (regs) {
        free(regs);
        regs = NULL;
    }
}

// Micro-architecture: fetch-decode-execute
void execute_instruction(Instruction *inst) {
    switch (inst->opcode) {
        case NOP:
            break;
        case MOV:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] = inst->imm;
            break;
        case ADD:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count)
                regs[inst->op1] += regs[inst->op2];
            break;
        case SUB:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count)
                regs[inst->op1] -= regs[inst->op2];
            break;
        case LOAD:
            if (inst->op1 < current_reg_count && inst->imm < MEM_SIZE)
                regs[inst->op1] = memory[inst->imm];
            break;
        case STORE:
            if (inst->op1 < current_reg_count && inst->imm < MEM_SIZE)
                memory[inst->imm] = regs[inst->op1] & 0xFF;
            break;
        case JMP:
            if (inst->imm < MEM_SIZE)
                pc = inst->imm - 1;
            break;
//        case CMP:
//            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count)
//                flag_zero = (regs[inst->op1] == regs[inst->op2]);
//            break;
        case JE:
            if (flag_zero && inst->imm < MEM_SIZE)
                pc = inst->imm - 1;
            break;
        case HALT:
            running = 0;
            break;
        case AND:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count)
                regs[inst->op1] &= regs[inst->op2];
            break;
        case ORR:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count)
                regs[inst->op1] |= regs[inst->op2];
            break;
        case EOR:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count)
                regs[inst->op1] ^= regs[inst->op2];
            break;
        case LSL:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] <<= inst->imm;
            break;
        case LSR:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] >>= inst->imm;
            break;
        case MUL:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count)
                regs[inst->op1] *= regs[inst->op2];
            break;
        case UDIV:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count && regs[inst->op2] != 0)
                regs[inst->op1] /= regs[inst->op2];
            break;
        case NEG:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] = -regs[inst->op1];
            break;
        case MOVZ:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] = (uint16_t)inst->imm;
            break;
        case MOVN:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] = ~(inst->imm);
            break;
        case PRINT:
            // PRINT REG idx  or PRINT MEM idx
            if (inst->op1 == 0 && inst->imm < current_reg_count) // REG
                printf("R%u = %u\n", inst->imm, regs[inst->imm]);
            else if (inst->op1 == 1 && inst->imm < MEM_SIZE) // MEM
                printf("MEM[%u] = %u\n", inst->imm, memory[inst->imm]);
            break;
        case TDRAW_CLEAR:
            tdraw_clear();
            break;
        case TDRAW_PIXEL:
            {
                uint16_t rx = inst->imm & 0xFF;
                uint16_t ry = (inst->imm >> 8) & 0xFF;
                if (rx < current_reg_count && ry < current_reg_count) {
                    tdraw_pixel(regs[rx], regs[ry], (char)((inst->imm >> 16) & 0xFF));
                }
            }
            break;
        case PUSH:
            // push regs[op1] to stack
            if (sp < STACK_SIZE && inst->op1 < current_reg_count) {
                stack[sp++] = regs[inst->op1];
            } else {
                printf("Stack overflow or invalid register in PUSH\n");
                running = 0;
            }
            break;
        case POP:
            // pop value from stack to regs[op1]
            if (sp > 0 && inst->op1 < current_reg_count) {
                regs[inst->op1] = stack[--sp];
            } else {
                printf("Stack underflow or invalid register in POP\n");
                running = 0;
            }
            break;
        case CALL:
            // push current pc to stack, set pc to imm
            if (sp < STACK_SIZE && inst->imm < MEM_SIZE) {
                stack[sp++] = pc;
                pc = inst->imm - 1;
            } else {
                printf("Stack overflow or invalid address in CALL\n");
                running = 0;
            }
            break;
        case RET:
            // pop pc from stack
            if (sp > 0) {
                pc = stack[--sp];
            } else {
                printf("Stack underflow in RET\n");
                running = 0;
            }
            break;
        case IN:
            // read integer from stdin to regs[op1]
            if (inst->op1 < current_reg_count) {
                printf("Input for R%u: ", inst->op1);
                fflush(stdout);
                if (scanf("%u", &regs[inst->op1]) != 1) {
                    regs[inst->op1] = 0;
                    while (getchar() != '\n' && !feof(stdin)); // clear input
                }
            }
            break;
        case OUT:
            // print regs[op1] to stdout
            if (inst->op1 < current_reg_count) {
                printf("%u\n", regs[inst->op1]);
            }
            break;
        case MOVB:
            // regs[op1] = memory[imm];
            if (inst->op1 < current_reg_count && inst->imm < MEM_SIZE) {
                regs[inst->op1] = memory[inst->imm];
            }
            break;
        case STRB:
            // memory[imm] = regs[op1] & 0xFF;
            if (inst->op1 < current_reg_count && inst->imm < MEM_SIZE) {
                memory[inst->imm] = regs[inst->op1] & 0xFF;
            }
            break;
        case MEMCPY:
            // memcpy(&memory[op1], &memory[op2], imm);
            if (inst->op1 + inst->imm <= MEM_SIZE && inst->op2 + inst->imm <= MEM_SIZE) {
                memcpy(&memory[inst->op1], &memory[inst->op2], inst->imm);
            }
            break;
        case JNE:
            // if (!flag_zero) pc = imm - 1;
            if (!flag_zero && inst->imm < MEM_SIZE) {
                pc = inst->imm - 1;
            }
            break;
        case JG:
            // if (last_cmp > 0) pc = imm - 1;
            if (last_cmp > 0 && inst->imm < MEM_SIZE) {
                pc = inst->imm - 1;
            }
            break;
        case JL:
            // if (last_cmp < 0) pc = imm - 1;
            if (last_cmp < 0 && inst->imm < MEM_SIZE) {
                pc = inst->imm - 1;
            }
            break;
        case CMP:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count) {
                flag_zero = (regs[inst->op1] == regs[inst->op2]);
                // signed comparison for JG/JL
                int32_t a = (int32_t)regs[inst->op1];
                int32_t b = (int32_t)regs[inst->op2];
                last_cmp = (a > b) ? 1 : (a < b) ? -1 : 0;
            }
            break;
        default:
            break;
    }
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

void print_memory_information() {
    printf("Memory Size: %d bytes\n", MEM_SIZE);
    printf("Registers: %u available (R0-R%u), max possible: %u\n", 
           current_reg_count, current_reg_count - 1, MAX_REGISTERS);
}

// Enhanced shell with 16-bit register support
void shell() {
    char line[MAX_LINE];
    printf("mighf-v2 (16-bit register support)\n");
    print_system_info();
    print_memory_information();
    printf("Type 'help' for commands.\n");
    while (1) {
        printf("coreshell> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strncmp(line, "exit", 4) == 0) break;
        else if (strncmp(line, "help", 4) == 0) {
            printf("Commands:\n");
            printf("  load <file>       - Load program\n");
            printf("  run               - Run program\n");
            printf("  regs              - Show first 32 registers\n");
            printf("  reg <num>         - Show specific register\n");
            printf("  regcount          - Show register count info\n");
            printf("  resize <count>    - Change number of available registers\n");
            printf("  mem <addr>        - Show memory at addr\n");
            printf("  exit              - Exit shell\n");
        }
        else if (strncmp(line, "regcount", 8) == 0) {
            printf("Current register count: %u (max: %u)\n", current_reg_count, MAX_REGISTERS);
        }
        else if (strncmp(line, "resize", 6) == 0) {
            uint32_t new_count = atoi(line + 7);
            if (new_count > 0 && new_count <= MAX_REGISTERS) {
                init_registers(new_count);
                printf("Resized to %u registers\n", current_reg_count);
            } else {
                printf("Invalid count. Must be 1-%u\n", MAX_REGISTERS);
            }
        }
        else if (strncmp(line, "regs", 4) == 0) {
            printf("Showing first 32 registers (total available: %u):\n", current_reg_count);
            uint32_t display_count = (current_reg_count > 32) ? 32 : current_reg_count;
            for (uint32_t i = 0; i < display_count; i++) {
                printf("R%u: %u\n", i, regs[i]);
            }
            if (current_reg_count > 32) {
                printf("... and %u more registers (use 'reg <num>' to view specific ones)\n", 
                       current_reg_count - 32);
            }
        }
        else if (strncmp(line, "reg ", 4) == 0) {
            uint32_t reg_num = atoi(line + 4);
            if (reg_num < current_reg_count) {
                printf("R%u: %u\n", reg_num, regs[reg_num]);
            } else {
                printf("Register R%u not available (max: R%u)\n", reg_num, current_reg_count - 1);
            }
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
    // Initialize with default register count
    init_registers(DEFAULT_REG_COUNT);
    
    memset(memory, 0, sizeof(memory));
    memset(program, 0, sizeof(program));

    // Initialize program counter structure
    ProgramCounter prog_counter;
    pc_init(&prog_counter);

    int loaded = 0;
    if (argc > 1) {
        run_file(argv[1]);
        loaded = 1;
    } else {
        shell();
        loaded = 1; // shell loads program if "run" is called
    }

    // If a program was loaded, run the fetch-decode-execute loop
    if (loaded) {
        running = 1;
        pc_set(&prog_counter, 0);
        while (running && pc_get(&prog_counter) < MEM_SIZE) {
            Instruction *inst = &program[pc_get(&prog_counter)];
            execute_instruction(inst);
            pc_increment(&prog_counter);
        }
        printf("Program finished.\n");
    }

    cleanup_registers();
    return 0;
}