/* test_debug_file_xemu.c — the debug file (#202): built with cc45 -g,
 * test_xemu.sh reads the values this program leaves behind at the
 * addresses <program>.dbg gives its variables (globals, statics, a
 * function's static local, locals in static storage, struct members) and
 * checks its functions and source lines against the program.
 *
 * Results at $C0C0: [0] $AA when main finished.
 */
struct pt { int x; signed char y; struct pt *next; };

int counter;
static signed long total;
char name[8] = "lamb";
struct pt shared;

int add(int x, int y) { int result = x + y; counter += result; return result; }

int sum(struct pt *p) {
    static int calls;
    int s = 0;
    calls++;
    while (p) { s += p->x; p = p->next; }
    return s;
}

int main(void) {
    struct pt a, b;
    a.x = add(1, 2);
    a.y = -5;
    a.next = &b;
    b.x = 4;
    b.y = 6;
    b.next = 0;
    shared = b;
    total = sum(&a) + sum(&b);
    *(volatile unsigned char *)0xC0C0 = 0xAA;
    return (int)total;
}
