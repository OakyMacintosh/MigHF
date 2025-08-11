#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MEM_SIZE 65536
#define NUM_REGS 16
#define STACK_SIZE 256
#define MAX_INTERRUPTS 16

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
    uint16_t sp;            // Stack pointer
    uint8_t flags;          // Status flags: ZCNV (Zero, Carry, Negative, oVerflow)
    uint8_t memory[MEM_SIZE];
    int running;
    
    // Privileged state
    uint8_t privilege_level; // 0=kernel, 3=user
    uint16_t interrupt_table[MAX_INTERRUPTS]; // Interrupt vector table
    uint8_t interrupt_enabled;
    uint8_t pending_interrupts;
    
    // Memory management
    uint16_t page_table_base;
    uint8_t mmu_enabled;
    
    // Timer
    uint32_t timer_counter;
    uint32_t timer_interval;
} MighfCPU;

// Instruction opcodes - organized by category
// Basic Operations (0x00-0x0F)
#define OP_NOP      0x00
#define OP_HALT     0x01
#define OP_BREAK    0x02  // Breakpoint/debug
#define OP_SYSCALL  0x03  // System call

// Data Movement (0x10-0x1F)
#define OP_LOAD_IMM8    0x10  // LOAD reg, imm8
#define OP_LOAD_IMM16   0x11  // LOAD reg, imm16 (reg pair)
#define OP_LOAD_MEM     0x12  // LOAD reg, [addr]
#define OP_STORE_MEM    0x13  // STORE [addr], reg
#define OP_LOAD_REG     0x14  // LOAD reg1, reg2
#define OP_LOAD_INDIRECT 0x15 // LOAD reg, [reg]
#define OP_STORE_INDIRECT 0x16 // STORE [reg], reg
#define OP_PUSH         0x17  // PUSH reg
#define OP_POP          0x18  // POP reg
#define OP_XCHG         0x19  // XCHG reg1, reg2

// Arithmetic (0x20-0x2F)
#define OP_ADD_REG      0x20  // ADD reg1, reg2
#define OP_ADD_IMM      0x21  // ADD reg, imm
#define OP_SUB_REG      0x22  // SUB reg1, reg2
#define OP_SUB_IMM      0x23  // SUB reg, imm
#define OP_MUL_REG      0x24  // MUL reg1, reg2
#define OP_DIV_REG      0x25  // DIV reg1, reg2
#define OP_MOD_REG      0x26  // MOD reg1, reg2
#define OP_INC          0x27  // INC reg
#define OP_DEC          0x28  // DEC reg
#define OP_NEG          0x29  // NEG reg (two's complement)

// Logical Operations (0x30-0x3F)
#define OP_AND_REG      0x30  // AND reg1, reg2
#define OP_AND_IMM      0x31  // AND reg, imm
#define OP_OR_REG       0x32  // OR reg1, reg2
#define OP_OR_IMM       0x33  // OR reg, imm
#define OP_XOR_REG      0x34  // XOR reg1, reg2
#define OP_XOR_IMM      0x35  // XOR reg, imm
#define OP_NOT          0x36  // NOT reg
#define OP_SHL          0x37  // SHL reg, count
#define OP_SHR          0x38  // SHR reg, count
#define OP_ROL          0x39  // ROL reg, count
#define OP_ROR          0x3A  // ROR reg, count

// Comparison (0x40-0x4F)
#define OP_CMP_REG      0x40  // CMP reg1, reg2
#define OP_CMP_IMM      0x41  // CMP reg, imm
#define OP_TEST_REG     0x42  // TEST reg1, reg2 (AND without storing)
#define OP_TEST_IMM     0x43  // TEST reg, imm

// Control Flow (0x50-0x6F)
#define OP_JMP          0x50  // JMP addr
#define OP_JMP_REG      0x51  // JMP reg
#define OP_JZ           0x52  // Jump if zero
#define OP_JNZ          0x53  // Jump if not zero
#define OP_JC           0x54  // Jump if carry
#define OP_JNC          0x55  // Jump if not carry
#define OP_JN           0x56  // Jump if negative
#define OP_JNN          0x57  // Jump if not negative
#define OP_JV           0x58  // Jump if overflow
#define OP_JNV          0x59  // Jump if not overflow
#define OP_JL           0x5A  // Jump if less (signed)
#define OP_JLE          0x5B  // Jump if less or equal (signed)
#define OP_JG           0x5C  // Jump if greater (signed)
#define OP_JGE          0x5D  // Jump if greater or equal (signed)
#define OP_JB           0x5E  // Jump if below (unsigned)
#define OP_JBE          0x5F  // Jump if below or equal (unsigned)
#define OP_JA           0x60  // Jump if above (unsigned)
#define OP_JAE          0x61  // Jump if above or equal (unsigned)
#define OP_CALL         0x62  // CALL addr
#define OP_CALL_REG     0x63  // CALL reg
#define OP_RET          0x64  // RET
#define OP_LOOP         0x65  // LOOP addr (dec reg0, jnz)

// Privileged Instructions (0x70-0x7F) - Kernel mode only
#define OP_LDIDT        0x70  // Load interrupt descriptor table
#define OP_LIDT         0x71  // Load interrupt table register
#define OP_STI          0x72  // Set interrupt flag (enable interrupts)
#define OP_CLI          0x73  // Clear interrupt flag (disable interrupts)
#define OP_IRET         0x74  // Interrupt return
#define OP_HLT_PRIV     0x75  // Privileged halt (wait for interrupt)
#define OP_LGDT         0x76  // Load global descriptor table
#define OP_LTR          0x77  // Load task register
#define OP_LLDT         0x78  // Load local descriptor table
#define OP_INVLPG       0x79  // Invalidate page in TLB
#define OP_WRMSR        0x7A  // Write model specific register
#define OP_RDMSR        0x7B  // Read model specific register
#define OP_CPUID        0x7C  // CPU identification
#define OP_RDTSC        0x7D  // Read time stamp counter
#define OP_IN           0x7E  // Input from port
#define OP_OUT          0x7F  // Output to port

// Memory Management (0x80-0x8F)
#define OP_LOAD_CR      0x80  // Load control register
#define OP_STORE_CR     0x81  // Store control register
#define OP_LOAD_DR      0x82  // Load debug register
#define OP_STORE_DR     0x83  // Store debug register
#define OP_INVD         0x84  // Invalidate cache
#define OP_WBINVD       0x85  // Write back and invalidate cache
#define OP_PREFETCH     0x86  // Prefetch memory
#define OP_FLUSH_TLB    0x87  // Flush translation lookaside buffer

// String/Block Operations (0x90-0x9F)
#define OP_MOVS         0x90  // Move string
#define OP_CMPS         0x91  // Compare strings
#define OP_SCAS         0x92  // Scan string
#define OP_LODS         0x93  // Load string
#define OP_STOS         0x94  // Store string
#define OP_REP          0x95  // Repeat prefix
#define OP_REPZ         0x96  // Repeat while zero
#define OP_REPNZ        0x97  // Repeat while not zero

// Atomic Operations (0xA0-0xAF)
#define OP_XADD         0xA0  // Exchange and add
#define OP_CMPXCHG      0xA1  // Compare and exchange
#define OP_LOCK         0xA2  // Lock prefix
#define OP_MFENCE       0xA3  // Memory fence
#define OP_SFENCE       0xA4  // Store fence
#define OP_LFENCE       0xA5  // Load fence

// Floating Point (0xB0-0xBF) - Basic FP support
#define OP_FADD         0xB0  // Floating point add
#define OP_FSUB         0xB1  // Floating point subtract
#define OP_FMUL         0xB2  // Floating point multiply
#define OP_FDIV         0xB3  // Floating point divide
#define OP_FLD          0xB4  // Load floating point
#define OP_FST          0xB5  // Store floating point
#define OP_FCMP         0xB6  // Compare floating point

// Flag operations
#define FLAG_ZERO    0x01
#define FLAG_CARRY   0x02
#define FLAG_NEGATIVE 0x04
#define FLAG_OVERFLOW 0x08

// Control registers
#define CR0_PE  0x01  // Protection enable
#define CR0_MP  0x02  // Monitor coprocessor
#define CR0_EM  0x04  // Emulation
#define CR0_TS  0x08  // Task switched
#define CR0_PG  0x80  // Paging

void update_flags(MighfCPU *cpu, int result, int overflow) {
    cpu->flags = 0;
    if (result == 0) cpu->flags |= FLAG_ZERO;
    if (result < 0) cpu->flags |= FLAG_NEGATIVE;
    if (result > 255) cpu->flags |= FLAG_CARRY;
    if (overflow) cpu->flags |= FLAG_OVERFLOW;
}

uint16_t read16(MighfCPU *cpu, uint16_t addr) {
    return cpu->memory[addr] | (cpu->memory[addr + 1] << 8);
}

void write16(MighfCPU *cpu, uint16_t addr, uint16_t value) {
    cpu->memory[addr] = value & 0xFF;
    cpu->memory[addr + 1] = (value >> 8) & 0xFF;
}

void push(MighfCPU *cpu, uint8_t value) {
    cpu->sp--;
    cpu->memory[cpu->sp] = value;
}

uint8_t pop(MighfCPU *cpu) {
    uint8_t value = cpu->memory[cpu->sp];
    cpu->sp++;
    return value;
}

void trigger_interrupt(MighfCPU *cpu, uint8_t int_num) {
    if (!cpu->interrupt_enabled) return;
    
    // Save state
    push(cpu, cpu->flags);
    push(cpu, (cpu->pc >> 8) & 0xFF);
    push(cpu, cpu->pc & 0xFF);
    
    // Jump to interrupt handler
    cpu->pc = cpu->interrupt_table[int_num & 0x0F];
    cpu->interrupt_enabled = 0; // Disable interrupts during handler
}

int check_privilege(MighfCPU *cpu, uint8_t required_level) {
    return cpu->privilege_level <= required_level;
}

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
    cpu->sp = MEM_SIZE - 1; // Stack grows downward
    cpu->running = 1;
    cpu->privilege_level = 0; // Start in kernel mode
    cpu->interrupt_enabled = 1;
    
    // Initialize timer
    cpu->timer_counter = 0;
    cpu->timer_interval = 10000; // Timer interrupt every 10000 cycles

    while (cpu->running) {
        // Timer interrupt simulation
        cpu->timer_counter++;
        if (cpu->timer_counter >= cpu->timer_interval) {
            cpu->timer_counter = 0;
            trigger_interrupt(cpu, 0); // Timer interrupt on vector 0
        }

        uint8_t opcode = cpu->memory[cpu->pc++];

        switch (opcode) {
            case OP_NOP:
                break;

            case OP_HALT:
                printf("\n[HALT] CPU halted.\n");
                cpu->running = 0;
                break;

            case OP_BREAK:
                printf("[BREAK] Debug breakpoint at 0x%04X\n", cpu->pc - 1);
                break;

            case OP_SYSCALL:
                // System call - implementation would depend on OS
                printf("[SYSCALL] System call %d\n", cpu->regs[0]);
                break;

            // Data Movement Instructions
            case OP_LOAD_IMM8: {
                uint8_t reg = cpu->memory[cpu->pc++];
                uint8_t imm = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) cpu->regs[reg] = imm;
                break;
            }

            case OP_LOAD_IMM16: {
                uint8_t reg = cpu->memory[cpu->pc++];
                uint16_t imm = read16(cpu, cpu->pc);
                cpu->pc += 2;
                if (reg < NUM_REGS - 1) {
                    cpu->regs[reg] = imm & 0xFF;
                    cpu->regs[reg + 1] = (imm >> 8) & 0xFF;
                }
                break;
            }

            case OP_LOAD_MEM: {
                uint8_t reg = cpu->memory[cpu->pc++];
                uint16_t addr = read16(cpu, cpu->pc);
                cpu->pc += 2;
                if (reg < NUM_REGS) {
                    cpu->regs[reg] = cpu->memory[addr];
                }
                break;
            }

            case OP_STORE_MEM: {
                uint16_t addr = read16(cpu, cpu->pc);
                cpu->pc += 2;
                uint8_t reg = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) {
                    cpu->memory[addr] = cpu->regs[reg];
                    
                    // Memory-mapped I/O
                    if (addr == 0xFF00) { // Console output
                        putchar(cpu->regs[reg]);
                        fflush(stdout);
                    } else if (addr == 0xFF01) { // Console input
                        cpu->memory[0xFF01] = getchar();
                    }
                }
                break;
            }

            case OP_LOAD_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    cpu->regs[reg1] = cpu->regs[reg2];
                }
                break;
            }

            case OP_PUSH: {
                uint8_t reg = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) {
                    push(cpu, cpu->regs[reg]);
                }
                break;
            }

            case OP_POP: {
                uint8_t reg = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) {
                    cpu->regs[reg] = pop(cpu);
                }
                break;
            }

            // Arithmetic Instructions
            case OP_ADD_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    int result = cpu->regs[reg1] + cpu->regs[reg2];
                    cpu->regs[reg1] = result & 0xFF;
                    update_flags(cpu, result, 0);
                }
                break;
            }

            case OP_SUB_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    int result = cpu->regs[reg1] - cpu->regs[reg2];
                    cpu->regs[reg1] = result & 0xFF;
                    update_flags(cpu, result, 0);
                }
                break;
            }

            case OP_MUL_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    int result = cpu->regs[reg1] * cpu->regs[reg2];
                    cpu->regs[reg1] = result & 0xFF;
                    update_flags(cpu, result, 0);
                }
                break;
            }

            case OP_INC: {
                uint8_t reg = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) {
                    cpu->regs[reg]++;
                    update_flags(cpu, cpu->regs[reg], 0);
                }
                break;
            }

            case OP_DEC: {
                uint8_t reg = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) {
                    cpu->regs[reg]--;
                    update_flags(cpu, cpu->regs[reg], 0);
                }
                break;
            }

            // Logical Instructions
            case OP_AND_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    cpu->regs[reg1] &= cpu->regs[reg2];
                    update_flags(cpu, cpu->regs[reg1], 0);
                }
                break;
            }

            case OP_OR_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    cpu->regs[reg1] |= cpu->regs[reg2];
                    update_flags(cpu, cpu->regs[reg1], 0);
                }
                break;
            }

            case OP_XOR_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    cpu->regs[reg1] ^= cpu->regs[reg2];
                    update_flags(cpu, cpu->regs[reg1], 0);
                }
                break;
            }

            case OP_NOT: {
                uint8_t reg = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) {
                    cpu->regs[reg] = ~cpu->regs[reg];
                    update_flags(cpu, cpu->regs[reg], 0);
                }
                break;
            }

            case OP_SHL: {
                uint8_t reg = cpu->memory[cpu->pc++];
                uint8_t count = cpu->memory[cpu->pc++];
                if (reg < NUM_REGS) {
                    cpu->regs[reg] <<= count;
                    update_flags(cpu, cpu->regs[reg], 0);
                }
                break;
            }

            // Comparison Instructions
            case OP_CMP_REG: {
                uint8_t reg1 = cpu->memory[cpu->pc++];
                uint8_t reg2 = cpu->memory[cpu->pc++];
                if (reg1 < NUM_REGS && reg2 < NUM_REGS) {
                    int result = cpu->regs[reg1] - cpu->regs[reg2];
                    update_flags(cpu, result, 0);
                }
                break;
            }

            // Control Flow Instructions
            case OP_JMP: {
                uint16_t addr = read16(cpu, cpu->pc);
                cpu->pc = addr;
                break;
            }

            case OP_JZ: {
                uint16_t addr = read16(cpu, cpu->pc);
                cpu->pc += 2;
                if (cpu->flags & FLAG_ZERO) {
                    cpu->pc = addr;
                }
                break;
            }

            case OP_JNZ: {
                uint16_t addr = read16(cpu, cpu->pc);
                cpu->pc += 2;
                if (!(cpu->flags & FLAG_ZERO)) {
                    cpu->pc = addr;
                }
                break;
            }

            case OP_CALL: {
                uint16_t addr = read16(cpu, cpu->pc);
                cpu->pc += 2;
                push(cpu, (cpu->pc >> 8) & 0xFF);
                push(cpu, cpu->pc & 0xFF);
                cpu->pc = addr;
                break;
            }

            case OP_RET: {
                cpu->pc = pop(cpu) | (pop(cpu) << 8);
                break;
            }

            // Privileged Instructions
            case OP_STI:
                if (check_privilege(cpu, 0)) {
                    cpu->interrupt_enabled = 1;
                } else {
                    trigger_interrupt(cpu, 13); // General protection fault
                }
                break;

            case OP_CLI:
                if (check_privilege(cpu, 0)) {
                    cpu->interrupt_enabled = 0;
                } else {
                    trigger_interrupt(cpu, 13); // General protection fault
                }
                break;

            case OP_IRET:
                if (check_privilege(cpu, 0)) {
                    cpu->pc = pop(cpu) | (pop(cpu) << 8);
                    cpu->flags = pop(cpu);
                    cpu->interrupt_enabled = 1;
                } else {
                    trigger_interrupt(cpu, 13);
                }
                break;

            case OP_HLT_PRIV:
                if (check_privilege(cpu, 0)) {
                    // Wait for interrupt
                    while (cpu->interrupt_enabled && cpu->running) {
                        // Simulate waiting - in real implementation would halt CPU
                        usleep(1000);
                        cpu->timer_counter += 1000;
                        if (cpu->timer_counter >= cpu->timer_interval) {
                            cpu->timer_counter = 0;
                            trigger_interrupt(cpu, 0);
                            break;
                        }
                    }
                } else {
                    trigger_interrupt(cpu, 13);
                }
                break;

            case OP_IN: {
                if (check_privilege(cpu, 0)) {
                    uint8_t reg = cpu->memory[cpu->pc++];
                    uint8_t port = cpu->memory[cpu->pc++];
                    if (reg < NUM_REGS) {
                        // Simulate port input
                        cpu->regs[reg] = 0; // Default value
                        printf("[IN] Port 0x%02X -> R%d\n", port, reg);
                    }
                } else {
                    cpu->pc += 2;
                    trigger_interrupt(cpu, 13);
                }
                break;
            }

            case OP_OUT: {
                if (check_privilege(cpu, 0)) {
                    uint8_t port = cpu->memory[cpu->pc++];
                    uint8_t reg = cpu->memory[cpu->pc++];
                    if (reg < NUM_REGS) {
                        printf("[OUT] R%d (0x%02X) -> Port 0x%02X\n", reg, cpu->regs[reg], port);
                    }
                } else {
                    cpu->pc += 2;
                    trigger_interrupt(cpu, 13);
                }
                break;
            }

            default:
                printf("[ERROR] Unknown opcode 0x%02X at 0x%04X\n", opcode, cpu->pc - 1);
                trigger_interrupt(cpu, 6); // Invalid opcode exception
                break;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s program.mhfb\n", argv[0]);
   /*     fprintf(stderr, "\nMHF Virtual CPU - Extended ISA for OS Development\n");
        fprintf(stderr, "Supports:\n");
        fprintf(stderr, "  - 16 general-purpose registers\n");
        fprintf(stderr, "  - Privileged/user mode separation\n");
        fprintf(stderr, "  - Interrupt handling\n");
        fprintf(stderr, "  - Memory management instructions\n");
        fprintf(stderr, "  - Comprehensive arithmetic, logical, and control flow\n");
        fprintf(stderr, "  - System calls and I/O operations\n"); */
        return 1;
    }

    MighfCPU cpu = {0};
    uint16_t entry_point = 0;

    load_mhfb(argv[1], &cpu, &entry_point);
    
    printf("Starting MHF CPU with extended instruction set...\n");
    printf("Entry point: 0x%04X\n", entry_point);
    
    run_cpu(&cpu, entry_point);

    printf("\nExecution finished.\n");
    printf("Final CPU state:\n");
    for (int i = 0; i < NUM_REGS; i++) {
        printf("R%d = 0x%02X (%d)\n", i, cpu.regs[i], cpu.regs[i]);
    }
    printf("PC = 0x%04X, SP = 0x%04X\n", cpu.pc, cpu.sp);
    printf("Flags = 0x%02X [%c%c%c%c]\n", cpu.flags,
           (cpu.flags & FLAG_ZERO) ? 'Z' : '-',
           (cpu.flags & FLAG_CARRY) ? 'C' : '-',
           (cpu.flags & FLAG_NEGATIVE) ? 'N' : '-',
           (cpu.flags & FLAG_OVERFLOW) ? 'V' : '-');

    return 0;
}
