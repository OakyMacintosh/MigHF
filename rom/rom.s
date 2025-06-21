mov r0 42
movz r1 0
movn r2 255
add r3 r0
sub r4 r0
mul r5 r0
udiv r6 r0
neg r7
mov r1 15
mov r2 8
and r8 r1
orr r9 r2
eor r10 r1
mov r11 2
lsl r11 2
lsr r11 1
mov r12 123
store r12 10
load r13 10
mov r14 5
mov r15 5
cmp r14 r15
je 26
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
tdraw_clear
mov r0 5
mov r1 10
tdraw_pixel r0 r1 0
tdraw_pixel r0 r1 1
halt