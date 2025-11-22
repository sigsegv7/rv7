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

#include <sys/param.h>
#include <sys/errno.h>
#include <vm/map.h>
#include <vm/vm.h>
#include <mu/mmu.h>

int
vm_map_pages(struct mmu_vas *vas, const struct vm_region *region, uint16_t prot)
{
    uintptr_t va, pa;
    int error;

    if (vas == NULL || region == NULL) {
        return -EINVAL;
    }

    va = ALIGN_DOWN(region->va, PAGESIZE);
    pa = ALIGN_DOWN(region->pa, PAGESIZE);
    for (size_t i = 0; i < region->page_count; ++i) {
        error = mu_pmap_map(
            vas,
            pa + (i*PAGESIZE),
            va + (i*PAGESIZE),
            prot,
            region->ps
        );

        if (error < 0) {
            vm_unmap_pages(vas, va, i, region->ps);
            return error;
        }
    }

    return 0;
}

int
vm_unmap_pages(struct mmu_vas *vas, uintptr_t va, size_t count, pagesize_t ps)
{
    int error;

    if (vas == NULL) {
        return -EINVAL;
    }

    for (size_t i = 0; i < count; ++i) {
        error = mu_pmap_unmap(
            vas,
            va + (i*PAGESIZE),
            ps
        );

        if (error < 0) {
            return error;
        }
    }

    return 0;
}
