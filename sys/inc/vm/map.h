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

#ifndef _VM_MAP_H_
#define _VM_MAP_H_

#include <sys/types.h>
#include <mu/mmu.h>

struct vm_region {
    uintptr_t pa;
    uintptr_t va;
    size_t page_count;
    pagesize_t ps;
};

/*
 * Map a contigious region of pages
 *
 * @vas: Address space to map within
 * @region: Region to start mapping at
 * @prot: Protection flags
 * @ps: Desired pagesize
 *
 * Returns zero on success
 */
int vm_map_pages(
    struct mmu_vas *vas, const struct vm_region *region,
    uint16_t prot
);

/*
 * Unmap a contigious range of pages
 *
 * @vas: Addresss space to unmap from
 * @va: Virtual address to unmap
 * @count: Number of pages to unmap
 * @ps: Page size of region
 *
 * Returns zero on success
 */
int vm_unmap_pages(struct mmu_vas *vas, uintptr_t va, size_t count, pagesize_t ps);

#endif  /* !_VM_MAP_H_ */
