#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 65536
#define NUM_REGS 8

typedef struct {
    char signature[4];      // "MHF\0"
    uint16_t version;
    uint16_t flags;
    uint32_t head_offset;
    uint32_t code_offset;
    uint32_t data_offset;
    uint32_t symbol_offset;
    uint32_t file_size;
} __attribute__((packed)) mhf_header_t;

typedef struct {
    uint8_t regs[NUM_REGS]; 
    uint16_t pc;
    uint8_t memory[MEM_SIZE];
    int running;
} MighfCPU;

void load_mhfb(const char *filename, MighfCPU *cpu, uint16_t *entry_point) {
    FILE *f = fopen(filename, "rb");
    if (!f) { perror("open"); exit(1); }

    mhf_header_t header;
    fread(&header, sizeof(header), 1, f);

    if (memcmp(header.signature, "MHF", 3) != 0) {
        fprintf(stderr, "Invalid signature\n");
        exit(1);
    }

    fread(cpu->memory + header.head_offset, header.file_size - sizeof(header), 1, f);
    fclose(f);

    *entry_point = header.code_offset;
}

void run_cpu(MighfCPU *cpu, uint16_t entry_point) {
    cpu->pc = entry_point;
    cpu->running = 1;

    while (cpu->running) {
        uint8_t opcode = cpu->memory[cpu->pc++];

        switch (opcode) {
            case 0x00: break; // NOP

            case 0x01: // HALT
                printf("\n[HALT] CPU halted.\n");
                cpu->running = 0;
                break;

            case 0x10: { // LOAD reg, imm
                uint8_t reg = cpu->memory[cpu->pc++];
                uint8_t imm = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) cpu->regs[reg] = imm;
                break;
            }

            case 0x20: { // ADD reg, reg
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS)
                    cpu->regs[reg1] += cpu->regs[reg2];
                break;
            }

            case 0x11: { // STORE reg, addr
                uint8_t reg = cpu->memory[cpu->pc++];
                uint8_t lo = cpu->memory[cpu->pc++];
                uint8_t hi = cpu->memory[cpu->pc++];
                uint16_t addr = (hi << 8) | lo;

                if (reg < NUM_REGS && addr < MEM_SIZE)
                    cpu->memory[addr] = cpu->regs[reg];

                // Memory-mapped output
                if (addr == 0xFF00) {
                    putchar(cpu->regs[reg]);
                    fflush(stdout);
                }
                break;
            }

            default:
                printf("[ERROR] Unknown opcode 0x%02X at 0x%04X\n", opcode, cpu->pc - 1);
                cpu->running = 0;
                break;
        }
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

    printf("Execution finished.\n");
    for (int i = 0; i < NUM_REGS; i++)
        printf("r%d = %d\n", i, cpu.regs[i]);

    return 0;
}

