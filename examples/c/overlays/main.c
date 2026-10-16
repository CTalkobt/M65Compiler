/* main.c — the overlay demo's resident part
 *
 * Code overlays let a program be larger than the memory it runs in: each
 * file with #pragma cc45 overlay(name) (or compiled with -foverlay=name)
 * puts its code in that overlay. ln45 links all overlays to run in one
 * region of bank 0 and writes them to OVLDEMO.OVL, which the program's
 * startup code loads into far memory (bank 5). A call into an overlay goes
 * through a small resident stub that copies the overlay into the region by
 * DMA when it is not there (about 0.1 ms per KB) -- or, linked with
 * ln45 --overlay-map (make OVERLAY_MAP=1), maps the window $6000-$7FFF to
 * it with MAP, no copying -- so calls work in every direction: resident
 * code to an overlay, an overlay to resident code, and one overlay to
 * another, and back. The overlays' variables stay resident.
 *
 * Build: make (cc45 -c for each file, ln45 links them and writes the
 * overlay file, disk45 puts both on ovldemo.d81). Run: make run.
 */
#include <stdio.h>
#include <overlay.h>
#include "demo.h"

static const char *const names[] = { "none", "primes", "hanoi", "words" };

static const char text[] =
    "Code overlays let a program be larger than the memory it runs in: "
    "the overlays share one region and come in from far memory when called.";

void say(const char *label, unsigned long value) {
    printf("  %-20s%lu\n", label, value);
}

/* which overlay the region holds now */
static void loaded(void) {
    printf("  (in the region: %s)\n\n", names[__ovl_current < 4 ? __ovl_current : 0]);
}

int main(void) {
    unsigned char ran = 0;
    printf("\x93" "cc45 code overlays: three tools in $%04x\n\n", (unsigned int)__ovl_region);

    printf("primes (overlay \"primes\")\n");
    say("primes below 10000", primes_count(10000));
    loaded();
    ran++;

    printf("towers of hanoi (overlay \"hanoi\")\n");
    say("moves for 12 discs", hanoi_moves(12));
    loaded();
    ran++;

    printf("text statistics (overlay \"words\")\n");
    words_stats(text);
    loaded();
    ran++;

    printf("%u/3 overlays ran\n", ran);
    return 0;
}
