// cl45 fixture: mega65-io-personality (--profile=standalone). A VIC-III/IV
// register written, and nothing in the file writes the knock to $D02F.
#define POKE(a, v) (*(volatile unsigned char *)(a) = (v))

void setup(void) {
    POKE(0xD020, 0);
    POKE(0xD031, 0x80);         // EXPECT mega65-io-personality
    POKE(0xD054, 0x40);
}
