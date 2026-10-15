/* atexit.c — Register functions to call at program exit
 *
 * exit() and a return from main call __atexit_run (crt0 has a weak empty
 * one for programs that never call atexit). */

#define ATEXIT_MAX 32

static void (*__atexit_funcs[ATEXIT_MAX])();
static unsigned char __atexit_count;

int atexit(void (*func)()) {
    if (__atexit_count >= ATEXIT_MAX) return 1; /* table full */
    __atexit_funcs[__atexit_count++] = func;
    return 0;
}

/* Called by exit() and after main returns: the handlers in reverse order of
 * registration, each once (a handler may call exit or atexit) */
void __atexit_run(void) {
    while (__atexit_count) __atexit_funcs[--__atexit_count]();
}
