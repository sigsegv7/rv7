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

#ifndef _OS_MOUNT_H_
#define _OS_MOUNT_H_ 1

#include <sys/types.h>
#include <sys/queue.h>
#include <kern/vnode.h>

/* Filesystem names */
#define MOUNT_TMPFS "tmpfs"

struct fs_info;

/*
 * Represents arguments passed to mount()
 */
struct mount_args {
    const char *source;
    const char *target;
    const char *fstype;
    uint32_t flags;
    void *data;
};

/*
 * Represents VFS operations that can be performed
 * on filesystem objects
 */
struct vfsops {
    int(*mount)(struct fs_info *fip, void *data);
};

/*
 * Represents filesystem operation
 *
 * @name: Name of filesystem
 * @vfsops: Represents operations that can be performed
 * @is_mounted: Set if filesystem is mounted
 */
struct fs_info {
    char *name;
    struct vfsops *vfsops;
    uint8_t is_mounted : 1;
};

/*
 * Represents a mountpoint
 *
 * @fip: Target filesystem interface
 * @vp: Vnode pointer length
 * @link: Connects mountpoints
 */
struct mount {
    struct fs_info *fip;
    struct vnode *vp;
    TAILQ_ENTRY(mount) link;
};

/*
 * Mount a filesystem and make it visible for access
 *
 * @margs: Mount arguments
 *
 * Returns zero on success
 */
int mount(struct mount_args *margs);

/*
 * Lookup an entry within the mount table
 *
 * @name: Name to lookup
 * @mres: Result pointer is written here
 *
 * Returns zero on success
 */
int mount_lookup(const char *name, struct mount **mres);

#endif  /* !_OS_MOUNT_H_ */
