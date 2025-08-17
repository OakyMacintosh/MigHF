import numpy as np

class MicroArchitecture:
    def __init__(self):
        self.registers = np.zeros(32)
        self.pc = 0
        self.memory = np.zeros(1024)
        self.stack_pointer = 0
        self.stack = np.zeros(1024)

    def AsmPyFunctions(self):
        def add(a, b):
            return a + b

        def sub(a, b):
            return a - b

        def mul(a, b):
            return a * b

        def xor(a, b):
            return a ^ b

        def or(a, b):
            return a | b

        def and(a, b):
            return a & b

        def not(a):
            return ~a

        def neg(a):
            return -a

        def xnor(a, b):
            return ~(a ^ b)

        def nand(a, b):
            return ~(a & b)

        def nor(a, b):
            return ~(a | b)

        def mov(a, b):
            return b

        def cmp(a, b):
            return a - b

        def push(a):
            self.stack[self.stack_pointer] = a
            self.stack_pointer += 1

        def pop():
            self.stack_pointer -= 1
            return self.stack[self.stack_pointer]

        def ret():
            self.pc = self.stack[self.stack_pointer]
            self.stack_pointer -= 1

        def call(a):
            self.stack[self.stack_pointer] = self.pc
            self.stack_pointer += 1
            self.pc = a

        def ld(a, b):
            self.registers[a] = self.memory[b]

        def st(a, b):
            self.memory[b] = self.registers[a]

        def ldsp(a, b):
            self.registers[a] = self.stack[b]

        def stsp(a, b):
            self.stack[b] = self.registers[a]

        def pushsp(a):
            self.stack[self.stack_pointer] = a
            self.stack_pointer += 1

        def jmp(label):
            self.pc = self.labels[label]

        # labels (start:, <Label name>:, like a function, invoque with jmp <Label name>)
        def label(a):
            self.labels[a] = self.pc

        return {
            'add': add,
            'sub': sub,
            'mul': mul,
            'xor': xor,
            'or': or,
            'and': and,
            'not': not,
            'neg': neg,
            'xnor': xnor,
            'nand': nand,
            'nor': nor,
            'mov': mov,
            'cmp': cmp,
            'push': push,
            'pop': pop,
            'ret': ret,
            'call': call,
            'ld': ld,
            'st': st,
            'ldsp': ldsp,
            'stsp': stsp,
            'pushsp': pushsp,
            'jmp': jmp,
            'label': label
        }
