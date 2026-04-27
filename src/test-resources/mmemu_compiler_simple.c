char *res = 0x4000;
void main() {
    *res = 0xAA;
    __asm__("brk");
}
