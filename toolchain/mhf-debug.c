#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 65536

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

typedef struct {
    uint8_t regs[16]; 
    uint16_t pc;
    uint8_t memory[MEM_SIZE];
    int running;
} MighfCPU;

void debug_memory(MighfCPU *cpu, uint16_t start, uint16_t count) {
    printf("Memory dump from 0x%04X:\n", start);
    for (int i = 0; i < count; i++) {
        if (i % 16 == 0) printf("%04X: ", start + i);
        printf("%02X ", cpu->memory[start + i]);
        if (i % 16 == 15) printf("\n");
    }
    if (count % 16 != 0) printf("\n");
}

void load_mhfb(const char *filename, MighfCPU *cpu, uint16_t *entry_point) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("open"); exit(1); }

    mhf_header_t header;
    size_t read_bytes = fread(&header, sizeof(header), 1, f);
    if (read_bytes != 1) {
        fprintf(stderr, "Failed to read header\n");
        exit(1);
    }

    printf("MHF Header Debug:\n");
    printf("  Signature: %.4s\n", header.signature);
    printf("  Version: %d\n", header.version);
    printf("  Head offset: 0x%08X\n", header.head_offset);
    printf("  Code offset: 0x%08X\n", header.code_offset);
    printf("  Data offset: 0x%08X\n", header.data_offset);
    printf("  Symbol offset: 0x%08X\n", header.symbol_offset);
    printf("  File size: 0x%08X\n", header.file_size);

    if (memcmp(header.signature, "MHF", 3) != 0) {
        fprintf(stderr, "Invalid signature\n");
        exit(1);
    }

    // Read the rest of the file
    uint32_t remaining = header.file_size - sizeof(header);
    printf("Reading %d bytes starting at offset 0x%04X\n", remaining, header.head_offset);
    
    if (header.head_offset < MEM_SIZE && remaining <= MEM_SIZE - header.head_offset) {
        read_bytes = fread(cpu->memory + header.head_offset, remaining, 1, f);
        if (read_bytes != 1) {
            fprintf(stderr, "Failed to read program data\n");
            exit(1);
        }
    }
    fclose(f);

    *entry_point = header.code_offset;
    printf("Entry point set to: 0x%04X\n", *entry_point);
    
    // Debug: Show first few bytes at entry point
    printf("Code at entry point:\n");
    debug_memory(cpu, header.code_offset, 16);
}

void run_cpu(MighfCPU *cpu, uint16_t entry_point) {
    cpu->pc = entry_point;
    cpu->running = 1;
    int instruction_count = 0;

    printf("Starting execution at PC=0x%04X\n", cpu->pc);

    while (cpu->running && instruction_count < 100) { // Safety limit
        printf("\nInstruction %d: PC=0x%04X, ", instruction_count, cpu->pc);
        
        if (cpu->pc >= MEM_SIZE) {
            printf("PC out of range!\n");
            break;
        }
        
        uint8_t opcode = cpu->memory[cpu->pc++];
        printf("Opcode=0x%02X ", opcode);

        switch (opcode) {
            case 0x00: // NOP
                printf("NOP\n");
                break;

            case 0x01: // HALT
                printf("HALT\n");
                cpu->running = 0;
                break;

            case 0x10: { // LOAD reg, imm8
                uint8_t reg = cpu->memory[cpu->pc++];
                uint8_t imm = cpu->memory[cpu->pc++];
                printf("LOAD R%d, #%d\n", reg, imm);
                if (reg < 16) cpu->regs[reg] = imm;
                break;
            }

            case 0x13: { // STORE addr, reg (changed from original to match assembler)
                uint16_t addr = cpu->memory[cpu->pc] | (cpu->memory[cpu->pc + 1] << 8);
                cpu->pc += 2;
                uint8_t reg = cpu->memory[cpu->pc++];
                printf("STMEM 0x%04X, R%d (value=%d)\n", addr, reg, cpu->regs[reg]);
                
                if (reg < 16 && addr < MEM_SIZE) {
                    cpu->memory[addr] = cpu->regs[reg];
                    
                    // Memory-mapped output
                    if (addr == 0xFF00) {
                        printf("  --> Console output: '%c' (0x%02X)\n", 
                               cpu->regs[reg], cpu->regs[reg]);
                        putchar(cpu->regs[reg]);
                        fflush(stdout);
                    }
                }
                break;
            }

            default:
                printf("UNKNOWN OPCODE!\n");
                printf("Memory around PC:\n");
                debug_memory(cpu, cpu->pc - 10, 20);
                cpu->running = 0;
                break;
        }
        
        instruction_count++;
    }
    
    if (instruction_count >= 100) {
        printf("Execution stopped - instruction limit reached\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s program.mhfb\n", argv[0]);
        return 1;
    }

    MighfCPU cpu = {0};
    uint16_t entry_point = 0;

    load_mhfb(argv[1], &cpu, &entry_point);
    run_cpu(&cpu, entry_point);

    printf("\nFinal CPU state:\n");
    for (int i = 0; i < 16; i++) {
        printf("R%d = 0x%02X (%d)\n", i, cpu.regs[i], cpu.regs[i]);
    }

    return 0;
}
