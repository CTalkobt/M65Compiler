// Minimal test - just write a single byte to memory
__asm__(".no_zp_save");

void main() {
    volatile unsigned char *result = (unsigned char *)0x4000;
    result[0] = 0x42;  // ASCII 'B'
}
