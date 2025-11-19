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

#include <kern/panic.h>
#include <vm/kalloc.h>
#include <vm/tlsf.h>
#include <vm/phys.h>
#include <mu/spinlock.h>
#include <vm/vm.h>

#define MEM_SIZE 0x200000

/*
 * XXX: This lock is shared globally, it might be a better
 *      idea to move the whole context per core sometime
 *      soon?
 */
static volatile size_t lock = 0;
static tlsf_t ctx;

void *
kalloc(size_t sz)
{
    void *tmp;

    mu_spinlock_acq(&lock, 0);
    tmp = tlsf_malloc(ctx, sz);
    mu_spinlock_rel(&lock, 0);
    return tmp;
}

void
kfree(void *ptr)
{
    mu_spinlock_acq(&lock, 0);
    tlsf_free(ctx, ptr);
    mu_spinlock_rel(&lock, 0);
}

void
vm_kalloc_init(void)
{
    uintptr_t phys, *virt;

    phys = vm_phys_alloc(MEM_SIZE / 4096);
    if (phys == 0) {
        panic("kalloc: could not allocate pages\n");
    }

    virt = PHYS_TO_VIRT(phys);
    ctx = tlsf_create_with_pool(virt, MEM_SIZE);
    if (ctx == NULL) {
        panic("kalloc: could not init context\n");
    }
}
