/* main.c — mapped code overlays, shown at work
 *
 * The 45GS02's program counter is 16 bits: it can only run code it sees in
 * its 64 KB. "Far code" therefore means changing what it sees. ln45
 * --overlay-map does that with the MAP instruction: the overlays stay where
 * the startup code loaded them (MAPDEMO.OV1, in bank 5 at $50000), and a
 * call into one points the CPU's window ($6000-$7FFF here, 8 KB blocks) at
 * it. Nothing is copied: the code runs from bank 5.
 *
 *   resident code  ---jsr--->  stub (resident, 21 bytes, made by ln45)
 *                              --> __ovl_call: MAP the window to the overlay,
 *                                  swap the return address for __ovl_ret
 *                              --> the function, in the window
 *                  <---rts---  __ovl_ret: MAP back the caller's overlay
 *
 * This program checks that claim four ways, each from both sides: what the
 * CPU sees (a window address, through MAP) and what is in bank 5 (read and
 * written by DMA, which does not go through MAP):
 *   1. each overlay runs at a window address that MAP translates into
 *      bank 5, and a different place in bank 5 for each overlay
 *   2. the byte at that bank 5 address (read by DMA) is the overlay's code
 *   3. a store through the window changes bank 5, and a change made in
 *      bank 5 (by DMA) is seen through the window, with no reload: the
 *      window is the memory in bank 5, not a copy of it
 *   4. an overlay calling another gives up the window and gets it back
 *
 * Build: make (the program links with ln45 --overlay-map; disk45 puts
 * mapdemo.prg and MAPDEMO.OV1 on mapdemo.d81). Run: make run.
 * Linked without --overlay-map (copied overlays) the checks of 1-3 fail:
 * each overlay would be copied to the same place in bank 0 and run from
 * there (2/6 pass: the window and the calls of 4 work either way).
 */
#include <stdio.h>
#include <dma.h>
#include <overlay.h>
#include "mapdemo.h"

static unsigned char passed, checks;

/* Called from an overlay: the overlay is still mapped while resident code
 * runs (resident code is below the window), so $011C/$011D are its map */
void look(struct seen *s, unsigned int cpu) {
    s->cpu = cpu;
    s->map_lo = *(volatile unsigned char *)0x011C;
    s->map_hi = *(volatile unsigned char *)0x011D;
    s->current = __ovl_current;
}

/* The physical address the CPU reaches at s->cpu. MAP's lower half: A is
 * the offset's bits 8-15, X's low nibble its bits 16-19, X's high nibble
 * which 8 KB blocks of $0000-$7FFF use the offset (bit 4: $0000, ..., bit
 * 7: $6000). An address in a block that does not use it is plain bank 0. */
static unsigned long physical(const struct seen *s) {
    unsigned long offset = ((unsigned long)(s->map_hi & 0x0F) << 16) | ((unsigned long)s->map_lo << 8);
    unsigned char block = s->cpu >> 13;
    if (block > 3 || !(s->map_hi & (0x10 << block))) return s->cpu;
    return (offset + s->cpu) & 0xFFFFFUL;
}

/* one byte of far memory, read by DMA (which ignores MAP) */
static unsigned char far_peek(unsigned long addr) {
    unsigned char b;
    dma_copy_far((unsigned long)(unsigned int)&b, addr, 1);
    return b;
}

static void check(int ok, const char *what) {
    checks++;
    if (ok) passed++;
    printf("  %s %s\n", ok ? "ok  " : "FAIL", what);
}

static void show(const char *name, const struct seen *s) {
    printf("  %-6s runs at $%04x  map $%02x $%02x  -> $%05lx\n",
           name, s->cpu, s->map_lo, s->map_hi, physical(s));
}

int main(void) {
    struct seen r, g, b, before, inside, after;
    unsigned long rp, gp, bp;

    printf("\x93" "mapped overlays: the window is $%04x\n\n", (unsigned int)__ovl_region);

    /* 1. Where does each overlay run? A window address for the CPU; MAP
     *    adds an offset, different for each overlay, into bank 5. */
    printf("1. where the code runs\n");
    red_where(&r);
    green_where(&g);
    blue_where(&b);
    show("red", &r);
    show("green", &g);
    show("blue", &b);
    rp = physical(&r);
    gp = physical(&g);
    bp = physical(&b);
    check(r.cpu >= (unsigned int)__ovl_region && g.cpu >= (unsigned int)__ovl_region &&
          b.cpu >= (unsigned int)__ovl_region, "the cpu sees them in the window");
    check((rp >> 16) == 5 && (gp >> 16) == 5 && (bp >> 16) == 5 && rp != gp && gp != bp && rp != bp,
          "map sends each to its own place in bank 5");

    /* 2. Is the code really there? DMA reads bank 5 directly. */
    printf("2. what is in bank 5 there (dma)\n");
    printf("  $%05lx: $%02x  $%05lx: $%02x  $%05lx: $%02x\n",
           rp, far_peek(rp), gp, far_peek(gp), bp, far_peek(bp));
    check(far_peek(rp) == 0x52 && far_peek(gp) == 0x47 && far_peek(bp) == 0x42,
          "the overlays' own code bytes");

    /* 3. One memory, two views. Red writes its own code byte through the
     *    window; DMA finds the new value in bank 5. Then DMA changes it in
     *    bank 5 while blue has the window; red, mapped in again, sees the
     *    change -- nothing was copied either way. */
    printf("3. one memory, seen two ways\n");
    red_poke(0x99);
    printf("  red stores $99 at $%04x; bank 5 has $%02x\n", r.cpu, far_peek(rp));
    check((rp >> 16) == 5 && far_peek(rp) == 0x99, "a store through the window is in bank 5");
    blue_where(&b);
    dma_fill_far(rp, 1, 0x77);
    printf("  dma stores $77 at $%05lx; red reads $%02x\n", rp, red_peek());
    check(red_peek() == 0x77, "bank 5 is what the window shows");
    red_poke(0x52);

    /* 4. Overlay to overlay: green calls blue. Blue takes the window; when
     *    blue returns, the runtime maps green back before green goes on. */
    printf("4. green calls blue\n");
    green_then_blue(&before, &inside, &after);
    show("green", &before);
    show("blue", &inside);
    show("green", &after);
    check(physical(&before) == gp && physical(&inside) == bp && physical(&after) == gp &&
          before.current == after.current && inside.current != before.current,
          "blue had the window, green got it back");

    printf("mapped overlays: %u/%u proven\n", passed, checks);
    return passed == checks ? 0 : 1;
}
