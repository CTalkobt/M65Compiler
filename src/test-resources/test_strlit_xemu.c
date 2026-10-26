/* test_strlit_xemu.c — string functions on literals (#78): folded at
 * compile time or turned into byte compares, with the library's results
 *
 * Results at $C0C0 (01 = pass):
 *   [0] strlen of literals: "", "hello", "a\nb", "ab\0cd" (2), s"@x" (the
 *       screen code of @ is 0: 0), through const char s[] and a
 *       const char *const, a local const char *p never assigned
 *   [1] strcmp / strncmp of two literals: -1 / 0 / 1 as the library
 *       gives (unsigned bytes: "a" vs "\x80"), strncmp lengths 0, 2, 9
 *   [2] strcmp(x, "lit") == 0 / != 0 / ! / in if and while: x equal,
 *       a prefix of it, longer, empty
 *   [3] strncmp(x, "lit", n) == 0 with n shorter and longer than the literal
 *   [4] the same compares in the other encodings: a"..." (ASCII) and
 *       #pragma encoding screencode
 *   [5] strcpy(d, "lit") (a memcpy): the bytes and the terminator, its value d
 *   [6] not folded: a pointer that is assigned, a literal longer than 8
 *       (the library), an ordering use (strcmp(x, "m") < 0), a file whose
 *       own function is named like one (static my_strlen: not touched)
 *   [7] $AA sentinel
 */
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

const char greeting[] = "hello";
const char *const motto = "kept";
char buf[16];

static int sign(int v) { return v < 0 ? -1 : v > 0 ? 1 : 0; }

int main(void) {
    unsigned char ok;
    char word[8];
    const char *w = word;
    const char *moving = "abc";
    char *r;

    {
        const char *p = "four";
        ok = strlen("") == 0 && strlen("hello") == 5 && strlen("a\nb") == 3 && strlen("ab\0cd") == 2 &&
             strlen(s"@x") == 0 && strlen(greeting) == 5 && strlen(motto) == 4 && strlen(p) == 4;
        OUT[0] = ok;
    }

    OUT[1] = strcmp("abc", "abd") == -1 && strcmp("abd", "abc") == 1 && strcmp("abc", "abc") == 0 &&
             strcmp("a", "\x80") == -1 && strcmp("", "a") == -1 && strncmp("abc", "abd", 2) == 0 &&
             strncmp("abc", "abd", 0) == 0 && strncmp("abc", "abd", 9) == -1 && strcmp(greeting, "help") == -1;

    ok = 1;
    strcpy(word, "GO");
    if (strcmp(w, "GO") != 0) ok = 0;
    if (!(strcmp(w, "GO") == 0)) ok = 0;
    if (strcmp(w, "G") == 0) ok = 0;          /* a prefix of it */
    if (!strcmp(w, "GOO")) ok = 0;            /* longer */
    if (!strcmp(w, "")) ok = 0;               /* empty */
    if (strcmp(w, "GO")) ok = 0;
    {
        unsigned char n = 0;
        while (strcmp(w, "GO") == 0 && n < 3) n++;
        if (n != 3) ok = 0;
    }
    {
        int eq = strcmp(w, "GO") == 0, ne = strcmp(w, "GX") != 0, nn = !strcmp(w, "GO");
        if (eq != 1 || ne != 1 || nn != 1) ok = 0;
    }
    OUT[2] = ok;

    OUT[3] = strncmp(w, "GOAT", 2) == 0 && strncmp(w, "GOAT", 3) != 0 && strncmp(w, "G", 5) != 0 &&
             strncmp(w, "GO", 9) == 0;

    ok = 1;
    word[0] = 0x41; word[1] = 0x42; word[2] = 0;          /* "AB" in ASCII */
    if (strcmp(w, a"AB") != 0 || strcmp(w, "AB") == 0) ok = 0;
#pragma encoding screencode
    word[0] = 1; word[1] = 2; word[2] = 0;                /* "AB" in screen codes */
    if (strcmp(w, "AB") != 0) ok = 0;
#pragma encoding petscii
    OUT[4] = ok;

    r = strcpy(buf, "copy me");
    OUT[5] = r == buf && buf[0] == 'c' && buf[6] == 'e' && buf[7] == 0 && strlen(buf) == 7;

    ok = 1;
    if (strlen(moving) != 3) ok = 0;
    moving = "longer one";
    if (strlen(moving) != 10) ok = 0;                     /* assigned: not folded */
    strcpy(word, "abc");
    if (strcmp(w, "abc") != 0) ok = 0;
    if (strcmp(buf, "copy me, longer") == 0) ok = 0;      /* 16 bytes: the library */
    if (!(strcmp(w, "m") < 0) || sign(strcmp(w, "ab")) != 1) ok = 0;
    OUT[6] = ok;
    OUT[7] = 0xAA;
    return 0;
}
