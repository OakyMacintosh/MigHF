#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_TOKENS 1000
#define MAX_BYTECODE 65536
#define MAX_VARS 256
#define MAX_FUNCTIONS 64
#define MAX_LABELS 256

// Token types
typedef enum {
    TOK_EOF, TOK_NUMBER, TOK_IDENTIFIER, TOK_STRING,
    TOK_SEMICOLON, TOK_LBRACE, TOK_RBRACE, TOK_LPAREN, TOK_RPAREN,
    TOK_ASSIGN, TOK_PLUS, TOK_MINUS, TOK_MULTIPLY, TOK_DIVIDE,
    TOK_EQ, TOK_NE, TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_RETURN,
    TOK_INT, TOK_CHAR, TOK_VOID, TOK_PRINTF, TOK_PUTCHAR,
    TOK_COMMA, TOK_INCREMENT, TOK_DECREMENT
} TokenType;

typedef struct {
    TokenType type;
    char value[256];
    int line;
} Token;

typedef struct {
    char name[64];
    int offset;  // Memory offset or register number
    int is_global;
} Variable;

typedef struct {
    char name[64];
    int address;
} Label;

typedef struct {
    Token tokens[MAX_TOKENS];
    int count;
    int current;
} TokenList;

typedef struct {
    uint8_t code[MAX_BYTECODE];
    int size;
    Variable vars[MAX_VARS];
    int var_count;
    int next_local_offset;
    Label labels[MAX_LABELS];
    int label_count;
    int current_line;
} Compiler;

// MHF header structure
typedef struct {
    char signature[4];
    uint16_t version;
    uint16_t flags;
    uint32_t head_offset;
    uint32_t code_offset;
    uint32_t data_offset;
    uint32_t symbol_offset;
    uint32_t file_size;
} __attribute__((packed)) mhf_header_t;

// Opcodes from the reference
#define OP_NOP    0x00
#define OP_HALT   0x01
#define OP_LOAD   0x10  // LOAD reg, imm
#define OP_STORE  0x11  // STORE reg, addr
#define OP_ADD    0x20  // ADD reg, reg

// Forward declarations
void compile_statement(Compiler *comp, TokenList *tokens);
void compile_expression(Compiler *comp, TokenList *tokens, int target_reg);

// Utility functions
void emit_byte(Compiler *comp, uint8_t byte) {
    if (comp->size >= MAX_BYTECODE) {
        fprintf(stderr, "Error: Bytecode buffer overflow\n");
        exit(1);
    }
    comp->code[comp->size++] = byte;
}

void emit_load_immediate(Compiler *comp, int reg, int value) {
    emit_byte(comp, OP_LOAD);
    emit_byte(comp, reg);
    emit_byte(comp, value & 0xFF);
}

void emit_store_memory(Compiler *comp, int reg, uint16_t addr) {
    emit_byte(comp, OP_STORE);
    emit_byte(comp, reg);
    emit_byte(comp, addr & 0xFF);      // Low byte
    emit_byte(comp, (addr >> 8) & 0xFF); // High byte
}

void emit_add_regs(Compiler *comp, int reg1, int reg2) {
    emit_byte(comp, OP_ADD);
    emit_byte(comp, reg1);
    emit_byte(comp, reg2);
}

// Tokenizer
int is_keyword(const char *str) {
    const char *keywords[] = {"int", "char", "void", "if", "else", "while", "for", "return", "printf", "putchar", NULL};
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(str, keywords[i]) == 0) return 1;
    }
    return 0;
}

TokenType keyword_to_token(const char *str) {
    if (strcmp(str, "int") == 0) return TOK_INT;
    if (strcmp(str, "char") == 0) return TOK_CHAR;
    if (strcmp(str, "void") == 0) return TOK_VOID;
    if (strcmp(str, "if") == 0) return TOK_IF;
    if (strcmp(str, "else") == 0) return TOK_ELSE;
    if (strcmp(str, "while") == 0) return TOK_WHILE;
    if (strcmp(str, "for") == 0) return TOK_FOR;
    if (strcmp(str, "return") == 0) return TOK_RETURN;
    if (strcmp(str, "printf") == 0) return TOK_PRINTF;
    if (strcmp(str, "putchar") == 0) return TOK_PUTCHAR;
    return TOK_IDENTIFIER;
}

void tokenize(const char *source, TokenList *tokens) {
    tokens->count = 0;
    tokens->current = 0;
    
    int pos = 0;
    int line = 1;
    
    while (source[pos]) {
        // Skip whitespace
        while (isspace(source[pos])) {
            if (source[pos] == '\n') line++;
            pos++;
        }
        
        if (!source[pos]) break;
        
        Token *tok = &tokens->tokens[tokens->count++];
        tok->line = line;
        
        // Numbers
        if (isdigit(source[pos])) {
            int start = pos;
            while (isdigit(source[pos])) pos++;
            strncpy(tok->value, source + start, pos - start);
            tok->value[pos - start] = 0;
            tok->type = TOK_NUMBER;
        }
        // Identifiers and keywords
        else if (isalpha(source[pos]) || source[pos] == '_') {
            int start = pos;
            while (isalnum(source[pos]) || source[pos] == '_') pos++;
            strncpy(tok->value, source + start, pos - start);
            tok->value[pos - start] = 0;
            
            if (is_keyword(tok->value)) {
                tok->type = keyword_to_token(tok->value);
            } else {
                tok->type = TOK_IDENTIFIER;
            }
        }
        // String literals
        else if (source[pos] == '"') {
            pos++; // Skip opening quote
            int start = pos;
            while (source[pos] && source[pos] != '"') {
                if (source[pos] == '\\') pos++; // Skip escape sequences
                pos++;
            }
            strncpy(tok->value, source + start, pos - start);
            tok->value[pos - start] = 0;
            if (source[pos] == '"') pos++; // Skip closing quote
            tok->type = TOK_STRING;
        }
        // Two-character operators
        else if (source[pos] == '=' && source[pos + 1] == '=') {
            strcpy(tok->value, "==");
            tok->type = TOK_EQ;
            pos += 2;
        }
        else if (source[pos] == '!' && source[pos + 1] == '=') {
            strcpy(tok->value, "!=");
            tok->type = TOK_NE;
            pos += 2;
        }
        else if (source[pos] == '<' && source[pos + 1] == '=') {
            strcpy(tok->value, "<=");
            tok->type = TOK_LE;
            pos += 2;
        }
        else if (source[pos] == '>' && source[pos + 1] == '=') {
            strcpy(tok->value, ">=");
            tok->type = TOK_GE;
            pos += 2;
        }
        else if (source[pos] == '+' && source[pos + 1] == '+') {
            strcpy(tok->value, "++");
            tok->type = TOK_INCREMENT;
            pos += 2;
        }
        else if (source[pos] == '-' && source[pos + 1] == '-') {
            strcpy(tok->value, "--");
            tok->type = TOK_DECREMENT;
            pos += 2;
        }
        // Single character tokens
        else {
            tok->value[0] = source[pos];
            tok->value[1] = 0;
            
            switch (source[pos]) {
                case ';': tok->type = TOK_SEMICOLON; break;
                case '{': tok->type = TOK_LBRACE; break;
                case '}': tok->type = TOK_RBRACE; break;
                case '(': tok->type = TOK_LPAREN; break;
                case ')': tok->type = TOK_RPAREN; break;
                case '=': tok->type = TOK_ASSIGN; break;
                case '+': tok->type = TOK_PLUS; break;
                case '-': tok->type = TOK_MINUS; break;
                case '*': tok->type = TOK_MULTIPLY; break;
                case '/': tok->type = TOK_DIVIDE; break;
                case '<': tok->type = TOK_LT; break;
                case '>': tok->type = TOK_GT; break;
                case ',': tok->type = TOK_COMMA; break;
                default:
                    fprintf(stderr, "Unknown character '%c' at line %d\n", source[pos], line);
                    break;
            }
            pos++;
        }
    }
    
    // Add EOF token
    tokens->tokens[tokens->count].type = TOK_EOF;
    tokens->tokens[tokens->count].line = line;
    tokens->count++;
}

// Parser helper functions
Token *current_token(TokenList *tokens) {
    if (tokens->current < tokens->count) {
        return &tokens->tokens[tokens->current];
    }
    return &tokens->tokens[tokens->count - 1]; // EOF
}

void advance_token(TokenList *tokens) {
    if (tokens->current < tokens->count - 1) {
        tokens->current++;
    }
}

int match_token(TokenList *tokens, TokenType type) {
    if (current_token(tokens)->type == type) {
        advance_token(tokens);
        return 1;
    }
    return 0;
}

// Variable management
Variable *find_variable(Compiler *comp, const char *name) {
    for (int i = 0; i < comp->var_count; i++) {
        if (strcmp(comp->vars[i].name, name) == 0) {
            return &comp->vars[i];
        }
    }
    return NULL;
}

Variable *declare_variable(Compiler *comp, const char *name) {
    if (comp->var_count >= MAX_VARS) {
        fprintf(stderr, "Error: Too many variables\n");
        exit(1);
    }
    
    Variable *var = &comp->vars[comp->var_count++];
    strcpy(var->name, name);
    var->offset = comp->next_local_offset++;
    var->is_global = 0;
    
    return var;
}

// Expression compiler
void compile_primary(Compiler *comp, TokenList *tokens, int target_reg) {
    Token *tok = current_token(tokens);
    
    if (tok->type == TOK_NUMBER) {
        int value = atoi(tok->value);
        emit_load_immediate(comp, target_reg, value);
        advance_token(tokens);
    }
    else if (tok->type == TOK_IDENTIFIER) {
        Variable *var = find_variable(comp, tok->value);
        if (!var) {
            fprintf(stderr, "Error: Undefined variable '%s'\n", tok->value);
            exit(1);
        }
        // For simplicity, store variables at fixed memory locations
        uint16_t addr = 0x1000 + var->offset;
        // Load from memory would need a LOAD mem,reg instruction
        // For now, just load the offset as a placeholder
        emit_load_immediate(comp, target_reg, var->offset);
        advance_token(tokens);
    }
    else if (match_token(tokens, TOK_LPAREN)) {
        compile_expression(comp, tokens, target_reg);
        if (!match_token(tokens, TOK_RPAREN)) {
            fprintf(stderr, "Error: Expected ')'\n");
            exit(1);
        }
    }
    else {
        fprintf(stderr, "Error: Unexpected token in expression\n");
        exit(1);
    }
}

void compile_expression(Compiler *comp, TokenList *tokens, int target_reg) {
    compile_primary(comp, tokens, target_reg);
    
    while (current_token(tokens)->type == TOK_PLUS ||
           current_token(tokens)->type == TOK_MINUS) {
        TokenType op = current_token(tokens)->type;
        advance_token(tokens);
        
        // Compile right operand into next register
        int right_reg = (target_reg + 1) % 8;
        compile_primary(comp, tokens, right_reg);
        
        if (op == TOK_PLUS) {
            emit_add_regs(comp, target_reg, right_reg);
        }
        // Subtract would need a SUB instruction
    }
}

// Statement compiler
void compile_assignment(Compiler *comp, TokenList *tokens, const char *var_name) {
    Variable *var = find_variable(comp, var_name);
    if (!var) {
        var = declare_variable(comp, var_name);
    }
    
    if (!match_token(tokens, TOK_ASSIGN)) {
        fprintf(stderr, "Error: Expected '='\n");
        exit(1);
    }
    
    // Compile expression into register 0
    compile_expression(comp, tokens, 0);
    
    // Store register 0 to variable's memory location
    uint16_t addr = 0x1000 + var->offset;
    emit_store_memory(comp, 0, addr);
}

void compile_printf(Compiler *comp, TokenList *tokens) {
    if (!match_token(tokens, TOK_LPAREN)) {
        fprintf(stderr, "Error: Expected '('\n");
        exit(1);
    }
    
    Token *tok = current_token(tokens);
    if (tok->type == TOK_STRING) {
        // Simple string output - convert each character to putchar
        for (int i = 0; tok->value[i]; i++) {
            emit_load_immediate(comp, 0, tok->value[i]);
            emit_store_memory(comp, 0, 0xFF00); // Memory-mapped output
        }
        advance_token(tokens);
    }
    
    if (!match_token(tokens, TOK_RPAREN)) {
        fprintf(stderr, "Error: Expected ')'\n");
        exit(1);
    }
}

void compile_putchar(Compiler *comp, TokenList *tokens) {
    if (!match_token(tokens, TOK_LPAREN)) {
        fprintf(stderr, "Error: Expected '('\n");
        exit(1);
    }
    
    // Compile expression for character value
    compile_expression(comp, tokens, 0);
    
    // Output character via memory-mapped I/O
    emit_store_memory(comp, 0, 0xFF00);
    
    if (!match_token(tokens, TOK_RPAREN)) {
        fprintf(stderr, "Error: Expected ')'\n");
        exit(1);
    }
}

void compile_statement(Compiler *comp, TokenList *tokens) {
    Token *tok = current_token(tokens);
    
    if (tok->type == TOK_INT || tok->type == TOK_CHAR) {
        // Variable declaration
        advance_token(tokens);
        tok = current_token(tokens);
        
        if (tok->type != TOK_IDENTIFIER) {
            fprintf(stderr, "Error: Expected identifier\n");
            exit(1);
        }
        
        declare_variable(comp, tok->value);
        advance_token(tokens);
        
        if (match_token(tokens, TOK_SEMICOLON)) {
            return;
        }
        
        if (current_token(tokens)->type == TOK_ASSIGN) {
            compile_assignment(comp, tokens, tok->value);
        }
    }
    else if (tok->type == TOK_IDENTIFIER) {
        char var_name[256];
        strcpy(var_name, tok->value);
        advance_token(tokens);
        compile_assignment(comp, tokens, var_name);
    }
    else if (tok->type == TOK_PRINTF) {
        advance_token(tokens);
        compile_printf(comp, tokens);
    }
    else if (tok->type == TOK_PUTCHAR) {
        advance_token(tokens);
        compile_putchar(comp, tokens);
    }
    else if (tok->type == TOK_LBRACE) {
        // Block statement
        advance_token(tokens);
        while (current_token(tokens)->type != TOK_RBRACE && 
               current_token(tokens)->type != TOK_EOF) {
            compile_statement(comp, tokens);
        }
        if (!match_token(tokens, TOK_RBRACE)) {
            fprintf(stderr, "Error: Expected '}'\n");
            exit(1);
        }
        return; // Don't consume semicolon
    }
    
    if (!match_token(tokens, TOK_SEMICOLON)) {
        fprintf(stderr, "Error: Expected ';'\n");
        exit(1);
    }
}

void compile_function(Compiler *comp, TokenList *tokens) {
    // Skip return type
    advance_token(tokens);
    
    // Function name
    Token *name_tok = current_token(tokens);
    if (name_tok->type != TOK_IDENTIFIER) {
        fprintf(stderr, "Error: Expected function name\n");
        exit(1);
    }
    advance_token(tokens);
    
    // Parameters
    if (!match_token(tokens, TOK_LPAREN)) {
        fprintf(stderr, "Error: Expected '('\n");
        exit(1);
    }
    
    // Skip parameter list for now
    while (current_token(tokens)->type != TOK_RPAREN && 
           current_token(tokens)->type != TOK_EOF) {
        advance_token(tokens);
    }
    
    if (!match_token(tokens, TOK_RPAREN)) {
        fprintf(stderr, "Error: Expected ')'\n");
        exit(1);
    }
    
    // Function body
    if (current_token(tokens)->type == TOK_LBRACE) {
        compile_statement(comp, tokens);
    }
    
    // Add HALT at end of main function
    if (strcmp(name_tok->value, "main") == 0) {
        emit_byte(comp, OP_HALT);
    }
}

void compile_program(Compiler *comp, TokenList *tokens) {
    while (current_token(tokens)->type != TOK_EOF) {
        Token *tok = current_token(tokens);
        
        if (tok->type == TOK_INT || tok->type == TOK_CHAR || tok->type == TOK_VOID) {
            compile_function(comp, tokens);
        } else {
            compile_statement(comp, tokens);
        }
    }
}

void write_mhf_file(Compiler *comp, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Error creating output file");
        exit(1);
    }
    
    mhf_header_t header = {0};
    strcpy(header.signature, "MHF");
    header.version = 1;
    header.flags = 0;
    header.head_offset = sizeof(header);
    header.code_offset = sizeof(header);
    header.data_offset = sizeof(header) + comp->size;
    header.symbol_offset = sizeof(header) + comp->size;
    header.file_size = sizeof(header) + comp->size;
    
    fwrite(&header, sizeof(header), 1, f);
    fwrite(comp->code, comp->size, 1, f);
    fclose(f);
    
    printf("Generated %s (%d bytes of bytecode)\n", filename, comp->size);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.c output.mhfb\n", argv[0]);
        return 1;
    }
    
    // Read source file
    FILE *f = fopen(argv[1], "r");
    if (!f) {
        perror("Error opening input file");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *source = malloc(size + 1);
    fread(source, 1, size, f);
    source[size] = 0;
    fclose(f);
    
    // Compile
    TokenList tokens;
    Compiler comp = {0};
    comp.next_local_offset = 0;
    
    printf("Tokenizing...\n");
    tokenize(source, &tokens);
    printf("Found %d tokens\n", tokens.count);
    
    printf("Compiling...\n");
    compile_program(&comp, &tokens);
    
    printf("Writing output...\n");
    write_mhf_file(&comp, argv[2]);
    
    free(source);
    return 0;
}
