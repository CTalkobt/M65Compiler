/* format.c — Output formatting
 *
 * Demonstrates: extern struct type usage, printf, separate compilation.
 * This file includes stdio.h but vec.c does not — each module only
 * pulls in what it needs.
 */

#include <stdio.h>
#include "util.h"

void print_vec(char *label, vec2d v) {
    printf("%s: (%d, %d)\n", label, v.x, v.y);
}

void print_int(char *label, int value) {
    printf("%s: %d\n", label, value);
}
