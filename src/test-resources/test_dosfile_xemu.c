/* test_dosfile_xemu.c — DOS commands on the command channel (#762) and C's
 * remove() / rename() (#763), on a scratch D81 in unit 8
 *
 * Results at $C0C0 (01 = pass):
 *   [0] cbm_rename: the file under its new name, not the old one; a missing
 *       file (62) and a name that exists (63) fail
 *   [1] cbm_delete: the file gone; nothing to scratch fails (01, 0 files);
 *       a wildcard scratches several
 *   [2] remove(): 0, the file gone; again: -1, ENOENT
 *   [3] rename(): 0; to a name that exists: -1, EEXIST; a missing file:
 *       -1, ENOENT; no name: EINVAL; strerror of the new codes
 *   [4] cbm_format: the disk cleared (a quick format, no id), its files gone;
 *       a file written after it
 *   [5] argument checks: device 0, NULL, empty names
 *   [6] $AA sentinel
 */
#pragma cc45 signed_default

#include <stdio.h>
#include <string.h>
#include <errno.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static int make(const char *name, const char *text) {
    FILE *f = fopen(name, "w");
    if (!f) return 0;
    fputs(text, f);
    return fclose(f) == 0;
}

/* the file's first characters match text */
static int has(const char *name, const char *text) {
    char buf[16];
    FILE *f = fopen(name, "r");
    int ok;
    if (!f) return 0;
    memset(buf, 0, sizeof buf);
    ok = fgets(buf, sizeof buf, f) != 0 && strcmp(buf, text) == 0;
    fclose(f);
    return ok;
}

static int exists(const char *name) {
    FILE *f = fopen(name, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

int main(void) {
    int ok;
    OUT[6] = 0;

    ok = make("ALPHA", "one") && make("BETA", "two");
    ok = ok && cbm_rename(8, "ALPHA", "GAMMA") == 0 && has("GAMMA", "one") && !exists("ALPHA");
    ok = ok && cbm_rename(8, "NOSUCH", "DELTA") == -1 && cbm_rename(8, "GAMMA", "BETA") == -1 && has("BETA", "two");
    OUT[0] = ok;

    ok = cbm_delete(8, "GAMMA") == 0 && !exists("GAMMA") && cbm_delete(8, "GAMMA") == -1;
    ok = ok && make("TMP1", "a") && make("TMP2", "b") && cbm_delete(8, "TMP*") == 0 && !exists("TMP1") && !exists("TMP2");
    OUT[1] = ok;

    errno = 0;
    ok = make("EPSILON", "e") && remove("EPSILON") == 0 && !exists("EPSILON");
    ok = ok && remove("EPSILON") == -1 && errno == ENOENT;
    OUT[2] = ok;

    ok = make("ZETA", "z") && rename("ZETA", "ETA") == 0 && has("ETA", "z");
    errno = 0;
    ok = ok && rename("ETA", "BETA") == -1 && errno == EEXIST && has("ETA", "z");
    errno = 0;
    ok = ok && rename("NOSUCH", "THETA") == -1 && errno == ENOENT;
    errno = 0;
    ok = ok && rename("", "THETA") == -1 && errno == EINVAL && remove(0) == -1;
    ok = ok && strcmp(strerror(ENOENT), "No such file or directory") == 0 && strcmp(strerror(EEXIST), "File exists") == 0;
    OUT[3] = ok;

    ok = cbm_format(8, "CLEARED", "") == 0 && !exists("BETA") && !exists("ETA");
    ok = ok && make("AFTER", "x") && has("AFTER", "x");
    OUT[4] = ok;

    OUT[5] = cbm_delete(0, "AFTER") == -1 && cbm_delete(8, 0) == -1 && cbm_delete(8, "") == -1 &&
             cbm_rename(8, "AFTER", "") == -1 && cbm_format(8, "", "") == -1 && cbm_format(8, "X", 0) == -1 &&
             exists("AFTER");
    OUT[6] = 0xAA;
    return 0;
}
