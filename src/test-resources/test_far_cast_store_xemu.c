/* test_far_cast_store_xemu.c — stores through a cast to a far pointer in a
 * file with no far pointer variable: the far code was only turned on when the
 * cast's type was first mapped, after the assignment had taken the near path
 * ("an object reached through a far pointer has no near address")
 *   [0] a byte stored in bank 4 and read back
 *   [1] a 16-bit value the same way
 *   [2] a byte stored at a variable's 28-bit address (bank 0), read as the variable
 *   [3] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static volatile unsigned char v;

int main(void) {
    unsigned char c = 0x5A;
    *(volatile unsigned char __far *)0x40100UL = c;
    *(volatile unsigned int __far *)0x40102UL = 0x1234;
    *(volatile unsigned char __far *)(unsigned long)(unsigned int)&v = 0x77;
    OUT[0] = *(volatile unsigned char __far *)0x40100UL == 0x5A;
    OUT[1] = *(volatile unsigned int __far *)0x40102UL == 0x1234;
    OUT[2] = v == 0x77;
    OUT[3] = 0xAA;
    return 0;
}
