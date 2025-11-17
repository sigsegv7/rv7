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

#include <sys/types.h>
#include <sys/cdefs.h>
#include <acpi/acpi.h>
#include <os/trace.h>
#include <kern/panic.h>
#include <dev/clkdev/hpet.h>
#include <lib/string.h>
#include <md/lapic.h>
#include <mu/cpu.h>
#include <vm/vm.h>

#define dtrace(fmt, ...) trace("mp: " fmt, ##__VA_ARGS__)

/*
 * The startup code is copied to the processor bring up area
 * to which it is executed in real mode. This area must fit
 * within a page and be no larger and no smaller.
 */
#define AP_BUA_LEN   0x1000                     /* Bring up area length in bytes */
#define AP_BUA_PADDR 0x1000                     /* Bring up area [physical] */
#define AP_BUA_VADDR PHYS_TO_VIRT(AP_BUA_PADDR) /* Bring up area [virtual] */

__section(".trampoline") static char ap_code[4096];

static int
cpu_lapic_cb(struct apic_header *h, size_t arg)
{
    struct local_apic *lapic;
    struct cpu_info *self;
    struct mcb *mcb;
    struct lapic_ipi ipi;
    int error;

    self = cpu_self();
    if (self == NULL) {
        panic("mp: could not get self\n");
    }

    mcb = &self->mcb;

    /* Skip ourselves */
    lapic = (struct local_apic *)h;
    if (lapic->apic_id == arg) {
        return -1;
    }

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

    dtrace("bootstrapping lapic %d\n", lapic->apic_id);
    return -1;  /* Keep going */
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
}
