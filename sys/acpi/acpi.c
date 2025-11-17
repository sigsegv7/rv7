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

#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <kern/panic.h>
#include <os/trace.h>
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <dev/clkdev/hpet.h>
#include <vm/vm.h>
#include <lib/limine.h>
#include <lib/string.h>

#define dtrace(fmt, ...) trace("acpi: " fmt, ##__VA_ARGS__)

static struct acpi_rsdp *rsdp = NULL;
static struct acpi_root_sdt *sdt;
static size_t sdt_entries = 0;

/*
 * We could just search for this ourselves but as we
 * boot both UEFI and BIOS, ask the loader to ensure
 * we can actually obtain it
 */
static struct limine_rsdp_response *rsdp_resp;
static volatile struct limine_rsdp_request rsdp_req = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

/*
 * Print the OEMID string found within the RSDP
 */
static void
acpi_oemid_print(struct acpi_rsdp *rsdp)
{
    uint8_t rev = rsdp->revision;

    /*
     * Some emulators might not bother to set the revision
     * in which cases it would most likely be ACPI 1.0, so
     * we'll just bump that up for the sake of clarity if
     * this happens.
     */
    if (rev == 0) {
        ++rev;
    }

    dtrace("detected ACPI %d.0 by ", rev);
    for (int i = 0; i < OEMID_SIZE; ++i) {
        trace("%c", rsdp->oemid[i]);
    }
    trace("\n");
}

/*
 * Verify the checksum of an ACPI header and return
 * zero if valid
 */
static int
acpi_checksum(struct acpi_header *hdr)
{
    uint8_t csum = 0;

    for (int i = 0; i < hdr->length; ++i) {
        csum += ((char *)hdr)[i];
    }

    return (csum == 0) ? 0 : -1;
}

void *
acpi_query(const char *s)
{
    struct acpi_header *hdr;

    for (int i = 0; i < sdt_entries; ++i) {
        hdr = PHYS_TO_VIRT((uintptr_t)sdt->tables[i]);
        if (memcmp(hdr->signature, s, 4) == 0) {
            return acpi_checksum(hdr) == 0 ? (void *)hdr : NULL;
        }
    }

    return NULL;
}

int
acpi_read_madt(uint32_t type, int(*cb)(struct apic_header *, size_t), size_t arg)
{
    static struct acpi_madt *madt;
    struct apic_header *hdr;
    uint8_t *cur, *end;
    int retval;

    if (cb == NULL) {
        return -EINVAL;
    }

    if (madt == NULL) {
        madt = acpi_query("APIC");
    }

    if (madt == NULL) {
        panic("acpi: could not read MADT\n");
    }

    cur = (uint8_t *)(madt + 1);
    end = (uint8_t *)madt + madt->hdr.length;

    while (cur < end) {
        hdr = (void *)cur;

        if (hdr->type == type) {
            retval = cb(hdr, arg);
        }

        if (retval >= 0) {
            return retval;
        }

        cur += hdr->length;
    }

    return -1;
}

void
acpi_init(void)
{
    int error;

    rsdp_resp = rsdp_req.response;
    if (__unlikely(rsdp_resp == NULL)) {
        panic("acpi: could not obtain rsdp\n");
    }

    rsdp = rsdp_resp->address;
    acpi_oemid_print(rsdp);

    /* XSDT if rev > 1.0 */
    if (rsdp->revision > 1) {
        sdt = PHYS_TO_VIRT(rsdp->xsdt_addr);
        sdt_entries = (sdt->hdr.length - sizeof(sdt->hdr)) / 8;
        dtrace("using xsdt as root sdt\n");
    } else if (rsdp->revision < 2) {
        sdt = PHYS_TO_VIRT(rsdp->rsdt_addr);
        sdt_entries = (sdt->hdr.length - sizeof(sdt->hdr)) / 4;
        dtrace("using rsdt as root sdt\n");
    }

    dtrace("verifying sdt integrity...\n");
    if (acpi_checksum(&sdt->hdr) != 0) {
        panic("acpi: bad checksum for sdt\n");
    }

    dtrace("OK\n");
    error = hpet_init();
#if defined(__x86_64__)
    if (error != 0) {
        panic("acpi: require HPET on RV7/x86_64\n");
    }
#endif  /* __x86_64__ */
}
