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

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/types.h>
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <kern/panic.h>
#include <os/mmio.h>
#include <os/trace.h>
#include <vm/vm.h>
#include <lib/stdbool.h>
#include <mu/cpu.h>
#include <md/lapic.h>
#include <md/i8254.h>
#include <md/cpuid.h>
#include <md/msr.h>

#define dtrace(fmt, ...) trace("lapic: " fmt, ##__VA_ARGS__)

/* IA32_APIC_BASE MSR bits */
#define LAPIC_GLOBAL_EN BIT(11)
#define LAPIC_X2APIC_EN BIT(10)

/*
 * Register offsets
 *
 * See section 2.3.2 of the x2APIC specification
 */
#define LAPIC_REG_ID        0x0020       /* ID register */
#define LAPIC_REG_SVR       0x00F0       /* Spurious vector register */
#define LAPIC_REG_TICR      0x0380       /* Timer initial counter register */
#define LAPIC_REG_TCCR      0x0390       /* Timer current counter register */
#define LAPIC_REG_TDCR      0x03E0       /* Timer divide configuration register */
#define LAPIC_REG_LVTTMR    0x0320       /* LVT timer entry */

/* LAPIC SVR bits */
#define LAPIC_SVR_EBS       BIT(12)     /* EOI-broadcast supression */
#define LAPIC_SVR_FPC       BIT(9)      /* Focus processor checking */
#define LAPIC_SVR_APIC_EN   BIT(8)      /* Software-enable Local APIC */

/* Local vector table */
#define LVT_MASK    BIT(16)

/* Samples to take */
#define LAPIC_TMR_SAMPLES 0xFFFF

/* Timer modes */
#define LAPIC_TMR_ONESHOT   0x00
#define LAPIC_TMR_PERIODIC  0x01

/* Accessed via RDMSR/WRMSR */
#define X2APIC_MSR_BASE 0x00000800

static struct acpi_madt *madt;

/*
 * Read from a local APIC register
 */
static uint64_t
lapic_read(struct mcb *mcb, uint32_t reg)
{
    uint32_t *xapic_base;

    if (mcb->has_x2apic) {
        return rdmsr(X2APIC_MSR_BASE + (reg >> 4));
    }

    xapic_base = PTR_OFFSET(mcb->xapic_io, reg);
    return mmio_read32(xapic_base);
}

/*
 * Write to a Local APIC register
 */
static void
lapic_write(struct mcb *mcb, uint16_t reg, uint64_t val)
{
    uint32_t *xapic_base;

    if (mcb->has_x2apic) {
        wrmsr(X2APIC_MSR_BASE + (reg >> 4), val);
        return;
    }

    xapic_base = PTR_OFFSET(mcb->xapic_io, reg);
    return mmio_write32(xapic_base, val);
}

/*
 * Returns true if the processor has a Local APIC
 * unit
 */
static inline bool
lapic_is_present(void)
{
    uint32_t edx, unused;

    CPUID(0x01, unused, unused, unused, edx);
    return ISSET(edx, BIT(9)) != 0;
}

/*
 * Returns true if the Local APIC supports x2APIC
 * mode
 */
static inline bool
lapic_has_x2apic(void)
{
    uint32_t ecx, unused;

    CPUID(0x01, unused, unused, ecx, unused);
    return ISSET(ecx, BIT(21)) != 0;
}

/*
 * Configure the Local APIC timer with a predefined
 * vector
 *
 * XXX: See LAPIC_TMR_* for mode definitions
 */
static void
lapic_tmr_enable(struct mcb *mcb, uint8_t mode)
{
    uint32_t lvt_tmr;

    lvt_tmr = lapic_read(mcb, LAPIC_REG_LVTTMR);

    /* Clear out stale values */
    lvt_tmr &= ~0xFF;               /* Clear vector */
    lvt_tmr &= ~(0x3 << 17);        /* Clear mode */
    lvt_tmr &= ~LVT_MASK;           /* Clear mask */

    /* Set them to our own values */
    lvt_tmr |= (mode & 0x3) << 17;  /* Set mode */
    lvt_tmr |= LAPIC_TMR_VEC;       /* Set vector */
    lapic_write(mcb, LAPIC_REG_LVTTMR, lvt_tmr);
}

/*
 * Disable the Local APIC timer
 */
static void
lapic_tmr_disable(struct mcb *mcb)
{
    uint32_t lvt_tmr;

    lvt_tmr = lapic_read(mcb, LAPIC_REG_LVTTMR);
    lvt_tmr |= LVT_MASK;        /* Set mask */
    lvt_tmr &= ~0xFF;           /* Clear vector; just in case */
    lapic_write(mcb, LAPIC_REG_LVTTMR, lvt_tmr);
}

/*
 * Calibrate the Local APIC timer
 */
static size_t
lapic_tmr_clbr(struct mcb *mcb)
{
    size_t freq;
    uint16_t ticks_total;
    uint16_t ticks_begin, ticks_end;
    uint32_t tmp;

    /*
     * The divide configuration register basically slices
     * up the base clock (typically TSC core crystal clock or
     * bus clock) which makes the counter decrement slower with
     * respect to higher values
     */
    tmp = lapic_read(mcb, LAPIC_REG_TDCR);
    tmp &= ~BIT(3);     /* Clear upper */
    tmp &= ~0x3;        /* Clear lower */
    tmp |= 0x01;        /* DCR=0b001[divide by 4] */
    lapic_write(mcb, LAPIC_REG_TDCR, tmp);

    lapic_tmr_disable(mcb);
    i8254_set_count(LAPIC_TMR_SAMPLES);

    /* Take some samples of the counter */
    ticks_begin = i8254_get_count();
    lapic_write(mcb, LAPIC_REG_TICR, LAPIC_TMR_SAMPLES);
    while (lapic_read(mcb, LAPIC_REG_TCCR) != 0);

    /* Compute the deviation [total ticks] */
    lapic_tmr_disable(mcb);
    ticks_end = i8254_get_count();
    ticks_total = ticks_end - ticks_begin;

    /* Compute the frequency */
    freq = (LAPIC_TMR_SAMPLES / ticks_total) * I8254_DIVIDEND;
    return freq;
}

/*
 * Enable the Local APIC unit
 */
static void
lapic_enable(struct mcb *mcb)
{
    uint64_t apic_base;
    uint64_t svr;
    bool has_x2apic = false;

    if (mcb == NULL) {
        return;
    }

    /* Hardware enable the Local APIC unit */
    has_x2apic = lapic_has_x2apic();
    apic_base = rdmsr(IA32_APIC_BASE);
    apic_base |= LAPIC_GLOBAL_EN;
    apic_base |= (has_x2apic << 10);
    wrmsr(IA32_APIC_BASE, apic_base);

    mcb->has_x2apic = has_x2apic;
    dtrace(
        "lapic enabled in %s mode\n",
        has_x2apic ?
        "x2apic"   :
        "xapic"
    );

    /* Software enable the Local APIC unit */
    svr = lapic_read(mcb, LAPIC_REG_SVR);
    svr |= LAPIC_SVR_APIC_EN;
    lapic_write(mcb, LAPIC_REG_SVR, svr);
}

void
lapic_init(void)
{
    struct cpu_info *ci;
    struct mcb *mcb;

    if (!lapic_is_present()) {
        panic("lapic: cpu lacks on-board local apic\n");
    }

    /* We need the MADT table */
    madt = acpi_query("APIC");
    if (__unlikely(madt == NULL)) {
        panic("lapic: acpi madt table not present\n");
    }

    ci = cpu_self();
    if (ci == NULL) {
        panic("lapic: could not get current processor\n");
    }

    mcb = &ci->mcb;
    mcb->xapic_io = PHYS_TO_VIRT((uintptr_t)madt->lapic_addr);

    lapic_enable(mcb);
    mcb->lapic_tmr_freq = lapic_tmr_clbr(mcb);
}
