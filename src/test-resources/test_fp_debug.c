// Test: Simple frame-relative pointer dereference WITHOUT function calls
// This should work because no frame pointer recalculation occurs
volatile char *r = (char *)0x4000;

void main() {
    short x = 10;
    short y = 20;
    short z = 30;
    short *p = &x;

    r[0] = z;      // 30 = 0x1E (direct local access)
    r[1] = *p;     // 10 = 0x0A (frame-relative pointer dereference, NO function call before it)
    r[2] = 0xAA;   // marker

    __asm__("brk"); // Signal test complete to mmemu
}
