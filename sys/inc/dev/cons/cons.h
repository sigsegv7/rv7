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

#ifndef _CONS_CONS_H_
#define _CONS_CONS_H_ 1

#include <sys/types.h>
#include <dev/video/vram.h>

/*
 * Represents a system console, these fields are mostly
 * internal and should not be written to directly.
 *
 * @vram: VRAM descriptor in-use
 * @fg: Foreground color in-use
 * @bg: Background color in-use
 * @tx: Text X position
 * @ty: Text Y position
 * @active: Set if active
 */
struct console {
    struct vram_dev vram;
    uint32_t fg;
    uint32_t bg;
    size_t tx;
    size_t ty;
    uint8_t active : 1;
};

/*
 * Reset a console into a known state, typically used
 * for initialization.
 *
 * Returns zero on success.
 */
int console_reset(struct console *cons);

/*
 * Write a stream of bytes to the console.
 *
 * Returns zero on success.
 */
int console_write(struct console *cons, const char *s, size_t len);

#endif  /* !_CONS_CONS_H_ */
