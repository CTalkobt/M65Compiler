// Minimal test: just offset 0
__asm__(".no_zp_save");

volatile char *result = (char *)0x4000;

void main() {
    char arr[2] = {0xAA, 0xBB};
    result[0] = arr[0];
    result[1] = arr[1];
    result[2] = 0xFF;
}
