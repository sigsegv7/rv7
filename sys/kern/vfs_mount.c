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
#include <sys/errno.h>
#include <sys/param.h>
#include <kern/spinlock.h>
#include <kern/panic.h>
#include <kern/mount.h>
#include <kern/vfs.h>
#include <kern/namei.h>
#include <lib/stdbool.h>
#include <lib/string.h>
#include <vm/kalloc.h>

/*
 * We can't quite distribute this lock in a sane way without
 * complicating things significantly and thus the practicality
 * of such is questionable. Unfortunately, having a global lock
 * comes with some performance penalties due to contention. At
 * the very least, we can prevent it from bouncing around between
 * caches on multicore systems by aligning it to a cacheline boundary.
 */
__cacheline_aligned
static struct spinlock mount_lock;

/* Mount list */
static TAILQ_HEAD(, mount) mountlist;
static bool is_mountlist_init = false;

/*
 * Initialize the mountlist
 */
static void
mountlist_init(void)
{
    if (is_mountlist_init) {
        return;
    }

    TAILQ_INIT(&mountlist);
    if (spinlock_init("mount", &mount_lock) != 0) {
        panic("mount: failed to initialize mountlist\n");
    }
    is_mountlist_init = true;
}

static int
mount_by_fsname(struct mount_args *margs, struct mount **mp_res)
{
    struct mount *mp;
    struct fs_info *fip;
    struct vfsops *vfsops;
    int error;

    if (margs == NULL || mp_res == NULL) {
        return -EINVAL;
    }

    error = vfs_byname(margs->fstype, &fip);
    if (error != 0) {
        return error;
    }

    vfsops = fip->vfsops;
    if (vfsops->mount == NULL) {
        return -ENOTSUP;
    }

    mp = kalloc(sizeof(*mp));
    if (mp == NULL) {
        return -ENOMEM;
    }

    mp->fip = fip;
    error = vfsops->mount(fip, mp);
    if (error < 0) {
        kfree(mp);
        return error;
    }

    *mp_res = mp;
}

int
mount(struct mount_args *margs)
{
    struct mount *mp;
    struct nameidata ndp;
    int error;

    if (margs == NULL) {
        return -EINVAL;
    }

    if (margs->target == NULL || margs->fstype == NULL) {
        return -EINVAL;
    }

    /* Initialize the mountlist if needed */
    if (!is_mountlist_init) {
        mountlist_init();
    }

    error = mount_by_fsname(margs, &mp);
    if (error != 0) {
        return error;
    }

    spinlock_acquire(&mount_lock, true);
    TAILQ_INSERT_TAIL(&mountlist, mp, link);
    spinlock_release(&mount_lock, true);
    return 0;
}

/*
 * XXX: Currently we are looking up entries by the fstype.
 *      This must be updated to actual name-based lookups
 *      in future revisions of rv7
 */
int
mount_lookup(const char *name, struct mount **mres)
{
    struct fs_info *fip;
    struct mount *iter, *mount = NULL;

    if (name == NULL || mres == NULL) {
        return -EINVAL;
    }

    spinlock_acquire(&mount_lock, true);
    TAILQ_FOREACH(iter, &mountlist, link) {
        fip = iter->fip;
        if (__likely(*name != *fip->name)) {
            continue;
        }

        if (strcmp(fip->name, name) == 0) {
            mount = iter;
            break;
        }
    }

    spinlock_release(&mount_lock, true);
    if (mount == NULL) {
        return -ENOENT;
    }

    *mres = mount;
    return 0;
}
