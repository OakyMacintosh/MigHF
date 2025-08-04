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
    
    CC65 PORT - Adapted for 8-bit systems with ANSI C compatibility
    
    DEVELOPER NOTES:
        This is a cc65 port of the original mighf virtual machine.
        Designed for 8-bit systems like Apple II, Commodore 64, Atari, etc.
        Memory usage has been optimized for limited RAM environments.
        
    LICENSE:
        This code is licensed under the MIT License.
        Original author: OakyMac (u/OakyMac)
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* cc65 doesn't have stdint.h, so we define our own types */
typedef unsigned char uint8_t;
typedef unsigned int uint16_t;
typedef unsigned long uint32_t;
typedef signed char int8_t;
typedef signed int int16_t;
typedef signed long int32_t;

/* Reduced memory footprint for 8-bit systems */
#define MEM_SIZE 512          /* Reduced from 2024 */
#define MAX_REGISTERS 64      /* Reduced from 65536 */
#define DEFAULT_REG_COUNT 16  /* Reduced from 256 */
#define MAX_PROGRAM_SIZE 256  /* Reduced from 1024 */
#define MAX_LINE 64           /* Reduced from 128 */
#define STACK_SIZE 32         /* Reduced from 1024 */

/* Opcode definitions - assuming these are in assembler.h */
typedef enum {
    NOP = 0, MOV, ADD, SUB, LOAD, STORE, JMP, JE, HALT,
    AND, ORR, EOR, LSL, LSR, MUL, UDIV, NEG, MOVZ, MOVN,
    PRINT, TDRAW_CLEAR, TDRAW_PIXEL, PUSH, POP, CALL, RET,
    IN, OUT, MOVB, STRB, MEMCPY, JNE, JG, JL, CMP
} Opcode;

/* Instruction structure */
typedef struct {
    Opcode opcode;
    uint16_t op1;
    uint16_t op2;
    uint16_t imm;
} Instruction;

/* Program Counter structure - simplified */
typedef struct {
    uint16_t pc;
} ProgramCounter;

/* Global variables - static allocation for cc65 */
static uint16_t regs[MAX_REGISTERS];
static uint8_t current_reg_count = DEFAULT_REG_COUNT;
static uint8_t memory[MEM_SIZE];
static uint16_t pc = 0;
static uint8_t running = 1;

/* Stack support */
static uint16_t stack[STACK_SIZE];
static uint8_t sp = 0;

/* Comparison result */
static int8_t last_cmp = 0;

/* Program memory */
static Instruction program[MAX_PROGRAM_SIZE];

/* Flags */
static uint8_t flag_zero = 0;

/* Function prototypes */
void init_registers(uint8_t count);
void execute_instruction(Instruction *inst);
void print_system_info(void);
void print_memory_information(void);
void shell(void);
void run_file(const char *fname);
void tdraw_clear(void);
void tdraw_pixel(int x, int y, char c);
int assemble(const char *line, Instruction *inst);

/* Program Counter functions - simplified */
void pc_init(ProgramCounter *pc_struct) {
    pc_struct->pc = 0;
}

void pc_set(ProgramCounter *pc_struct, uint16_t value) {
    pc_struct->pc = value;
}

uint16_t pc_get(ProgramCounter *pc_struct) {
    return pc_struct->pc;
}

void pc_increment(ProgramCounter *pc_struct) {
    pc_struct->pc++;
}

/* Initialize registers with specified count */
void init_registers(uint8_t count) {
    uint8_t i;
    current_reg_count = (count > MAX_REGISTERS) ? MAX_REGISTERS : count;
    
    /* Clear all registers */
    for (i = 0; i < current_reg_count; i++) {
        regs[i] = 0;
    }
}

/* Simplified assembler - basic implementation */
int assemble(const char *line, Instruction *inst) {
    char opcode_str[16];
    int result;
    
    /* Skip empty lines and comments */
    if (line[0] == '\0' || line[0] == '\n' || line[0] == ';') {
        return 0;
    }
    
    /* Parse the line - very basic implementation */
    result = sscanf(line, "%15s", opcode_str);
    if (result != 1) return 0;
    
    /* Convert to uppercase for comparison */
    {
        int i;
        for (i = 0; opcode_str[i]; i++) {
            opcode_str[i] = toupper(opcode_str[i]);
        }
    }
    
    /* Simple opcode matching */
    if (strcmp(opcode_str, "NOP") == 0) {
        inst->opcode = NOP;
        return 1;
    } else if (strcmp(opcode_str, "HALT") == 0) {
        inst->opcode = HALT;
        return 1;
    } else if (strcmp(opcode_str, "MOV") == 0) {
        inst->opcode = MOV;
        if (sscanf(line, "%*s %hu %hu", &inst->op1, &inst->imm) == 2) {
            return 1;
        }
    } else if (strcmp(opcode_str, "ADD") == 0) {
        inst->opcode = ADD;
        if (sscanf(line, "%*s %hu %hu", &inst->op1, &inst->op2) == 2) {
            return 1;
        }
    } else if (strcmp(opcode_str, "PRINT") == 0) {
        inst->opcode = PRINT;
        if (sscanf(line, "%*s %hu %hu", &inst->op1, &inst->imm) == 2) {
            return 1;
        }
    }
    
    return 0; /* Failed to parse */
}

/* Micro-architecture: fetch-decode-execute */
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
                memory[inst->imm] = (uint8_t)(regs[inst->op1] & 0xFF);
            break;
        case JMP:
            if (inst->imm < MAX_PROGRAM_SIZE)
                pc = inst->imm - 1;
            break;
        case JE:
            if (flag_zero && inst->imm < MAX_PROGRAM_SIZE)
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
                regs[inst->op1] = (uint16_t)(-(int16_t)regs[inst->op1]);
            break;
        case MOVZ:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] = inst->imm;
            break;
        case MOVN:
            if (inst->op1 < current_reg_count)
                regs[inst->op1] = ~(inst->imm);
            break;
        case PRINT:
            /* PRINT REG idx  or PRINT MEM idx */
            if (inst->op1 == 0 && inst->imm < current_reg_count) /* REG */
                printf("R%u = %u\n", inst->imm, regs[inst->imm]);
            else if (inst->op1 == 1 && inst->imm < MEM_SIZE) /* MEM */
                printf("MEM[%u] = %u\n", inst->imm, memory[inst->imm]);
            break;
        case TDRAW_CLEAR:
            tdraw_clear();
            break;
        case PUSH:
            if (sp < STACK_SIZE && inst->op1 < current_reg_count) {
                stack[sp++] = regs[inst->op1];
            } else {
                printf("Stack overflow or invalid register in PUSH\n");
                running = 0;
            }
            break;
        case POP:
            if (sp > 0 && inst->op1 < current_reg_count) {
                regs[inst->op1] = stack[--sp];
            } else {
                printf("Stack underflow or invalid register in POP\n");
                running = 0;
            }
            break;
        case CALL:
            if (sp < STACK_SIZE && inst->imm < MAX_PROGRAM_SIZE) {
                stack[sp++] = pc;
                pc = inst->imm - 1;
            } else {
                printf("Stack overflow or invalid address in CALL\n");
                running = 0;
            }
            break;
        case RET:
            if (sp > 0) {
                pc = stack[--sp];
            } else {
                printf("Stack underflow in RET\n");
                running = 0;
            }
            break;
        case IN:
            if (inst->op1 < current_reg_count) {
                printf("Input for R%u: ", inst->op1);
                scanf("%u", &regs[inst->op1]);
            }
            break;
        case OUT:
            if (inst->op1 < current_reg_count) {
                printf("%u\n", regs[inst->op1]);
            }
            break;
        case CMP:
            if (inst->op1 < current_reg_count && inst->op2 < current_reg_count) {
                flag_zero = (regs[inst->op1] == regs[inst->op2]);
                /* signed comparison for JG/JL */
                {
                    int16_t a = (int16_t)regs[inst->op1];
                    int16_t b = (int16_t)regs[inst->op2];
                    last_cmp = (a > b) ? 1 : (a < b) ? -1 : 0;
                }
            }
            break;
        case JNE:
            if (!flag_zero && inst->imm < MAX_PROGRAM_SIZE) {
                pc = inst->imm - 1;
            }
            break;
        case JG:
            if (last_cmp > 0 && inst->imm < MAX_PROGRAM_SIZE) {
                pc = inst->imm - 1;
            }
            break;
        case JL:
            if (last_cmp < 0 && inst->imm < MAX_PROGRAM_SIZE) {
                pc = inst->imm - 1;
            }
            break;
        default:
            break;
    }
}

/* Print system information - simplified for cc65 */
void print_system_info() {
    printf("CC65 System\n");
    printf("Architecture: 8-bit\n");
    printf("Processors: 1\n");
}

void print_memory_information() {
    printf("Memory Size: %d bytes\n", MEM_SIZE);
    printf("Registers: %u available (R0-R%u), max possible: %u\n", 
           current_reg_count, current_reg_count - 1, MAX_REGISTERS);
}

/* Enhanced shell - simplified for cc65 */
void shell() {
    char line[MAX_LINE];
    printf("mighf-cc65 (8-bit port)\n");
    print_system_info();
    print_memory_information();
    printf("Type 'help' for commands.\n");
    
    while (1) {
        printf("0 > ");
        if (!fgets(line, sizeof(line), stdin)) break;
        
        if (strncmp(line, "exit", 4) == 0) break;
        else if (strncmp(line, "help", 4) == 0) {
            printf("Commands:\n");
            printf("  load <file>       - Load program\n");
            printf("  run               - Run program\n");
            printf("  regs              - Show registers\n");
            printf("  reg <num>         - Show specific register\n");
            printf("  mem <addr>        - Show memory at addr\n");
            printf("  exit              - Exit shell\n");
        }
        else if (strncmp(line, "regs", 4) == 0) {
            uint8_t i;
            printf("Registers (total available: %u):\n", current_reg_count);
            for (i = 0; i < current_reg_count; i++) {
                printf("R%u: %u\n", i, regs[i]);
            }
        }
        else if (strncmp(line, "reg ", 4) == 0) {
            uint8_t reg_num = (uint8_t)atoi(line + 4);
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
            char fname[32];
            if (sscanf(line, "load %31s", fname) == 1) {
                FILE *f = fopen(fname, "r");
                if (!f) { printf("Cannot open file\n"); continue; }
                {
                    char pline[MAX_LINE];
                    int idx = 0;
                    while (fgets(pline, sizeof(pline), f) && idx < MAX_PROGRAM_SIZE) {
                        if (assemble(pline, &program[idx]))
                            idx++;
                    }
                    fclose(f);
                    printf("Loaded %d instructions\n", idx);
                }
            } else {
                printf("Usage: load <file>\n");
            }
        }
        else if (strncmp(line, "run", 3) == 0) {
            pc = 0;
            running = 1;
            while (running && pc < MAX_PROGRAM_SIZE) {
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

/* CLI mode: load and run file */
void run_file(const char *fname) {
    FILE *f = fopen(fname, "r");
    if (!f) {
        printf("Cannot open file: %s\n", fname);
        return;
    }
    {
        char pline[MAX_LINE];
        int idx = 0;
        while (fgets(pline, sizeof(pline), f) && idx < MAX_PROGRAM_SIZE) {
            if (assemble(pline, &program[idx]))
                idx++;
        }
        fclose(f);
        printf("Loaded %d instructions from %s\n", idx, fname);
        pc = 0;
        running = 1;
        while (running && pc < MAX_PROGRAM_SIZE) {
            execute_instruction(&program[pc]);
            pc++;
        }
        printf("Program finished.\n");
    }
}

void tdraw_clear() {
    /* For cc65, we'll use simple screen clearing */
    printf("\f"); /* Form feed - clears screen on many systems */
}

void tdraw_pixel(int x, int y, char c) {
    printf("*"); /* Simple pixel representation */
}

int main(int argc, char **argv) {
    /* Initialize with default register count */
    init_registers(DEFAULT_REG_COUNT);
    
    /* Clear memory and program */
    {
        int i;
        for (i = 0; i < MEM_SIZE; i++) {
            memory[i] = 0;
        }
        for (i = 0; i < MAX_PROGRAM_SIZE; i++) {
            program[i].opcode = NOP;
            program[i].op1 = 0;
            program[i].op2 = 0;
            program[i].imm = 0;
        }
    }

    if (argc > 1) {
        run_file(argv[1]);
    } else {
        shell();
    }

    return 0;
}
