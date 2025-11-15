    .text
    .globl pio_outb
pio_outb:
    mov %rdi, %rdx      // PORT
    mov %rsi, %rax      // VAL
    outb %al, %dx
    retq

    .globl pio_outw
pio_outw:
    mov %rdi, %rdx      // PORT
    mov %rsi, %rax      // VAL
    outw %ax, %dx
    retq

    .globl pio_outl
pio_outl:
    mov %rdi, %rdx      // PORT
    mov %rsi, %rax      // VAL
    outl %eax, %dx
    retq

    .globl pio_inb
pio_inb:
    mov %rdi, %rdx      // PORT
    xor %rax, %rax
    inb %dx, %al
    retq

.global pio_inw
pio_inw:
    mov %rdi, %rdx      // PORT
    xor %rax, %rax
    inw %dx, %ax
    retq

.global pio_inl
pio_inl:
    mov %rdi, %rdx      // PORT
    xor %rax, %rax
    inl %dx, %eax
    retq

/* vim: ft=gas :
*/
