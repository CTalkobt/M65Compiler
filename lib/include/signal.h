/* signal.h — signals (C99) for cc45 / MEGA65
 *
 * Nothing raises a signal but raise and abort (abort raises SIGABRT in a
 * program that uses signal). raise calls the handler signal installed,
 * first putting back SIG_DFL; SIG_IGN does nothing; SIG_DFL ends the
 * program: SIGABRT as abort does, the others with _exit(EXIT_FAILURE).
 */
#pragma once

typedef unsigned char sig_atomic_t;
typedef void (*__sigfunc)(int);

#define SIGABRT     0
#define SIGFPE      1
#define SIGILL      2
#define SIGINT      3
#define SIGSEGV     4
#define SIGTERM     5
#define SIGUSR1     6
#define SIGUSR2     7
#define __SIG_COUNT 8

#define SIG_DFL     ((__sigfunc)0)
#define SIG_IGN     ((__sigfunc)1)
#define SIG_ERR     ((__sigfunc)-1)

/* func for signal sig from now on; the earlier one, or SIG_ERR (errno
 * EINVAL) for an unknown signal */
__sigfunc signal(int sig, __sigfunc func);
/* 0, or nonzero for an unknown signal */
int raise(int sig);
