    .globl _start
_start:
1:  cli
    hlt
    jmp 1b
