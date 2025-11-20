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
#include <sys/errno.h>
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
#include <md/idt.h>

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
#define LAPIC_REG_EOI       0x00B0
#define LAPIC_REG_ICRLO     0x0300U      /* Interrupt Command Low Register */
#define LAPIC_REG_ICRHI     0x0310U      /* Interrupt Command High Register */

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

extern void lapic_tmr_isr(void);
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
 * Used to serialize inter-processor interrupts when
 * operating in xAPIC mode
 */
static void
lapic_ipi_poll(struct mcb *mcb)
{
    uint32_t icr_lo;

    do {
        icr_lo = lapic_read(mcb, LAPIC_REG_ICRLO);
        __asmv("pause");
    } while (ISSET(icr_lo, BIT(12)));
}

int
lapic_send_ipi(struct mcb *mcb, struct lapic_ipi *ipi)
{
    const uint16_t X2APIC_SELF = 0x083F;
    uint64_t icr_lo, icr_hi;

    if (ipi == NULL) {
        return -EINVAL;
    }

    /*
     * Section 2.4.5 of the x2APIC spec states that x2APICs
     * have a special self IPI register that can be used
     * instead of just sending out an interrupt on the system
     * bus (or APIC bus) and having it loop all the way back.
     * This interface is an optimized shorthand path
     */
    if (ipi->shorthand == IPI_SHAND_SELF && mcb->has_x2apic) {
        wrmsr(X2APIC_SELF, ipi->vector);
        return 0;
    }

    /* Clamp to 8 bits if xAPIC */
    if (!mcb->has_x2apic) {
        ipi->dest_id &= 0xFF;
    }

    /* Encode the ICR high bits */
    if (!mcb->has_x2apic) {
        icr_hi = lapic_read(mcb, LAPIC_REG_ICRHI);
        icr_hi &= ~(0xFFUL << 56);
        icr_hi |= (ipi->dest_id << 56);
        lapic_write(mcb, LAPIC_REG_ICRHI, icr_hi);
    } else {
        icr_lo = lapic_read(mcb, LAPIC_REG_ICRLO);
        icr_lo &= ~0xFFFFFFFFFFFFFFFF;
        icr_lo |= (ipi->dest_id << 32);
    }

    /*
     * Encode the low bits of the ICR. If we are x2APIC,
     * there is no need to read the low DWORD again as
     * reads load the entire register.
     */
    if (!mcb->has_x2apic) {
        icr_lo = lapic_read(mcb, LAPIC_REG_ICRLO);
    }
    icr_lo |= ipi->vector;
    icr_lo |= (ipi->delmod << 8);
    icr_lo |= (ipi->logical_dest << 11);
    icr_lo |= (ipi->shorthand << 18);
    lapic_write(mcb, LAPIC_REG_ICRLO, icr_lo);

    /* Poll if in x2APIC mode */
    if (!mcb->has_x2apic) {
        lapic_ipi_poll(mcb);
    }

    /*
     * The x2APIC queues IPIs up and thus polling is not needed as
     * the delivery status bit is not used nor is it present.
     */
    return 0;
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
    lapic_write(mcb, LAPIC_REG_SVR, svr | 0xFF);
}

static void
lapic_timer_oneshot(struct mcb *mcb, size_t count)
{
    if (mcb == NULL) {
        return;
    }

    lapic_tmr_enable(mcb, LAPIC_TMR_ONESHOT);
    lapic_write(mcb, LAPIC_REG_TICR, count);
}

uint32_t
lapic_read_id(struct mcb *mcb)
{
    uint32_t id;

    if (!mcb->has_x2apic) {
        return (lapic_read(mcb, LAPIC_REG_ID) >> 24) & 0xF;
    } else {
        return lapic_read(mcb, LAPIC_REG_ID);
    }
}

void
lapic_oneshot_usec(struct mcb *mcb, size_t usec)
{
    if (mcb == NULL) {
        return;
    }

    lapic_timer_oneshot(mcb, mcb->lapic_tmr_freq / 1000000);
}

void
lapic_eoi(struct mcb *mcb)
{
    lapic_write(mcb, LAPIC_REG_EOI, 0);
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
    idt_set_gate(LAPIC_TMR_VEC, INT_GATE, (uintptr_t)lapic_tmr_isr, 0);
}
