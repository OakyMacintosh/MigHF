/*
 * MIGHF Universal Micro-Architecture Assembler
 * mighf-unknown-gnuabi-as.c
 * 
 * Assembles .mhf assembly files into MIGHF bytecode format
 * Supports heads, metadata, and code specifications
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_LINE_LENGTH 1024
#define MAX_LABEL_LENGTH 64
#define MAX_SYMBOLS 1024
#define MAX_RELOCATIONS 512
#define MAX_CODE_SIZE 65536

/* MIGHF File Format Structures */
typedef struct {
    char signature[4];      /* "MHF\0" */
    uint16_t version;       /* Format version */
    uint16_t flags;         /* Assembly flags */
    uint32_t head_offset;   /* Offset to head section */
    uint32_t code_offset;   /* Offset to code section */
    uint32_t data_offset;   /* Offset to data section */
    uint32_t symbol_offset; /* Offset to symbol table */
    uint32_t file_size;     /* Total file size */
} mhf_header_t;

typedef struct {
    char name[MAX_LABEL_LENGTH];
    uint32_t address;
    uint8_t type;           /* 0=local, 1=global, 2=external */
    uint8_t section;        /* 0=code, 1=data, 2=bss */
} symbol_t;

typedef struct {
    uint32_t offset;        /* Offset in code to patch */
    uint16_t symbol_index;  /* Index into symbol table */
    uint8_t type;           /* Relocation type */
} relocation_t;

/* Assembler State */
typedef struct {
    FILE *input;
    FILE *output;
    char *filename;
    int line_number;
    
    /* Code generation */
    uint8_t code_buffer[MAX_CODE_SIZE];
    uint32_t code_size;
    uint32_t current_address;
    
    /* Symbol management */
    symbol_t symbols[MAX_SYMBOLS];
    int symbol_count;
    
    /* Relocations */
    relocation_t relocations[MAX_RELOCATIONS];
    int relocation_count;
    
    /* Metadata */
    char *head_data;
    size_t head_size;
    
    /* Flags */
    int pass;               /* 1 or 2 */
    int errors;
} assembler_t;

/* Forward declarations */
void assemble_file(assembler_t *as);
void pass1(assembler_t *as);
void pass2(assembler_t *as);
int parse_line(assembler_t *as, char *line);
int parse_instruction(assembler_t *as, char *mnemonic, char *operands);
int parse_directive(assembler_t *as, char *directive, char *args);
void emit_byte(assembler_t *as, uint8_t byte);
void emit_word(assembler_t *as, uint16_t word);
void emit_dword(assembler_t *as, uint32_t dword);
int add_symbol(assembler_t *as, const char *name, uint32_t address, uint8_t type, uint8_t section);
int find_symbol(assembler_t *as, const char *name);
void add_relocation(assembler_t *as, uint32_t offset, uint16_t symbol_index, uint8_t type);
void write_output_file(assembler_t *as);
void error(assembler_t *as, const char *msg);
void warning(assembler_t *as, const char *msg);

/* MIGHF Instruction Set - Basic set for demonstration */
typedef struct {
    const char *mnemonic;
    uint8_t opcode;
    uint8_t operand_types;  /* Bitmask: 1=reg, 2=imm, 4=mem */
} instruction_t;

static const instruction_t instructions[] = {
    {"NOP",  0x00, 0},
    {"HALT", 0x01, 0},
    {"LOAD", 0x10, 3},      /* reg, imm */
    {"STORE",0x11, 3},      /* reg, mem */
    {"MOVE", 0x12, 1},      /* reg, reg */
    {"ADD",  0x20, 3},      /* reg, reg/imm */
    {"SUB",  0x21, 3},      /* reg, reg/imm */
    {"MUL",  0x22, 3},      /* reg, reg/imm */
    {"DIV",  0x23, 3},      /* reg, reg/imm */
    {"CMP",  0x30, 3},      /* reg, reg/imm */
    {"JMP",  0x40, 6},      /* imm/mem */
    {"JEQ",  0x41, 6},      /* imm/mem */
    {"JNE",  0x42, 6},      /* imm/mem */
    {"JLT",  0x43, 6},      /* imm/mem */
    {"JGT",  0x44, 6},      /* imm/mem */
    {"CALL", 0x50, 6},      /* imm/mem */
    {"RET",  0x51, 0},
    {"PUSH", 0x60, 3},      /* reg/imm */
    {"POP",  0x61, 1},      /* reg */
    {NULL, 0, 0}
};

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.mhf> [output.mhfb]\n", argv[0]);
        return 1;
    }
    
    assembler_t as = {0};
    as.filename = argv[1];
    
    /* Open input file */
    as.input = fopen(argv[1], "r");
    if (!as.input) {
        perror(argv[1]);
        return 1;
    }
    
    /* Open output file */
    char output_name[256];
    if (argc > 2) {
        strcpy(output_name, argv[2]);
    } else {
        strcpy(output_name, argv[1]);
        char *ext = strrchr(output_name, '.');
        if (ext) *ext = '\0';
        strcat(output_name, ".mhfb");
    }
    
    as.output = fopen(output_name, "wb");
    if (!as.output) {
        perror(output_name);
        fclose(as.input);
        return 1;
    }
    
    printf("MIGHF Assembler v1.0\n");
    printf("Assembling %s -> %s\n", argv[1], output_name);
    
    /* Two-pass assembly */
    assemble_file(&as);
    
    if (as.errors == 0) {
        write_output_file(&as);
        printf("Assembly successful: %d bytes, %d symbols\n", 
               as.code_size, as.symbol_count);
    } else {
        printf("Assembly failed with %d errors\n", as.errors);
    }
    
    fclose(as.input);
    fclose(as.output);
    free(as.head_data);
    
    return as.errors ? 1 : 0;
}

void assemble_file(assembler_t *as) {
    /* Pass 1: Collect symbols and calculate addresses */
    as->pass = 1;
    as->line_number = 0;
    as->current_address = 0;
    as->code_size = 0;
    
    printf("Pass 1: Symbol collection...\n");
    pass1(as);
    
    if (as->errors > 0) return;
    
    /* Pass 2: Generate code and resolve references */
    rewind(as->input);
    as->pass = 2;
    as->line_number = 0;
    as->current_address = 0;
    as->code_size = 0;
    
    printf("Pass 2: Code generation...\n");
    pass2(as);
}

void pass1(assembler_t *as) {
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), as->input)) {
        as->line_number++;
        
        /* Remove newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        
        /* Skip empty lines and comments */
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') continue;
        
        if (parse_line(as, trimmed) < 0) {
            as->errors++;
        }
    }
}

void pass2(assembler_t *as) {
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), as->input)) {
        as->line_number++;
        
        /* Remove newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        
        /* Skip empty lines and comments */
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') continue;
        
        if (parse_line(as, trimmed) < 0) {
            as->errors++;
        }
    }
}

int parse_line(assembler_t *as, char *line) {
    char *token = strtok(line, " \t");
    if (!token) return 0;
    
    /* Check for label */
    if (token[strlen(token) - 1] == ':') {
        token[strlen(token) - 1] = '\0';  /* Remove colon */
        
        if (as->pass == 1) {
            if (add_symbol(as, token, as->current_address, 1, 0) < 0) {
                error(as, "Duplicate label");
                return -1;
            }
        }
        
        /* Get next token */
        token = strtok(NULL, " \t");
        if (!token) return 0;
    }
    
    /* Check for directive */
    if (token[0] == '.') {
        char *args = strtok(NULL, "");
        return parse_directive(as, token, args);
    }
    
    /* Must be an instruction */
    char *operands = strtok(NULL, "");
    return parse_instruction(as, token, operands);
}

int parse_instruction(assembler_t *as, char *mnemonic, char *operands) {
    /* Find instruction in table */
    const instruction_t *inst = NULL;
    for (int i = 0; instructions[i].mnemonic; i++) {
        if (strcasecmp(mnemonic, instructions[i].mnemonic) == 0) {
            inst = &instructions[i];
            break;
        }
    }
    
    if (!inst) {
        error(as, "Unknown instruction");
        return -1;
    }
    
    if (as->pass == 2) {
        emit_byte(as, inst->opcode);
        
        /* Parse operands - simplified for demo */
        if (operands && *operands) {
            char *op1 = strtok(operands, ",");
            char *op2 = strtok(NULL, ",");
            
            /* Handle immediate values and registers */
            if (op1) {
                if (op1[0] == '#') {
                    /* Immediate value */
                    int value = strtol(op1 + 1, NULL, 0);
                    emit_word(as, (uint16_t)value);
                } else if (op1[0] == 'R' || op1[0] == 'r') {
                    /* Register */
                    int reg = strtol(op1 + 1, NULL, 10);
                    emit_byte(as, (uint8_t)reg);
                } else {
                    /* Symbol reference */
                    int sym_idx = find_symbol(as, op1);
                    if (sym_idx >= 0) {
                        emit_word(as, as->symbols[sym_idx].address);
                    } else {
                        add_relocation(as, as->code_size, 0, 1);
                        emit_word(as, 0);  /* Placeholder */
                    }
                }
            }
            
            if (op2) {
                if (op2[0] == '#') {
                    int value = strtol(op2 + 1, NULL, 0);
                    emit_word(as, (uint16_t)value);
                } else if (op2[0] == 'R' || op2[0] == 'r') {
                    int reg = strtol(op2 + 1, NULL, 10);
                    emit_byte(as, (uint8_t)reg);
                }
            }
        }
    } else {
        /* Pass 1: Just advance address */
        as->current_address += 1;  /* Opcode */
        
        /* Estimate operand size - simplified */
        if (operands && *operands) {
            as->current_address += 2;  /* Assume word operands */
        }
    }
    
    return 0;
}

int parse_directive(assembler_t *as, char *directive, char *args) {
    if (strcasecmp(directive, ".head") == 0) {
        /* Store head metadata */
        if (as->pass == 1 && args) {
            as->head_size = strlen(args) + 1;
            as->head_data = malloc(as->head_size);
            strcpy(as->head_data, args);
        }
    } else if (strcasecmp(directive, ".org") == 0) {
        /* Set origin address */
        if (args) {
            as->current_address = strtol(args, NULL, 0);
        }
    } else if (strcasecmp(directive, ".db") == 0) {
        /* Define bytes */
        if (as->pass == 2 && args) {
            char *token = strtok(args, ",");
            while (token) {
                int value = strtol(token, NULL, 0);
                emit_byte(as, (uint8_t)value);
                token = strtok(NULL, ",");
            }
        } else if (as->pass == 1) {
            /* Count bytes */
            char *temp = strdup(args);
            char *token = strtok(temp, ",");
            while (token) {
                as->current_address++;
                token = strtok(NULL, ",");
            }
            free(temp);
        }
    } else if (strcasecmp(directive, ".dw") == 0) {
        /* Define words */
        if (as->pass == 2 && args) {
            char *token = strtok(args, ",");
            while (token) {
                int value = strtol(token, NULL, 0);
                emit_word(as, (uint16_t)value);
                token = strtok(NULL, ",");
            }
        } else if (as->pass == 1) {
            char *temp = strdup(args);
            char *token = strtok(temp, ",");
            while (token) {
                as->current_address += 2;
                token = strtok(NULL, ",");
            }
            free(temp);
        }
    }
    
    return 0;
}

void emit_byte(assembler_t *as, uint8_t byte) {
    if (as->code_size >= MAX_CODE_SIZE) {
        error(as, "Code size exceeded");
        return;
    }
    as->code_buffer[as->code_size++] = byte;
    as->current_address++;
}

void emit_word(assembler_t *as, uint16_t word) {
    emit_byte(as, word & 0xFF);
    emit_byte(as, (word >> 8) & 0xFF);
}

void emit_dword(assembler_t *as, uint32_t dword) {
    emit_word(as, dword & 0xFFFF);
    emit_word(as, (dword >> 16) & 0xFFFF);
}

int add_symbol(assembler_t *as, const char *name, uint32_t address, uint8_t type, uint8_t section) {
    if (as->symbol_count >= MAX_SYMBOLS) return -1;
    
    /* Check for duplicates */
    for (int i = 0; i < as->symbol_count; i++) {
        if (strcmp(as->symbols[i].name, name) == 0) {
            return -1;  /* Duplicate */
        }
    }
    
    symbol_t *sym = &as->symbols[as->symbol_count++];
    strncpy(sym->name, name, MAX_LABEL_LENGTH - 1);
    sym->name[MAX_LABEL_LENGTH - 1] = '\0';
    sym->address = address;
    sym->type = type;
    sym->section = section;
    
    return as->symbol_count - 1;
}

int find_symbol(assembler_t *as, const char *name) {
    for (int i = 0; i < as->symbol_count; i++) {
        if (strcmp(as->symbols[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

void add_relocation(assembler_t *as, uint32_t offset, uint16_t symbol_index, uint8_t type) {
    if (as->relocation_count >= MAX_RELOCATIONS) return;
    
    relocation_t *rel = &as->relocations[as->relocation_count++];
    rel->offset = offset;
    rel->symbol_index = symbol_index;
    rel->type = type;
}

void write_output_file(assembler_t *as) {
    mhf_header_t header = {0};
    
    /* Write header */
    memcpy(header.signature, "MHF\0", 4);
    header.version = 1;
    header.flags = 0;
    header.head_offset = sizeof(mhf_header_t);
    header.code_offset = header.head_offset + (as->head_size ? as->head_size : 0);
    header.data_offset = header.code_offset + as->code_size;
    header.symbol_offset = header.data_offset;
    header.file_size = header.symbol_offset + (as->symbol_count * sizeof(symbol_t));
    
    fwrite(&header, sizeof(header), 1, as->output);
    
    /* Write head section */
    if (as->head_data && as->head_size > 0) {
        fwrite(as->head_data, as->head_size, 1, as->output);
    }
    
    /* Write code section */
    fwrite(as->code_buffer, as->code_size, 1, as->output);
    
    /* Write symbol table */
    fwrite(as->symbols, sizeof(symbol_t), as->symbol_count, as->output);
}

void error(assembler_t *as, const char *msg) {
    fprintf(stderr, "%s:%d: Error: %s\n", as->filename, as->line_number, msg);
}

void warning(assembler_t *as, const char *msg) {
    fprintf(stderr, "%s:%d: Warning: %s\n", as->filename, as->line_number, msg);
}