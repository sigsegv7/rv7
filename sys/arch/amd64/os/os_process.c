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

#include <sys/errno.h>
#include <mu/process.h>
#include <mu/mmu.h>
#include <vm/phys.h>
#include <lib/string.h>

#define STACK_TOP 0xBFFFFFFF
#define GDT_KERNCODE 0x08
#define GDT_KERNDATA 0x10
#define GDT_USERCODE 0x18
#define GDT_USERDATA 0x20

int
mu_process_init(struct process *process, uintptr_t ip, int flags)
{
    struct trapframe *tf;
    struct pcb *pcb;
    uintptr_t stack_base;
    uint8_t cs, ds;
    int error;

    if (process == NULL) {
        return -EINVAL;
    }

    pcb = &process->pcb;
    tf = &pcb->tf;

    error = mu_pmap_forkvas(&pcb->vas);
    if (error < 0) {
        return error;
    }

    /* Allocate a new stack */
    stack_base = vm_phys_alloc(1);
    if (stack_base == 0) {
        vm_phys_free(pcb->vas.cr3, 1);
        return -ENOMEM;
    }

    /* Get the segment selectors based on mode */
    if (ISSET(flags, PROC_KERN)) {
        cs = GDT_KERNCODE;
        ds = GDT_KERNDATA;
    } else {
        cs = GDT_USERCODE | 3;
        ds = GDT_USERDATA | 3;
    }

    memset(tf, 0, sizeof(*tf));
    tf->rip = ip;
    tf->rflags = 0x202;
    tf->cs = cs;
    tf->ss = ds;

    /* Map out the stack */
    error = mu_pmap_map(
        &pcb->vas,
        stack_base,
        STACK_TOP,
        PROT_READ | PROT_WRITE,
        PAGESIZE_4K
    );

    if (error < 0) {
        vm_phys_free(stack_base, 1);
        vm_phys_free(pcb->vas.cr3, 1);
        return error;
    }

    tf->rsp = ALIGN_DOWN(STACK_TOP + (PAGESIZE - 1), 16);
    return 0;
}
