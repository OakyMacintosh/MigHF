/*
________  ________   ________  _______   _____ ______   ________  ___       _______   ________     
|\   __  \|\   ____\ |\   ____\|\  ___ \ |\   _ \  _   \|\   __  \|\  \     |\  ___ \ |\   __  \    
\ \  \|\  \ \  \___|_\ \  \___|\ \   __/|\ \  \\\__\ \  \ \  \|\ /\ \  \    \ \   __/|\ \  \|\  \   
 \ \   __  \ \_____  \\ \_____  \ \  \_|/_\ \  \\|__| \  \ \   __  \ \  \    \ \  \_|/_\ \   _  _\  
  \ \  \ \  \|____|\  \\|____|\  \ \  \_|\ \ \  \    \ \  \ \  \|\  \ \  \____\ \  \_|\ \ \  \\  \| 
   \ \__\ \__\____\_\  \ ____\_\  \ \_______\ \__\    \ \__\ \_______\ \_______\ \_______\ \__\\ _\ 
    \|__|\|__|\_________\\_________\|_______|\|__|     \|__|\|_______|\|_______|\|_______|\|__|\|__|
             \|_________\|_________|                                                                

*/

#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>

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
    PRINT,  // 20
    TDRAW_CLEAR,
    TDRAW_PIXEL,

    // New instructions for advanced programs:
    PUSH,       // PUSH reg
    POP,        // POP reg
    CALL,       // CALL addr
    RET,        // RET
    IN,         // IN reg
    OUT,        // OUT reg
    MOVB,       // MOVB reg, addr (move byte from memory to reg)
    STRB,       // STRB reg, addr (store byte from reg to memory)
    MEMCPY,     // MEMCPY dst_addr, src_addr, len
    JNE,        // JNE addr (jump if not equal)
    JG,         // JG addr (jump if greater)
    JL,         // JL addr (jump if less)
    // ...add more as needed...
} Instr;

typedef struct {
    Instr opcode;
    uint16_t op1;    // 16-bit register addressing (0-65535)
    uint16_t op2;    // 16-bit register addressing (0-65535)
    uint32_t imm;
} Instruction;

int assemble(const char *line, Instruction *inst);
int parse_register_number(const char *reg_str);

#endif // ASSEMBLER_H

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int assemble(const char *line, Instruction *inst) {
    char cleanline[256];
    size_t i = 0, j = 0;

    // Remove comments starting with '#'
    while (line[i] && j < sizeof(cleanline) - 1) {
        if (line[i] == '#') break;
        cleanline[j++] = line[i++];
    }
    cleanline[j] = '\0';

    // Skip empty or comment-only lines
    for (i = 0; cleanline[i]; i++) {
        if (!isspace((unsigned char)cleanline[i])) break;
    }
    if (!cleanline[i]) return 0;

    char op[32], arg1[32], arg2[32], arg3[32];
    int n = sscanf(cleanline, "%31s %31s %31s %31s", op, arg1, arg2, arg3);
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
    else if (strcmp(op, "PUSH") == 0 && n == 2) {
        inst->opcode = PUSH;
        inst->op1 = parse_register_number(arg1);
    }
    else if (strcmp(op, "POP") == 0 && n == 2) {
        inst->opcode = POP;
        inst->op1 = parse_register_number(arg1);
    }
    else if (strcmp(op, "CALL") == 0 && n == 2) {
        inst->opcode = CALL;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "RET") == 0 && n == 1) {
        inst->opcode = RET;
    }
    else if (strcmp(op, "IN") == 0 && n == 2) {
        inst->opcode = IN;
        inst->op1 = parse_register_number(arg1);
    }
    else if (strcmp(op, "OUT") == 0 && n == 2) {
        inst->opcode = OUT;
        inst->op1 = parse_register_number(arg1);
    }
    else if (strcmp(op, "MOVB") == 0 && n == 3) {
        inst->opcode = MOVB;
        inst->op1 = parse_register_number(arg1);
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "STRB") == 0 && n == 3) {
        inst->opcode = STRB;
        inst->op1 = parse_register_number(arg1);
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "MEMCPY") == 0 && n == 4) {
        inst->opcode = MEMCPY;
        inst->op1 = atoi(arg1); // dst_addr
        inst->op2 = atoi(arg2); // src_addr
        inst->imm = atoi(arg3); // len
    }
    else if (strcmp(op, "JNE") == 0 && n == 2) {
        inst->opcode = JNE;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "JG") == 0 && n == 2) {
        inst->opcode = JG;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "JL") == 0 && n == 2) {
        inst->opcode = JL;
        inst->imm = atoi(arg1);
    }
    else return 0;
    return 1;
}

int parse_register_number(const char *reg_str) {
    // Validate input: must not be null and must start with 'R' or 'r'
    if (!reg_str || (reg_str[0] != 'R' && reg_str[0] != 'r')) {
        return -1;
    }
    
    // Parse the number part after 'R'
    char *endptr;
    long reg_num = strtol(reg_str + 1, &endptr, 10);
    
    // Validate: must be valid number, no extra characters, within 16-bit range
    if (*endptr != '\0' || reg_num < 0 || reg_num > 65535) {
        return -1;
    }
    
    return (int)reg_num;
}