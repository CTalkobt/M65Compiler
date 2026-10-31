#pragma cc45 no_zp_save
/* test_paren_declarator_xemu.c — a declarator's name in parentheses (#376):
 * void (f)(int) declares f, and keeps a function-like macro f from expanding
 * there (a macro that wraps the function of its name); __fastcall__ before
 * or after the return type
 *
 * Expected at $C0C0: 01 03 02 05 07 09 0B AA
 */
#define R(i) (*(volatile unsigned char *)(0xC0C0 + (i)))
unsigned char calls, seen;
__fastcall__ void g(void) { calls++; }
void (f)(unsigned char x);                  /* declared before the macro */
#define f(x) (g(), (f)(x))
void (f)(unsigned char x) { seen = x; }     /* defined after it */
void __fastcall__ (h)(unsigned char x) { R(3) = x; }
static unsigned char ((twice))(unsigned char x) { return x + x; }
unsigned char (v) = 7, *(pv) = &v;
unsigned char add(unsigned char (a), unsigned char (b)) { return a + b; }
void main() {
    f(3);
    R(0) = calls;
    R(1) = seen;
    R(2) = (unsigned char)(calls + 1);
    h(5);
    {
        unsigned char (w) = 9;
        R(4) = *pv;
        R(5) = w;
    }
    R(6) = add(twice(2), 7);
    R(7) = 0xAA;
}
