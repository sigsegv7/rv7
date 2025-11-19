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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <os/trace.h>
#include <mu/panic.h>

/*
 * XXX: We could implement panic() as an assembly stub
 *      that takes a register snapshot and passes it to
 *      an MI routine which calls into the MD side using
 *      it as an argument, that could be better...
 */
void
mu_panic_dump(void)
{
    static uint64_t cr4, cr3, cr2, cr0;
    static uint64_t rax, rbx, rcx, rdx;
    static uint64_t r[16], rbp, rsp;

    __asmv(
        "mov %%cr4, %0\n\t"
        "mov %%cr3, %1\n\t"
        "mov %%cr2, %2\n\t"
        "mov %%cr0, %3"
        : "=r" (cr4),
          "=r" (cr3),
          "=r" (cr2),
          "=r" (cr0)
        :
        : "memory"
    );

    __asmv(
        "mov %%r8, %0\n\t"
        "mov %%r9, %1\n\t"
        "mov %%r10, %2\n\t"
        "mov %%r11, %3\n\t"
        "mov %%r12, %4\n\t"
        "mov %%r13, %5\n\t"
        "mov %%r14, %6\n\t"
        "mov %%r15, %7\n\t"
        "mov %%rax, %8\n\t"
        "mov %%rbx, %8\n\t"
        "mov %%rcx, %9\n\t"
        "mov %%rdx, %10\n\t"
        "mov %%rbp, %11\n\t"
        "mov %%rsp, %12"
        : "=r" (r[0]),
          "=r" (r[1]),
          "=r" (r[2]),
          "=r" (r[3]),
          "=r" (r[4]),
          "=r" (r[5]),
          "=r" (r[6]),
          "=r" (r[7]),
          "=r" (rax),
          "=r" (rbx),
          "=r" (rcx),
          "=r" (rdx),
          "=r" (rbp),
          "=r" (rsp)
        :
        : "memory"
    );

    trace(
        "CR0=%p CR2=%p\nCR3=%p CR4=%p\n",
        cr0, cr2, cr3, cr4
    );

    trace(
        "---------------------------------------------\n"
        "RAX=%p RBX=%p\nRCX=%p RDX=%p\n"
        "RBP=%p RSP=%p\n",
        rax, rbx, rcx, rdx,
        rbp, rsp
    );

    trace(
        "---------------------------------------------\n"
        "R15=%p R14=%p\nR13=%p R12=%p\nR11=%p R10=%p\n"
        "R9=%p R8=%p",
        r[7], r[6], r[5],
        r[4], r[3], r[2],
        r[1], r[0]
    );

}

void
mu_panic_hcf(void)
{
    for (;;) {
        asm volatile("cli; hlt");
    }
}
