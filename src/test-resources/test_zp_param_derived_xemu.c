/* test_zp_param_derived_xemu.c — a pointer parameter whose fields are read
 * after calls (#470)
 *
 * In the ZP convention a parameter used only before a block's first call is
 * used where the caller put it ($10...). A pointer's field address (p + 2) is
 * fused into ($10),y: a load through it after a call reads the parameter
 * block, which that call's arguments overwrote. Such a parameter must be
 * copied at the entry.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the issue's function: an early return, then three calls reading
 *       p->x / p->y each time
 *   [1] the early-return path
 *   [2] a field read after a call in the same block
 *   [3] a pointer computed from the parameter (q = p + 1) used after a call
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

struct part { unsigned char type, flags; int x, y, a, b; };

unsigned char shadow;
static int got[16];
static unsigned char ngot;

NOINLINE void blob(int ax, int ay, int bx, int by, unsigned char r, unsigned char ramp, unsigned char style) {
    got[ngot++] = ax + ay + bx + by + r + ramp + style;
}

NOINLINE void bumper(const struct part *p, unsigned char slot) {
    unsigned char r = (unsigned char)p->a;
    if (shadow) {
        blob(p->x, p->y, p->x, p->y, (unsigned char)(r + 1), 0, 0);
        return;
    }
    blob(p->x, p->y, p->x, p->y, r, 96, 0);
    blob(p->x, p->y, p->x, p->y, (unsigned char)(r - 3), 128, 0);
    blob(p->x, p->y, p->x, p->y, (unsigned char)(r - 5), slot, 0);
}

NOINLINE static int after(const struct part *p) {
    int s;
    blob(1, 2, 3, 4, 5, 6, 7);
    s = p->x;
    blob(0, 0, 0, 0, 0, 0, 0);
    return s + p->y + p->b;
}

NOINLINE static int via(const struct part *p) {
    const struct part *q = p + 1;
    blob(9, 9, 9, 9, 9, 9, 9);
    return q->x - q->y;
}

static const struct part parts[2] = {{1, 0, 100, 50, 20, 7}, {2, 0, 300, 40, 0, 0}};

int main(void) {
    OUT[4] = 0;
    ngot = 0;
    shadow = 0;
    bumper(&parts[0], 9);
    OUT[0] = (ngot == 3 && got[0] == 300 + 20 + 96 && got[1] == 300 + 17 + 128 && got[2] == 300 + 15 + 9) ? 1 : 2;
    ngot = 0;
    shadow = 1;
    bumper(&parts[0], 9);
    OUT[1] = (ngot == 1 && got[0] == 300 + 21) ? 1 : 2;
    ngot = 0;
    OUT[2] = after(&parts[0]) == 157 ? 1 : 2;
    OUT[3] = via(&parts[0]) == 260 ? 1 : 2;
    OUT[4] = 0xAA;
    return 0;
}
