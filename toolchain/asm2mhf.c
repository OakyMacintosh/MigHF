#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#include <unistd.h>
#endif

#define VERSION "0.1.0-toolchain'"

void asm2mgf() {
    printf("MIGHF Toolchain - Assembler to Mighf RAW Converter\n");
    printf("Version: %s\n", VERSION);
    printf("This tool converts assembly code to Mighf RAW format.\n");
    
    printf("Usage: asm2mgf <input.asm> <output.mgf>\n");
    printf("Example: asm2mgf program.asm program.mgf\n");
    printf("Note: Ensure the input file is a valid MIGHF assembly file.\n");

    if (argc != 3) {
        fprintf(stderr, "Error: Invalid number of arguments.\n");
        fprintf(stderr, "Usage: asm2mgf <input.asm> <output.mgf>\n");
        return;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];
 
    FILE *input = fopen(input_file, "r");
    if (!input) {
        fprintf(stderr, "Error: Could not open input file '%s'.\n", input_file);
        return;
    }

    FILE *output = fopen(output_file, "w");
    if (!output) {
        fprintf(stderr, "Error: Could not open output file '%s'.\n", output_file);
        fclose(input);
        return;
    }

    void convert(FILE *input, FILE *output) {
        char line[256];
        while (fgets(line, sizeof(line), input)) {
            if (line[0] == '\n' || line[0] == ';') continue; // Skip empty lines and comments

            
            fprintf(output, "Converted: %s", line);
        }
    }

    convert(input, output);
    printf("Conversion complete. Output written to '%s'.\n", output_file);
    fclose(input);
    fclose(output);
}