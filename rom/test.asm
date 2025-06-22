mov r0 10         # r0 = 10
mov r1 20         # r1 = 20
add r2 r0         # r2 = r2 + r0 (should be 10)
add r2 r1         # r2 = r2 + r1 (should be 30)
sub r2 r0         # r2 = r2 - r0 (should be 20)
mul r3 r1         # r3 = r3 * r1 (should be 0, then 0*20=0)
add r3 r0         # r3 = r3 + r0 (should be 10)
mul r3 r1         # r3 = r3 * r1 (should be 200)
udiv r3 r0        # r3 = r3 / r0 (should be 20)
neg r4            # r4 = -r4 (should be 0)
add r4 r0         # r4 = r4 + r0 (should be 10)
neg r4            # r4 = -r4 (should be -10, but unsigned)

# Test bitwise
mov r5 15         # r5 = 15
mov r6 8          # r6 = 8
and r7 r5         # r7 = r7 & r5 (should be 0)
orr r7 r6         # r7 = r7 | r6 (should be 8)
eor r7 r5         # r7 = r7 ^ r5 (should be 15 ^ 8 = 7)

# Test shifts
mov r8 2
lsl r8 3          # r8 = r8 << 3 (should be 16)
lsr r8 2          # r8 = r8 >> 2 (should be 4)

# Test memory
mov r9 123
store r9 50       # memory[50] = 123
load r10 50       # r10 = memory[50] (should be 123)

# Test stack
push r10
mov r10 0
pop r10           # r10 should be 123

# Test call/ret (simple subroutine at 40)
call 40
jmp 42
# Subroutine at 40:
mov r11 99
ret

# Test comparison and branching
mov r12 5
mov r13 5
cmp r12 r13
je 50             # Should jump to 50
mov r14 1         # Should be skipped
jmp 51
mov r14 2         # Should be executed (at 50)
# End of branch test

# Test I/O
mov r15 42
out r15           # Should print 42
in r15            # Read a value into r15
out r15           # Print the value read

# Test memcpy, movb, strb
mov r0 65
strb r0 100       # memory[100] = 65 ('A')
movb r1 100       # r1 = memory[100] (should be 65)
mov r2 66
strb r2 101       # memory[101] = 66 ('B')
memcpy 110 100 2  # memory[110] = memory[100..101]
movb r3 110       # r3 = memory[110] (should be 65)
movb r4 111       # r4 = memory[111] (should be 66)

# Print results
print reg 0
print reg 1
print reg 2
print reg 3
print reg 4
print reg 5
print reg 6
print reg 7
print reg 8
print reg 9
print reg 10
print reg 11
print reg 12
print reg 13
print reg 14
print reg 15

# Comment test