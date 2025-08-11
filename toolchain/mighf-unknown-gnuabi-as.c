/*
 * MIGHF Universal Micro-Architecture Assembler - Extended Edition
 * mighf-unknown-gnuabi-as.c
 * 
 * Assembles .mhf assembly files into MIGHF bytecode format
 * Supports complete extended ISA for OS development
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 1024
#define MAX_LABEL_LENGTH 64
#define MAX_SYMBOLS 1024
#define MAX_RELOCATIONS 512
#define MAX_CODE_SIZE 65536
#define MAX_INCLUDE_DEPTH 16

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
} __attribute__((packed)) mhf_header_t;

typedef struct {
    char name[MAX_LABEL_LENGTH];
    uint32_t address;
    uint8_t type;           /* 0=local, 1=global, 2=external */
    uint8_t section;        /* 0=code, 1=data, 2=bss */
    uint8_t flags;          /* Additional flags */
} symbol_t;

typedef struct {
    uint32_t offset;        /* Offset in code to patch */
    uint16_t symbol_index;  /* Index into symbol table */
    uint8_t type;           /* Relocation type */
    uint8_t size;           /* 1=byte, 2=word, 4=dword */
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
    uint32_t origin;
    
    /* Symbol management */
    symbol_t symbols[MAX_SYMBOLS];
    int symbol_count;
    
    /* Relocations */
    relocation_t relocations[MAX_RELOCATIONS];
    int relocation_count;
    
    /* Metadata */
    char *head_data;
    size_t head_size;
    
    /* Macros and constants */
    struct {
        char name[MAX_LABEL_LENGTH];
        char *definition;
    } macros[256];
    int macro_count;
    
    /* Conditional assembly */
    int if_stack[16];
    int if_depth;
    int skip_code;
    
    /* Flags */
    int pass;               /* 1 or 2 */
    int errors;
    int warnings;
    int verbose;
} assembler_t;

/* Operand types */
#define OP_NONE    0x00
#define OP_REG     0x01
#define OP_IMM8    0x02
#define OP_IMM16   0x04
#define OP_MEM     0x08
#define OP_REL     0x10  /* Relative address */
#define OP_ABS     0x20  /* Absolute address */

/* Extended MIGHF Instruction Set */
typedef struct {
    const char *mnemonic;
    uint8_t opcode;
    uint8_t operand_count;
    uint8_t operand_types[3];  /* Up to 3 operands */
    const char *description;
} instruction_t;

static const instruction_t instructions[] = {
    // Basic Operations (0x00-0x0F)
    {"NOP",      0x00, 0, {OP_NONE}, "No operation"},
    {"HALT",     0x01, 0, {OP_NONE}, "Halt processor"},
    {"BREAK",    0x02, 0, {OP_NONE}, "Debug breakpoint"},
    {"SYSCALL",  0x03, 0, {OP_NONE}, "System call"},
    
    // Data Movement (0x10-0x1F)
    {"LOAD",     0x10, 2, {OP_REG, OP_IMM8}, "Load immediate 8-bit"},
    {"LOAD16",   0x11, 2, {OP_REG, OP_IMM16}, "Load immediate 16-bit"},
    {"LDMEM",    0x12, 2, {OP_REG, OP_ABS}, "Load from memory"},
    {"STMEM",    0x13, 2, {OP_ABS, OP_REG}, "Store to memory"},
    {"MOV",      0x14, 2, {OP_REG, OP_REG}, "Move register to register"},
    {"LDINDIR",  0x15, 2, {OP_REG, OP_REG}, "Load indirect [reg]"},
    {"STINDIR",  0x16, 2, {OP_REG, OP_REG}, "Store indirect [reg]"},
    {"PUSH",     0x17, 1, {OP_REG}, "Push register"},
    {"POP",      0x18, 1, {OP_REG}, "Pop register"},
    {"XCHG",     0x19, 2, {OP_REG, OP_REG}, "Exchange registers"},
    
    // Arithmetic (0x20-0x2F)
    {"ADD",      0x20, 2, {OP_REG, OP_REG}, "Add registers"},
    {"ADDI",     0x21, 2, {OP_REG, OP_IMM8}, "Add immediate"},
    {"SUB",      0x22, 2, {OP_REG, OP_REG}, "Subtract registers"},
    {"SUBI",     0x23, 2, {OP_REG, OP_IMM8}, "Subtract immediate"},
    {"MUL",      0x24, 2, {OP_REG, OP_REG}, "Multiply registers"},
    {"DIV",      0x25, 2, {OP_REG, OP_REG}, "Divide registers"},
    {"MOD",      0x26, 2, {OP_REG, OP_REG}, "Modulo registers"},
    {"INC",      0x27, 1, {OP_REG}, "Increment register"},
    {"DEC",      0x28, 1, {OP_REG}, "Decrement register"},
    {"NEG",      0x29, 1, {OP_REG}, "Negate register"},
    
    // Logical Operations (0x30-0x3F)
    {"AND",      0x30, 2, {OP_REG, OP_REG}, "Bitwise AND"},
    {"ANDI",     0x31, 2, {OP_REG, OP_IMM8}, "AND immediate"},
    {"OR",       0x32, 2, {OP_REG, OP_REG}, "Bitwise OR"},
    {"ORI",      0x33, 2, {OP_REG, OP_IMM8}, "OR immediate"},
    {"XOR",      0x34, 2, {OP_REG, OP_REG}, "Bitwise XOR"},
    {"XORI",     0x35, 2, {OP_REG, OP_IMM8}, "XOR immediate"},
    {"NOT",      0x36, 1, {OP_REG}, "Bitwise NOT"},
    {"SHL",      0x37, 2, {OP_REG, OP_IMM8}, "Shift left"},
    {"SHR",      0x38, 2, {OP_REG, OP_IMM8}, "Shift right"},
    {"ROL",      0x39, 2, {OP_REG, OP_IMM8}, "Rotate left"},
    {"ROR",      0x3A, 2, {OP_REG, OP_IMM8}, "Rotate right"},
    
    // Comparison (0x40-0x4F)
    {"CMP",      0x40, 2, {OP_REG, OP_REG}, "Compare registers"},
    {"CMPI",     0x41, 2, {OP_REG, OP_IMM8}, "Compare immediate"},
    {"TEST",     0x42, 2, {OP_REG, OP_REG}, "Test (AND without store)"},
    {"TESTI",    0x43, 2, {OP_REG, OP_IMM8}, "Test immediate"},
    
    // Control Flow (0x50-0x6F)
    {"JMP",      0x50, 1, {OP_ABS}, "Unconditional jump"},
    {"JMPR",     0x51, 1, {OP_REG}, "Jump to register"},
    {"JZ",       0x52, 1, {OP_ABS}, "Jump if zero"},
    {"JNZ",      0x53, 1, {OP_ABS}, "Jump if not zero"},
    {"JC",       0x54, 1, {OP_ABS}, "Jump if carry"},
    {"JNC",      0x55, 1, {OP_ABS}, "Jump if not carry"},
    {"JN",       0x56, 1, {OP_ABS}, "Jump if negative"},
    {"JNN",      0x57, 1, {OP_ABS}, "Jump if not negative"},
    {"JV",       0x58, 1, {OP_ABS}, "Jump if overflow"},
    {"JNV",      0x59, 1, {OP_ABS}, "Jump if not overflow"},
    {"JL",       0x5A, 1, {OP_ABS}, "Jump if less (signed)"},
    {"JLE",      0x5B, 1, {OP_ABS}, "Jump if less or equal"},
    {"JG",       0x5C, 1, {OP_ABS}, "Jump if greater (signed)"},
    {"JGE",      0x5D, 1, {OP_ABS}, "Jump if greater or equal"},
    {"JB",       0x5E, 1, {OP_ABS}, "Jump if below (unsigned)"},
    {"JBE",      0x5F, 1, {OP_ABS}, "Jump if below or equal"},
    {"JA",       0x60, 1, {OP_ABS}, "Jump if above (unsigned)"},
    {"JAE",      0x61, 1, {OP_ABS}, "Jump if above or equal"},
    {"CALL",     0x62, 1, {OP_ABS}, "Call subroutine"},
    {"CALLR",    0x63, 1, {OP_REG}, "Call register"},
    {"RET",      0x64, 0, {OP_NONE}, "Return from subroutine"},
    {"LOOP",     0x65, 1, {OP_ABS}, "Loop (decrement R0, jump if not zero)"},
    
    // Privileged Instructions (0x70-0x7F)
    {"LDIDT",    0x70, 1, {OP_ABS}, "Load interrupt descriptor table"},
    {"LIDT",     0x71, 1, {OP_ABS}, "Load interrupt table register"},
    {"STI",      0x72, 0, {OP_NONE}, "Set interrupt flag"},
    {"CLI",      0x73, 0, {OP_NONE}, "Clear interrupt flag"},
    {"IRET",     0x74, 0, {OP_NONE}, "Interrupt return"},
    {"HLT",      0x75, 0, {OP_NONE}, "Privileged halt"},
    {"LGDT",     0x76, 1, {OP_ABS}, "Load global descriptor table"},
    {"LTR",      0x77, 1, {OP_REG}, "Load task register"},
    {"LLDT",     0x78, 1, {OP_REG}, "Load local descriptor table"},
    {"INVLPG",   0x79, 1, {OP_ABS}, "Invalidate page in TLB"},
    {"WRMSR",    0x7A, 2, {OP_REG, OP_REG}, "Write MSR"},
    {"RDMSR",    0x7B, 1, {OP_REG}, "Read MSR"},
    {"CPUID",    0x7C, 0, {OP_NONE}, "CPU identification"},
    {"RDTSC",    0x7D, 0, {OP_NONE}, "Read timestamp counter"},
    {"IN",       0x7E, 2, {OP_REG, OP_IMM8}, "Input from port"},
    {"OUT",      0x7F, 2, {OP_IMM8, OP_REG}, "Output to port"},
    
    // Memory Management (0x80-0x8F)
    {"LDCR",     0x80, 2, {OP_REG, OP_REG}, "Load control register"},
    {"STCR",     0x81, 2, {OP_REG, OP_REG}, "Store control register"},
    {"LDDR",     0x82, 2, {OP_REG, OP_REG}, "Load debug register"},
    {"STDR",     0x83, 2, {OP_REG, OP_REG}, "Store debug register"},
    {"INVD",     0x84, 0, {OP_NONE}, "Invalidate cache"},
    {"WBINVD",   0x85, 0, {OP_NONE}, "Write back and invalidate"},
    {"PREFETCH", 0x86, 1, {OP_ABS}, "Prefetch memory"},
    {"FLUSHTLB", 0x87, 0, {OP_NONE}, "Flush TLB"},
    
    // String/Block Operations (0x90-0x9F)
    {"MOVS",     0x90, 0, {OP_NONE}, "Move string"},
    {"CMPS",     0x91, 0, {OP_NONE}, "Compare strings"},
    {"SCAS",     0x92, 0, {OP_NONE}, "Scan string"},
    {"LODS",     0x93, 0, {OP_NONE}, "Load string"},
    {"STOS",     0x94, 0, {OP_NONE}, "Store string"},
    {"REP",      0x95, 0, {OP_NONE}, "Repeat prefix"},
    {"REPZ",     0x96, 0, {OP_NONE}, "Repeat while zero"},
    {"REPNZ",    0x97, 0, {OP_NONE}, "Repeat while not zero"},
    
    // Atomic Operations (0xA0-0xAF)
    {"XADD",     0xA0, 2, {OP_REG, OP_REG}, "Exchange and add"},
    {"CMPXCHG",  0xA1, 2, {OP_REG, OP_REG}, "Compare and exchange"},
    {"LOCK",     0xA2, 0, {OP_NONE}, "Lock prefix"},
    {"MFENCE",   0xA3, 0, {OP_NONE}, "Memory fence"},
    {"SFENCE",   0xA4, 0, {OP_NONE}, "Store fence"},
    {"LFENCE",   0xA5, 0, {OP_NONE}, "Load fence"},
    
    // Floating Point (0xB0-0xBF)
    {"FADD",     0xB0, 2, {OP_REG, OP_REG}, "Floating point add"},
    {"FSUB",     0xB1, 2, {OP_REG, OP_REG}, "Floating point subtract"},
    {"FMUL",     0xB2, 2, {OP_REG, OP_REG}, "Floating point multiply"},
    {"FDIV",     0xB3, 2, {OP_REG, OP_REG}, "Floating point divide"},
    {"FLD",      0xB4, 1, {OP_REG}, "Load floating point"},
    {"FST",      0xB5, 1, {OP_REG}, "Store floating point"},
    {"FCMP",     0xB6, 2, {OP_REG, OP_REG}, "Compare floating point"},
    
    {NULL, 0, 0, {0}, NULL}
};

/* Forward declarations */
void assemble_file(assembler_t *as);
void pass1(assembler_t *as);
void pass2(assembler_t *as);
int parse_line(assembler_t *as, char *line);
int parse_instruction(assembler_t *as, const char *mnemonic, char *operands);
int parse_directive(assembler_t *as, const char *directive, char *args);
int parse_operand(assembler_t *as, char *operand, uint8_t expected_type);
void emit_byte(assembler_t *as, uint8_t byte);
void emit_word(assembler_t *as, uint16_t word);
void emit_dword(assembler_t *as, uint32_t dword);
int add_symbol(assembler_t *as, const char *name, uint32_t address, uint8_t type, uint8_t section);
int find_symbol(assembler_t *as, const char *name);
void add_relocation(assembler_t *as, uint32_t offset, uint16_t symbol_index, uint8_t type, uint8_t size);
void write_output_file(assembler_t *as);
void error(assembler_t *as, const char *msg);
void warning(assembler_t *as, const char *msg);
void print_help(void);
void print_instructions(void);

int main(int argc, char *argv[]) {
    assembler_t as = {0};
    int opt;
    
    while ((opt = getopt(argc, argv, "hvl")) != -1) {
        switch (opt) {
            case 'h':
                print_help();
                return 0;
            case 'v':
                as.verbose = 1;
                break;
            case 'l':
                print_instructions();
                return 0;
            default:
                fprintf(stderr, "Use -h for help\n");
                return 1;
        }
    }
    
    if (optind >= argc) {
        fprintf(stderr, "Usage: %s [-hvl] <input.mhf> [output.mhfb]\n", argv[0]);
        fprintf(stderr, "Use -h for help\n");
        return 1;
    }
    
    as.filename = argv[optind];
    
    /* Open input file */
    as.input = fopen(argv[optind], "r");
    if (!as.input) {
        perror(argv[optind]);
        return 1;
    }
    
    /* Open output file */
    char output_name[256];
    if (optind + 1 < argc) {
        strcpy(output_name, argv[optind + 1]);
    } else {
        strcpy(output_name, argv[optind]);
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
    
    printf("MIGHF Extended Assembler v2.0\n");
    printf("Assembling %s -> %s\n", argv[optind], output_name);
    
    /* Two-pass assembly */
    assemble_file(&as);
    
    if (as.errors == 0) {
        write_output_file(&as);
        printf("Assembly successful: %d bytes, %d symbols", as.code_size, as.symbol_count);
        if (as.warnings > 0) printf(", %d warnings", as.warnings);
        printf("\n");
    } else {
        printf("Assembly failed with %d errors", as.errors);
        if (as.warnings > 0) printf(", %d warnings", as.warnings);
        printf("\n");
        unlink(output_name); /* Remove incomplete output file */
    }
    
    fclose(as.input);
    fclose(as.output);
    free(as.head_data);
    
    return as.errors ? 1 : 0;
}

void print_help(void) {
    printf("MIGHF Extended Assembler v2.0\n");
    printf("Usage: mighf-as [options] input.mhf [output.mhfb]\n\n");
    printf("Options:\n");
    printf("  -h        Show this help\n");
    printf("  -v        Verbose output\n");
    printf("  -l        List all instructions\n\n");
    printf("Assembly syntax:\n");
    printf("  ; Comments start with semicolon\n");
    printf("  label:    Labels end with colon\n");
    printf("  .directive args   Assembler directives\n");
    printf("  MNEMONIC operands Instructions\n\n");
    printf("Supported directives:\n");
    printf("  .org addr     Set origin address\n");
    printf("  .head \"text\"  Set metadata header\n");
    printf("  .db byte,...  Define bytes\n");
    printf("  .dw word,...  Define words\n");
    printf("  .equ name,val Define constant\n");
    printf("  .include file Include another file\n");
    printf("  .ifdef name   Conditional assembly\n");
    printf("  .ifndef name  Conditional assembly\n");
    printf("  .endif        End conditional\n");
}

void print_instructions(void) {
    printf("MIGHF Extended Instruction Set:\n\n");
    
    const char *categories[] = {
        "Basic Operations (0x00-0x0F)",
        "Data Movement (0x10-0x1F)",
        "Arithmetic (0x20-0x2F)",
        "Logical Operations (0x30-0x3F)",
        "Comparison (0x40-0x4F)",
        "Control Flow (0x50-0x6F)",
        "Privileged Instructions (0x70-0x7F)",
        "Memory Management (0x80-0x8F)",
        "String/Block Operations (0x90-0x9F)",
        "Atomic Operations (0xA0-0xAF)",
        "Floating Point (0xB0-0xBF)"
    };
    
    int cat_ranges[][2] = {
        {0x00, 0x0F}, {0x10, 0x1F}, {0x20, 0x2F}, {0x30, 0x3F},
        {0x40, 0x4F}, {0x50, 0x6F}, {0x70, 0x7F}, {0x80, 0x8F},
        {0x90, 0x9F}, {0xA0, 0xAF}, {0xB0, 0xBF}
    };
    
    for (int cat = 0; cat < 11; cat++) {
        printf("%s:\n", categories[cat]);
        for (int i = 0; instructions[i].mnemonic; i++) {
            if (instructions[i].opcode >= cat_ranges[cat][0] && 
                instructions[i].opcode <= cat_ranges[cat][1]) {
                printf("  %-8s 0x%02X  %s\n", 
                       instructions[i].mnemonic, 
                       instructions[i].opcode,
                       instructions[i].description);
            }
        }
        printf("\n");
    }
}

void assemble_file(assembler_t *as) {
    /* Pass 1: Collect symbols and calculate addresses */
    as->pass = 1;
    as->line_number = 0;
    as->current_address = 0;
    as->code_size = 0;
    as->origin = 0;
    
    if (as->verbose) printf("Pass 1: Symbol collection...\n");
    pass1(as);
    
    if (as->errors > 0) return;
    
    /* Pass 2: Generate code and resolve references */
    rewind(as->input);
    as->pass = 2;
    as->line_number = 0;
    as->current_address = as->origin;
    as->code_size = 0;
    as->skip_code = 0;
    as->if_depth = 0;
    
    if (as->verbose) printf("Pass 2: Code generation...\n");
    pass2(as);
}

void pass1(assembler_t *as) {
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), as->input)) {
        as->line_number++;
        
        /* Remove newline and trim */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        
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
        
        /* Remove newline and trim */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '\0' || *trimmed == ';') continue;
        
        if (parse_line(as, trimmed) < 0) {
            as->errors++;
        }
    }
}

int parse_line(assembler_t *as, char *line) {
    if (as->skip_code && strncmp(line, ".endif", 6) != 0 && 
        strncmp(line, ".else", 5) != 0) {
        return 0; /* Skip code in false conditional block */
    }
    
    char *line_copy = strdup(line);
    char *token = strtok(line_copy, " \t");
    if (!token) {
        free(line_copy);
        return 0;
    }
    
    /* Check for label */
    if (token[strlen(token) - 1] == ':') {
        token[strlen(token) - 1] = '\0';  /* Remove colon */
        
        if (as->pass == 1) {
            if (add_symbol(as, token, as->current_address, 1, 0) < 0) {
                error(as, "Duplicate label");
                free(line_copy);
                return -1;
            }
        }
        
        /* Get next token */
        token = strtok(NULL, " \t");
        if (!token) {
            free(line_copy);
            return 0;
        }
    }
    
    /* Check for directive */
    if (token[0] == '.') {
        char *args = strtok(NULL, "");
        int result = parse_directive(as, token, args);
        free(line_copy);
        return result;
    }
    
    /* Must be an instruction */
    char *operands = strtok(NULL, "");
    int result = parse_instruction(as, token, operands);
    free(line_copy);
    return result;
}

int parse_instruction(assembler_t *as, const char *mnemonic, char *operands) {
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
        
        /* Parse operands */
        if (inst->operand_count > 0 && operands) {
            char *operand_copy = strdup(operands);
            char *operand_list[3] = {NULL};
            int operand_count = 0;
            
            /* Split operands by comma */
            char *token = strtok(operand_copy, ",");
            while (token && operand_count < 3) {
                /* Trim whitespace */
                while (isspace(*token)) token++;
                char *end = token + strlen(token) - 1;
                while (end > token && isspace(*end)) *end-- = '\0';
                
                operand_list[operand_count++] = token;
                token = strtok(NULL, ",");
            }
            
            /* Process each operand */
            for (int i = 0; i < inst->operand_count && i < operand_count; i++) {
                if (parse_operand(as, operand_list[i], inst->operand_types[i]) < 0) {
                    free(operand_copy);
                    return -1;
                }
            }
            
            free(operand_copy);
        }
    } else {
        /* Pass 1: Calculate instruction size */
        as->current_address += 1;  /* Opcode */
        
        /* Estimate operand sizes */
        for (int i = 0; i < inst->operand_count; i++) {
            uint8_t type = inst->operand_types[i];
            if (type & (OP_REG)) {
                as->current_address += 1;
            } else if (type & (OP_IMM8)) {
                as->current_address += 1;
            } else if (type & (OP_IMM16 | OP_ABS | OP_REL)) {
                as->current_address += 2;
            }
        }
    }
    
    return 0;
}

int parse_operand(assembler_t *as, char *operand, uint8_t expected_type) {
    if (!operand) return -1;
    
    /* Register operand: R0, R1, etc. */
    if ((expected_type & OP_REG) && (operand[0] == 'R' || operand[0] == 'r')) {
        int reg_num = strtol(operand + 1, NULL, 10);
        if (reg_num < 0 || reg_num > 15) {
            error(as, "Invalid register number (0-15)");
            return -1;
        }
        emit_byte(as, (uint8_t)reg_num);
        return 0;
    }
    
    /* Immediate operand: #123, #0x45 */
    if ((expected_type & (OP_IMM8 | OP_IMM16)) && operand[0] == '#') {
        long value = strtol(operand + 1, NULL, 0);
        
        if (expected_type & OP_IMM8) {
            if (value < -128 || value > 255) {
                warning(as, "Immediate value truncated to 8 bits");
            }
            emit_byte(as, (uint8_t)value);
        } else {
            if (value < -32768 || value > 65535) {
                warning(as, "Immediate value truncated to 16 bits");
            }
            emit_word(as, (uint16_t)value);
        }
        return 0;
    }
    
    /* Memory/Address operand: label, 0x1000, [R1] */
    if (expected_type & (OP_ABS | OP_REL | OP_MEM)) {
        /* Direct numeric address */
        if (isdigit(operand[0]) || (operand[0] == '0' && operand[1] == 'x')) {
            uint16_t addr = (uint16_t)strtol(operand, NULL, 0);
            emit_word(as, addr);
            return 0;
        }
        
        /* Symbol reference */
        int sym_idx = find_symbol(as, operand);
        if (sym_idx >= 0) {
            uint16_t addr = as->symbols[sym_idx].address;
            
            /* Calculate relative address for relative jumps */
            if (expected_type & OP_REL) {
                int16_t rel_addr = addr - (as->current_address + 2);
                emit_word(as, (uint16_t)rel_addr);
            } else {
                emit_word(as, addr);
            }
            return 0;
        } else {
            /* Forward reference - add relocation */
            add_relocation(as, as->code_size, 0, 1, 2);
            emit_word(as, 0);  /* Placeholder */
            return 0;
        }
    }
    
    error(as, "Invalid operand format");
    return -1;
}

int parse_directive(assembler_t *as, const char *directive, char *args) {
    if (strcasecmp(directive, ".head") == 0) {
        /* Store head metadata */
        if (as->pass == 1 && args) {
            /* Remove quotes if present */
            if (args[0] == '"') {
                args++;
                char *end_quote = strrchr(args, '"');
                if (end_quote) *end_quote = '\0';
            }
            as->head_size = strlen(args) + 1;
            as->head_data = malloc(as->head_size);
            strcpy(as->head_data, args);
        }
    } 
    else if (strcasecmp(directive, ".org") == 0) {
        /* Set origin address */
        if (args) {
            uint32_t new_origin = strtol(args, NULL, 0);
            as->origin = new_origin;
            as->current_address = new_origin;
        }
    }
    else if (strcasecmp(directive, ".db") == 0) {
        /* Define bytes */
        if (as->pass == 2 && args) {
            char *args_copy = strdup(args);
            char *token = strtok(args_copy, ",");
            while (token) {
                while (isspace(*token)) token++;
                
                if (token[0] == '"') {
                    /* String literal */
                    token++;
                    char *end = strchr(token, '"');
                    if (end) *end = '\0';
                    for (char *p = token; *p; p++) {
                        emit_byte(as, (uint8_t)*p);
                    }
                } else {
                    /* Numeric value */
                    int value = strtol(token, NULL, 0);
                    emit_byte(as, (uint8_t)value);
                }
                token = strtok(NULL, ",");
            }
            free(args_copy);
        } else if (as->pass == 1 && args) {
            /* Count bytes for pass 1 */
            char *args_copy = strdup(args);
            char *token = strtok(args_copy, ",");
            while (token) {
                while (isspace(*token)) token++;
                if (token[0] == '"') {
                    token++;
                    char *end = strchr(token, '"');
                    if (end) *end = '\0';
                    as->current_address += strlen(token);
                } else {
                    as->current_address++;
                }
                token = strtok(NULL, ",");
            }
            free(args_copy);
        }
    }
    else if (strcasecmp(directive, ".dw") == 0) {
        /* Define words */
        if (as->pass == 2 && args) {
            char *args_copy = strdup(args);
            char *token = strtok(args_copy, ",");
            while (token) {
                while (isspace(*token)) token++;
                int value = strtol(token, NULL, 0);
                emit_word(as, (uint16_t)value);
                token = strtok(NULL, ",");
            }
            free(args_copy);
        } else if (as->pass == 1 && args) {
            char *args_copy = strdup(args);
            char *token = strtok(args_copy, ",");
            while (token) {
                as->current_address += 2;
                token = strtok(NULL, ",");
            }
            free(args_copy);
        }
    }
    else if (strcasecmp(directive, ".equ") == 0) {
        /* Define constant */
        if (as->pass == 1 && args) {
            char *name = strtok(args, ",");
            char *value_str = strtok(NULL, ",");
            if (name && value_str) {
                while (isspace(*name)) name++;
                while (isspace(*value_str)) value_str++;
                uint32_t value = strtol(value_str, NULL, 0);
                add_symbol(as, name, value, 2, 0); /* Type 2 = constant */
            }
        }
    }
    else if (strcasecmp(directive, ".include") == 0) {
        /* Include another file - simplified implementation */
        if (args) {
            /* Remove quotes */
            if (args[0] == '"') {
                args++;
                char *end = strrchr(args, '"');
                if (end) *end = '\0';
            }
            printf("Note: .include directive found (%s) - not implemented\n", args);
            warning(as, "Include directive not yet implemented");
        }
    }
    else if (strcasecmp(directive, ".ifdef") == 0) {
        /* Conditional assembly - if defined */
        if (args && as->if_depth < 15) {
            while (isspace(*args)) args++;
            int sym_idx = find_symbol(as, args);
            as->if_stack[as->if_depth] = (sym_idx >= 0) ? 1 : 0;
            if (!as->if_stack[as->if_depth]) {
                as->skip_code = 1;
            }
            as->if_depth++;
        }
    }
    else if (strcasecmp(directive, ".ifndef") == 0) {
        /* Conditional assembly - if not defined */
        if (args && as->if_depth < 15) {
            while (isspace(*args)) args++;
            int sym_idx = find_symbol(as, args);
            as->if_stack[as->if_depth] = (sym_idx < 0) ? 1 : 0;
            if (!as->if_stack[as->if_depth]) {
                as->skip_code = 1;
            }
            as->if_depth++;
        }
    }
    else if (strcasecmp(directive, ".else") == 0) {
        /* Conditional assembly - else */
        if (as->if_depth > 0) {
            as->if_stack[as->if_depth - 1] = !as->if_stack[as->if_depth - 1];
            as->skip_code = !as->if_stack[as->if_depth - 1];
        }
    }
    else if (strcasecmp(directive, ".endif") == 0) {
        /* End conditional assembly */
        if (as->if_depth > 0) {
            as->if_depth--;
            as->skip_code = 0;
            /* Check if still in nested conditional */
            for (int i = 0; i < as->if_depth; i++) {
                if (!as->if_stack[i]) {
                    as->skip_code = 1;
                    break;
                }
            }
        }
    }
    else {
        error(as, "Unknown directive");
        return -1;
    }
    
    return 0;
}

void emit_byte(assembler_t *as, uint8_t byte) {
    if (as->code_size >= MAX_CODE_SIZE) {
        error(as, "Code size exceeded maximum");
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
    if (as->symbol_count >= MAX_SYMBOLS) {
        error(as, "Too many symbols");
        return -1;
    }
    
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
    sym->flags = 0;
    
    if (as->verbose) {
        printf("Symbol: %s = 0x%04X (type=%d, section=%d)\n", 
               name, address, type, section);
    }
    
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

void add_relocation(assembler_t *as, uint32_t offset, uint16_t symbol_index, uint8_t type, uint8_t size) {
    if (as->relocation_count >= MAX_RELOCATIONS) {
        warning(as, "Too many relocations");
        return;
    }
    
    relocation_t *rel = &as->relocations[as->relocation_count++];
    rel->offset = offset;
    rel->symbol_index = symbol_index;
    rel->type = type;
    rel->size = size;
}

void write_output_file(assembler_t *as) {
    mhf_header_t header = {0};
    
    /* Calculate section offsets */
    uint32_t current_offset = sizeof(mhf_header_t);
    
    /* Write header */
    memcpy(header.signature, "MHF\0", 4);
    header.version = 2;  /* Extended format version */
    header.flags = 0;
    header.head_offset = current_offset;
    
    /* Head section */
    current_offset += (as->head_size ? as->head_size : 0);
    header.code_offset = current_offset;
    
    /* Code section */
    current_offset += as->code_size;
    header.data_offset = current_offset;
    
    /* Data section (empty for now) */
    header.symbol_offset = current_offset;
    
    /* Symbol table */
    current_offset += (as->symbol_count * sizeof(symbol_t));
    header.file_size = current_offset;
    
    /* Write header */
    fwrite(&header, sizeof(header), 1, as->output);
    
    /* Write head section */
    if (as->head_data && as->head_size > 0) {
        fwrite(as->head_data, as->head_size, 1, as->output);
    }
    
    /* Write code section */
    fwrite(as->code_buffer, as->code_size, 1, as->output);
    
    /* Write symbol table */
    if (as->symbol_count > 0) {
        fwrite(as->symbols, sizeof(symbol_t), as->symbol_count, as->output);
    }
    
    if (as->verbose) {
        printf("Output file structure:\n");
        printf("  Header:     0x%08X - 0x%08X (%d bytes)\n", 
               0, (int)sizeof(header), (int)sizeof(header));
        printf("  Head:       0x%08X - 0x%08X (%d bytes)\n", 
               header.head_offset, header.code_offset, (int)as->head_size);
        printf("  Code:       0x%08X - 0x%08X (%d bytes)\n", 
               header.code_offset, header.data_offset, as->code_size);
        printf("  Symbols:    0x%08X - 0x%08X (%d symbols)\n", 
               header.symbol_offset, header.file_size, as->symbol_count);
    }
}

void error(assembler_t *as, const char *msg) {
    fprintf(stderr, "%s:%d: Error: %s\n", as->filename, as->line_number, msg);
    as->errors++;
}

void warning(assembler_t *as, const char *msg) {
    fprintf(stderr, "%s:%d: Warning: %s\n", as->filename, as->line_number, msg);
    as->warnings++;
}
