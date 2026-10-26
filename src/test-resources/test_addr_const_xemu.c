/* test_addr_const_xemu.c — static initializers that use an object's
 * address as a truth value: an address is never null, so &x != 0 is 1,
 * &x == 0 and !&x are 0, (_Bool)&x and a _Bool initialized with an
 * address are 1 (they stored the address's low byte)
 *
 * Results at $C0C0: 01 01 00 01 01 01 00 01 05 01 01 (g1 .. g10, s1)
 */
#define OUT ((volatile unsigned char *)0xC0C0)
int x;
char buf[4];
static void fn(void) {}
_Bool g1 = &x != 0;
_Bool g2 = &x;
int g3 = &x == 0;
_Bool g4 = (&x != 0) && 1;
_Bool g5 = buf;
_Bool g6 = fn;
int g7 = !&x;
_Bool g8 = "str";
int g9 = &x ? 5 : 6;
static _Bool g10 = (_Bool)&buf[2];
int main(void) {
    static _Bool s1 = &x;
    OUT[0]=g1; OUT[1]=g2; OUT[2]=g3; OUT[3]=g4; OUT[4]=g5; OUT[5]=g6; OUT[6]=g7; OUT[7]=g8; OUT[8]=g9; OUT[9]=g10; OUT[10]=s1;
    return 0;
}
