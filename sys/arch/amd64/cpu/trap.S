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

    .set TRAPSTR_ENTRIES, 14
    .section .data
trapstr_convtab:
    .quad vstr0
    .quad vstr1
    .quad vstr2
    .quad vstr3
    .quad vstr4
    .quad vstr5
    .quad vstr6
    .quad vstr7
    .quad vstr8
    .quad vstr9
    .quad vstr10
    .quad vstr11
    .quad vstr12
    .quad vstr13
    .quad vstr14
trapstr_tab:
    vstr0: .ascii "divide error\n\0"              /* v: 0x00 */
    vstr1: .ascii "debug exception\n\0"           /* v: 0x01 */
    vstr2: .ascii "non-maskable interrupt\n\0"    /* v: 0x02 */
    vstr3: .ascii "breakpoint exception\n\0"      /* v: 0x03 */
    vstr4: .ascii "overflow\n\0"                  /* v: 0x04 */
    vstr5: .ascii "bound range exceeded\n\0"      /* v: 0x05 */
    vstr6: .ascii "invalid opcode\n\0"            /* v: 0x06 */
    vstr7: .ascii "no math co-processor\n\0"      /* v: 0x07 */
    vstr8: .ascii "double fault\n\0"              /* v: 0x08 */
    vstr9: .ascii "reserved fault\n\0"            /* v: 0x09 */
    vstr10: .ascii "invalid TSS\n\0"              /* v: 0x0A */
    vstr11: .ascii "segment not present\n\0"      /* v: 0x0B */
    vstr12: .ascii "stack segment fault\n\0"      /* v: 0x0C */
    vstr13: .ascii "general protection fault\n\0" /* v: 0x0D */
    vstr14: .ascii "page fault\n\0"               /* v: 0xDE */

    .text
    .globl trap_dispatch
    .extern panic
trap_dispatch:
    mov 0(%rdi), %rax               /* Vector */
    cmp $TRAPSTR_ENTRIES, %rax      /* Too big? */
    jg .unknown_trap                /* Yeah... */

    mov %rax, %rcx                  /* Vector -> RCX */
    lea trapstr_convtab(%rip), %rdi /* Load the base here */
    leaq (%rdi, %rcx, 8), %rdi      /* Scale by the vector */
    mov (%rdi), %rdi                /* Get the error string address */
    call panic
    retq
.unknown_trap:
    lea error_unknown(%rip), %rdi
    call uart_puts
1:  cli
    hlt
    jmp 1b

    .section .rodata
error_header: .ascii "!! fatal \0"
error_unknown: .ascii "got unknown trap\n\0"

/* vim: ft=gas :
*/
