/* test_string_prefix_xemu.c — string and character literal encoding
 * prefixes (as ca45 and basic45): p"..." PETSCII, s"..." screen codes,
 * a"..." / @"..." ASCII. A prefix touches the opening quote and overrides
 * #pragma encoding for that literal; unprefixed string and character
 * literals follow the pragma.
 *
 * Results at $C0C0 (01 = pass):
 *   [0] p"..." / p'x': PETSCII ('a' $41, 'A' $C1)
 *   [1] s"..." / s'x': screen codes (@ 0, A-Z and a-z 1-26, [\]^_ 27-31)
 *   [2] a"..." / a'x' and @"...": raw ASCII
 *   [3] a prefix overrides #pragma encoding (in functions)
 *   [4] global initializers: pointers, pointer arrays, char arrays
 *   [5] constant expressions: enum, switch case, static char array
 *   [6] identifiers named p, s and a; concatenation; macros
 *   [7] escapes (\n is $0D in PETSCII and screen codes, $0A in ASCII)
 *   [8] character constants follow #pragma encoding (enum, switch case,
 *       global, local; a prefix overrides it)
 *   [9] $AA sentinel
 */
#define OUT ((volatile unsigned char *)0xC0C0)

static int same(const char *s, const unsigned char *want, int n) {
    for (int i = 0; i < n; i++)
        if ((unsigned char)s[i] != want[i]) return 0;
    return s[n] == 0;
}

/* [4] global initializers (the pragma is still PETSCII here) */
const char *g_s = s"AB@";
const char *g_p = p"aB";
const char *g_a = a"aB";
const char *g_tbl[] = { s"C", p"c", a"c", "c" };
char g_arr[] = s"XYZ";
char g_parr[4] = p"Ab";

/* [5] constant expressions */
enum { E_S = s'A', E_P = p'a', E_A = a'a', E_AT = s'@' };
static const unsigned char g_chars[] = { s'Z', p'z', a'z', s'_' };

static int classify(int c) {
    switch (c) {
    case s'B': return 1;    /* 2 */
    case p'b': return 2;    /* $42 */
    case a'b': return 3;    /* $62 */
    default:   return 0;
    }
}

/* [6] identifiers named like the prefixes */
static int p(int s) { return s + 1; }
static int a_fn(int a) { int s = a * 2; return s; }
#define ID(x) x
#define STR(x) #x

static int t_petscii(void) {
    static const unsigned char want[] = { 0x41, 0xC2, 0x40, 0x31, 0x5A, 0xDA };
    return same(p"aB@1zZ", want, 6) && p'a' == 0x41 && p'A' == 0xC1 && p'1' == 0x31 &&
           p'a' == 'a' && p'Z' == 'Z';
}

static int t_screen(void) {
    static const unsigned char want[] = { 0x01, 0x1A, 0x00, 0x1B, 0x1F, 0x20, 0x31, 0x01, 0x1A, 0x21 };
    return same(s"Az@[_ 1aZ!", want, 10) && s'A' == 1 && s'a' == 1 && s'@' == 0 &&
           s'z' == 26 && s'[' == 27 && s'_' == 31 && s' ' == 0x20 && s'0' == 0x30;
}

static int t_ascii(void) {
    static const unsigned char want[] = { 0x61, 0x42, 0x40 };
    return same(a"aB@", want, 3) && same(@"aB@", want, 3) && a'a' == 0x61 && a'A' == 0x41 &&
           @'a' == 0x61;
}

static int t_globals(void) {
    static const unsigned char ws[] = { 1, 2, 0 };
    static const unsigned char wp[] = { 0x41, 0xC2 };
    static const unsigned char wa[] = { 0x61, 0x42 };
    static const unsigned char wx[] = { 24, 25, 26 };
    return same(g_s, ws, 2) && g_s[2] == 0 && same(g_p, wp, 2) && same(g_a, wa, 2) &&
           g_tbl[0][0] == 3 && g_tbl[1][0] == 0x43 && g_tbl[2][0] == 0x63 && g_tbl[3][0] == 0x43 &&
           same(g_arr, wx, 3) && sizeof(g_arr) == 4 &&
           (unsigned char)g_parr[0] == 0xC1 && (unsigned char)g_parr[1] == 0x42 && g_parr[2] == 0;
}

static int t_const(void) {
    char loc[] = s"HI";
    return E_S == 1 && E_P == 0x41 && E_A == 0x61 && E_AT == 0 &&
           g_chars[0] == 26 && g_chars[1] == 0x5A && g_chars[2] == 0x7A && g_chars[3] == 31 &&
           classify(2) == 1 && classify(0x42) == 2 && classify(0x62) == 3 && classify(0x22) == 0 &&
           loc[0] == 8 && loc[1] == 9 && loc[2] == 0 && sizeof(loc) == 3;
}

static int t_names(void) {
    int s = 5, a = 7;
    int ok = p(s) == 6 && a_fn(a) == 14 && p (1) == 2;
    s = a; a = s + 1;
    ok = ok && s == 7 && a == 8;
    /* concatenation: the first literal's encoding is used */
    static const unsigned char w1[] = { 1, 2 };
    static const unsigned char w2[] = { 0xC1, 0xC2 };
    static const unsigned char w3[] = { 0x41, 0x62 };
    ok = ok && same(s"A" "B", w1, 2) && same("A" s"B", w2, 2) && same(a"A" "b", w3, 2);
    /* through macros: the prefix stays with the literal */
    ok = ok && ID(s"A")[0] == 1 && ID(s'B') == 2;
    const char *st = STR(s"A");          /* "s\"A\"" (PETSCII text) */
    ok = ok && st[0] == 0x53 && st[1] == '"' && st[2] == 0xC1 && st[4] == 0;
    return ok;
}

static int t_escapes(void) {
    static const unsigned char ws[] = { 0x0D, 0x0A, 0x41, 0x09, 0x01 };
    static const unsigned char wp[] = { 0x0D, 0x0A, 0x41 };
    static const unsigned char wa[] = { 0x0A, 0x0D, 0x41 };
    return same(s"\n\r\x41\tA", ws, 5) && same(p"\n\r\x41", wp, 3) && same(a"\n\r\x41", wa, 3) &&
           s'\n' == 0x0D && p'\n' == 0x0D && a'\n' == 0x0A && s'\x41' == 0x41 && s'\0' == 0;
}

/* [3] prefixes override the pragma */
#pragma encoding screencode
enum { E_SCR = 'A' };
static const char g_chr_scr = 'z';
static int t_chr_screen(void) {
    char c = 'b';
    return E_SCR == 1 && g_chr_scr == 26 && c == 2 && '@' == 0 && p'A' == 0xC1;
}
static int t_pragma_screen(void) {
    static const unsigned char w0[] = { 1, 2 };
    static const unsigned char wp[] = { 0xC1, 0x42 };
    static const unsigned char wa[] = { 0x41, 0x62 };
    return same("AB", w0, 2) && same(p"Ab", wp, 2) && same(a"Ab", wa, 2) && same(s"AB", w0, 2);
}
#pragma encoding ascii
static int chr_case(int c) {
    switch (c) { case 'a': return 1; case 'A': return 2; case '\n': return 3; }
    return 0;
}
static int t_chr_ascii(void) {
    char c = 'b';
    return 'A' == 0x41 && c == 0x62 && chr_case(0x61) == 1 && chr_case(0x41) == 2 &&
           chr_case(0x0A) == 3 && s'A' == 1;
}
static int t_pragma_ascii(void) {
    static const unsigned char w0[] = { 0x41, 0x62 };
    static const unsigned char wp[] = { 0xC1, 0x42 };
    static const unsigned char ws[] = { 1, 2 };
    return same("Ab", w0, 2) && same(p"Ab", wp, 2) && same(s"Ab", ws, 2);
}
const char *g_pragma_p = p"Ab";   /* global pointer under the ascii pragma */
const char *g_pragma_d = "Ab";
#pragma encoding petscii
static int t_chr_petscii(void) { return 'A' == 0xC1 && 'a' == 0x41 && '\n' == 0x0D; }

int main(void) {
    static const unsigned char wp[] = { 0xC1, 0x42 };
    static const unsigned char wd[] = { 0x41, 0x62 };
    OUT[0] = t_petscii();
    OUT[1] = t_screen();
    OUT[2] = t_ascii();
    OUT[3] = t_pragma_screen() && t_pragma_ascii() && same(g_pragma_p, wp, 2) && same(g_pragma_d, wd, 2);
    OUT[4] = t_globals();
    OUT[5] = t_const();
    OUT[6] = t_names();
    OUT[7] = t_escapes();
    OUT[8] = t_chr_screen() && t_chr_ascii() && t_chr_petscii();
    OUT[9] = 0xAA;
    return 0;
}
