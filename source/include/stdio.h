#ifndef STDIO_H
#define STDIO_H

// Print integer register (maps to MigHF print reg)
void printf(const char *fmt, int reg);

// Read integer into register (maps to MigHF in)
void scanf(const char *fmt, int *reg);

#endif