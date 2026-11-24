/* tgi_bmpsetup.c — <tgi.h>'s bitmap drivers' install, init, done and clear
 * (stdlib_asm/tgi_bmp.s45 has their descriptions and pixels)
 *
 * install: a far_malloc block of 16384 bytes in bank 4 or 5; the bitmap at
 * its first 8 KB boundary (8000 bytes), the cells' colours (1000) in the
 * room before it or after it (one of them has 1000 bytes: the two make
 * 8384). init: the screen editor's 40x25 (ESC 4: LINESTEP, CHRCOUNT and
 * the colour RAM's 1000 bytes as the bitmap's cells want them), then the
 * VIC-IV's hot registers off (a $D011 / $D016 write then sets nothing
 * else), the bitmap ($D011 bit 5; multicolour $D016 bit 4), the character
 * pointer at the bitmap, the screen pointer at the colours. done: all of it
 * back, then the screen's size as it was (ESC 8 / ESC 5 / ESC 4).
 */

#include <tgi.h>
#include <tgi/tgi-kernel.h>
#include <tgi/tgi-error.h>
#include <far_ptr.h>
#include <dma.h>
#include <stdio.h>

#define REG(a) (*(volatile unsigned char *)(a))

extern unsigned long __tgi_b_bmp, __tgi_b_scr;
extern unsigned char __tgi_fontptr[3];    /* tgi_text.s45: the font's character pointer (0: $D068) */


static far_ptr_t block;
static unsigned char saved, old_size, old_hot, old_d011, old_d016, old_d054;
static unsigned char old_scr[4], old_chr[3];

static unsigned char multicolour(void) {
    return tgi_drv == tgi_static_mcm;
}

__fastcall__ unsigned char __tgi_b_install(void) {
    unsigned long base, bmp;
    block = far_malloc(16384u, BANK4);
    if (far_is_null(block)) block = far_malloc(16384u, BANK5);
    if (far_is_null(block)) return TGI_ERR_NO_RES;
    base = far_to_linear(block);
    bmp = (base + 0x1FFFUL) & ~0x1FFFUL;
    __tgi_b_bmp = bmp;
    __tgi_b_scr = bmp - base >= 1000 ? base : bmp + 8000;
    return TGI_ERR_OK;
}

__fastcall__ void __tgi_b_uninstall(void) {
    if (!far_is_null(block)) far_free(block);
    block = far_make(0, 0);
}

static void esc(unsigned char c) {
    putchar(27);
    putchar(c);
}

__fastcall__ void __tgi_b_clear(void) {
    dma_fill_far(__tgi_b_bmp, 8000, 0);
    /* hires: foreground 1 (white), background 0; multicolour: both 0 */
    dma_fill_far(__tgi_b_scr, 1000, multicolour() ? 0x00 : 0x10);
    if (multicolour())
        dma_fill_far(0xFF80000UL + (REG(0xD064) | ((unsigned int)REG(0xD065) << 8)), 1000, 0);
}

__fastcall__ void __tgi_b_init(void) {
    unsigned char i;
    old_size = REG(0xD031) & 0x88;
    esc('4');                       /* 40x25 */
    old_hot = REG(0xD05D) & 0x80;
    REG(0xD05D) &= 0x7F;
    old_d011 = REG(0xD011);
    old_d016 = REG(0xD016);
    old_d054 = REG(0xD054);
    for (i = 0; i < 4; i++) old_scr[i] = REG(0xD060 + i);
    for (i = 0; i < 3; i++) __tgi_fontptr[i] = old_chr[i] = REG(0xD068 + i);
    saved = 1;
    REG(0xD054) = (unsigned char)(old_d054 & 0xF8);         /* no CHR16, no full-colour */
    REG(0xD068) = (unsigned char)__tgi_b_bmp;
    REG(0xD069) = (unsigned char)(__tgi_b_bmp >> 8);
    REG(0xD06A) = (unsigned char)(__tgi_b_bmp >> 16);
    REG(0xD060) = (unsigned char)__tgi_b_scr;
    REG(0xD061) = (unsigned char)(__tgi_b_scr >> 8);
    REG(0xD062) = (unsigned char)(__tgi_b_scr >> 16);
    REG(0xD063) = (unsigned char)((old_scr[3] & 0xF0) | ((__tgi_b_scr >> 24) & 0x0F));
    __tgi_b_clear();
    REG(0xD011) = (unsigned char)(old_d011 | 0x20);
    REG(0xD016) = (unsigned char)(multicolour() ? (old_d016 | 0x10) : (old_d016 & 0xEF));
}

__fastcall__ void __tgi_b_done(void) {
    unsigned char i;
    if (!saved) return;
    saved = 0;
    REG(0xD011) = old_d011;
    REG(0xD016) = old_d016;
    REG(0xD054) = old_d054;
    for (i = 0; i < 4; i++) REG(0xD060 + i) = old_scr[i];
    for (i = 0; i < 3; i++) {
        REG(0xD068 + i) = old_chr[i];
        __tgi_fontptr[i] = 0;
    }
    REG(0xD05D) |= old_hot;
    esc(old_size == 0x88 ? '5' : old_size == 0x80 ? '8' : '4');
}
