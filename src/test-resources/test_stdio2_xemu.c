/* test_stdio2_xemu.c — the stdio functions test_stdio does not reach
 *
 * Run by test_xemu.sh with an empty D81 attached as drive 8 (xemu -8).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] sscanf: %d %s %x %c %u, the count of conversions, a short input
 *   [1] vsnprintf: the full length returned, the output cut and terminated
 *   [2] vfprintf to a file, read back
 *   [3] perror writes "s: <strerror(errno)>" and a newline (the screen row
 *       compared with the same text written by fputs)
 *   [4] getchar returns keys from the KERNAL keyboard buffer ($02B0, count
 *       at $00D0: absolute addresses below $0100)
 *   [5] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static char buf[64];

static int vsn(char *dst, int size, char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vsnprintf(dst, size, fmt, ap);
    va_end(ap);
    return n;
}

static int vf(FILE *f, char *fmt, ...) {
    va_list ap;
    int n;
    va_start(ap, fmt);
    n = vfprintf(f, fmt, ap);
    va_end(ap);
    return n;
}

int main(void) {
    signed int d = 0;
    unsigned int x = 0, u = 0;
    char c = 0, w[16];
    FILE *f;
    int n, ok;

    /* [0] */
    n = sscanf("  -12 word 7f Q 65535", "%d %s %x %c %u", &d, w, &x, &c, &u);
    ok = n == 5 && d == -12 && strcmp(w, "word") == 0 && x == 0x7F && c == 'Q' && u == 65535u;
    n = sscanf("42", "%d %d", &d, &u);
    OUT[0] = ok && n == 1 && d == 42;

    /* [1] */
    memset(buf, 'z', sizeof buf);
    n = vsn(buf, 8, "%s-%d", "abcdef", 1234);
    ok = n == 11 && strcmp(buf, "abcdef-") == 0 && buf[8] == 'z';
    n = vsn(buf, sizeof buf, "%u/%x", 7u, 255u);
    OUT[1] = ok && n == 4 && strcmp(buf, "7/ff") == 0;

    /* [2] */
    ok = 0;
    f = fopen("VF", "w");
    if (f) {
        n = vf(f, "n=%d s=%s", -7, "ok");
        fclose(f);
        f = fopen("VF", "r");
        if (f) {
            ok = n == 9 && fgets(buf, sizeof buf, f) && strcmp(buf, "n=-7 s=ok") == 0;
            fclose(f);
        }
    }
    OUT[2] = ok;

    /* [3] the row perror wrote equals the row fputs wrote */
    {
        volatile unsigned char *scr = (volatile unsigned char *)
            (*(volatile unsigned char *)0xD060 | (*(volatile unsigned char *)0xD061 << 8));
        unsigned int step = *(volatile unsigned char *)0xD058 | (*(volatile unsigned char *)0xD059 << 8);
        unsigned int i, len;
        putchar(147);               /* clear the screen: rows 0 and 1 */
        errno = EDOM;
        perror("ab");
        fputs("ab: ", stderr);
        fputs(strerror(EDOM), stderr);
        fputc('\n', stderr);
        len = 4 + strlen(strerror(EDOM));
        ok = scr[0] != ' ' && scr[0] != 0x20 ? 1 : 0;
        for (i = 0; i < len; i++)
            if (scr[i] != scr[step + i]) ok = 0;
        OUT[3] = ok && len > 4;
    }

    /* [4] */
    {
        volatile unsigned char *keyd = (volatile unsigned char *)0x02B0;
        volatile unsigned char *ndx = (volatile unsigned char *)0x00D0;
        keyd[0] = 'Q'; keyd[1] = 'z'; keyd[2] = '7';
        *ndx = 3;
        n = getchar();
        ok = n == 'Q';
        n = getchar();
        ok = ok && n == 'z';
        OUT[4] = ok && getchar() == '7' && *ndx == 0;
    }

    OUT[5] = 0xAA;
    return 0;
}
