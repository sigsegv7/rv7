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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/param.h>
#include <kern/panic.h>
#include <mu/spinlock.h>
#include <os/trace.h>
#include <vm/phys.h>
#include <vm/vm.h>
#include <lib/limine.h>
#include <lib/string.h>
#include <lib/stdbool.h>

#define dtrace(fmt, ...) trace("phys: " fmt, ##__VA_ARGS__)

#define MEM_GIB 0x40000000
#define MEM_MIB 0x100000

/* Aliases for portability */
#define MEM_USABLE              (LIMINE_MEMMAP_USABLE)
#define MEM_RESERVED            (LIMINE_MEMMAP_RESERVED)
#define MEM_ACPI_RECLAIMABLE    (LIMINE_MEMMAP_ACPI_RECLAIMABLE)
#define MEM_ACPI_NVS            (LIMINE_MEMMAP_ACPI_NVS)
#define MEM_BAD                 (LIMINE_MEMMAP_BAD_MEMORY)
#define MEM_BOOTLOADER          (LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
#define MEM_KERNEL              (LIMINE_MEMMAP_KERNEL_AND_MODULES)
#define MEM_FRAMEBUFFER         (LIMINE_MEMMAP_FRAMEBUFFER)

#define MEM_ENTRY(INDEX) (memmap_resp->entries[(INDEX)])
#define MEM_ENTRY_COUNT  (memmap_resp->entry_count)

typedef struct limine_memmap_entry mementry_t;

/* Bitmap */
static volatile size_t bitmap_lock = 0;
static uint8_t *bitmap = NULL;
static size_t last_index = 0;

/* Memory statistics */
static size_t total_mem = 0;
static size_t free_mem = 0;
static size_t reserved_mem = 0;
static size_t bitmap_size = 0;
static uintptr_t highest_usable = 0;

/*
 * Request a memor map from the bootloader
 */
static struct limine_memmap_response *memmap_resp;
static volatile struct limine_memmap_request memmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

/*
 * Print memory size related stats
 */
static void
vm_printstat(const char *name, size_t size)
{
    if (size >= MEM_GIB) {
        dtrace("%s %d GiB\n", name, size / MEM_GIB);
    } else {
        dtrace("%s %d MiB\n", name, size / MEM_MIB);
    }
}

/*
 * Set a range in a bitmap as allocated or free
 */
static void
bitmap_set_range(uintptr_t start, uintptr_t end, bool alloc)
{
    /* Clamp the range */
    start = ALIGN_DOWN(start, PAGESIZE);
    end = ALIGN_UP(end, PAGESIZE);

    for (uintptr_t p = start; p < end; p += PAGESIZE) {
        if (alloc) {
            setbit(bitmap, p / PAGESIZE);
        } else {
            clrbit(bitmap, p / PAGESIZE);
        }
    }
}

/*
 * Populate bitmap entries based on what is free and
 * what is not
 */
static void
vm_fill_bitmap(void)
{
    uintptr_t start, end;
    size_t entries_set = 0;
    mementry_t *entry;

    for (size_t i = 0; i < MEM_ENTRY_COUNT; ++i) {
        entry = MEM_ENTRY(i);

        /* Drop unusable entries */
        if (entry->type != MEM_USABLE) {
            continue;
        }

        start = entry->base;
        end = entry->base + entry->length;
        bitmap_set_range(start, end, false);
        ++entries_set;
    }

    dtrace("populated %d entries\n", entries_set);
}

/*
 * Find a physical memory hole big enough to fit the
 * bitmap
 */
static void
vm_alloc_bitmap(void)
{
    mementry_t *entry;

    for (size_t i = 0; i < MEM_ENTRY_COUNT; ++i) {
        entry = MEM_ENTRY(i);

        /* Drop unusable entries */
        if (entry->type != MEM_USABLE) {
            continue;
        }

        /* Does the bitmap fit here? */
        if (entry->length >= bitmap_size) {
            bitmap = PHYS_TO_VIRT(entry->base);
            entry->length -= bitmap_size;
            entry->base += bitmap_size;
            break;
        }
    }

    if (__unlikely(bitmap == NULL)) {
        panic("vm: unable to allocate framedb\n");
    }

    /* Populate the bitmap */
    memset(bitmap, 0xFF, bitmap_size);
    vm_fill_bitmap();
}

/*
 * Probe for usable memory
 */
static void
vm_probe(void)
{
    mementry_t *entry;

    for (size_t i = 0; i < MEM_ENTRY_COUNT; ++i) {
        entry = MEM_ENTRY(i);

        if (entry->type != MEM_USABLE) {
            total_mem += entry->length;
            reserved_mem += entry->length;
            continue;
        }

        total_mem += entry->length;
        free_mem += entry->length;

        if ((entry->base + entry->length) > highest_usable) {
            highest_usable = entry->base + entry->length;
        }
    }

    /* Print some stats */
    vm_printstat("memory installed", total_mem);
    vm_printstat("memory usable", free_mem);
    vm_printstat("memory reserved", reserved_mem);
    dtrace("usable top @ %p\n", highest_usable);

    /* Compute the bitmap size */
    bitmap_size = highest_usable / PAGESIZE;
    bitmap_size /= 8;
    dtrace("framedb len : %d bytes\n", bitmap_size);
    vm_alloc_bitmap();
}

/*
 * Lockless frame allocation routine
 */
static uintptr_t
__vm_phys_alloc(size_t count)
{
    size_t frames_found = 0;
    size_t max_index;
    ssize_t index_start = -1;
    uintptr_t start, end;

    max_index = highest_usable / PAGESIZE;
    for (size_t i = last_index; i < max_index; ++i) {
        if (!testbit(bitmap, i)) {
            if (index_start < 0)
                index_start = i;
            if ((++frames_found) >= count)
                break;

            continue;
        }

        index_start = -1;
    }

    if (index_start < 0) {
        return 0;
    }

    start = index_start * PAGESIZE;
    end = start + (count * PAGESIZE);
    bitmap_set_range(start, end, true);
    return start;
}

void
vm_phys_free(uintptr_t base, size_t count)
{
    uintptr_t end;

    base = ALIGN_DOWN(base, PAGESIZE);
    end = base + (count * PAGESIZE);

    mu_spinlock_acq(&bitmap_lock, 0);
    bitmap_set_range(base, end, false);
    mu_spinlock_rel(&bitmap_lock, 0);
}

uintptr_t
vm_phys_alloc(size_t count)
{
    uintptr_t base;

    mu_spinlock_acq(&bitmap_lock, 0);
    base = __vm_phys_alloc(count);
    if (base == 0) {
        last_index = 0;
        base = __vm_phys_alloc(count);
    }
    mu_spinlock_rel(&bitmap_lock, 0);
    return base;
}

void
vm_phys_init(void)
{
    memmap_resp = memmap_req.response;
    if (__unlikely(memmap_resp == NULL)) {
        panic("vm: unable to get memory map\n");
    }

    dtrace("checking memory resources...\n");
    vm_probe();
}
