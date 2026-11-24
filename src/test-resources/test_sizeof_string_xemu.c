/* test_sizeof_string_xemu.c — sizeof of a string literal inside a larger
 * integer constant expression (#749): array sizes, enum values,
 * _Static_assert, case labels, as in an initializer and at run time
 *
 * Results at $C0C0 (01 = pass):
 *   [0] file-scope arrays: 10 - sizeof "abc", sizeof("abc") + 1, Rogue's
 *       MAXMSG (NUMCOLS - sizeof "--More--"), 2 * MAXMSG + 1
 *   [1] enum values, a case label
 *   [2] escapes ("a\n" 3, "\x41\0" 3), concatenation ("ab" "cd" 5), an
 *       empty string (1), L"ab" (6), an encoding prefix (s"ab" 3)
 *   [3] the same in a block-scope array and an initializer
 *   [4] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

#define NUMCOLS 80
#define MAXMSG (NUMCOLS - sizeof "--More--")

static char a[10 - sizeof "abc"];
static char b[sizeof("abc") + 1];
static char msgbuf[2 * MAXMSG + 1];

enum { N = sizeof "abc", M = sizeof "abc" * 2 - 1 };

_Static_assert(sizeof "abc" == 4, "sizeof a string literal");
_Static_assert(sizeof "abc" - 1 == 3, "in a larger expression");
_Static_assert(MAXMSG == 71, "Rogue's MAXMSG");

static char esc1[sizeof "a\n" * 10];
static char esc2[sizeof "\x41\0" + 0];
static char cat[sizeof "ab" "cd" + 0];
static char empty[sizeof "" + 0];
static char wide[sizeof L"ab" + 0];
static char scr[sizeof s"ab" + 0];

static int sel(int v) {
    switch (v) {
    case sizeof "abc" - 1: return 1;
    default: return 0;
    }
}

int main(void) {
    char local[10 - sizeof "abc"];
    static const unsigned int init = sizeof "abc" - 1;
    (void)local;
    OUT[4] = 0;
    OUT[0] = sizeof a == 6 && sizeof b == 5 && MAXMSG == 71 && sizeof msgbuf == 143;
    OUT[1] = N == 4 && M == 7 && sel(3) && !sel(4);
    OUT[2] = sizeof esc1 == 30 && sizeof esc2 == 3 && sizeof cat == 5 && sizeof empty == 1 &&
             sizeof wide == 6 && sizeof scr == 3;
    OUT[3] = sizeof local == 6 && init == 3;
    OUT[4] = 0xAA;
    return 0;
}
