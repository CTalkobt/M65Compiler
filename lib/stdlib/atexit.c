/* atexit.c — Register functions to call at program exit */

/* Table of registered exit handlers (called in reverse order) */
static void (*__atexit_funcs[8])();
static int __atexit_count = 0;

int atexit(void (*func)()) {
    if (__atexit_count >= 8) return 1; /* table full */
    __atexit_funcs[__atexit_count++] = func;
    return 0;
}

/* Called by exit() before termination */
void __atexit_run(void) {
    int i;
    for (i = __atexit_count - 1; i >= 0; i--) {
        __atexit_funcs[i]();
    }
}
