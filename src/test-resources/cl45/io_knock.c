// cl45 fixture: mega65-io-personality (--profile=standalone), negative: the
// file knocks.
#define POKE(a, v) (*(volatile unsigned char *)(a) = (v))

void setup(void) {
    POKE(0xD02F, 0x47);
    POKE(0xD02F, 0x53);
    POKE(0xD031, 0x80);
}
