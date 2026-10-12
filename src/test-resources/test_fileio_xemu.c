/* test_fileio_xemu.c — fopen/fclose/fputs/fgets/... on a disk (drive 8)
 *
 * Run by test_xemu.sh with an empty D81 attached as drive 8 (xemu -8).
 *
 * Results at $C0C0 (01 = pass):
 *   [0] write a file, read it back (fgets), the last byte included
 *   [1] "w" on an existing file replaces it (the DOS name gets "@0:")
 *   [2] "a" appends
 *   [3] fopen of a missing file for reading returns NULL (drive status)
 *   [4] fgetc to the end: EOF, feof()
 *   [5] "wb"/"rb" (USR file), fwrite/fread of binary bytes
 *   [6] two files open at once; fprintf
 *   [7] $AA sentinel
 */
#include <stdio.h>
#include <string.h>
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
    FILE *f, *g;
    int c, n;
    static unsigned char bin[4] = { 0x00, 0xFF, 0x0D, 0x80 };
    static unsigned char back[8];

    f = fopen("one", "w");
    if (f) { fputs("hello disk", f); fclose(f); }
    f = fopen("one", "r");
    buf[0] = 0;
    if (f) { fgets(buf, sizeof buf, f); fclose(f); }
    OUT[0] = strcmp(buf, "hello disk") == 0;

    f = fopen("one", "w");
    if (f) { fputs("second", f); fclose(f); }
    OUT[1] = read_all("one", buf, sizeof buf) == 6 && strcmp(buf, "second") == 0;

    f = fopen("one", "a");
    if (f) { fputs("+more", f); fclose(f); }
    OUT[2] = read_all("one", buf, sizeof buf) == 11 && strcmp(buf, "second+more") == 0;

    OUT[3] = fopen("missing", "r") == NULL;

    f = fopen("one", "r");
    n = 0;
    if (f) {
        while ((c = fgetc(f)) != EOF) n++;
        OUT[4] = n == 11 && feof(f);
        fclose(f);
    }

    f = fopen("data", "wb");
    if (f) { fwrite(bin, 1, 4, f); fclose(f); }
    f = fopen("data", "rb");
    n = 0;
    if (f) { n = (int)fread(back, 1, 8, f); fclose(f); }
    OUT[5] = n == 4 && memcmp(bin, back, 4) == 0;

    f = fopen("aa", "w");
    g = fopen("bb", "w");
    if (f && g) {
        fprintf(f, "x=%d", 42);
        fputs("yy", g);
        fclose(g);
        fclose(f);
    }
    OUT[6] = read_all("aa", buf, sizeof buf) == 4 && strcmp(buf, "x=42") == 0 &&
             read_all("bb", buf, sizeof buf) == 2 && strcmp(buf, "yy") == 0;
    OUT[7] = 0xAA;
    return 0;
}
