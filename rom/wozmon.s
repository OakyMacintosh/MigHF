# wozmon for MigHF - label-free version

mov r1 0          # (0) address pointer = 0
mov r5 1          # (1) loop increment = 1

# Main loop
print reg 1       # (2) Show address pointer
out r1            # (3)
in r0             # (4) Read command/input char

mov r3 82         # (5) ASCII 'R'
cmp r0 r3         # (6)
je 13             # (7) If 'R', jump to read_mem

mov r3 87         # (8) ASCII 'W'
cmp r0 r3         # (9)
je 18             # (10) If 'W', jump to write_mem

mov r3 74         # (11) ASCII 'J'
cmp r0 r3         # (12)
je 23             # (13) If 'J', jump to jump_sub

mov r3 81         # (14) ASCII 'Q'
cmp r0 r3         # (15)
je 27             # (16) If 'Q', jump to quit

jmp 2             # (17) Loop back to main

# read_mem:
load r2 r1        # (18) r2 = memory[r1]
print reg 2       # (19)
out r2            # (20)
add r1 r5         # (21) r1 += 1
jmp 2             # (22) Loop back to main

# write_mem:
in r2             # (23) Get value to write
store r2 r1       # (24) memory[r1] = r2
print reg 2       # (25)
out r2            # (26)
add r1 r5         # (27) r1 += 1
jmp 2             # (28) Loop back to main

# jump_sub:
call r1           # (29) Call address in r1
jmp 2             # (30) Loop back to main

# quit:
halt              # (31)