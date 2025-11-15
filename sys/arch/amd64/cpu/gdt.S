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
    .globl gdt_load
gdt_load:
    /*
     * void gdt_load(GDTR *GDTR);
     */

    push %r12
    push %r13
    push %r14
    push %r15
    push %rbx
    push %rbp

    lgdt (%rdi)             /* Load our own GDT */
    lea 1f(%rip), %rbx      /* Save the drop point */
    push $0x08              /* FRAME.CS */
    push %rbx               /* FRAME.RIP */
    lretq                   /* Flush prefetch and update CS */
1:  mov $0x10, %ax          /* Our DS */
    mov %ax, %ds            /* Data segment */
    mov %ax, %fs            /* F segment */
    mov %ax, %ss            /* Stack segment */
    mov %ax, %es            /* Extra segment */
    xor %ax, %ax            /* We want GS in a known state */
    mov %ax, %gs            /* Load it with NULL */

    pop %rbp
    pop %rbx
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    retq

    .section .data
    .globl GDT
GDT:
.NULL:
    .quad 0x0000000000000000
.CODE:
    .quad 0x00209A0000000000
.DATA:
    .quad 0x0000920000000000

    .globl GDTR
GDTR:
    .word . - GDT - 1
    .quad GDT


/* vim: ft=gas :
*/
