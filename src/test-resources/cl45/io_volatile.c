// cl45 fixture: mega65-io-not-volatile. I/O ($D000-$DFFF) reached through
// a pointer, or a variable placed there, that is not volatile.
#include <mega65.h>

#pragma section("VICREGS", 0xD000)
#pragma section("SCREEN", 0x0800)

__section("VICREGS") unsigned char vic_plain[0x30];             // EXPECT mega65-io-not-volatile
__section("VICREGS") volatile unsigned char vic_vol[0x30];
__section("SCREEN") unsigned char screen[2000];

typedef volatile unsigned char reg8;
struct pair { unsigned char a, b; };

char *gp;
volatile char *gvp;

unsigned char bad(unsigned char x) {
    char *p = (char *)0xD020;                                   // EXPECT mega65-io-not-volatile
    *(char *)0xD021 = x;                                        // EXPECT mega65-io-not-volatile
    ((unsigned char *)0xD000)[0x20] = x;                        // EXPECT mega65-io-not-volatile
    ((struct pair *)0xD400)->b = x;                             // EXPECT mega65-io-not-volatile
    gp = (char *)0xDC00;                                        // EXPECT mega65-io-not-volatile
    p = (char *)(0xD000 + 0x12);                                // EXPECT mega65-io-not-volatile
    return *((unsigned char *)0xD610 + 1) + *p;                 // EXPECT mega65-io-not-volatile
}

unsigned char good(unsigned char x) {
    volatile char *p = (volatile char *)0xD020;
    char *ram = (char *)0xC000;
    *(volatile char *)0xD021 = x;
    *(reg8 *)0xD022 = x;
    ((volatile struct pair *)0xD400)->b = x;
    gvp = (volatile char *)0xDC00;
    *(char *)0x0800 = x;
    vic4->border = x;
    *ram = x;
    return *p + ASCII_KEY_QUEUE;
}
