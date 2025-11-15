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

#include <md/kfence.h>

#define KERNEL_CS 0x08

#define INT_GATE 0x8E
#define TRAP_GATE 0x8F

.macro set_trap vector, isr
    movq \vector, %rdi
    movq $TRAP_GATE, %rsi
    leaq \isr(%rip), %rdx
    xorq %rcx, %rcx
    callq idt_set_gate
.endm

.macro push_frame vector
.if \vector == 10 || \vector == 11 || \vector == 12 || \vector == 13 || \vector == 14
    subq $8, %rsp
.endif
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    pushq $\vector
.endm

.macro pop_frame vector
    add $8, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
.if \vector == 10 || \vector == 11 || \vector == 12 || \vector == 13 || \vector == 14
    add $8, %rsp
.endif
.endm

    .text
    .globl idt_set_gate
idt_set_gate:
    /*
     * Set an interrupt gate descriptor in the IDT
     *
     * [RDI] {0}: (uint8_t)vector
     * [RSI] {1}: (uint8_t)type
     * [RDX] {2}: (uintptr_t)isr
     * [RCX] {3}: (uint8_t)ist
     */
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15

    shlq $4, %rdi               /* Scale vector by 16 */
    leaq IDT(%rip), %rbx        /* Load the IDT base */
    addq %rdi, %rbx             /* Descriptor base -> RBX */

    movw %dx, 0(%rbx)           /* Offset low */
    movw $KERNEL_CS, 2(%rbx)    /* Segment selector */
    andq $0x38, %rcx            /* Zero IST[7:3] */
    movb %cl, 4(%rbx)           /* IST */
    movb $0b10000000, 5(%rbx)   /* P, DPL[0], ZERO */
    andq $0xF, %rsi             /* Zero TYPE[7:5] */
    orb %sil, 5(%rbx)           /* Type */

    shrq $16, %rdx              /* RDX >>= 16 */
    movw %dx, 6(%rbx)           /* Offset middle */
    shrq $16, %rdx              /* RDX >>= 16 */
    movl %edx, 8(%rbx)          /* Offset high */

    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    retq

    .globl idt_load
idt_load:
    call set_traps
    lea IDTR(%rip), %rdx
    lidt (%rdx)
    retq

set_traps:
    set_trap $0x00, diverr
    set_trap $0x01, debug_except
    set_trap $0x02, nmi
    set_trap $0x03, breakpoint
    set_trap $0x04, overflow
    set_trap $0x05, bound_range
    set_trap $0x06, invalid_tss
    set_trap $0x07, no_coproc
    set_trap $0x08, double_fault
    set_trap $0x0A, invalid_tss
    set_trap $0x0B, seg_np
    set_trap $0x0C, ss_fault
    set_trap $0x0D, gpf
    set_trap $0x0E, page_fault
    retq

diverr:
    KFENCE
    push_frame 0x0
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x0
    KFENCE
1:  cli
    hlt
    jmp 1b

debug_except:
    KFENCE
    push_frame 0x1
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x1
    KFENCE
1:  cli
    hlt
    jmp 1b

nmi:
    KFENCE
    push_frame 0x2
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x2
    KFENCE
1:  cli
    hlt
    jmp 1b

breakpoint:
    KFENCE
    push_frame 0x3
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x3
    KFENCE
1:  cli
    hlt
    jmp 1b

overflow:
    KFENCE
    push_frame 0x4
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x4
    KFENCE
1:  cli
    hlt
    jmp 1b

bound_range:
    KFENCE
    push_frame 0x5
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x5
    KFENCE
1:  cli
    hlt
    jmp 1b

invl_opc:
    KFENCE
    push_frame 0x6
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x6
    KFENCE
1:  cli
    hlt
    jmp 1b

no_coproc:
    KFENCE
    push_frame 0x7
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x7
    KFENCE
1:  cli
    hlt
    jmp 1b

double_fault:
    KFENCE_EC
    push_frame 0x8
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0x8
    KFENCE_EC
1:  cli
    hlt
    jmp 1b

invalid_tss:
    KFENCE_EC
    push_frame 0xA
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0xA
    KFENCE_EC
1:  cli
    hlt
    jmp 1b

seg_np:
    KFENCE_EC
    push_frame 0xB
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0xB
    KFENCE_EC
1:  cli
    hlt
    jmp 1b

ss_fault:
    KFENCE_EC
    push_frame 0xC
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0xC
    KFENCE_EC
1:  cli
    hlt
    jmp 1b

gpf:
    KFENCE_EC
    push_frame 0xD
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0xD
    KFENCE_EC
1:  cli
    hlt
    jmp 1b

page_fault:
    KFENCE_EC
    push_frame 0xE
    mov %rsp, %rdi
    call trap_dispatch
    pop_frame  0xE
    KFENCE_EC
1:  cli
    hlt
    jmp 1b
    hlt

    .section .data
    .align 8
IDT:
    .fill 256, 8, 512
IDTR:
limit:
    .word 0xFFF
    .quad IDT

/* vim: ft=gas :
*/
