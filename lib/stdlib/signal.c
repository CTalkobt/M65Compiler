/* signal.c — signal, raise, and an abort that raises SIGABRT (<signal.h>)
 *
 * The abort here replaces the library's weak one (stdlib_asm/abort.s45)
 * when a program uses signal or raise: a SIGABRT handler runs first. */

#pragma cc45 signed_default

#include <signal.h>
#include <stdlib.h>
#include <errno.h>

/* crt0's end of an aborted program (exit status $FFFE) */
_Noreturn void _abort(void);

static __sigfunc handlers[__SIG_COUNT];

__sigfunc signal(int sig, __sigfunc func) {
    __sigfunc old;
    if (sig < 0 || sig >= __SIG_COUNT || func == SIG_ERR) {
        errno = EINVAL;
        return SIG_ERR;
    }
    old = handlers[sig];
    handlers[sig] = func;
    return old;
}

int raise(int sig) {
    __sigfunc h;
    if (sig < 0 || sig >= __SIG_COUNT) return -1;
    h = handlers[sig];
    if (h == SIG_IGN) return 0;
    if (h == SIG_DFL) {
        if (sig == SIGABRT) _abort();
        _exit(EXIT_FAILURE);
    }
    handlers[sig] = SIG_DFL;
    h(sig);
    return 0;
}

__fastcall__ _Noreturn void abort(void) {
    raise(SIGABRT);
    _abort();
}
