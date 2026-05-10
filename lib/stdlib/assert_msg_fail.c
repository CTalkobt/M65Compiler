/* assert_msg_fail.c — Runtime handler for assert_msg macro (stack convention)
 *
 * Called by assert_msg() when an assertion fails.
 * Prints assertion condition, file:line, and custom diagnostic message,
 * then halts execution.
 */

#include <stdio.h>

/* Declared in assert.h; halts execution */
extern void assert_fail_abort(void);

void assert_msg_fail(const char *expr, const char *file, int line, const char *msg) {
    printf("Assertion failed: %s (%s:%d)\n", expr, file, line);
    printf("  Message: %s\n", msg);
    assert_fail_abort();
}
