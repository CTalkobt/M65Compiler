/* main.c — __far functions
 *
 * The 45GS02 runs only code it sees in its 64 KB, so "far" code has to be
 * brought into view. A function declared __far is kept in far memory
 * (bank 5) and run through a window: cc45 puts its code in the text
 * segment "far", ln45 packs each file's far code into automatic overlays
 * (far1, far2, ... of 8 KB) and writes them to FARDEMO.OVL, which the
 * startup code loads into bank 5. A call into far code goes through a small
 * resident stub that points the window ($6000-$7FFF) at the overlay with
 * MAP (no copying) and maps the caller's code back on return.
 *
 * Nothing else changes in the source: no pragma, no option, prototypes
 * without __far. Only code goes far; data (strings, tables, variables)
 * stays resident. This file mixes both: main and add_up stay, checksum is
 * far. Each line shows the overlay mapped when the call came back: this
 * demo's far code (about 1 KB) fits in one, far1; a program with more gets
 * far2, ... and a call between them switches the map.
 *
 * Build: make (fardemo.prg, fardemo.ovl, fardemo.d81). Run: make run.
 * The linker map (fardemo.map) shows the overlays, their banks and the
 * stubs; objdump45 -d fardemo.ovl disassembles the far code.
 */
#include <stdio.h>
#include <string.h>
#include <overlay.h>
#include "fardemo.h"

static unsigned char ran, checks;

/* resident and far: the same code, one kept in each place */
static unsigned int add_up(const char *s) {
    unsigned int sum = 0;
    while (*s) sum = sum * 31 + (unsigned char)*s++;
    return sum;
}

static __far unsigned int checksum(const char *s) {
    unsigned int sum = 0;
    while (*s) sum = sum * 31 + (unsigned char)*s++;
    return sum;
}

/* a result, the overlay mapped when it came back, and whether it is right */
static void show(const char *what, unsigned long value, unsigned long want) {
    checks++;
    if (value == want) ran++;
    printf("  %-22s%-8lu far%u %s\n", what, value, __ovl_current, value == want ? "" : "wrong");
}

int main(void) {
    char text[] = "hello far world";

    printf("\x93" "__far functions: kept in bank 5,\n");
    printf("run in the window at $%04x\n\n", (unsigned int)__ovl_region);

    show("primes below 1000", primes_below(1000), 168);
    show("fib(30)", fib(30), 832040UL);
    show("gcd(1071, 462)", gcd(1071, 462), 21);
    show("words", count_words("the quick brown fox jumps over the lazy dog"), 9);
    show("checksum (main.c)", checksum("cc45"), add_up("cc45"));

    caesar(text, 3);
    report(text);
    checks++;
    if (strcmp(text, "khoor idu zruog") == 0) ran++;

    printf("\n%u/%u far functions ran\n", ran, checks);
    return ran == checks ? 0 : 1;
}
