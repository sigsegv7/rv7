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
#include <sys/types.h>
#include <sys/param.h>
#include <lib/string.h>
#include <dev/cons/font.h>
#include <dev/cons/cons.h>

#define DEFAULT_FG 0x808080
#define DEFAULT_BG 0x000000

/*
 * Render a character to the screen
 */
static void
cons_blit_ch(struct console *cons, uint32_t x, uint32_t y, char c)
{
    struct vram_dev *vram;
    size_t idx;
    uint8_t *glyph;
    uint32_t fg, bg;

    vram = &cons->vram;
    glyph = &g_CONS_FONT[(int)c*16];

    fg = cons->fg;
    bg = cons->bg;

    for (uint32_t cy = 0; cy < FONT_HEIGHT; ++cy) {
        idx = vram_index(vram, x + (FONT_WIDTH - 1), y + cy);
        for (uint32_t cx = 0; cx < FONT_WIDTH; ++cx) {
            vram->io[idx--] = ISSET(glyph[cy], BIT(cx)) ? fg : bg;
        }
    }
}

/*
 * Clear the console to its background color
 */
static void
cons_clear(struct console *cons)
{
    struct vram_dev *vram;

    vram = &cons->vram;
    if (vram->io == NULL) {
        return;
    }

    memset(vram->io, cons->bg, vram->pitch * vram->height);
}

/*
 * Print a newline to the console
 */
static void
cons_newline(struct console *cons)
{
    struct vram_dev *vram;

    vram = &cons->vram;
    cons->ty += FONT_HEIGHT;
    cons->tx = 0;
    if (cons->ty >= vram->height - FONT_HEIGHT) {
        cons->tx = 0;
        cons->ty = 0;
        cons_clear(cons);
    }
}

/*
 * Handle a special character
 *
 * Returns the character given if it is a special
 * character, otherwise -1
 */
static int
cons_special(struct console *cons, char c)
{
    switch (c) {
    case '\n':
        cons_newline(cons);
        return c;
    }

    return -1;
}

/*
 * Write a single character to the console
 */
static void
console_putch(struct console *cons, char c)
{
    struct vram_dev *vram;

    if (cons_special(cons, c) == c) {
        return;
    }

    vram = &cons->vram;
    cons_blit_ch(cons, cons->tx, cons->ty, c);
    cons->tx += FONT_WIDTH;
    if (cons->tx >= vram->width - FONT_WIDTH) {
        cons_newline(cons);
    }
}

int
console_write(struct console *cons, const char *s, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        console_putch(cons, *s++);
    }
}

int
console_reset(struct console *cons)
{
    int error;

    if (cons == NULL) {
        return -EINVAL;
    }

    /* Try to acquire the VRAM descriptor */
    if ((error = vram_getdev(&cons->vram)) < 0) {
        return error;
    }

    cons->fg = DEFAULT_FG;
    cons->bg = DEFAULT_BG;
    cons->tx = 0;
    cons->ty = 0;
    cons->active = 1;
    return 0;
}
