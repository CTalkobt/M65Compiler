/* test_stdio3_xemu.c — the scanf family on files, setvbuf, setbuf,
 * tmpfile, tmpnam, freopen
 *
 * Run by test_xemu.sh with an empty D81 attached as drive 8 (xemu -8).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] fscanf (integers, %s, %[...], %n), vfscanf, ungetc of the next
 *       character, EOF at the end; sscanf %f (the float scanf is linked)
 *   [1] setvbuf (_IONBF, own buffer, refused after I/O, a bad mode), setbuf,
 *       tmpfile, tmpnam, freopen
 *   [2] $AA sentinel
 */
/* Standard C signedness */
#pragma cc45 signed_default

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static int vfs(FILE *f, const char *fmt, ...) {
    va_list ap;
    int r;
    va_start(ap, fmt);
    r = vfscanf(f, fmt, ap);
    va_end(ap);
    return r;
}

static int t_scanf(void) {
    FILE *f;
    signed int a, b, n;
    char w[16], t[16];
    float x;
    int ok;
    f = fopen("scan,s,w", "w");
    if (!f) return 0;
    fputs("12 -34 word abc;9\n77 tail", f);
    fclose(f);
    f = fopen("scan,s,r", "r");
    if (!f) return 0;
    ok = fscanf(f, "%d %d %s %[a-z]%n;", &a, &b, w, t, &n) == 4 && a == 12 && b == -34 &&
         strcmp(w, "word") == 0 && strcmp(t, "abc") == 0 && n == 15;
    /* the character after the number stays in the stream (ungetc) */
    ok = ok && vfs(f, "%d", &a) == 1 && a == 9 && fgetc(f) == '\n';
    ok = ok && fscanf(f, "%d %s", &a, w) == 2 && a == 77 && strcmp(w, "tail") == 0;
    ok = ok && fscanf(f, "%d", &a) == EOF;
    fclose(f);
    ok = ok && sscanf("1.5e1", "%f", &x) == 1 && x == 15.0;
    return ok;
}

static int t_buffers(void) {
    static char mybuf[64];
    char name[L_tmpnam];
    char *nm;
    FILE *f;
    int ok, i;
    /* unbuffered, then an own buffer */
    f = fopen("vbuf,s,w", "w");
    if (!f) return 0;
    ok = setvbuf(f, 0, _IONBF, 0) == 0;
    fputs("ab", f);
    ok = ok && setvbuf(f, mybuf, _IOFBF, sizeof mybuf) == -1;  /* after output */
    fclose(f);
    f = fopen("vbuf,s,r", "r");
    if (!f) return 0;
    ok = ok && setvbuf(f, mybuf, _IOFBF, sizeof mybuf) == 0 && setvbuf(f, 0, 7, 0) == -1;
    ok = ok && fgetc(f) == 'a' && fgetc(f) == 'b' && fgetc(f) == EOF;
    fclose(f);
    f = fopen("vbuf,s,r", "r");
    setbuf(f, 0);
    ok = ok && f && fgetc(f) == 'a';
    fclose(f);
    /* tmpfile: a read/write stream in memory */
    f = tmpfile();
    if (!f) return 0;
    for (i = 0; i < 300; i++) fputc(i & 0xFF, f);
    rewind(f);
    for (i = 0; i < 300; i++) if (fgetc(f) != (i & 0xFF)) ok = 0;
    ok = ok && fgetc(f) == EOF;
    fclose(f);
    /* tmpnam: names no file has */
    nm = tmpnam(name);
    ok = ok && nm == name && strncmp(name, "TMP", 3) == 0 && strlen(name) == 7;
    f = fopen(name, "r");
    ok = ok && f == 0;
    f = fopen(name, "w");
    if (f) { fputs("t", f); fclose(f); }
    nm = tmpnam(0);
    ok = ok && nm && strcmp(nm, name) != 0;
    /* freopen: the same FILE on another file */
    f = fopen("vbuf,s,r", "r");
    f = freopen("scan,s,r", "r", f);
    ok = ok && f && fgetc(f) == '1';
    if (f) fclose(f);
    return ok;
}

int main(void) {
    OUT[0] = t_scanf();
    OUT[1] = t_buffers();
    OUT[2] = 0xAA;
    return 0;
}
