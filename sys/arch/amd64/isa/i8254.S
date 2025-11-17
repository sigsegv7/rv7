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

    .set I8254_COMMAND,   0x43
    .set I8254_CHANNEL0,  0x40

    .text
    .globl i8254_set_count
i8254_set_count:
    /*
     * RDI: Count
     */

    push %r12
    push %r13
    push %r14
    push %r15
    push %rbx
    push %rbp

    mov %rdi, %rax              /* Count -> RAX */
    mov $0x34, %al              /* Latch lo/hi bytes */
    out %al, $I8254_COMMAND     /* Send the command */
    out %al, $I8254_CHANNEL0    /* Write low byte */
    shr $8, %ah                 /* Shift down, get high byte */
    out %al, $I8254_CHANNEL0    /* Write high byte */

    pop %rbp
    pop %rbx
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    retq

    .globl i8254_get_count
i8254_get_count:
    push %r12
    push %r13
    push %r14
    push %r15
    push %rbx
    push %rbp

    mov $0x00, %al              /* Get the current counter */
    out %al, $I8254_COMMAND     /* Send the command */
    xor %rdx, %rdx              /* Make sure RDX is zeroed */
    in $I8254_CHANNEL0, %al     /* Read low byte */
    mov %al, %dl                /* Set low byte */
    in $I8254_CHANNEL0, %al     /* Read high byte */
    shl $8, %dh                 /* Shift to make room */
    or %al, %dl                 /* Merge high and low */
    mov %rdx, %rax              /* Return what we got */

    pop %rbp
    pop %rbx
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    retq

/* vim: ft=gas :
*/
