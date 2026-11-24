/* test_relfile_xemu.c — REL files through the drive (#783) and their
 * truncation (#764), on a scratch D81 in unit 8
 *
 * Results at $C0C0 (01 = pass):
 *   [0] cbm_rel_open creates a REL file; records written out of order
 *       (1, 3: record 2 created empty); cbm_rel_size 3
 *   [1] closed and reopened: the size found again (3, not the empty records
 *       the DOS fills the block with); records read back (a short record: its
 *       bytes, the rest 0); record 2 empty (0 bytes); past the end: 0
 *   [2] cbm_rel_update / cbm_rel_append (record 4) / cbm_rel_delete (record
 *       3 empty, read as 0 bytes); update of a missing record fails
 *   [3] cbm_rel_truncate(2): size 2, records 1-2 kept; after a reopen
 *       still 2 (on the disk); record 3 gone; the copy not left behind
 *   [4] cbm_rel_truncate to more records: the file grows
 *   [5] errors: an invalid handle, record 0, a closed handle; two files open
 *       at once on their own channels
 *   [6] $AA sentinel
 */
#pragma cc45 signed_default

#include <stdio.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char buf[20];

static int rec_is(int h, unsigned int rec, const char *text) {
    int n = cbm_rel_read(h, rec, buf, 10);
    return n == (int)strlen(text) && memcmp(buf, text, n) == 0 && (n == 10 || buf[n] == 0);
}

int main(void) {
    int h, h2, ok;
    FILE *f;
    OUT[6] = 0;

    h = cbm_rel_open(8, "RECS", 10, CBM_REL_WRITE);
    ok = h >= 0 && cbm_rel_size(h) == 0 && cbm_rel_record_size(h) == 10;
    ok = ok && cbm_rel_write(h, 1, "FIRST", 5) == 5 && cbm_rel_write(h, 3, "THIRDREC10", 10) == 10;
    ok = ok && cbm_rel_size(h) == 3 && cbm_rel_tell(h) == 3;
    ok = ok && cbm_rel_close(h) == 0;
    OUT[0] = ok;

    h = cbm_rel_open(8, "RECS", 10, CBM_REL_READ);
    ok = h >= 0 && cbm_rel_size(h) == 3;
    ok = ok && rec_is(h, 1, "FIRST") && rec_is(h, 3, "THIRDREC10");
    ok = ok && cbm_rel_read(h, 2, buf, 10) == 0 && buf[0] == 0;
    ok = ok && cbm_rel_read(h, 9, buf, 10) == 0;
    OUT[1] = ok;

    ok = cbm_rel_update(h, 1, "ONE", 3) == 3 && rec_is(h, 1, "ONE");
    ok = ok && cbm_rel_append(h, "FOURTH", 6) == 4 && cbm_rel_size(h) == 4 && rec_is(h, 4, "FOURTH");
    ok = ok && cbm_rel_delete(h, 3) == 0 && cbm_rel_read(h, 3, buf, 10) == 0 && cbm_rel_size(h) == 4;
    ok = ok && cbm_rel_update(h, 7, "X", 1) == -1;
    OUT[2] = ok;

    ok = cbm_rel_write(h, 2, "TWO", 3) == 3 && cbm_rel_truncate(h, 2) == 0 && cbm_rel_size(h) == 2 && rec_is(h, 1, "ONE");
    ok = ok && cbm_rel_close(h) == 0;
    h = cbm_rel_open(8, "RECS", 10, CBM_REL_READ);
    ok = ok && h >= 0 && cbm_rel_size(h) == 2 && rec_is(h, 1, "ONE") && rec_is(h, 2, "TWO") && cbm_rel_read(h, 3, buf, 10) == 0;
    f = fopen("RELTRUNC.TMP", "r");
    ok = ok && f == 0;
    if (f) fclose(f);
    OUT[3] = ok;

    ok = cbm_rel_truncate(h, 5) == 0 && cbm_rel_size(h) == 5 && rec_is(h, 1, "ONE");
    OUT[4] = ok;

    h2 = cbm_rel_open(8, "OTHER", 4, CBM_REL_WRITE);
    ok = h2 >= 0 && h2 != h && cbm_rel_write(h2, 1, "ABCD", 4) == 4 && rec_is(h, 1, "ONE");
    ok = ok && cbm_rel_read(h2, 1, buf, 4) == 4 && memcmp(buf, "ABCD", 4) == 0;
    ok = ok && cbm_rel_read(9, 1, buf, 4) == -1 && cbm_rel_read(h, 0, buf, 4) == -1;
    ok = ok && cbm_rel_close(h2) == 0 && cbm_rel_close(h) == 0 && cbm_rel_close(h) == -1 &&
         cbm_rel_lasterror(h) == CBM_REL_ERR_INVALID && cbm_rel_open(8, "", 10, 0) == -1;
    OUT[5] = ok;
    OUT[6] = 0xAA;
    return 0;
}
