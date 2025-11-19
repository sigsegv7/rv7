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

#include <sys/atomic.h>
#include <sys/types.h>
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <acpi/acpi.h>
#include <os/trace.h>
#include <kern/panic.h>
#include <dev/clkdev/hpet.h>
#include <lib/string.h>
#include <md/lapic.h>
#include <md/msr.h>
#include <md/cpu.h>
#include <mu/cpu.h>
#include <os/process.h>
#include <os/sched.h>
#include <vm/vm.h>
#include <vm/phys.h>
#include <vm/kalloc.h>

#define MAX_CPUS 256

#define dtrace(fmt, ...) trace("mp: " fmt, ##__VA_ARGS__)

/*
 * The startup code is copied to the processor bring up area
 * to which it is executed in real mode. This area must fit
 * within a page and be no larger and no smaller.
 */
#define AP_BUA_LEN   0x1000                     /* Bring up area length in bytes */
#define AP_BUA_PADDR 0x8000                     /* Bring up area [physical] */
#define AP_BUA_VADDR PHYS_TO_VIRT(AP_BUA_PADDR) /* Bring up area [virtual] */

/* Bring up descriptor area */
#define AP_BUDA_PADDR 0x9000
#define AP_BUDA_VADDR PHYS_TO_VIRT(AP_BUDA_PADDR)

/*
 * A bring up descriptor area gives a processor everything
 * it needs to have to get up on its paws without doing it
 * from scratch.
 *
 * @cr3: Virtual address space to switch to
 * @rsp: Stack pointer to switch to
 * @lm_entry: Long mode entry trampoline
 *
 * XXX: Each field is to be 8 bytes for alignment purposes
 *
 * XXX: Do *not* reorder this as it is acessed from the trampoline
 *      located in apboot.asm
 */
struct __packed ap_buda {
    uint64_t cr3;               /* 0x00 */
    uint64_t rsp;               /* 0x08 */
    uint64_t lm_entry;          /* 0x10 */
    uint64_t is_booted;         /* 0x18 */
};

/*
 * Represents the bootstrap address space used
 * for bringing up the APs
 */
struct ap_bootspace {
    uintptr_t pml4;
    uintptr_t pml3;
    uintptr_t pml2;
    uintptr_t pml1;
};

/*
 * A temporary area to save the BSPs MTRRs so they
 * can be loaded into the APs
 */
struct mtrr_save {
    uintptr_t physbase[256];
    uintptr_t physmask[256];
} mtrr_save;

static struct cpu_info *cpu_list[MAX_CPUS];
static struct ap_bootspace bs;
static volatile size_t ap_sync = 0;
static uint32_t ap_count = 0;
static volatile uint32_t aps_up = 0;
__section(".trampoline") static char ap_code[4096];

static void
cpu_idle(void)
{
    for (;;) {
        __asmv("hlt");
    }
}

static void
cpu_mtrr_save(void)
{
    uint64_t mtrr_cap;
    uint64_t physbase, physmask;
    uint8_t mtrr_count;

    mtrr_cap = rdmsr(IA32_MTRR_CAP);
    mtrr_count = mtrr_cap & 0xFF;

    for (size_t i = 0; i < mtrr_count; ++i) {
        mtrr_save.physbase[i] = rdmsr(IA32_MTRR_PHYSBASE + (2 * i));
        mtrr_save.physmask[i] = rdmsr(IA32_MTRR_PHYSMASK + (2 * i));
    }
}

static void
cpu_mtrr_fetch(void)
{
    uint64_t mtrr_cap;
    uint64_t physbase, physmask;
    uint8_t mtrr_count;

    mtrr_cap = rdmsr(IA32_MTRR_CAP);
    mtrr_count = mtrr_cap & 0xFF;

    for (size_t i = 0; i < mtrr_count; ++i) {
        wrmsr(IA32_MTRR_PHYSBASE + (2 * i), mtrr_save.physbase[i]);
        wrmsr(IA32_MTRR_PHYSMASK + (2 * i), mtrr_save.physmask[i]);
    }
}

/*
 * Initialize the boot address space
 */
static int
cpu_init_bootspace(struct ap_bootspace *bs)
{
    uintptr_t old_pml4_pa, *old_pml4;
    uintptr_t *new_pml4;
    uintptr_t *pml3, *pml2, *pml1;

    if (bs == NULL) {
        return -EINVAL;
    }

    bs->pml4 = vm_phys_alloc(1);
    if (bs->pml4 == 0) {
        return -ENOMEM;
    }

    bs->pml3 = vm_phys_alloc(1);
    if (bs->pml3 == 0) {
        vm_phys_free(bs->pml4, 1);
        return -ENOMEM;
    }

    bs->pml2 = vm_phys_alloc(1);
    if (bs->pml2 == 0) {
        vm_phys_free(bs->pml4, 1);
        vm_phys_free(bs->pml3, 1);
        return -ENOMEM;
    }

    bs->pml1 = vm_phys_alloc(1);
    if (bs->pml1 == 0) {
        vm_phys_free(bs->pml4, 1);
        vm_phys_free(bs->pml3, 1);
        vm_phys_free(bs->pml2, 1);
        return -ENOMEM;
    }

    /* Fork our old one */
    __asmv("mov %%cr3, %0" : "=r" (old_pml4_pa) :: "memory");
    old_pml4 = PHYS_TO_VIRT(old_pml4_pa);
    new_pml4 = PHYS_TO_VIRT(bs->pml4);
    for (uintptr_t i = 0; i < 512; ++i) {
        new_pml4[i] = old_pml4[i];
    }

    pml3 = PHYS_TO_VIRT(bs->pml3);
    pml2 = PHYS_TO_VIRT(bs->pml2);
    pml1 = PHYS_TO_VIRT(bs->pml1);

    /*
     * Now we link the tables up and identity map the
     * first 2 megs
     */
    new_pml4[0] = bs->pml3 | 3;     /* P+RW */
    pml3[0] = bs->pml2     | 3;     /* P+RW */
    pml2[0] = bs->pml1     | 3;     /* P+RW */
    for (uint16_t i = 0; i < 256; ++i) {
        pml1[i] = (0x1000 * i) | 3; /* P+RW */
    }
    return 0;
}

static int
cpu_free_bootspace(struct ap_bootspace *bs)
{
    if (bs == NULL) {
        return -EINVAL;
    }

    vm_phys_free(bs->pml3, 1);
    vm_phys_free(bs->pml2, 1);
    vm_phys_free(bs->pml1, 1);
    memset(bs, 0, sizeof(*bs));
    return 0;
}

/*
 * Processor long-mode entrypoint
 */
static void
cpu_lm_entry(void)
{
    struct cpu_info *ci;

    /*
     * Put the processor in no cache fill mode so that we can safely
     * update MTRRs without worrying about the ground moving under
     * us...
     */
    __asmv(
        "mov %%cr0, %%rax\n\t"        /* CR0 -> RAX */
        "or $0x40000000, %%rax\n\t"   /* Set CR0.CD */
        "mov $0xDFFFFFFF, %%rbx\n\t"  /* ~(1 << 31) -> RBX */
        "and %%rbx, %%rax\n\t"        /* Unset CR0.NW */
        "mov %%rax, %%cr0\n\t"        /* Write it back */
        :
        :
        : "rax", "rbx"
    );

    /* Flush all caches */
    __asmv(
        "wbinvd\n\t"                /* Write-back and flush dcache */
        "mov %%cr3, %%rax\n\t"      /* CR3 -> RAX */
        "mov %%rax, %%cr3"          /* RAX -> CR3; flush TLB */
        :
        :
        : "memory", "rax"
    );

    cpu_mtrr_fetch();

    /*
     * Now we load all the MTRRs given to us by the BSP
     * before we re-enable normal caching operation
     */
    __asmv(
        "mov %%cr0, %%rax\n\t"          /* CR0 -> RAX */
        "mov $0xBFFFFFFF, %%rbx\n\t"    /* ~(1 << 30) -> RBX */
        "and %%rbx, %%rax\n\t"          /* Unset CR0.CD */
        "mov %%rax, %%cr0\n\t"          /* Write it back */
        :
        :
        : "rax", "rbx"
    );

    ci = kalloc(sizeof(*ci));
    if (ci == NULL) {
        panic("mp: could not allocate processor\n");
    }

    ci->id = aps_up + 1;
    cpu_loinit();
    cpu_conf(ci);

    atomic_inc_int(&aps_up);
    cpu_list[aps_up] = ci;

    cpu_idle();
    __builtin_unreachable();
}

static int
cpu_lapic_cb(struct apic_header *h, size_t arg)
{
    struct local_apic *lapic;
    struct cpu_info *self;
    struct mcb *mcb;
    struct lapic_ipi ipi;
    struct ap_buda *buda;
    uintptr_t stack;
    int error;

    self = cpu_self();
    if (self == NULL) {
        panic("mp: could not get self\n");
    }

    /* Skip ourselves */
    lapic = (struct local_apic *)h;
    if (lapic->apic_id == arg) {
        return -1;
    }

    /* If not online capable, continue */
    if (!ISSET(lapic->flags, 0x3)) {
        return -1;
    }

    mcb = &self->mcb;
    buda = (struct ap_buda *)AP_BUDA_VADDR;
    stack = vm_phys_alloc(1);
    if (stack == 0) {
        panic("mp: failed to allocate stack\n");
    }

    buda->rsp = (stack + (PAGESIZE - 1)) + KERN_BASE;
    buda->lm_entry = (uintptr_t)cpu_lm_entry;
    buda->cr3 = bs.pml4;

    /* Prepare the IPI packet */
    ipi.dest_id = lapic->apic_id;
    ipi.vector = 0;
    ipi.delmod = IPI_DELMOD_INIT;
    ipi.shorthand = IPI_SHAND_NONE;
    ipi.logical_dest = 0;
    error = lapic_send_ipi(mcb, &ipi);
    if (error < 0) {
        panic("mp: failed to send INIT IPI\n");
    }

    /* Give it 20ms then prep a STARTUP */
    hpet_msleep(20);
    ipi.delmod = IPI_DELMOD_STARTUP;
    ipi.vector = (AP_BUA_PADDR >> 12);

    /* The MPSpec says to send two */
    for (uint8_t i = 0; i < 2; ++i) {
        error = lapic_send_ipi(mcb, &ipi);
        if (error != 0) {
            panic("mp: failed to send STARTUP IPI\n");
        }
        hpet_msleep(2);
    }

    /* Wait until AP is booted */
    while (!buda->is_booted);
    buda->is_booted = 0;

    /* Don't overflow */
    if ((++ap_count) >= MAX_CPUS - 1) {
        return 0;
    }

    return -1;  /* Keep going */
}

static void
cpu_start_idle(void)
{
    struct process *p;
    int error;

    for (size_t i = 0; i < ap_count; ++i) {
        p = kalloc(sizeof(*p));
        if (p == NULL) {
            panic("mp: could not allocate idle thread\n");
        }

        /* Initialize the process */
        error = process_init(p, (uintptr_t)cpu_idle, PROC_KERN);
        if (error < 0) {
            panic("mp: could not initialize process\n");
        }

        sched_enqueue_proc(p);
    }
}

struct cpu_info *
cpu_get(uint32_t index)
{
    if (index >= aps_up + 1) {
        return NULL;
    }

    return cpu_list[index];
}

size_t
cpu_count(void)
{
    return ap_count + 1;
}

void
cpu_start_aps(struct cpu_info *ci)
{
    struct cpu_info *self;
    struct mcb *mcb;
    size_t bua_len;
    uint8_t *bua;

    if (ci == NULL) {
        return;
    }

    self = cpu_self();
    if (self == NULL) {
        panic("mp: could not get current processor\n");
    }

    cpu_list[0] = ci;

    /* Initialize the bootspace */
    cpu_init_bootspace(&bs);
    cpu_mtrr_save();

    /* Copy the bring up code to the BUA */
    bua = AP_BUA_VADDR;
    memcpy(bua, ap_code, AP_BUA_LEN);

    /* Start up the APs */
    mcb = &self->mcb;
    dtrace("bringing up application processors...\n");
    acpi_read_madt(
        APIC_TYPE_LOCAL_APIC,
        cpu_lapic_cb,
        lapic_read_id(mcb)
    );

    /* Wait for all processors to be up */
    while (aps_up < ap_count) {
        __asmv("rep; nop");
    }

    if (aps_up == 0) {
        dtrace("cpu only has a single core\n");
    } else {
        dtrace("%d processor(s) up\n", aps_up);
    }

    cpu_start_idle();
}
