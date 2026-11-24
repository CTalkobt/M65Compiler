/* __builtin_trap() ends the program (#761): exit status $FFFE (as abort()),
 * no code after it, no atexit handler and no user abort() run.
 * $C0C0: 01 01 01 AA 00 — [4] would be $EE had anything run after the trap;
 * test_xemu.sh also checks the status $FFFE at $FD/$FE. */
#include <stdlib.h>

static volatile unsigned char *R = (unsigned char *)0xC0C0;   /* written only: $C000-$CFFF reads as ROM */
static volatile int one = 1;

/* a user abort(): __builtin_trap must not call it */
void abort(void) { R[4] = 0xEE; for (;;) ; }

static void on_exit_handler(void) { R[4] = 0xEE; }

static int checked(int x) {
    if (x == 0) __builtin_trap();     /* not taken */
    return x + 1;
}

int main(void) {
    R[0] = 1;
    atexit(on_exit_handler);
    R[1] = checked(one) == 2;
    R[2] = 1;
    R[3] = 0xAA;
    __builtin_trap();
    R[4] = 0xEE;
    return 0;
}
