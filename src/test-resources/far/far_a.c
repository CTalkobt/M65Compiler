/* far_a.c — __far functions; about 5 KB of far code, so far_b.c's goes to
 * another automatic overlay (8 KB windows) */
#include <overlay.h>
#include "far.h"

/* static, called only from this group */
static __far int sq(int x) { return x * x; }

__far int fa_add(int a, int b) { counter++; return a + b; }

/* far -> far (static), far -> resident */
__far int fa_sq_inc(int x) { return sq(x) + resident_inc(x); }

/* a call through a pointer from far code */
__far int fa_apply(int (*f)(int), int x) { return f(x) + 1; }

/* recursion bouncing between two automatic overlays */
#pragma cc45 recurse
__far int fa_rec(int n) { return n <= 0 ? 0 : 1 + fb_rec(n - 1); }

__far unsigned char fa_group(void) { return __ovl_current; }

/* the address of a byte of this code: where the CPU sees it */
__far __naked static unsigned char *here(void) {
    __asm__("lda #<a_mark");
    __asm__("ldx #>a_mark");
    __asm__("rts");
    __asm__("a_mark: .byte $41");
}
__far unsigned int fa_where(void) { return (unsigned int)here(); }

/* longjmp out of two overlays: far1 -> far2 -> longjmp to a resident setjmp */
__far void fa_throw_via_b(void) { fb_throw(); counter = -1; }

/* setjmp in far code: far2 longjmps back here, and far1 must be mapped again
   (the code after setjmp is this function's) */
__far int fa_catch(void) {
    if (setjmp(jb)) return fa_group() == 1 ? 7 : 8;
    fb_throw();
    return 9;
}

__far __naked void fa_filler(void) {
    __asm__("rts");
    __asm__(".res 5000");
}
