/* test_long_strings_xemu.c — the string functions on strings longer than
 * 255 bytes (an 8-bit Y loop wraps there)
 *
 * Results at $C0C0 (01 = pass):
 *   [0] strlen   [1] strcpy  [2] strcat  [3] strcmp (and its sign: it
 *   returns signed int)  [4] strchr  [5] strrchr  [6] strncpy  [7] strncat
 *   [8] strncmp  [9] puts: all 300 characters are on the screen (screen
 *   RAM at $0800, 80 columns: wrapped lines are contiguous)
 *   [10] memset/memcpy/memcmp over 300 bytes   [11] $AA
 */
#include <string.h>
#include <stdio.h>
#define OUT ((volatile unsigned char *)0xC0C0)
#define N 300

static char a[N + 1], b[N + 1], c[2 * N + 2], m1[N], m2[N];

/* the screen codes of s (PETSCII letters $41-$5A are 1-26) as one run */
static unsigned char on_screen(const char *s, unsigned int n) {
    const unsigned char *scr = (const unsigned char *)0x0800;
    unsigned int p, i;
    for (p = 0; p + n <= 2000; p++) {
        for (i = 0; i < n; i++)
            if (scr[p + i] != (unsigned char)(s[i] - 0x40)) break;
        if (i == n) return 1;
    }
    return 0;
}

int main(void) {
    unsigned int i;
    for (i = 0; i < N; i++) a[i] = (char)('a' + i % 26);
    a[N] = 0;

    OUT[0] = strlen(a) == N;
    strcpy(b, a);
    OUT[1] = b[N - 1] == a[N - 1] && b[N] == 0 && memcmp(a, b, N + 1) == 0;
    strcpy(c, a);
    strcat(c, a);
    OUT[2] = strlen(c) == 2 * N && c[N] == 'a' && c[2 * N] == 0;
    b[N - 2] = 'z' + 0;       /* differs only past 255 */
    b[N - 2]++;
    OUT[3] = strcmp(a, b) < 0 && strcmp(b, a) > 0 && strcmp(a, a) == 0;
    a[280] = '#';
    OUT[4] = strchr(a, '#') == a + 280;
    OUT[5] = strrchr(a, '#') == a + 280 && strrchr(a, 'a') == a + 286;
    a[280] = (char)('a' + 280 % 26);
    memset(b, 'x', N);
    b[N] = 0;
    strncpy(b, a, 290);
    OUT[6] = b[289] == a[289] && b[290] == 'x';
    strcpy(c, a);
    strncat(c, a, 270);
    OUT[7] = strlen(c) == N + 270 && c[N + 269] == a[269];
    strcpy(b, a);
    b[270] = 'Z';
    OUT[8] = strncmp(a, b, 260) == 0 && strncmp(a, b, 280) != 0;
    OUT[9] = puts(a) == 0 && on_screen(a, N);
    memset(m1, 0x5A, N);
    memcpy(m2, m1, N);
    OUT[10] = m2[N - 1] == 0x5A && memcmp(m1, m2, N) == 0;
    OUT[11] = 0xAA;
    return 0;
}
