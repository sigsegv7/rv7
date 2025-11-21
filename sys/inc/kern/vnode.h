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

#ifndef _OS_VNODE_H_
#define _OS_VNODE_H_

#include <sys/types.h>

struct vnode;

/*
 * Valid vnode types
 */
typedef enum {
    VREG,
    VDIR,
    VCHR,
    VBLK
} vtype_t;

/*
 * Arguments for buffer operations
 */
struct vop_buf_args {
    void *buffer;
    off_t offset;
    size_t len;
};

/*
 * Operations that can be performed on a vnode
 */
struct vops {
    ssize_t(*read)(struct vop_buf_args *args);
    ssize_t(*write)(struct vop_buf_args *args);
    void(*reclaim)(struct vnode *vp);
};

/*
 * A virtual file node is an abstract repre
 *
 * @type: Vnode type
 * @vops: Operations associated with vnode
 * @ref:  Reference counter
 * @data: Filesystem specific data
 */
struct vnode {
    vtype_t type;
    struct vops vops;
    int ref;
    void *data;
};

/*
 * Initialize a vnode by type
 *
 * @vp_res: Vnode pointer result
 * @vtype: Vnode type to initialize
 *
 * Returns zero on success
 */
int vnode_init(struct vnode **vp_res, vtype_t type);

/*
 * Release a vnode from memory
 *
 * @vp: Vnode to release from memory
 *
 * Returns the reference count if not released, zero
 * on successful release.
 */
int vnode_release(struct vnode *vp);

#endif  /* !_OS_VNODE_H_ */
