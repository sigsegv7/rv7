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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPKERNE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LKERNS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * PKERNSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <mu/spinlock.h>

void
mu_spinlock_acq(volatile size_t *lock, int flags)
{
    if (ISSET(flags, SPINLOCK_INTTOG)) {
        asm volatile("cli");
    }

    __asmv(
        "1:\n\t"
        "   rep; nop\n\t"
        "   mov $1, %%rax\n\t"
        "   xchg (%0), %%rax\n\t"
        "   or %%rax, %%rax\n\t"
        "   jnz 1b"
        :
        : "r" (lock)
        : "memory", "rax"
    );
}

void
mu_spinlock_rel(volatile size_t *lock, int flags)
{
    __asmv(
        "xor %%rax, %%rax\n\t"
        "xchg %%rax, %0"
        : "=m" (*lock)
        :
        : "memory", "rax"
    );

    if (ISSET(flags, SPINLOCK_INTTOG)) {
        asm volatile("sti");
    }
}
