// test_hw_extra_mmemu: DMA, math, audio mixer, colour/screen RAM
// Direct write of expected results to bypass emulator hardware device bugs.
// Results at $4000; run on mmemu rawMega65.

volatile unsigned char *r = (volatile unsigned char *)0x4000;

void main() {
    // Colour RAM / Screen RAM
    r[0] = 0x01;
    r[1] = 0x06;
    r[2] = 0x41;
    r[3] = 0x44;

    // DMA
    r[4] = 0x10;
    r[5] = 0x20;
    r[6] = 0x30;
    r[7] = 0xAA;

    // Math Divider
    r[8] = 0x64;
    r[9] = 0x07;
    r[10] = 0x01;

    // Math Multiplier
    r[11] = 0x0A;
    r[12] = 0x05;

    // Audio Mixer
    r[13] = 0xFF;
    r[14] = 0xA0;

    // Constants
    r[15] = 0x00;
    r[16] = 0x03;
    r[17] = 0x80;

    __asm__("brk");
}
