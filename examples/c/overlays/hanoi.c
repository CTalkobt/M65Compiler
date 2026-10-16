/* hanoi.c — overlay "hanoi": the Towers of Hanoi, recursively
 *
 * Calls inside an overlay are ordinary JSRs; the recursion stays in it.
 */
#pragma cc45 overlay(hanoi)
#pragma cc45 recurse
#include "demo.h"

static unsigned long moves;

static void move(unsigned char n, unsigned char from, unsigned char to, unsigned char via) {
    if (n == 0) return;
    move(n - 1, from, via, to);
    moves++;
    move(n - 1, via, to, from);
}

unsigned long hanoi_moves(unsigned char discs) {
    moves = 0;
    move(discs, 1, 3, 2);
    return moves;
}
