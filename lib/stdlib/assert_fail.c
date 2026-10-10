/* assert_fail.c — Runtime handler for the assert() macro
 *
 * Called by assert() when an assertion fails: prints the condition and
 * location, then halts execution.
 */
#include <stdio.h>

extern void assert_fail_abort(void);

void assert_fail(const char *expr, const char *file, int line) {
    printf("Assertion failed: %s (file %s, line %d)\n", expr, file, line);
    assert_fail_abort();
}
