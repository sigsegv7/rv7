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
#include <sys/limits.h>
#include <kern/mount.h>
#include <kern/namei.h>

int
namei(struct nameidata *ndp)
{
    struct vops *vops;
    struct mount *mpoint = NULL;
    struct vnode *vp = NULL;
    const char *p;
    int error;
    char namebuf[NAME_MAX];
    size_t namebuf_idx = 0;

    if (ndp == NULL) {
        return -EINVAL;
    }

    if (ndp->pathname == NULL || ndp->vp_res == NULL) {
        return -EINVAL;
    }

    /* Iterate through the path */
    p = ndp->pathname;
    while (*p != '\0') {
        /* Skip leading slashes */
        while (*p != '\0' && *p == '/') {
            *p++;
        }

        if (*p == '\0') {
            break;
        }

        /* Fill the name buffer */
        while (*p != '\0' && *p != '/') {
            namebuf[namebuf_idx++] = *p++;
            if (namebuf_idx >= sizeof(namebuf) - 1) {
                break;
            }
            namebuf[namebuf_idx] = '\0';
        }

        /*
         * If we have't yet looked up a mountpoint, do that and
         * see if it was found.
         */
        if (mpoint == NULL) {
            error = mount_lookup(namebuf, &mpoint);

            if (error != 0)
                return error;
            if ((vp = mpoint->vp) == NULL)
                return -EIO;

            namebuf_idx = 0;
            continue;
        }

        vops = &vp->vops;
        error = vnode_lookup(vp, namebuf, &vp);
        if (error != 0) {
            return error;
        }

        namebuf_idx = 0;
    }

    *ndp->vp_res = vp;
    return 0;
}
