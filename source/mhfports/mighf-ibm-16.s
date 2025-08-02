; ========================================================================
; MIGHF Virtual Architecture - IBM PC Port
; ========================================================================
;
;  ________  ________  ___  __        ___    ___ _____ ______   ________  ________     
; |\   __  \|\   __  \|\  \|\  \     |\  \  /  /|\   _ \  _   \|\   __  \|\   ____\    
; \ \  \|\  \ \  \|\  \ \  \/  /|_   \ \  \/  / | \  \\\__\ \  \ \  \|\  \ \  \___|    
;  \ \  \\\  \ \   __  \ \   ___  \   \ \    / / \ \  \\|__| \  \ \   __  \ \  \       
;   \ \  \\\  \ \  \ \  \ \  \\ \  \   \/  /  /   \ \  \    \ \  \ \  \ \  \ \  \____  
;    \ \_______\ \__\ \__\ \__\\ \__\__/  / /      \ \__\    \ \__\ \__\ \__\ \_______\
;     \|_______|\|__|\|__|\|__| \|__|\___/ /        \|__|     \|__|\|__|\|__|\|_______|
;                                   \|___|/                                            
;
; IBM PC 16-bit Assembly Language Port
; Original author: OakyMac
; Assembly port for 8086/8088 processors
;
; Assemble with: nasm -f bin mighf-ibmpc.asm -o mighf.com
; Run with: mighf.com [program.mighf]
;
; ========================================================================

        org 0x100           ; COM file format starts at 0x100

section .text

; ========================================================================
; CONSTANTS AND DEFINITIONS
; ========================================================================

; Memory and register limits
MEM_SIZE        equ 512
MAX_REGISTERS   equ 32
STACK_SIZE      equ 64
MAX_PROGRAM     equ 256
MAX_LINE        equ 64

; Opcodes
OP_NOP          equ 0
OP_MOV          equ 1
OP_ADD          equ 2
OP_SUB          equ 3
OP_LOAD         equ 4
OP_STORE        equ 5
OP_JMP          equ 6
OP_JE           equ 7
OP_HALT         equ 8
OP_AND          equ 9
OP_OR           equ 10
OP_XOR          equ 11
OP_SHL          equ 12
OP_SHR          equ 13
OP_MUL          equ 14
OP_DIV          equ 15
OP_CMP          equ 16
OP_JNE          equ 17
OP_JG           equ 18
OP_JL           equ 19
OP_PUSH         equ 20
OP_POP          equ 21
OP_CALL         equ 22
OP_RET          equ 23
OP_PRINT        equ 24
OP_INPUT        equ 25

; ========================================================================
; MAIN PROGRAM ENTRY POINT
; ========================================================================

start:
        ; Initialize VM
        call init_vm
        
        ; Check command line arguments
        mov si, 0x81        ; Command line buffer in PSP
        call skip_spaces
        cmp byte [si], 0x0D ; Check if empty command line
        je shell_mode
        
        ; File mode - load and run program
        call load_program
        jc error_exit
        call run_program
        jmp exit_program
        
shell_mode:
        call interactive_shell
        
exit_program:
        ; Exit to DOS
        mov ax, 0x4C00
        int 0x21

error_exit:
        ; Exit with error code
        mov ax, 0x4C01
        int 0x21

; ========================================================================
; VM INITIALIZATION
; ========================================================================

init_vm:
        ; Clear registers
        mov di, registers
        mov cx, MAX_REGISTERS
        xor ax, ax
        rep stosw
        
        ; Clear memory
        mov di, memory
        mov cx, MEM_SIZE
        rep stosb
        
        ; Clear program memory
        mov di, program
        mov cx, MAX_PROGRAM * 4    ; 4 words per instruction
        rep stosw
        
        ; Initialize VM state
        mov word [pc], 0
        mov word [sp_vm], 0
        mov byte [running], 1
        mov byte [flag_zero], 0
        mov byte [last_cmp], 0
        
        ret

; ========================================================================
; INTERACTIVE SHELL
; ========================================================================

interactive_shell:
        ; Print banner
        mov si, banner_msg
        call print_string
        
.shell_loop:
        ; Print prompt
        mov si, prompt_msg
        call print_string
        
        ; Read command line
        mov di, input_buffer
        mov cx, MAX_LINE
        call read_line
        
        ; Parse and execute command
        mov si, input_buffer
        call parse_command
        
        cmp byte [running], 0
        jne .shell_loop
        
        ret

; ========================================================================
; COMMAND PARSING
; ========================================================================

parse_command:
        call skip_spaces
        
        ; Check for empty line
        cmp byte [si], 0
        je .done
        
        ; Compare with known commands
        mov di, cmd_help
        call compare_command
        jc .cmd_help
        
        mov di, cmd_exit
        call compare_command
        jc .cmd_exit
        
        mov di, cmd_regs
        call compare_command
        jc .cmd_regs
        
        mov di, cmd_mem
        call compare_command
        jc .cmd_mem
        
        mov di, cmd_load
        call compare_command
        jc .cmd_load
        
        mov di, cmd_run
        call compare_command
        jc .cmd_run
        
        ; Unknown command
        mov si, unknown_msg
        call print_string
        jmp .done
        
.cmd_help:
        mov si, help_msg
        call print_string
        jmp .done
        
.cmd_exit:
        mov byte [running], 0
        jmp .done
        
.cmd_regs:
        call show_registers
        jmp .done
        
.cmd_mem:
        call show_memory
        jmp .done
        
.cmd_load:
        call load_program
        jmp .done
        
.cmd_run:
        call run_program
        jmp .done
        
.done:
        ret

; ========================================================================
; REGISTER DISPLAY
; ========================================================================

show_registers:
        mov si, regs_header_msg
        call print_string
        
        mov cx, MAX_REGISTERS
        mov bx, 0               ; Register index
        
.reg_loop:
        ; Print "R"
        mov al, 'R'
        call print_char
        
        ; Print register number
        mov ax, bx
        call print_decimal
        
        ; Print ": "
        mov al, ':'
        call print_char
        mov al, ' '
        call print_char
        
        ; Print register value
        mov ax, bx
        shl ax, 1               ; Convert to word offset
        mov si, registers
        add si, ax
        mov ax, [si]
        call print_decimal
        
        ; Print newline
        call print_newline
        
        inc bx
        loop .reg_loop
        
        ret

; ========================================================================
; MEMORY DISPLAY
; ========================================================================

show_memory:
        mov si, mem_header_msg
        call print_string
        
        mov cx, 16              ; Show first 16 bytes
        mov bx, 0               ; Memory index
        
.mem_loop:
        ; Print address
        mov ax, bx
        call print_hex
        
        ; Print ": "
        mov al, ':'
        call print_char
        mov al, ' '
        call print_char
        
        ; Print memory value
        mov si, memory
        add si, bx
        mov al, [si]
        mov ah, 0
        call print_hex_byte
        
        ; Print newline
        call print_newline
        
        inc bx
        loop .mem_loop
        
        ret

; ========================================================================
; PROGRAM EXECUTION
; ========================================================================

run_program:
        mov word [pc], 0
        mov byte [running], 1
        
.exec_loop:
        cmp byte [running], 0
        je .done
        
        ; Fetch instruction
        mov ax, [pc]
        cmp ax, MAX_PROGRAM
        jae .done
        
        ; Calculate instruction address (4 words per instruction)
        shl ax, 3               ; Multiply by 8 (4 words * 2 bytes)
        mov si, program
        add si, ax
        
        ; Load instruction
        mov ax, [si]            ; opcode
        mov [current_opcode], ax
        mov ax, [si+2]          ; op1
        mov [current_op1], ax
        mov ax, [si+4]          ; op2
        mov [current_op2], ax
        mov ax, [si+6]          ; immediate
        mov [current_imm], ax
        
        ; Execute instruction
        call execute_instruction
        
        ; Increment PC
        inc word [pc]
        
        jmp .exec_loop
        
.done:
        mov si, program_finished_msg
        call print_string
        ret

; ========================================================================
; INSTRUCTION EXECUTION
; ========================================================================

execute_instruction:
        mov ax, [current_opcode]
        
        cmp ax, OP_NOP
        je .op_nop
        cmp ax, OP_MOV
        je .op_mov
        cmp ax, OP_ADD
        je .op_add
        cmp ax, OP_SUB
        je .op_sub
        cmp ax, OP_HALT
        je .op_halt
        cmp ax, OP_CMP
        je .op_cmp
        cmp ax, OP_JE
        je .op_je
        cmp ax, OP_JNE
        je .op_jne
        cmp ax, OP_JMP
        je .op_jmp
        cmp ax, OP_PRINT
        je .op_print
        
        ; Unknown opcode
        ret
        
.op_nop:
        ret
        
.op_mov:
        ; MOV reg, immediate
        mov bx, [current_op1]
        cmp bx, MAX_REGISTERS
        jae .invalid_reg
        shl bx, 1               ; Convert to word offset
        mov si, registers
        add si, bx
        mov ax, [current_imm]
        mov [si], ax
        ret
        
.op_add:
        ; ADD reg1, reg2
        mov bx, [current_op1]
        cmp bx, MAX_REGISTERS
        jae .invalid_reg
        mov dx, [current_op2]
        cmp dx, MAX_REGISTERS
        jae .invalid_reg
        
        shl bx, 1
        mov si, registers
        add si, bx              ; si = &registers[op1]
        shl dx, 1
        mov di, registers
        add di, dx              ; di = &registers[op2]
        
        mov ax, [si]
        add ax, [di]
        mov [si], ax
        ret
        
.op_sub:
        ; SUB reg1, reg2
        mov bx, [current_op1]
        cmp bx, MAX_REGISTERS
        jae .invalid_reg
        mov dx, [current_op2]
        cmp dx, MAX_REGISTERS
        jae .invalid_reg
        
        shl bx, 1
        mov si, registers
        add si, bx              ; si = &registers[op1]
        shl dx, 1
        mov di, registers
        add di, dx              ; di = &registers[op2]
        
        mov ax, [si]
        sub ax, [di]
        mov [si], ax
        ret
        
.op_halt:
        mov byte [running], 0
        ret
        
.op_cmp:
        ; CMP reg1, reg2
        mov bx, [current_op1]
        cmp bx, MAX_REGISTERS
        jae .invalid_reg
        mov dx, [current_op2]
        cmp dx, MAX_REGISTERS
        jae .invalid_reg
        
        shl bx, 1
        mov si, registers
        add si, bx              ; si = &registers[op1]
        shl dx, 1
        mov di, registers
        add di, dx              ; di = &registers[op2]
        
        mov ax, [si]
        cmp ax, [di]
        je .cmp_equal
        ja .cmp_greater
        
        ; Less than
        mov byte [flag_zero], 0
        mov byte [last_cmp], 0xFF   ; -1
        ret
        
.cmp_equal:
        mov byte [flag_zero], 1
        mov byte [last_cmp], 0
        ret
        
.cmp_greater:
        mov byte [flag_zero], 0
        mov byte [last_cmp], 1
        ret
        
.op_je:
        cmp byte [flag_zero], 1
        jne .no_jump
        mov ax, [current_imm]
        cmp ax, MAX_PROGRAM
        jae .no_jump
        dec ax                  ; Adjust for PC increment
        mov [pc], ax
.no_jump:
        ret
        
.op_jne:
        cmp byte [flag_zero], 0
        jne .jne_jump
        ret
.jne_jump:
        mov ax, [current_imm]
        cmp ax, MAX_PROGRAM
        jae .no_jump
        dec ax
        mov [pc], ax
        ret
        
.op_jmp:
        mov ax, [current_imm]
        cmp ax, MAX_PROGRAM
        jae .no_jump
        dec ax
        mov [pc], ax
        ret
        
.op_print:
        ; PRINT type, index (type: 0=reg, 1=mem)
        mov ax, [current_op1]
        cmp ax, 0
        je .print_reg
        cmp ax, 1
        je .print_mem
        ret
        
.print_reg:
        mov bx, [current_imm]
        cmp bx, MAX_REGISTERS
        jae .invalid_reg
        
        ; Print "R"
        mov al, 'R'
        call print_char
        
        ; Print register number
        mov ax, bx
        call print_decimal
        
        ; Print " = "
        mov si, equals_msg
        call print_string
        
        ; Print register value
        shl bx, 1
        mov si, registers
        add si, bx
        mov ax, [si]
        call print_decimal
        call print_newline
        ret
        
.print_mem:
        mov bx, [current_imm]
        cmp bx, MEM_SIZE
        jae .invalid_mem
        
        ; Print "MEM["
        mov si, mem_prefix_msg
        call print_string
        
        ; Print address
        mov ax, bx
        call print_decimal
        
        ; Print "] = "
        mov si, mem_suffix_msg
        call print_string
        
        ; Print memory value
        mov si, memory
        add si, bx
        mov al, [si]
        mov ah, 0
        call print_decimal
        call print_newline
        ret
        
.invalid_reg:
.invalid_mem:
        ret

; ========================================================================
; MIGHF ASSEMBLY FILE LOADER
; ========================================================================

load_program:
        ; Parse filename from command line or prompt user
        mov si, input_buffer
        call skip_spaces
        cmp byte [si], 0
        jne .has_filename
        
        ; Prompt for filename
        mov si, filename_prompt_msg
        call print_string
        mov di, filename_buffer
        mov cx, 64
        call read_line
        mov si, filename_buffer
        
.has_filename:
        ; Copy filename to our buffer
        mov di, filename_buffer
        call copy_string
        
        ; Check file extension
        call check_mhf_extension
        jc .invalid_extension
        
        ; Open file
        mov dx, filename_buffer
        mov ax, 0x3D00          ; Open file for reading
        int 0x21
        jc .file_error
        mov [file_handle], ax
        
        ; Initialize program counter
        mov word [program_index], 0
        
.read_loop:
        ; Read a line from file
        call read_file_line
        jc .end_of_file
        
        ; Parse the line
        mov si, file_line_buffer
        call parse_assembly_line
        jc .parse_error
        
        ; Check if we have room for more instructions
        mov ax, [program_index]
        cmp ax, MAX_PROGRAM
        jae .program_too_large
        
        jmp .read_loop
        
.end_of_file:
        ; Close file
        mov bx, [file_handle]
        mov ah, 0x3E
        int 0x21
        
        ; Print success message
        mov si, program_loaded_msg
        call print_string
        mov ax, [program_index]
        call print_decimal
        mov si, instructions_loaded_msg
        call print_string
        
        clc                     ; Success
        ret
        
.invalid_extension:
        mov si, invalid_extension_msg
        call print_string
        stc
        ret
        
.file_error:
        mov si, file_error_msg
        call print_string
        stc
        ret
        
.parse_error:
        mov si, parse_error_msg
        call print_string
        stc
        ret
        
.program_too_large:
        mov si, program_too_large_msg
        call print_string
        stc
        ret

; Check if filename has .mhf extension
check_mhf_extension:
        mov si, filename_buffer
        
        ; Find the end of the string
        mov cx, 0               ; String length counter
.find_end:
        cmp byte [si], 0
        je .check_extension
        inc si
        inc cx
        jmp .find_end
        
.check_extension:
        ; Check if string is long enough for .mhf extension
        cmp cx, 4               ; Need at least "a.mhf"
        jb .invalid
        
        ; Go back 4 characters to check extension
        sub si, 4
        
        ; Check for ".mhf" (case insensitive)
        mov al, [si]
        cmp al, '.'
        jne .invalid
        
        inc si
        mov al, [si]
        cmp al, 'M'
        je .check_h
        cmp al, 'm'
        jne .invalid
        
.check_h:
        inc si
        mov al, [si]
        cmp al, 'H'
        je .check_f
        cmp al, 'h'
        jne .invalid
        
.check_f:
        inc si
        mov al, [si]
        cmp al, 'F'
        je .valid
        cmp al, 'f'
        jne .invalid
        
.valid:
        clc                     ; Valid extension
        ret
        
.invalid:
        stc                     ; Invalid extension
        ret

; Read a line from the open file
read_file_line:
        mov di, file_line_buffer
        mov cx, 0               ; Character count
        
.read_char:
        ; Read one character
        mov bx, [file_handle]
        mov dx, temp_char
        mov ah, 0x3F            ; Read from file
        mov cx, 1
        int 0x21
        jc .error
        
        ; Check if we read anything
        cmp ax, 0
        je .eof
        
        ; Get the character
        mov al, [temp_char]
        
        ; Check for end of line
        cmp al, 10              ; LF
        je .end_line
        cmp al, 13              ; CR
        je .read_char           ; Skip CR, continue reading
        
        ; Store character
        mov [di], al
        inc di
        inc cx
        
        ; Check buffer limit
        cmp cx, MAX_LINE - 1
        jb .read_char
        
.end_line:
        mov byte [di], 0        ; Null terminate
        clc                     ; Success
        ret
        
.eof:
        cmp cx, 0               ; If no characters read, it's EOF
        je .error
        mov byte [di], 0        ; Null terminate partial line
        clc
        ret
        
.error:
        stc                     ; Error
        ret

; Parse an assembly language line
parse_assembly_line:
        ; Skip leading whitespace
        call skip_spaces
        
        ; Skip empty lines and comments
        cmp byte [si], 0
        je .success
        cmp byte [si], ';'
        je .success
        cmp byte [si], 10
        je .success
        cmp byte [si], 13
        je .success
        
        ; Extract opcode
        mov di, opcode_buffer
        call extract_word
        
        ; Convert opcode to uppercase
        mov di, opcode_buffer
        call to_uppercase
        
        ; Match opcode
        call match_opcode
        jc .unknown_opcode
        
        ; AX now contains the opcode number
        mov [current_opcode], ax
        
        ; Parse operands based on instruction type
        call parse_operands
        jc .operand_error
        
        ; Store instruction in program array
        call store_instruction
        
.success:
        clc
        ret
        
.unknown_opcode:
        stc
        ret
        
.operand_error:
        stc
        ret

; Extract a word (space-delimited) from SI to DI
extract_word:
        ; Skip leading spaces
        call skip_spaces
        
.copy_loop:
        mov al, [si]
        cmp al, 0
        je .done
        cmp al, ' '
        je .done
        cmp al, 9               ; Tab
        je .done
        cmp al, ','
        je .done
        cmp al, 10              ; LF
        je .done
        cmp al, 13              ; CR
        je .done
        
        mov [di], al
        inc si
        inc di
        jmp .copy_loop
        
.done:
        mov byte [di], 0        ; Null terminate
        ret

; Convert string at DI to uppercase
to_uppercase:
.loop:
        mov al, [di]
        cmp al, 0
        je .done
        cmp al, 'a'
        jb .next
        cmp al, 'z'
        ja .next
        sub al, 32              ; Convert to uppercase
        mov [di], al
.next:
        inc di
        jmp .loop
.done:
        ret

; Match opcode string to opcode number
match_opcode:
        mov di, opcode_buffer
        
        ; Compare with known opcodes
        mov si, str_nop
        call compare_strings
        jc .found_nop
        
        mov si, str_mov
        call compare_strings
        jc .found_mov
        
        mov si, str_add
        call compare_strings
        jc .found_add
        
        mov si, str_sub
        call compare_strings
        jc .found_sub
        
        mov si, str_cmp
        call compare_strings
        jc .found_cmp
        
        mov si, str_jmp
        call compare_strings
        jc .found_jmp
        
        mov si, str_je
        call compare_strings
        jc .found_je
        
        mov si, str_jne
        call compare_strings
        jc .found_jne
        
        mov si, str_jg
        call compare_strings
        jc .found_jg
        
        mov si, str_jl
        call compare_strings
        jc .found_jl
        
        mov si, str_print
        call compare_strings
        jc .found_print
        
        mov si, str_halt
        call compare_strings
        jc .found_halt
        
        ; Unknown opcode
        stc
        ret
        
.found_nop:   
        mov ax, OP_NOP   
        jmp .found
.found_mov:   
        mov ax, OP_MOV   
        jmp .found
.found_add:   
        mov ax, OP_ADD   
        jmp .found
.found_sub:   
        mov ax, OP_SUB   
        jmp .found
.found_cmp:   
        mov ax, OP_CMP   
        jmp .found
.found_jmp:   
        mov ax, OP_JMP   
        jmp .found
.found_je:    
        mov ax, OP_JE    
        jmp .found
.found_jne:   
        mov ax, OP_JNE   
        jmp .found
.found_jg:    
        mov ax, OP_JG    
        jmp .found
.found_jl:    
        mov ax, OP_JL    
        jmp .found
.found_print: 
        mov ax, OP_PRINT 
        jmp .found
.found_halt:  
        mov ax, OP_HALT  
        jmp .found

.found:
        clc
        ret

; Parse operands for the current instruction
parse_operands:
        mov ax, [current_opcode]
        
        cmp ax, OP_NOP
        je .no_operands
        cmp ax, OP_HALT
        je .no_operands
        
        cmp ax, OP_MOV
        je .parse_mov
        cmp ax, OP_ADD
        je .parse_two_regs
        cmp ax, OP_SUB
        je .parse_two_regs
        cmp ax, OP_CMP
        je .parse_two_regs
        
        cmp ax, OP_JMP
        je .parse_address
        cmp ax, OP_JE
        je .parse_address
        cmp ax, OP_JNE
        je .parse_address
        cmp ax, OP_JG
        je .parse_address
        cmp ax, OP_JL
        je .parse_address
        
        cmp ax, OP_PRINT
        je .parse_print
        
        ; Default: no operands
.no_operands:
        mov word [current_op1], 0
        mov word [current_op2], 0
        mov word [current_imm], 0
        clc
        ret
        
.parse_mov:
        ; MOV Rx, immediate
        call parse_register
        jc .error
        mov [current_op1], ax
        
        call skip_comma
        call parse_number
        jc .error
        mov [current_imm], ax
        mov word [current_op2], 0
        clc
        ret
        
.parse_two_regs:
        ; Operation Rx, Ry
        call parse_register
        jc .error
        mov [current_op1], ax
        
        call skip_comma
        call parse_register
        jc .error
        mov [current_op2], ax
        mov word [current_imm], 0
        clc
        ret
        
.parse_address:
        ; Jump address
        call parse_number
        jc .error
        mov [current_imm], ax
        mov word [current_op1], 0
        mov word [current_op2], 0
        clc
        ret
        
.parse_print:
        ; PRINT REG/MEM, index
        mov di, opcode_buffer
        call extract_word
        call to_uppercase
        
        mov si, str_reg
        call compare_strings
        jc .print_reg
        mov si, str_mem
        call compare_strings
        jc .print_mem
        stc
        ret
        
.print_reg:
        mov word [current_op1], 0
        call skip_comma
        call parse_number
        jc .error
        mov [current_imm], ax
        mov word [current_op2], 0
        clc
        ret
        
.print_mem:
        mov word [current_op1], 1
        call skip_comma
        call parse_number
        jc .error
        mov [current_imm], ax
        mov word [current_op2], 0
        clc
        ret
        
.error:
        stc
        ret

; Parse a register (Rx format)
parse_register:
        call skip_spaces
        
        ; Check for 'R' or 'r'
        mov al, [si]
        cmp al, 'R'
        je .has_r
        cmp al, 'r'
        je .has_r
        stc
        ret
        
.has_r:
        inc si
        call parse_number
        ret

; Parse a decimal number
parse_number:
        call skip_spaces
        
        mov ax, 0               ; Result
        mov bx, 10              ; Base
        
.digit_loop:
        mov al, [si]
        cmp al, '0'
        jb .done
        cmp al, '9'
        ja .done
        
        ; Convert digit
        sub al, '0'
        mov dx, ax              ; Save digit
        
        ; Multiply result by 10
        mov ax, [temp_number]
        mul bx
        add ax, dx
        mov [temp_number], ax
        
        inc si
        jmp .digit_loop
        
.done:
        mov ax, [temp_number]
        mov word [temp_number], 0
        clc
        ret

; Skip comma and whitespace
skip_comma:
        call skip_spaces
        cmp byte [si], ','
        jne .no_comma
        inc si
        call skip_spaces
.no_comma:
        ret

; Store the current instruction in the program array
store_instruction:
        mov ax, [program_index]
        shl ax, 3               ; Multiply by 8 (4 words * 2 bytes)
        mov di, program
        add di, ax
        
        ; Store instruction
        mov ax, [current_opcode]
        stosw
        mov ax, [current_op1]
        stosw
        mov ax, [current_op2]
        stosw
        mov ax, [current_imm]
        stosw
        
        inc word [program_index]
        ret

; Compare two strings (SI and DI), return carry if equal
compare_strings:
.loop:
        mov al, [si]
        cmp al, [di]
        jne .not_equal
        cmp al, 0
        je .equal
        inc si
        inc di
        jmp .loop
        
.equal:
        stc
        ret
        
.not_equal:
        clc
        ret

; Copy string from SI to DI
copy_string:
.loop:
        mov al, [si]
        mov [di], al
        cmp al, 0
        je .done
        inc si
        inc di
        jmp .loop
.done:
        ret

; ========================================================================
; UTILITY FUNCTIONS
; ========================================================================

; Print string pointed to by SI
print_string:
        push ax
        push si
.loop:
        lodsb
        cmp al, 0
        je .done
        call print_char
        jmp .loop
.done:
        pop si
        pop ax
        ret

; Print character in AL
print_char:
        push ax
        push dx
        mov dl, al
        mov ah, 0x02
        int 0x21
        pop dx
        pop ax
        ret

; Print newline
print_newline:
        mov al, 13
        call print_char
        mov al, 10
        call print_char
        ret

; Print decimal number in AX
print_decimal:
        push ax
        push bx
        push cx
        push dx
        
        mov bx, 10
        mov cx, 0
        
.div_loop:
        xor dx, dx
        div bx
        push dx
        inc cx
        cmp ax, 0
        jne .div_loop
        
.print_loop:
        pop dx
        add dl, '0'
        mov al, dl
        call print_char
        loop .print_loop
        
        pop dx
        pop cx
        pop bx
        pop ax
        ret

; Print hex byte in AL
print_hex_byte:
        push ax
        
        mov ah, al
        shr al, 4
        call .print_hex_digit
        
        mov al, ah
        and al, 0x0F
        call .print_hex_digit
        
        pop ax
        ret
        
.print_hex_digit:
        cmp al, 10
        jb .digit
        add al, 'A' - 10
        jmp .print
.digit:
        add al, '0'
.print:
        call print_char
        ret

; Print hex word in AX
print_hex:
        push ax
        mov al, ah
        call print_hex_byte
        pop ax
        call print_hex_byte
        ret

; Read line into buffer at DI, max CX characters
read_line:
        push ax
        push bx
        push dx
        
        mov bx, 0               ; Character count
        
.read_loop:
        mov ah, 0x01            ; Read character
        int 0x21
        
        cmp al, 13              ; Enter key
        je .done
        cmp al, 8               ; Backspace
        je .backspace
        
        cmp bx, cx              ; Check buffer limit
        jae .read_loop
        
        mov [di + bx], al
        inc bx
        jmp .read_loop
        
.backspace:
        cmp bx, 0
        je .read_loop
        dec bx
        ; Echo backspace sequence
        mov al, 8
        call print_char
        mov al, ' '
        call print_char
        mov al, 8
        call print_char
        jmp .read_loop
        
.done:
        mov byte [di + bx], 0   ; Null terminate
        call print_newline
        
        pop dx
        pop bx
        pop ax
        ret

; Skip spaces in string pointed to by SI
skip_spaces:
        cmp byte [si], ' '
        jne .done
        inc si
        jmp skip_spaces
.done:
        ret

; Compare command at SI with command at DI
; Returns carry set if match
compare_command:
        push ax
        push si
        push di
        
.compare_loop:
        mov al, [si]
        cmp al, [di]
        jne .no_match
        
        cmp al, 0
        je .match
        
        inc si
        inc di
        jmp .compare_loop
        
.match:
        stc                     ; Set carry
        jmp .done
        
.no_match:
        clc                     ; Clear carry
        
.done:
        pop di
        pop si
        pop ax
        ret

; ========================================================================
; DATA SECTION
; ========================================================================

section .data

; VM State
pc              dw 0
sp_vm           dw 0
running         db 1
flag_zero       db 0
last_cmp        db 0

; Current instruction
current_opcode  dw 0
current_op1     dw 0
current_op2     dw 0
current_imm     dw 0

; Messages
banner_msg      db 'MIGHF Virtual Machine - IBM PC Assembly Port', 13, 10
                db 'Type "help" for commands.', 13, 10, 0

prompt_msg      db '0 > ', 0
help_msg        db 'Commands:', 13, 10
                db '  help  - Show this help', 13, 10
                db '  regs  - Show registers', 13, 10
                db '  mem   - Show memory', 13, 10
                db '  load [file] - Load MIGHF assembly file (.mhf)', 13, 10
                db '  run   - Run program', 13, 10
                db '  exit  - Exit program', 13, 10, 0

unknown_msg     db 'Unknown command. Type "help" for help.', 13, 10, 0
regs_header_msg db 'Registers:', 13, 10, 0
mem_header_msg  db 'Memory (first 16 bytes):', 13, 10, 0
program_loaded_msg db 'Program loaded: ', 0
instructions_loaded_msg db ' instructions.', 13, 10, 0
program_finished_msg db 'Program execution finished.', 13, 10, 0

filename_prompt_msg db 'Enter filename (.mhf): ', 0
file_error_msg  db 'Error: Cannot open file.', 13, 10, 0
invalid_extension_msg db 'Error: Only .mhf files are allowed.', 13, 10, 0
parse_error_msg db 'Error: Parse error in assembly file.', 13, 10, 0
program_too_large_msg db 'Error: Program too large.', 13, 10, 0

equals_msg      db ' = ', 0
mem_prefix_msg  db 'MEM[', 0
mem_suffix_msg  db '] = ', 0

; Commands
cmd_help        db 'help', 0
cmd_exit        db 'exit', 0
cmd_regs        db 'regs', 0
cmd_mem         db 'mem', 0
cmd_load        db 'load', 0
cmd_run         db 'run', 0

; Opcode strings for parsing
str_nop         db 'NOP', 0
str_mov         db 'MOV', 0
str_add         db 'ADD', 0
str_sub         db 'SUB', 0
str_cmp         db 'CMP', 0
str_jmp         db 'JMP', 0
str_je          db 'JE', 0
str_jne         db 'JNE', 0
str_jg          db 'JG', 0
str_jl          db 'JL', 0
str_print       db 'PRINT', 0
str_halt        db 'HALT', 0
str_reg         db 'REG', 0
str_mem         db 'MEM', 0

; File handling
file_handle     dw 0
program_index   dw 0
temp_number     dw 0
temp_char       db 0

; ========================================================================
; BSS SECTION (UNINITIALIZED DATA)
; ========================================================================

section .bss

; VM Memory and Registers
registers       resw MAX_REGISTERS
memory          resb MEM_SIZE
vm_stack        resw STACK_SIZE
program         resw MAX_PROGRAM * 4    ; 4 words per instruction

; Input buffer
input_buffer    resb MAX_LINE
filename_buffer resb 64
file_line_buffer resb MAX_LINE
opcode_buffer   resb 16