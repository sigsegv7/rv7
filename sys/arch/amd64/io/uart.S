    .set UART_COM1, 0x3F8

.macro bus_outb port, val
    mov $\port, %rdi
    mov $\val, %rsi
    call pio_outb
.endm

    .text
    .globl uart_init
uart_init:
    /*
     * void uart_init(void);
     */

    push %r12
    push %r13
    push %r14
    push %r15
    push %rbx
    push %rbp

    bus_outb UART_COM1+1, 0x00      /* Disable interrupts */
    bus_outb UART_COM1+3, 1<<7      /* Set DLAB */
    bus_outb UART_COM1+0, 0x02      /* 57600 buad */
    bus_outb UART_COM1+1, 0x00      /* High bits */
    bus_outb UART_COM1+3, 0x03      /* Line control [data+stop,no parity] */
    bus_outb UART_COM1+2, 0x00      /* Set FIFO, 14-byte threshold */
    bus_outb UART_COM1+4, 0x0B      /* DTR+RTS */

    pop %rbp
    pop %rbx
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    retq

uart_write:
    /*
     * void uart_write(const char *s, size_t len);
     */

    push %r12
    push %r13
    push %r14
    push %r15
    push %rbx
    push %rbp

    mov %rsi, %rcx
    mov %rdi, %rsi
.strloop:
    lodsb
    mov $UART_COM1, %dx
    out %al, %dx
    loop .strloop

    pop %rbp
    pop %rbx
    pop %r15
    pop %r14
    pop %r13
    pop %r12
.done:
    retq

/* vim: ft=gas :
*/
