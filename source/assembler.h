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
    TDRAW_PIXEL
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
    char op[32], arg1[32], arg2[32], arg3[32];
    int n = sscanf(line, "%31s %31s %31s %31s", op, arg1, arg2, arg3);
    if (n < 1) return 0;

    // Convert op to uppercase for case-insensitive matching
    for (int i = 0; op[i]; i++) op[i] = toupper((unsigned char)op[i]);

    if (strcmp(op, "NOP") == 0) { 
        inst->opcode = NOP; 
    }
    else if (strcmp(op, "MOV") == 0 && n == 3) {
        inst->opcode = MOV;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "ADD") == 0 && n == 3) {
        inst->opcode = ADD;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "SUB") == 0 && n == 3) {
        inst->opcode = SUB;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "LOAD") == 0 && n == 3) {
        inst->opcode = LOAD;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "STORE") == 0 && n == 3) {
        inst->opcode = STORE;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "JMP") == 0 && n == 2) {
        inst->opcode = JMP;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "CMP") == 0 && n == 3) {
        inst->opcode = CMP;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "JE") == 0 && n == 2) {
        inst->opcode = JE;
        inst->imm = atoi(arg1);
    }
    else if (strcmp(op, "HALT") == 0) { 
        inst->opcode = HALT; 
    }
    else if (strcmp(op, "AND") == 0 && n == 3) {
        inst->opcode = AND;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "ORR") == 0 && n == 3) {
        inst->opcode = ORR;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "EOR") == 0 && n == 3) {
        inst->opcode = EOR;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "LSL") == 0 && n == 3) {
        inst->opcode = LSL;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "LSR") == 0 && n == 3) {
        inst->opcode = LSR;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "MUL") == 0 && n == 3) {
        inst->opcode = MUL;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "UDIV") == 0 && n == 3) {
        inst->opcode = UDIV;
        int reg1 = parse_register_number(arg1);
        int reg2 = parse_register_number(arg2);
        if (reg1 < 0 || reg2 < 0) return 0;
        inst->op1 = reg1;
        inst->op2 = reg2;
    }
    else if (strcmp(op, "NEG") == 0 && n == 2) {
        inst->opcode = NEG;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
    }
    else if (strcmp(op, "MOVZ") == 0 && n == 3) {
        inst->opcode = MOVZ;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
        inst->imm = atoi(arg2);
    }
    else if (strcmp(op, "MOVN") == 0 && n == 3) {
        inst->opcode = MOVN;
        int reg = parse_register_number(arg1);
        if (reg < 0) return 0;
        inst->op1 = reg;
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
    } 
    else if (strcmp(op, "TDRAW_PIXEL") == 0 && n == 4) {
        inst->opcode = TDRAW_PIXEL;
        int rx = parse_register_number(arg1);
        int ry = parse_register_number(arg2);
        if (rx < 0 || ry < 0) return 0;
        int ch = arg3[0];
        inst->imm = rx | (ry << 8) | (ch << 16);
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