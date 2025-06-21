#ifndef PROGRAM_COUNTER_H
#define PROGRAM_COUNTER_H

#include <stdint.h>

// Program Counter structure
typedef struct {
    uint32_t value;
} ProgramCounter;

// Initialize the program counter to zero
static inline void pc_init(ProgramCounter *pc) {
    pc->value = 0;
}

// Set the program counter to a specific value
static inline void pc_set(ProgramCounter *pc, uint32_t val) {
    pc->value = val;
}

// Increment the program counter by 1
static inline void pc_increment(ProgramCounter *pc) {
    pc->value += 1;
}

// Get the current value of the program counter
static inline uint32_t pc_get(const ProgramCounter *pc) {
    return pc->value;
}

#endif // PROGRAM_COUNTER_H