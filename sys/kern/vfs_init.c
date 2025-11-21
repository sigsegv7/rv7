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
#include <sys/cdefs.h>
#include <lib/string.h>
#include <fs/tmpfs.h>
#include <kern/vfs.h>
#include <kern/mount.h>
#include <os/trace.h>

#define dtrace(fmt, ...) trace("vfs: " fmt, ##__VA_ARGS__)

static uint16_t fs_count = 0;
static struct fs_info fs_list[] = {
    { MOUNT_TMPFS, &g_tmpfs_ops }
};

int
vfs_byname(const char *name, struct fs_info **res)
{
    if (name == NULL || res == NULL) {
        return -ENOMEM;
    }

    if (fs_count == 0) {
        fs_count = NELEM(fs_list);
    }

    for (uint16_t i = 0; i < fs_count; ++i) {
        if (__likely(*name != *fs_list[i].name)) {
            continue;
        }

        if (strcmp(name, fs_list[i].name) == 0) {
            *res = &fs_list[i];
            return 0;
        }
    }

    return -ENOENT;
}

void
vfs_init(void)
{
    struct fs_info *fip;
    struct vfsops *ops;
    int error;

    fs_count = NELEM(fs_list);
    for (uint16_t i = 0; i < fs_count; ++i) {
        fip = &fs_list[i];
        ops = fip->vfsops;

        /* We rely on the name */
        if (__unlikely(fip->name == NULL)) {
            continue;
        }

        /* Initialize the filesystem */
        if (ops->init != NULL) {
            error = ops->init(fip);
        }

        if (error != 0) {
            dtrace("failed to init %s\n", fip->name);
            continue;
        }

        dtrace("initialized %s\n", fip->name);
    }
}
