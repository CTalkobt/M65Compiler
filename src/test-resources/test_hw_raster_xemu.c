/* test_hw_raster_xemu.c — <hw_raster.h> (#645): raster interrupts and timing
 *
 * Results at $C0C0 (01 = pass):
 *   [0] without the handler: hw_raster_get_line's range over a frame,
 *       wait_line (the line reached), in_vblank (at 100 no, after
 *       wait_vblank yes), get_x in range
 *   [1] hw_raster_init (the vector the library's), two __interrupt
 *       functions at lines 80 and 180: each once a frame, at its line
 *       (hw_raster_current, the raster when it ran)
 *   [2] hw_raster_get_frame counts the frames; the KERNAL's handler still
 *       runs (clock() goes on)
 *   [3] hw_raster_set_callback: a plain function (a stack frame: #pragma
 *       cc45 recurse) called with its line and user data; cleared: no
 *       more; line 400: 0
 *   [4] hw_raster_disable: the program's lines stop, the KERNAL's goes on;
 *       hw_raster_enable: again
 *   [5] hw_raster_set_line: the KERNAL's at 200 (clock() goes on); a
 *       function at the KERNAL's line: both run
 *   [6] 8 lines of the program's: the 9th refused; clear_callback frees one
 *   [7] hw_raster_done: the KERNAL's vector back, the functions no longer
 *       called
 *   [8] $AA sentinel
 */
#include <hw_raster.h>
#include <time.h>

#define OUT ((volatile unsigned char *)0xC0C0)
#define REG(a) (*(volatile unsigned char *)(a))

static volatile unsigned int hits_a, hits_b, hits_c, cb_hits;
static volatile int at_a, ras_a, at_b, cb_line;
static void *volatile cb_user;
static char tag;

__interrupt void isr_a(void) {
    hits_a++;
    at_a = hw_raster_current();
    ras_a = hw_raster_get_line();
}

__interrupt void isr_b(void) {
    hits_b++;
    at_b = hw_raster_current();
}

__interrupt void isr_c(void) {
    hits_c++;
}

#pragma cc45 recurse
static void plain(int line, void *user) {
    cb_hits++;
    cb_line = line;
    cb_user = user;
}

static void frames(unsigned char n) {
    while (n--) hw_raster_wait_vblank();
}

/* about n, give or take one */
static int about(unsigned int v, unsigned int n) {
    return v + 1 >= n && v <= n + 1;
}

int main(void) {
    unsigned int lo = 999, hi = 0, l, i, f0, h;
    unsigned char v0 = REG(0x0314), v1 = REG(0x0315);
    clock_t c0;
    int ok;
    OUT[8] = 0;

    /* [0] */
    for (i = 0; i < 40000u; i++) {
        l = hw_raster_get_line();
        if (l < lo) lo = l;
        if (l > hi) hi = l;
    }
    ok = lo == 0 && hi >= 300 && hi <= 311;
    hw_raster_wait_line(100);
    l = hw_raster_get_line();
    ok = ok && (l == 100 || l == 101) && !hw_raster_in_vblank();
    hw_raster_wait_vblank();
    ok = ok && hw_raster_in_vblank() && hw_raster_get_x() < 16384;
    OUT[0] = ok;

    /* [1] */
    hw_raster_init();
    ok = REG(0x0314) != v0 || REG(0x0315) != v1;
    ok = ok && hw_raster_set_isr(80, isr_a) && hw_raster_set_isr(180, isr_b);
    frames(1);
    hits_a = hits_b = 0;
    f0 = hw_raster_get_frame();
    c0 = clock();
    frames(10);
    ok = ok && about(hits_a, 10) && about(hits_b, 10) && at_a == 80 && at_b == 180 && ras_a >= 80 && ras_a <= 82;
    OUT[1] = ok;

    /* [2] */
    ok = about(hw_raster_get_frame() - f0, 10) && about((unsigned int)(clock() - c0), 10);
    OUT[2] = ok;

    /* [3] */
    ok = hw_raster_set_callback(130, plain, &tag) && hw_raster_set_callback(400, plain, &tag) == 0;
    frames(5);
    ok = ok && cb_hits >= 4 && cb_line == 130 && cb_user == &tag;
    hw_raster_clear_callback(130);
    frames(1);
    h = cb_hits;
    frames(3);
    ok = ok && cb_hits == h && about(hits_a, 19);
    OUT[3] = ok;

    /* [4] */
    hw_raster_disable();
    frames(1);
    h = hits_a;
    f0 = hw_raster_get_frame();
    c0 = clock();
    frames(5);
    ok = hits_a == h && about(hw_raster_get_frame() - f0, 5) && about((unsigned int)(clock() - c0), 5);
    hw_raster_enable();
    frames(5);
    ok = ok && hits_a >= h + 4;
    OUT[4] = ok;

    /* [5] */
    ok = hw_raster_set_line(200) && hw_raster_set_line(400) == 0;
    c0 = clock();
    frames(5);
    ok = ok && about((unsigned int)(clock() - c0), 5);
    ok = ok && hw_raster_set_isr(200, isr_c);
    hits_c = 0;
    c0 = clock();
    frames(5);
    ok = ok && about(hits_c, 5) && about((unsigned int)(clock() - c0), 5);
    OUT[5] = ok;

    /* [6] lines 80, 180, 200 taken: 5 more, then the 9th refused */
    ok = 1;
    for (i = 0; i < 5; i++) ok = ok && hw_raster_set_isr(20 + i * 10, isr_c);
    ok = ok && hw_raster_set_isr(240, isr_c) == 0;
    hw_raster_clear_callback(20);
    ok = ok && hw_raster_set_isr(240, isr_c);
    frames(2);
    OUT[6] = ok;

    /* [7] */
    hw_raster_done();
    ok = REG(0x0314) == v0 && REG(0x0315) == v1;
    h = hits_a;
    c0 = clock();
    frames(5);
    ok = ok && hits_a == h && about((unsigned int)(clock() - c0), 5);
    OUT[7] = ok;
    OUT[8] = 0xAA;
    return 0;
}
