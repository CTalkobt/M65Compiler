/* test_derefptr_xemu.c — dereferencing a pointer to a pointer (Issue #303):
 * **p of a char ** reads one char (it read 16 bits: the next byte became
 * the high byte, and a compare was 16-bit)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] **p < 'x' through a const char ** (the issue's case), the byte after
 *       the character not 0
 *   [1] **p returned as a char; *p + 1 steps one char
 *   [2] three levels: ***q of a char ***, local and global
 *   [3] a global char **: **g, and (*g)[2]
 *   [4] int **: still 16 bits
 *   [5] a pointer to a pointer to a struct: (*pp)->b, (**pp).a
 *   [6] an array of strings: **(names + 1), *names[2], names[1][1]
 *   [7] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)
#define NOINLINE __attribute__((noinline))

static const char text[] = "aZqy";       /* 'a' then 'Z' (non-zero) after it */

NOINLINE static int below(const char **p) { return **p < 'x'; }
NOINLINE static int below_q(const char **p) { return **p < 'b'; }
NOINLINE static char first(char **p) { return **p; }
NOINLINE static char second(char **p) { return *(*p + 1); }
NOINLINE static char three(char ***q) { return ***q; }

char buf[] = "hello";
char *bp = buf;
char **g = &bp;
char ***t = &g;

NOINLINE static int wide(int **p) { return **p; }

struct pt { unsigned char a; unsigned int b; };

NOINLINE static unsigned int field(struct pt **pp) { return (*pp)->b + (**pp).a; }

static const char *names[] = { "zero", "one", "two" };

int main(void) {
    const char *s = text + 2;            /* "qy": 'q' < 'x' */
    const char *u = text + 3;            /* "y" */
    const char *a = text;
    char *m = buf;
    char *mp[1];
    char **mpp = mp;
    int n = 0x1234, *np = &n;
    struct pt v = { 7, 300 }, *vp = &v;

    OUT[0] = below(&s) == 1 && below(&u) == 0 && below_q(&a) == 1;
    OUT[1] = first(&m) == 'h' && second(&m) == 'e';
    mp[0] = buf + 1;
    OUT[2] = three(&mpp) == 'e' && ***t == 'h' && three(t) == 'h';
    OUT[3] = **g == 'h' && (*g)[2] == 'l';
    OUT[4] = wide(&np) == 0x1234;
    OUT[5] = field(&vp) == 307;
    OUT[6] = **(names + 1) == 'o' && *names[2] == 't' && names[1][1] == 'n';
    OUT[7] = 0xAA;
    return 0;
}
