/* main.c — Multi-module example entry point
 *
 * Demonstrates: multi-file project with shared header, separate
 * compilation, and linking. Each .c file compiles independently
 * to a .o45 object; ln45 links them together with the stdlib.
 *
 * Build:
 *   make              Build program.prg
 *   make run          Build and run in xemu
 *   make disasm       Generate disassembly listings
 */

#include <stdio.h>
#include "util.h"

static int test_count = 0;
static int pass_count = 0;

static void check(char *name, int condition) {
    test_count++;
    if (condition) {
        pass_count++;
    } else {
        printf("FAIL: %s\n", name);
    }
}

int main() {
    vec2d a;
    vec2d b;
    vec2d r;
    int d;

    puts("multi-module example");
    puts("--------------------");

    a.x = 3; a.y = 4;
    b.x = 1; b.y = 2;

    /* Test vec_add */
    r = vec_add(a, b);
    print_vec("add", r);
    check("add.x", r.x == 4);
    check("add.y", r.y == 6);

    /* Test vec_scale */
    r = vec_scale(a, 3);
    print_vec("scale", r);
    check("scale.x", r.x == 9);
    check("scale.y", r.y == 12);

    /* Test vec_dot */
    d = vec_dot(a, b);
    print_int("dot", d);
    check("dot", d == 11);

    /* Test vec_magnitude_sq */
    d = vec_magnitude_sq(a);
    print_int("mag_sq", d);
    check("mag_sq", d == 25);

    /* Summary */
    printf("\n%d/%d tests passed\n", pass_count, test_count);

    return 0;
}
