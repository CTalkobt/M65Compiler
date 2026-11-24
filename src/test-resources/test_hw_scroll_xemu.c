/* test_hw_scroll_xemu.c — <hw_scroll.h> (#645): the VIC-IV's registers as
 * hardware scrolling sets them (the expected values from the screen's own
 * LINESTEP, H640 / V400 and CHR16 bits)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] init, position 0: the pointers and text position as they were
 *   [1] set_x(19): 2 characters on (screen and colour pointers), 3 pixels
 *       of fine scroll (the text position less 3 or 6); the registers'
 *       high nibbles kept
 *   [2] set_y(10): a row on (LINESTEP), 2 pixels; set_fine_x / _fine_y
 *       (the character kept); get_state
 *   [3] 16-bit characters ($D054 bit 0): two bytes a column
 *   [4] scroll_to over 1000 ms: update's steps (a quarter, a half), the
 *       end (0, at the target, not active); backwards; duration 0: at once
 *   [5] reset: the origin's registers again
 *   [6] $AA sentinel
 */
#include <hw_scroll.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static unsigned long scr(void) {
    return REG(0xD060) | ((unsigned long)REG(0xD061) << 8) | ((unsigned long)REG(0xD062) << 16) |
           ((unsigned long)(REG(0xD063) & 0x0F) << 24);
}
static unsigned int col(void) { return REG(0xD064) | ((unsigned int)REG(0xD065) << 8); }
static unsigned int tx(void) { return REG(0xD04C) | ((unsigned int)(REG(0xD04D) & 0x0F) << 8); }
static unsigned int ty(void) { return REG(0xD04E) | ((unsigned int)(REG(0xD04F) & 0x0F) << 8); }

int main(void) {
    unsigned long s0 = scr();
    unsigned int c0 = col(), x0 = tx(), y0 = ty(), step = REG(0xD058) | ((unsigned int)REG(0xD059) << 8);
    unsigned char hx = (REG(0xD031) & 0x80) ? 1 : 2, hy = (REG(0xD031) & 0x08) ? 1 : 2;
    unsigned char d4d = REG(0xD04D), d4f = REG(0xD04F);
    hw_scroll_state_t st;
    int ok;
    OUT[6] = 0;

    hw_scroll_init();
    hw_scroll_set_x(0);
    ok = scr() == s0 && col() == c0 && tx() == x0 && ty() == y0 && !hw_scroll_is_active();
    OUT[0] = ok;

    hw_scroll_set_x(19);
    ok = scr() == s0 + 2 && col() == c0 + 2 && tx() == x0 - 3 * hx && ty() == y0 &&
         (REG(0xD04D) & 0xF0) == (d4d & 0xF0);
    OUT[1] = ok;

    hw_scroll_set_y(10);
    ok = scr() == s0 + step + 2 && col() == c0 + step + 2 && ty() == y0 - 2 * hy &&
         (REG(0xD04F) & 0xF0) == (d4f & 0xF0);
    hw_scroll_set_fine_x(5);
    hw_scroll_set_fine_y(0);
    st = hw_scroll_get_state();
    ok = ok && st.x == 21 && st.y == 8 && st.fine_x == 5 && st.fine_y == 0 && scr() == s0 + step + 2 &&
         tx() == x0 - 5 * hx && ty() == y0;
    OUT[2] = ok;

    REG(0xD054) |= 0x01;                 /* (16-bit characters: only the registers are looked at) */
    hw_scroll_set_x(16);
    ok = scr() == s0 + step + 4 && col() == c0 + step + 4;
    REG(0xD054) &= 0xFE;
    hw_scroll_set_x(16);
    ok = ok && scr() == s0 + step + 2;
    OUT[3] = ok;

    hw_scroll_set_x(0);
    hw_scroll_set_y(0);
    hw_scroll_to(80, 16, 1000);
    ok = hw_scroll_is_active() && hw_scroll_update(250) == 1;
    st = hw_scroll_get_state();
    ok = ok && st.x == 20 && st.y == 4;
    ok = ok && hw_scroll_update(250) == 1;
    st = hw_scroll_get_state();
    ok = ok && st.x == 40 && st.y == 8 && scr() == s0 + step + 5;
    ok = ok && hw_scroll_update(600) == 0 && !hw_scroll_is_active();
    st = hw_scroll_get_state();
    ok = ok && st.x == 80 && st.y == 16 && scr() == s0 + 2 * step + 10 && hw_scroll_update(10) == 0;
    hw_scroll_to(0, 16, 100);
    ok = ok && hw_scroll_update(50) == 1;
    st = hw_scroll_get_state();
    ok = ok && st.x == 40 && st.y == 16;
    hw_scroll_to(8, 8, 0);
    st = hw_scroll_get_state();
    ok = ok && !hw_scroll_is_active() && st.x == 8 && st.y == 8 && scr() == s0 + step + 1;
    OUT[4] = ok;

    hw_scroll_reset();
    st = hw_scroll_get_state();
    ok = scr() == s0 && col() == c0 && tx() == x0 && ty() == y0 && st.x == 0 && st.y == 0;
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
