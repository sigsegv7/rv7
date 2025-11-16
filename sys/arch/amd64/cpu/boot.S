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

    .text
    .globl _start
    .extern uart_init
    .extern uart_puts
    .extern idt_load
    .extern gdt_load
    .extern GDTR
_start:
    cli
    cld

    xor %rbp, %rbp          /* Terminate callstack */
    call uart_init          /* Initialize platform UART */

    lea GDTR(%rip), %rdi    /* Our GDTR */
    call gdt_load           /* Load our GDT */
    call idt_load           /* Load our IDT */

    /*
     * RV7 will default to APIC operation, as per a section
     * somewhere around 5.2.12 of the ACPI spec, we'll need
     * to disable the PC-AT dual 8259 chips.
     */
    mov $0xFF, %al          /* Mask all inputs */
    out %al, $0x21          /* Disable the master PIC */
    out %al, $0xA1          /* Disable the slave PIC */

    lea bootmsg(%rip), %rdi
    call uart_puts

    call kmain              /* Call our kernel entrypoint */
1:  cli
    hlt
    jmp 1b

    .section .rodata
bootmsg:
    .ascii "[ preparing since 2025 ]\n"
    .ascii "[ 00:00 delta @ crev : 89 seconds]\n"
    .ascii "** booting rv7 ...\n"
    .byte 0x00

/* vim: ft=gas :
*/
