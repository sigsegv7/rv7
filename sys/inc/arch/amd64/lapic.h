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

#ifndef _MACHINE_LAPIC_H_
#define _MACHINE_LAPIC_H_ 1

#include <sys/types.h>
#include <lib/stdbool.h>
#include <mu/cpu.h>

#define LAPIC_TMR_VEC 0x81

/*
 * Represents possible values of the destination shorthand
 * field for inter-processor interrupts
 *
 * @IPI_SHAND_NONE: No shorthand
 * @IPI_SHAND_SELF: Address self
 * @IPI_SHAND_AIS:  All including self
 * @IPIS_HAND_AXS:  All excluding self
 */
typedef enum {
    IPI_SHAND_NONE,
    IPI_SHAND_SELF,
    IPI_SHAND_AIS,
    IPI_SHAND_AXS
} ipi_shand_t;

/*
 * Represents possible values of the delivery mode
 * field for inter-processor interrupts
 *
 * @IPI_DELMOD_FIXED:       Deliver vector to processor target(s)
 * @IPI_DELMOD_LOPRI:       Lowest priority [SDM advises against its use]
 * @IPI_DELMOD_SMI:         Reserved
 * @IPI_DELMOD_RESERVED:    Reserved
 * @IPI_DELMOD_NMI:         Deliver a non-maskable interrupt [vector unused]
 * @IPI_DELMOD_INIT:        Park a processor to the reset vector
 * @IPI_DELMOD_STARTUP:     Bring a processor up into real mode
 */
typedef enum {
    IPI_DELMOD_FIXED,
    IPI_DELMOD_LOPRI,
    IPI_DELMOD_SMI,
    IPI_DELMOD_RESERVED,
    IPI_DELMOD_NMI,
    IPI_DELMOD_INIT,
    IPI_DELMOD_STARTUP
} ipi_delmod_t;

/*
 * Represents an inter-processor interrupt descriptor
 *
 * @dest_id: APIC ID of destination processor
 * @vector: Interrupt vector to send
 * @delmod: Delivery mode
 * @shorthand: Destination shorthand
 * @logical_dest: Set if destination mode should be logical
 */
struct lapic_ipi {
    uint64_t dest_id;
    uint8_t vector;
    ipi_delmod_t delmod : 3;
    ipi_shand_t shorthand : 2;
    uint8_t logical_dest : 1;
};

/*
 * Put the local APIC timer in one shot mode and fire it
 * off
 */
void lapic_oneshot_usec(struct mcb *mcb, size_t usec);

/*
 * Read the current local APIC id
 */
uint32_t lapic_read_id(struct mcb *mcb);

/*
 * Send an interprocessor interrupt
 *
 * Returns zero on success
 */
int lapic_send_ipi(struct mcb *mcb, struct lapic_ipi *ipi);

/*
 * Initialize the Local APIC on-board the
 * processor for the current core
 */
void lapic_init(void);

#endif  /* !_MACHINE_LAPIC_H_ */
