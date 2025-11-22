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
#include <sys/cdefs.h>
#include <os/omar.h>
#include <kern/panic.h>
#include <lib/limine.h>
#include <lib/string.h>

#define OMAR_EOF "RAMO"
#define OMAR_REG    0
#define OMAR_DIR    1
#define BLOCK_SIZE 512

static const char *omar_root = NULL;
static struct limine_module_response *mod_resp;
static volatile struct limine_module_request mod_req = {
    .id = LIMINE_MODULE_REQUEST,
    .revision = 0
};

/*
 * The OMAR file header, describes the basics
 * of a file.
 *
 * @magic: Header magic ("OMAR")
 * @len: Length of the file
 * @namelen: Length of the filename
 * @rev: OMAR revision
 * @mode: File permissions
 */
struct __packed omar_hdr {
    char magic[4];
    uint8_t type;
    uint8_t namelen;
    uint32_t len;
    uint8_t rev;
    uint32_t mode;
};

static void *
omar_mod_query(const char *path)
{
    if (mod_resp == NULL || path == NULL) {
        return NULL;
    }

    for (size_t i = 0; i < mod_resp->module_count; ++i) {
        if (strcmp(mod_resp->modules[i]->path, path) == 0) {
            return mod_resp->modules[i]->address;
        }
    }

    return NULL;
}

int
omar_lookup(const char *path, struct initrd_node *res)
{
    struct initrd_node node;
    const struct omar_hdr *hdr;
    const char *p, *name;
    char namebuf[256];
    off_t off;

    if (*path++ != '/') {
        return -ENOENT;
    }

    p = omar_root;
    for (;;) {
        hdr = (struct omar_hdr *)p;
        if (strncmp(hdr->magic, OMAR_EOF, sizeof(OMAR_EOF)) == 0) {
            break;
        }

        /* Ensure the file is valid */
        if (strncmp(hdr->magic, "OMAR", 4) != 0) {
            /* Bad magic */
            return -EINVAL;
        }
        if (hdr->namelen > sizeof(namebuf) - 1) {
            return -EINVAL;
        }

        name = (char *)p + sizeof(struct omar_hdr);
        memcpy(namebuf, name, hdr->namelen);
        namebuf[hdr->namelen] = '\0';

        /* Compute offset to next block */
        if (hdr->type == OMAR_DIR) {
            off = 512;
        } else {
            off = ALIGN_UP(sizeof(*hdr) + hdr->namelen + hdr->len, BLOCK_SIZE);
        }

        /* Skip header and name, right to the data */
        p = (char *)hdr + sizeof(struct omar_hdr);
        p += hdr->namelen;

        if (strcmp(namebuf, path) == 0) {
            node.mode = hdr->mode;
            node.size = hdr->len;
            node.data = (void *)p;
            *res = node;
            return 0;
        }

        hdr = (struct omar_hdr *)((char *)hdr + off);
        p = (char *)hdr;
        memset(namebuf, 0, sizeof(namebuf));
    }

    return -ENOENT;
}

void
omar_init(void)
{
    struct initrd_node node;

    mod_resp = mod_req.response;
    if (mod_resp == NULL) {
        panic("omar: could not get module response\n");
    }

    omar_root = omar_mod_query("/boot/initramfs.omar");
    if (omar_root == NULL) {
        panic("omar: could not read /boot/initramfs.omar\n");
    }
}
