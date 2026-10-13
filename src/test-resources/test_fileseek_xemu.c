/* test_fileseek_xemu.c — fopen "r+"/"w+"/"a+" and fseek/ftell/rewind on a
 * disk (drive 8)
 *
 * Run by test_xemu.sh with an empty D81 attached as drive 8 (xemu -8).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] "w+": write, fseek back, read it back; ftell
 *   [1] "r+": overwrite in the middle; the file on disk has the change
 *   [2] "r": fseek forward, backward (reopen), SEEK_CUR, SEEK_END; ftell
 *   [3] "a+": writes go to the end, reads from anywhere
 *   [4] "w": fseek to where it is succeeds, elsewhere fails with ESPIPE
 *   [5] "r+" of a missing file is NULL; fseek past the end of a memory
 *       stream and a write there fills the gap with zeros
 *   [6] ungetc then fgetc keeps ftell right; rewind clears EOF
 *   [7] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static char buf[64];

static int read_all(const char *name, char *dst, int max) {
    FILE *f = fopen(name, "r");
    int n = 0, c;
    if (!f) return -1;
    while (n < max - 1 && (c = fgetc(f)) != EOF) dst[n++] = (char)c;
    dst[n] = 0;
    fclose(f);
    return n;
}

int main(void) {
    FILE *f;
    int ok;
    long p;

    /* [0] w+ */
    f = fopen("seek", "w+");
    ok = f != 0;
    if (f) {
        fputs("hello world", f);
        ok = ok && ftell(f) == 11;
        ok = ok && fseek(f, 0L, SEEK_SET) == 0 && ftell(f) == 0;
        buf[0] = 0;
        ok = ok && fgets(buf, sizeof buf, f) && strcmp(buf, "hello world") == 0;
        fclose(f);
    }
    OUT[0] = ok && read_all("seek", buf, sizeof buf) == 11 && strcmp(buf, "hello world") == 0;

    /* [1] r+ */
    f = fopen("seek", "r+");
    ok = f != 0;
    if (f) {
        ok = fseek(f, 6L, SEEK_SET) == 0;
        fputs("mega6", f);
        rewind(f);
        ok = ok && fgetc(f) == 'h';
        fclose(f);
    }
    OUT[1] = ok && read_all("seek", buf, sizeof buf) == 11 && strcmp(buf, "hello mega6") == 0;

    /* [2] r */
    f = fopen("seek", "r");
    ok = f != 0;
    if (f) {
        ok = ok && fseek(f, 6L, SEEK_SET) == 0 && fgetc(f) == 'm' && ftell(f) == 7;
        ok = ok && fseek(f, 1L, SEEK_SET) == 0 && fgetc(f) == 'e';            /* back: reopened */
        ok = ok && fseek(f, 2L, SEEK_CUR) == 0 && fgetc(f) == 'o';            /* 2 -> 4 */
        ok = ok && fseek(f, -1L, SEEK_END) == 0 && fgetc(f) == '6' && fgetc(f) == EOF;
        p = ftell(f);
        ok = ok && p == 11;
        fclose(f);
    }
    OUT[2] = ok;

    /* [3] a+ */
    f = fopen("seek", "a+");
    ok = f != 0;
    if (f) {
        ok = fseek(f, 0L, SEEK_SET) == 0;
        fputc('!', f);                                  /* at the end anyway */
        ok = ok && fseek(f, 0L, SEEK_SET) == 0 && fgetc(f) == 'h';
        fclose(f);
    }
    OUT[3] = ok && read_all("seek", buf, sizeof buf) == 12 && strcmp(buf, "hello mega6!") == 0;

    /* [4] w */
    f = fopen("seq2", "w");
    ok = f != 0;
    if (f) {
        fputs("abc", f);
        ok = ok && fseek(f, 0L, SEEK_CUR) == 0 && fseek(f, 3L, SEEK_SET) == 0;
        errno = 0;
        ok = ok && fseek(f, 0L, SEEK_SET) == -1 && errno == ESPIPE;
        fclose(f);
    }
    OUT[4] = ok && read_all("seq2", buf, sizeof buf) == 3 && strcmp(buf, "abc") == 0;

    /* [5] r+ missing; a gap past the end */
    ok = fopen("nothere", "r+") == 0;
    f = fopen("gap", "w+");
    if (f) {
        fputc('A', f);
        ok = ok && fseek(f, 3L, SEEK_SET) == 0;
        fputc('B', f);
        rewind(f);
        ok = ok && fgetc(f) == 'A' && fgetc(f) == 0 && fgetc(f) == 0 && fgetc(f) == 'B' && fgetc(f) == EOF;
        fclose(f);
    } else {
        ok = 0;
    }
    f = fopen("gap", "rb");
    if (f) {
        static unsigned char g[8];
        ok = ok && fread(g, 1, 8, f) == 4 && g[0] == 'A' && g[1] == 0 && g[2] == 0 && g[3] == 'B';
        fclose(f);
    }
    OUT[5] = ok;

    /* [6] ungetc and ftell; rewind clears EOF */
    f = fopen("seek", "r");
    ok = f != 0;
    if (f) {
        int c = fgetc(f);
        ungetc(c, f);
        ok = ok && ftell(f) == 0 && fgetc(f) == 'h' && ftell(f) == 1;
        while (fgetc(f) != EOF) ;
        ok = ok && feof(f);
        rewind(f);
        ok = ok && !feof(f) && fgetc(f) == 'h';
        fclose(f);
    }
    OUT[6] = ok;

    OUT[7] = 0xAA;
    return 0;
}
