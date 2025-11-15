/*
 * Copyright (c) 2025 Ian Marco Moffett and the Osmora Team.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Hyra nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

    .globl uart_write
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
