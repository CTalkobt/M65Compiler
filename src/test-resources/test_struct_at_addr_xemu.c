#pragma cc45 no_zp_save
/* test_struct_at_addr_xemu.c — members of a struct at a cast constant
 * address, (*(struct rec *)ADDR).m, written and read (#378: the store went
 * through the address of a temporary holding the pointer)
 *
 * Expected at $C0C0: 65 01 78 56 34 12 66 03 65 01 AA
 */
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
struct rec { unsigned char a, b; unsigned long c; };
#define REC (*(struct rec *)0x1E40)
unsigned char guard = 0x5A;
#pragma cc45 recurse
void framed(void) {                     /* the same in a function with a stack frame */
    REC.b += 2;
}
void main() {
    REC.a = 0;
    REC.b = 0;
    REC.a = 0x65;
    REC.b = 1;
    REC.c = 0x12345678;
    R(0) = *(unsigned char *)0x1E40;
    R(1) = *(unsigned char *)0x1E41;
    R(2) = *(unsigned char *)0x1E42;
    R(3) = *(unsigned char *)0x1E43;
    R(4) = *(unsigned char *)0x1E44;
    R(5) = *(unsigned char *)0x1E45;
    REC.a++;
    framed();
    R(6) = REC.a;
    R(7) = REC.b;
    R(8) = guard == 0x5A ? 0x65 : 0;    /* nothing else written */
    R(9) = REC.c == 0x12345678 ? 1 : 0;
    R(10) = 0xAA;
}
