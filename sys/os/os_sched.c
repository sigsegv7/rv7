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

#include <sys/queue.h>
#include <os/sched.h>
#include <os/trace.h>
#include <mu/cpu.h>

struct cpu_info *
sched_enqueue_proc(struct process *proc)
{
    struct cpu_info *core;
    size_t ncpu, i;

    if (proc == NULL) {
        return NULL;
    }

    if (proc->affinity >= 0) {
        if ((core = cpu_get(proc->affinity)) != NULL)
            return core;
    }

    /*
     * Here, we derive the processor index by using the lower
     * byte MOD the number of cores, this works best and most
     * evenly when the PID assignment increments monotonically
     * though could potentially work with random assignment, just
     * more sporadic.
     */
    ncpu = cpu_count();
    i = (proc->pid & 0xFF) % ncpu;
    while ((core = cpu_get(i++)) == NULL) {
        if (i > ncpu) {
            i = 0;
        }
    }
    TAILQ_INSERT_TAIL(&core->pqueue, proc, link);
    return core;
}

struct process *
sched_dequeue_proc(void)
{
    struct cpu_info *core;
    struct process *proc;

    core = cpu_self();
    if (core == NULL) {
        return NULL;
    }

    proc = TAILQ_FIRST(&core->pqueue);
    if (proc != NULL) {
        TAILQ_REMOVE(&core->pqueue, proc, link);
    }
    return proc;
}
