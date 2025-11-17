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
#include <sys/cdefs.h>
#include <sys/errno.h>
#include <os/mmio.h>
#include <os/trace.h>
#include <kern/panic.h>
#include <lib/stdbool.h>
#include <vm/vm.h>
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <dev/clkdev/hpet.h>

#define dtrace(fmt, ...) trace("hpet: " fmt, ##__VA_ARGS__)

/* HPET register offsets */
#define HPET_GCAP_ID    0x00    /* Global capabilities and ID */
#define HPET_GCONF      0x10    /* General configuration register */
#define HPET_COUNTER0   0xF0    /* Main counter register */

#define CAP_REV_ID(caps)        (caps & 0xFF)
#define CAP_NUM_TIM(caps)       (caps >> 8) & 0x1F
#define CAP_CLK_PERIOD(caps)    (caps >> 32)

static volatile uint64_t *hpet_base;
static bool hpet_enabled = false;

/*
 * Read a 64-bit unsigned value from an HPET
 * register
 */
static inline uint64_t
hpet_readq(uint16_t reg)
{
    uint64_t *base;

    base = PTR_OFFSET(hpet_base, reg);
    return mmio_read64(base);
}

/*
 * Write a 64-bit unsigned value to an HPET
 * register
 */
static inline void
hpet_writeq(uint16_t reg, uint64_t val)
{
    uint64_t *base;

    base = PTR_OFFSET(hpet_base, reg);
    mmio_write32(base, val);
}

static void
hpet_sleep(uint64_t n, uint64_t units)
{
    uint64_t period, caps;
    uint64_t counter;
    uint64_t ticks;

    if (!hpet_enabled) {
        return;
    }

    caps = hpet_readq(HPET_GCAP_ID);
    period = CAP_CLK_PERIOD(caps);
    counter = hpet_readq(HPET_COUNTER0);
    ticks = counter + (n * (units / period));

    while (hpet_readq(HPET_COUNTER0) < ticks);
}

void
hpet_msleep(size_t ms)
{
    hpet_sleep(ms, 1000000000000);
}

int
hpet_init(void)
{
    struct acpi_hpet *hpet;
    struct acpi_gas *gas;
    uint64_t gcap, gconf, counter;
    uint32_t clk_period;
    uint8_t rev_id;
    uint8_t num_timer;

    hpet = acpi_query("HPET");
    if (hpet == NULL) {
        return -ENODEV;
    }

    gas = &hpet->gas;
    hpet_base = PHYS_TO_VIRT(gas->address);
    gcap = hpet_readq(HPET_GCAP_ID);

    /* Verify its values */
    clk_period = CAP_CLK_PERIOD(gcap);
    num_timer = CAP_NUM_TIM(gcap);
    rev_id = gcap & 0xFF;
    if (rev_id == 0) {
        dtrace("bad hpet revision, cannot be zero\n");
        panic("hpet: system self test failure\n");
    }

    /* Verify the clock period */
    if (clk_period == 0 || clk_period > 0x05F5E100) {
        dtrace("bad hpet clock period\n");
        panic("hpet: system self test failure\n");
    }

    dtrace("rev=%d, num_timer=%d\n", rev_id, num_timer);
    dtrace("clk_period=%d\n", clk_period);

    hpet_writeq(HPET_COUNTER0, 0);      /* Clear the counter */
    hpet_writeq(HPET_GCONF, 1);         /* Enable timer */

    hpet_enabled = true;
    dtrace("OK\n");
    return 0;
}
