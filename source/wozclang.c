#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>

#define MAX_LINE 1024
#define MAX_PROGRAM_SIZE 4096
#define INCLUDE_PATH "./source/include/"

// Simple structure to hold the output MigHF assembly
typedef struct {
    char **lines;
    size_t count;
    size_t capacity;
} OutputBuffer;

void ob_init(OutputBuffer *ob) {
    ob->capacity = 128;
    ob->count = 0;
    ob->lines = malloc(ob->capacity * sizeof(char*));
}
void ob_free(OutputBuffer *ob) {
    for (size_t i = 0; i < ob->count; ++i) free(ob->lines[i]);
    free(ob->lines);
}
void ob_add(OutputBuffer *ob, const char *line) {
    if (ob->count >= ob->capacity) {
        ob->capacity *= 2;
        ob->lines = realloc(ob->lines, ob->capacity * sizeof(char*));
    }
    ob->lines[ob->count++] = strdup(line);
}

// Very basic C to MigHF transpiler (only supports a subset)
void compile_c_file(const char *filename, OutputBuffer *ob);

void process_include(const char *line, OutputBuffer *ob) {
    // Only supports #include "header.h"
    const char *start = strchr(line, '"');
    if (!start) return;
    const char *end = strchr(start + 1, '"');
    if (!end) return;
    char header[256];
    snprintf(header, sizeof(header), "%.*s", (int)(end - start - 1), start + 1);

    char path[512];
    snprintf(path, sizeof(path), INCLUDE_PATH "%s", header);
    compile_c_file(path, ob);
}

void compile_c_file(const char *filename, OutputBuffer *ob) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        return;
    }
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        // Remove trailing newline
        line[strcspn(line, "\r\n")] = 0;
        // Handle includes
        if (strncmp(line, "#include", 8) == 0) {
            process_include(line, ob);
            continue;
        }
        // Ignore comments and empty lines
        if (line[0] == '/' && line[1] == '/') continue;
        if (line[0] == 0) continue;

        // --- Simple C parsing and translation ---
        // Variable declaration: int r0 = 5;
        int regnum, value;
        if (sscanf(line, "int r%d = %d;", &regnum, &value) == 2) {
            char buf[64];
            snprintf(buf, sizeof(buf), "mov r%d %d", regnum, value);
            ob_add(ob, buf);
            continue;
        }
        // Assignment: r0 = 5;
        if (sscanf(line, "r%d = %d;", &regnum, &value) == 2) {
            char buf[64];
            snprintf(buf, sizeof(buf), "mov r%d %d", regnum, value);
            ob_add(ob, buf);
            continue;
        }
        // Addition: r0 = r1 + r2;
        int regdst, regsrc1, regsrc2;
        if (sscanf(line, "r%d = r%d + r%d;", &regdst, &regsrc1, &regsrc2) == 3) {
            char buf1[32], buf2[32];
            snprintf(buf1, sizeof(buf1), "mov r%d 0", regdst);
            snprintf(buf2, sizeof(buf2), "add r%d r%d", regdst, regsrc1);
            ob_add(ob, buf1);
            ob_add(ob, buf2);
            snprintf(buf2, sizeof(buf2), "add r%d r%d", regdst, regsrc2);
            ob_add(ob, buf2);
            continue;
        }
        // Subtraction: r0 = r1 - r2;
        if (sscanf(line, "r%d = r%d - r%d;", &regdst, &regsrc1, &regsrc2) == 3) {
            char buf1[32], buf2[32];
            snprintf(buf1, sizeof(buf1), "mov r%d 0", regdst);
            snprintf(buf2, sizeof(buf2), "add r%d r%d", regdst, regsrc1);
            ob_add(ob, buf1);
            ob_add(ob, buf2);
            snprintf(buf2, sizeof(buf2), "sub r%d r%d", regdst, regsrc2);
            ob_add(ob, buf2);
            continue;
        }
        // Print: printf("reg: %d\n", r0);
        if (strstr(line, "printf") && strstr(line, "r")) {
            if (sscanf(line, "printf(\"%%d\", r%d);", &regnum) == 1 ||
                sscanf(line, "printf(\"reg: %%d\", r%d);", &regnum) == 1) {
                char buf[32];
                snprintf(buf, sizeof(buf), "print reg %d", regnum);
                ob_add(ob, buf);
                continue;
            }
        }
        // Input: scanf("%d", &r0);
        if (strstr(line, "scanf") && strstr(line, "&r")) {
            if (sscanf(line, "scanf(\"%%d\", &r%d);", &regnum) == 1) {
                char buf[32];
                snprintf(buf, sizeof(buf), "in r%d", regnum);
                ob_add(ob, buf);
                continue;
            }
        }
        // Memory store: memory[10] = r0;
        int addr;
        if (sscanf(line, "memory[%d] = r%d;", &addr, &regnum) == 2) {
            char buf[32];
            snprintf(buf, sizeof(buf), "store r%d %d", regnum, addr);
            ob_add(ob, buf);
            continue;
        }
        // Memory load: r0 = memory[10];
        if (sscanf(line, "r%d = memory[%d];", &regnum, &addr) == 2) {
            char buf[32];
            snprintf(buf, sizeof(buf), "load r%d %d", regnum, addr);
            ob_add(ob, buf);
            continue;
        }
        // Halt: return 0;
        if (strstr(line, "return 0;")) {
            ob_add(ob, "halt");
            continue;
        }
        // Fallback: add as comment
        char buf[MAX_LINE + 16];
        snprintf(buf, sizeof(buf), "# %s", line);
        ob_add(ob, buf);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: wozclang <file.c>\n");
        return 1;
    }
    OutputBuffer ob;
    ob_init(&ob);

    compile_c_file(argv[1], &ob);

    // Output the generated MigHF assembly to stdout
    for (size_t i = 0; i < ob.count; ++i) {
        puts(ob.lines[i]);
    }

    ob_free(&ob);
    return 0;
}