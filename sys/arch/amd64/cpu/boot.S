    .globl _start
    .extern uart_init
_start:
    cli
    cld

    xor %rbp, %rbp
    call uart_init

1:  cli
    hlt
    jmp 1b

/* vim: ft=gas :
*/
