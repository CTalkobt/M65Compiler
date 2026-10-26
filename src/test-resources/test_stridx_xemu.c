/* test_stridx_xemu.c — stridx(s, "a", "b", ..., NULL) (#79): which of
 * several strings s is (1-based, 0 for none). Built at -O2 (a table and
 * __stridx_table) and at -O3 (an inline trie) by test_xemu.sh
 *
 * Results at $C0C0 (01 = pass):
 *   [0] the commands of a little shell: each found, an unknown one 0
 *   [1] prefixes: "sub", "substring", "substance", "su" (the longest match
 *       is not taken: the whole string must be equal); empty string and ""
 *   [2] the first of equal candidates; no NULL written (literals)
 *   [3] the library: candidates in variables (not literals), with and
 *       without the NULL (one is added)
 *   [4] a literal subject: the index at compile time
 *   [5] other encodings: a"..." (ASCII) candidates for ASCII text, and
 *       #pragma encoding screencode
 *   [6] in a switch, as the issue's example
 *   [7] $AA sentinel
 */
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static int command(const char *cmd) {
    return stridx(cmd, "load", "save", "quit", "list", NULL);
}

int main(void) {
    char w[12];
    const char *p = w;
    const char *c1 = "red", *c2 = "green";
    unsigned char ok;

    OUT[0] = command("load") == 1 && command("save") == 2 && command("quit") == 3 && command("list") == 4 &&
             command("lis") == 0 && command("lists") == 0 && command("") == 0;

    ok = 1;
    strcpy(w, "substance");
    if (stridx(p, "sub", "substring", "substance", "su", NULL) != 3) ok = 0;
    strcpy(w, "su");
    if (stridx(p, "sub", "substring", "substance", "su", NULL) != 4) ok = 0;
    strcpy(w, "subs");
    if (stridx(p, "sub", "substring", "substance", "su", NULL) != 0) ok = 0;
    w[0] = 0;
    if (stridx(p, "a", "", "b", NULL) != 2 || stridx(p, "a", "b", NULL) != 0) ok = 0;
    OUT[1] = ok;

    strcpy(w, "twice");
    OUT[2] = stridx(p, "once", "twice", "twice") == 2 && stridx(p, "x") == 0;

    strcpy(w, "green");
    OUT[3] = stridx(p, c1, c2, NULL) == 2 && stridx(p, c1, c2) == 2 && stridx(p, c1, NULL) == 0;

    OUT[4] = stridx("quit", "load", "quit", NULL) == 2 && stridx("no", "load", NULL) == 0;

    ok = 1;
    w[0] = 0x6F; w[1] = 0x6B; w[2] = 0;                 /* "ok" in ASCII */
    if (stridx(p, a"no", a"ok", NULL) != 2 || stridx(p, "no", "ok", NULL) != 0) ok = 0;
#pragma encoding screencode
    w[0] = 15; w[1] = 11; w[2] = 0;                     /* "ok" in screen codes */
    if (stridx(p, "no", "ok", NULL) != 2) ok = 0;
#pragma encoding petscii
    OUT[5] = ok;

    strcpy(w, "save");
    switch (stridx(p, "load", "save", "quit", NULL)) {
        case 1: ok = 0; break;
        case 2: ok = 1; break;
        default: ok = 0;
    }
    OUT[6] = ok;
    OUT[7] = 0xAA;
    return 0;
}
