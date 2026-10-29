/* test_sprites_xemu.c — <sprites.h>: the VIC-IV's hardware sprites
 *
 * Results at $C0C0 (01 = pass):
 *   +0  sprite_init: 16-bit pointers at the library's table (16-byte aligned), all off
 *   +1  sprite_image: the pointer is the address / 64
 *   +2  sprite_pos: the low bytes, bit 8 of x ($D010) and y ($D077), bit 9 ($D05F, $D078)
 *   +3  sprite_move and sprite_x / sprite_y
 *   +4  sprite_show / sprite_hide
 *   +5  sprite_mode: multicolour, 16-colour (its colour 0), hi-res
 *   +6  sprite_size, sprite_height, sprite_behind, sprite_h640
 *   +7  sprite_color, sprite_multicolors
 *   +8  sprite_overlap: normal, wide, expanded, tall boxes
 *   +9  the hardware's sprite-sprite collision (two solid sprites overlapping)
 *   +10 sprite_done: the registers back
 *   +11 sprite_palette_bank, sprite_v400, sprite_tile, sprite_altcolors,
 *       sprite_alpha / sprite_alpha_level: their bits, the shared registers'
 *       other bits untouched, all back after sprite_done
 *   +12 $AA
 */
#include <sprites.h>
#include <dma.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned char solid[64];

static void frames(unsigned char n) {
    while (n--) {
        while (REG(0xD012) != 250) ;
        while (REG(0xD012) == 250) ;
    }
}

int main(void) {
    unsigned char ok, d06e = REG(0xD06E), d06c = REG(0xD06C), d027 = REG(0xD027);
    unsigned char d049, d04b, d04d, d04f, d070, d075;
    unsigned int table, i;
    const unsigned char *ptr;

    REG(0xD015) = 0x81;         /* (sprite_init turns them off) */
    sprite_init();
    table = REG(0xD06C) | (unsigned int)REG(0xD06D) << 8;
    ptr = (const unsigned char *)table;
    OUT[0] = REG(0xD06E) == 0x80 && (table & 15) == 0 && REG(0xD015) == 0 && REG(0xD010) == 0 && REG(0xD06B) == 0
             && ptr[0] == 0 && ptr[15] == 0;

    sprite_image(0, 0x40000UL);
    sprite_image(7, 0x12340UL);
    OUT[1] = ptr[0] == 0x00 && ptr[1] == 0x10 && ptr[14] == 0x8D && ptr[15] == 0x04;

    sprite_pos(1, 300, 260);
    ok = REG(0xD002) == 44 && REG(0xD003) == 4 && (REG(0xD010) & 0x02) && (REG(0xD077) & 0x02)
         && !(REG(0xD05F) & 0x02) && !(REG(0xD078) & 0x02);
    sprite_pos(2, 700, 600);    /* 700 = $2BC, 600 = $258 */
    ok = ok && REG(0xD004) == 0xBC && REG(0xD005) == 0x58 && !(REG(0xD010) & 0x04)
         && (REG(0xD05F) & 0x04) && !(REG(0xD077) & 0x04) && (REG(0xD078) & 0x04)
         && (REG(0xD010) & 0x02);
    OUT[2] = ok;

    sprite_move(1, -50, -10);
    OUT[3] = sprite_x(1) == 250 && sprite_y(1) == 250 && REG(0xD002) == 250 && REG(0xD003) == 250
             && !(REG(0xD010) & 0x02) && !(REG(0xD077) & 0x02);

    sprite_show(3);
    sprite_show(5);
    ok = REG(0xD015) == 0x28;
    sprite_hide(3);
    OUT[4] = ok && REG(0xD015) == 0x20;

    REG(0xD02C) = 7;
    sprite_mode(4, SPRITE_MULTICOLOR);
    sprite_mode(5, SPRITE_16COLOR);
    ok = REG(0xD01C) == 0x10 && REG(0xD06B) == 0x20 && REG(0xD02C) == 0;
    sprite_mode(4, SPRITE_HIRES);
    sprite_mode(5, SPRITE_HIRES);
    OUT[5] = ok && REG(0xD01C) == 0 && REG(0xD06B) == 0;

    sprite_size(6, SPRITE_EXPAND_X | SPRITE_TALL);
    sprite_size(2, SPRITE_EXPAND_Y | SPRITE_WIDE);
    sprite_height(40);
    sprite_behind(6, 1);
    sprite_h640(1);
    ok = REG(0xD01D) == 0x40 && REG(0xD055) == 0x40 && REG(0xD017) == 0x04 && REG(0xD057) == 0x04
         && REG(0xD056) == 40 && REG(0xD01B) == 0x40 && (REG(0xD054) & 0x10);
    sprite_size(6, SPRITE_NORMAL);
    sprite_behind(6, 0);
    sprite_h640(0);
    OUT[6] = ok && REG(0xD01D) == 0 && REG(0xD055) == 0 && REG(0xD01B) == 0 && !(REG(0xD054) & 0x10);

    sprite_color(0, 2);
    sprite_multicolors(5, 6);
    OUT[7] = REG(0xD027) == 2 && REG(0xD025) == 5 && REG(0xD026) == 6;

    /* boxes: 0 at (100, 100) 24 x 21 */
    sprite_pos(0, 100, 100);
    sprite_pos(1, 120, 110);
    ok = sprite_overlap(0, 1) && sprite_overlap(1, 0);
    sprite_pos(1, 124, 100);                   /* just right of it */
    ok = ok && !sprite_overlap(0, 1) && !sprite_overlap(1, 0);
    sprite_pos(1, 100, 121);                   /* just below */
    ok = ok && !sprite_overlap(0, 1);
    sprite_size(0, SPRITE_WIDE);               /* 64 wide */
    sprite_pos(1, 160, 100);
    ok = ok && sprite_overlap(0, 1);
    sprite_size(0, SPRITE_EXPAND_X);           /* 48 wide */
    ok = ok && !sprite_overlap(0, 1);
    sprite_size(0, SPRITE_TALL);               /* 40 high */
    sprite_pos(1, 100, 135);
    ok = ok && sprite_overlap(0, 1);
    sprite_size(0, SPRITE_NORMAL);
    ok = ok && !sprite_overlap(0, 1);
    sprite_size(2, SPRITE_NORMAL);
    OUT[8] = ok;

    /* two solid sprites over each other on the screen: the hardware's
       collision bits */
    for (i = 0; i < 63; i++) solid[i] = 0xFF;
    dma_copy_far(0x40000UL, (unsigned long)(unsigned int)solid, 64);
    sprite_image(0, 0x40000UL);
    sprite_image(1, 0x40000UL);
    sprite_pos(0, SPRITE_X0 + 50, SPRITE_Y0 + 50);
    sprite_pos(1, SPRITE_X0 + 60, SPRITE_Y0 + 55);
    sprite_hide(5);
    (void)sprite_collisions();
    sprite_show(0);
    sprite_show(1);
    frames(3);
    OUT[9] = (sprite_collisions() & 3) == 3;

    sprite_done();
    OUT[10] = REG(0xD015) == 0x81 && REG(0xD06E) == d06e && REG(0xD06C) == d06c && REG(0xD027) == d027;
    REG(0xD015) = 0;

    /* the newer registers: their sprite bits only */
    d049 = REG(0xD049); d04b = REG(0xD04B); d04d = REG(0xD04D); d04f = REG(0xD04F);
    d070 = REG(0xD070); d075 = REG(0xD075);
    sprite_init();
    ok = (REG(0xD070) & 0x0C) == (d070 & 0x0C);      /* (the bank kept) */
    sprite_palette_bank(2);
    sprite_v400(3, 1);
    sprite_tile(1, 1);
    sprite_tile(6, 1);
    sprite_altcolors(2, 1);
    sprite_altcolors(5, 1);
    sprite_alpha(4, 1);
    sprite_alpha_level(0x40);
    ok = ok && (REG(0xD070) & 0x0C) == 0x08 && (REG(0xD070) & 0xF3) == (d070 & 0xF3)
         && REG(0xD076) == 0x08
         && (REG(0xD04D) & 0xF0) == 0x20 && (REG(0xD04D) & 0x0F) == (d04d & 0x0F)
         && (REG(0xD04F) & 0xF0) == 0x40 && (REG(0xD04F) & 0x0F) == (d04f & 0x0F)
         && (REG(0xD049) & 0xF0) == 0x40 && (REG(0xD049) & 0x0F) == (d049 & 0x0F)
         && (REG(0xD04B) & 0xF0) == 0x20 && (REG(0xD04B) & 0x0F) == (d04b & 0x0F)
         && REG(0xD074) == 0x10 && REG(0xD075) == 0x40;
    sprite_tile(1, 0);
    sprite_v400(3, 0);
    ok = ok && (REG(0xD04D) & 0xF0) == 0 && REG(0xD076) == 0;
    sprite_done();
    OUT[11] = ok && REG(0xD070) == d070 && REG(0xD049) == d049 && REG(0xD04B) == d04b
              && REG(0xD04D) == d04d && REG(0xD04F) == d04f && REG(0xD074) == 0 && REG(0xD075) == d075;

    OUT[12] = 0xAA;
    return 0;
}
