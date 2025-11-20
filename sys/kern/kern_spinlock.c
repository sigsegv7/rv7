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
#include <mu/spinlock.h>
#include <kern/spinlock.h>
#include <lib/string.h>

int
spinlock_init(const char *name, struct spinlock *lock)
{
    size_t name_len;

    if (name == NULL || lock == NULL) {
        return -EINVAL;
    }

    name_len = strlen(name);
    if (name_len >= SPINLOCK_NAMELEN - 1) {
        return -ENAMETOOLONG;
    }

    memcpy(lock->name, name, name_len);
    lock->name[name_len] = '\0';
    lock->lock = 0;
    return 0;
}

void
spinlock_acquire(struct spinlock *lock, bool irqclr)
{
    int flags = 0;

    if (irqclr) {
        flags |= SPINLOCK_INTTOG;
    }

    mu_spinlock_acq(&lock->lock, flags);
}

void
spinlock_release(struct spinlock *lock, bool irqset)
{
    int flags = 0;

    if (irqset) {
        flags |= SPINLOCK_INTTOG;
    }

    mu_spinlock_rel(&lock->lock, flags);
}
