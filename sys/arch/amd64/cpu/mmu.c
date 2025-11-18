/*
 * Copyright (c) 2023-2025 Ian Marco Moffett and the Osmora Team.
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
#include <sys/errno.h>
#include <sys/cdefs.h>
#include <kern/panic.h>
#include <mu/mmu.h>
#include <vm/vm.h>
#include <vm/phys.h>
#include <md/vas.h>

/*
 * See Intel SDM Vol 3A, Section 4.5, Table 4-19
 */
#define PTE_ADDR_MASK   0x000FFFFFFFFFF000
#define PTE_P           BIT(0)        /* Present */
#define PTE_RW          BIT(1)        /* Writable */
#define PTE_US          BIT(2)        /* User r/w allowed */
#define PTE_PWT         BIT(3)        /* Page-level write-through */
#define PTE_PCD         BIT(4)        /* Page-level cache disable */
#define PTE_ACC         BIT(5)        /* Accessed */
#define PTE_DIRTY       BIT(6)        /* Dirty (written-to page) */
#define PTE_PS          BIT(7)        /* Page size */
#define PTE_GLOBAL      BIT(8)        /* Global / sticky map */
#define PTE_NX          BIT(63)       /* Execute-disable */

int
mu_pmap_readvas(struct mmu_vas *vas)
{
    __asmv(
        "mov %%cr3, %0"
        : "=r" (vas->cr3)
        :
        : "memory"
    );

    return 0;
}

int
mu_pmap_writevas(struct mmu_vas *vas)
{
    __asmv(
        "mov %0, %%cr3"
        :
        : "r" (vas->cr3)
        : "memory"
    );

    return 0;
}

int
mu_pmap_forkvas(struct mmu_vas *result)
{
    struct mmu_vas vas;
    uintptr_t paddr, *pml4_dest;
    uintptr_t *pml4_src;

    mu_pmap_readvas(&vas);
    paddr = vm_phys_alloc(1);
    if (paddr == 0) {
        return -ENOMEM;
    }

    pml4_dest = PHYS_TO_VIRT(paddr);
    pml4_src = PHYS_TO_VIRT(vas.cr3);
    for (uint16_t i = 0; i < 512; ++i) {
        if (i < 256) {
            pml4_dest[i] = 0;
        } else {
            pml4_dest[i] = pml4_src[i];
        }
    }

    return 0;
}

void
mu_pmap_init(void)
{
    struct mmu_vas cur_vas;
    uintptr_t *pml4;

    /* Tear down the old lower half mappings */
    mu_pmap_readvas(&cur_vas);
    pml4 = PHYS_TO_VIRT(cur_vas.cr3);
    for (uint16_t i = 0; i < 256; ++i) {
        pml4[i] = 0;
    }

    /*
     * The entries in the TLB may still refer to the old lower
     * half mappings which are now stale... To avoid this biting
     * us in the ass we should flush the *entire* TLB
     */
    mu_pmap_writevas(&cur_vas);
}
