/* test_far_string_xemu.c — <far_string.h>: memcpy/memset/strlen/strcpy/
 * strcmp/strcat/memcmp for every mix of local and far pointers, and the
 * typed far array helpers
 *
 * Results at $C0C0 (01 = pass):
 *   [0] memcpy (4 forms), the far-destination ones return the far pointer
 *   [1] memset (2 forms), strlen (2 forms; a far string over 256 bytes)
 *   [2] strcpy (4 forms; a 300-character string: no 256 limit)
 *   [3] strcmp (4 forms): equal, less, greater, a prefix; past 32 bytes
 *   [4] strcat (4 forms)
 *   [5] memcmp (4 forms): equal, a difference in the second chunk, signs
 *   [6] far_array_read/write int16, uint8, uint32 (an index past 16383)
 *   [7] $AA sentinel
 */
#pragma cc45 signed_default

#include <far_ops.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

static char a[320], b[320];

static far_ptr_t F(unsigned int off) { return far_make(BANK5, off); }

static int same(far_ptr_t x, far_ptr_t y) { return far_eq(x, y); }

static int t_memcpy(void) {
    unsigned int i;
    int ok;
    for (i = 0; i < 100; i++) a[i] = (char)(i * 5 + 1);
    ok = memcpy__pvpvst(b, a, 100) == b && memcmp(a, b, 100) == 0;
    ok = ok && same(memcpy__pvfpst(F(0x100), a, 100), F(0x100));
    ok = ok && same(memcpy__fpfpst(F(0x200), F(0x100), 100), F(0x200));
    memset(b, 0, sizeof b);
    ok = ok && memcpy__fpvst(b, F(0x200), 100) == b && memcmp(a, b, 100) == 0;
    return ok;
}

static int t_memset_strlen(void) {
    int ok;
    memset(a, 'x', 300);
    a[300] = 0;
    ok = memset__pvist(b, 7, 10) == b && b[0] == 7 && b[9] == 7;
    ok = ok && same(memset__fpist(F(0x400), 'q', 300), F(0x400));
    far_write_uint8(F(0x400), 300, 0);
    ok = ok && strlen__fp(F(0x400)) == 300 && strlen__pc(a) == 300 && far_read_uint8(F(0x400), 299) == 'q';
    return ok;
}

static int t_strcpy(void) {
    unsigned int i;
    int ok;
    for (i = 0; i < 300; i++) a[i] = (char)('a' + i % 26);
    a[300] = 0;
    ok = same(strcpy__fppc(F(0x600), a), F(0x600)) && strlen__fp(F(0x600)) == 300;
    ok = ok && same(strcpy__fpfp(F(0x800), F(0x600)), F(0x800));
    memset(b, 1, sizeof b);
    ok = ok && strcpy__pcfp(b, F(0x800)) == b && strcmp(a, b) == 0;
    ok = ok && strcpy__pcpc(b, "short") == b && strcmp(b, "short") == 0;
    return ok;
}

static int t_strcmp(void) {
    int ok;
    strcpy__fppc(F(0xA00), "apple");
    strcpy__fppc(F(0xA20), "apricot");
    strcpy__fppc(F(0xA40), "app");
    strcpy__fppc(F(0xA60), "apple");
    ok = strcmp__fppc(F(0xA00), "apple") == 0 && strcmp__fppc(F(0xA00), "apricot") < 0 &&
         strcmp__pcfp("apricot", F(0xA00)) > 0 && strcmp__pcfp("apple", F(0xA00)) == 0 &&
         strcmp__fpfp(F(0xA00), F(0xA60)) == 0 && strcmp__fpfp(F(0xA00), F(0xA20)) < 0 &&
         strcmp__fpfp(F(0xA20), F(0xA00)) > 0 && strcmp__fpfp(F(0xA40), F(0xA00)) < 0 &&
         strcmp__pcpc("b", "a") > 0;
    /* a difference past the first 32 bytes */
    memset(a, 'm', 40);
    a[40] = 0;
    strcpy__fppc(F(0xB00), a);
    a[35] = 'n';
    strcpy__fppc(F(0xB40), a);
    ok = ok && strcmp__fpfp(F(0xB00), F(0xB40)) < 0 && strcmp__pcfp(a, F(0xB00)) > 0;
    return ok;
}

static int t_strcat(void) {
    int ok;
    strcpy(b, "ab");
    ok = strcat__pcpc(b, "cd") == b && strcmp(b, "abcd") == 0;
    strcpy__fppc(F(0xC00), "ef");
    ok = ok && strcat__pcfp(b, F(0xC00)) == b && strcmp(b, "abcdef") == 0;
    ok = ok && same(strcat__fppc(F(0xC00), "gh"), F(0xC00)) && strcmp__fppc(F(0xC00), "efgh") == 0;
    strcpy__fppc(F(0xC20), "12");
    ok = ok && same(strcat__fpfp(F(0xC20), F(0xC00)), F(0xC20)) && strcmp__fppc(F(0xC20), "12efgh") == 0;
    return ok;
}

static int t_memcmp(void) {
    unsigned int i;
    int ok;
    for (i = 0; i < 100; i++) a[i] = (char)i;
    memcpy__pvfpst(F(0xD00), a, 100);
    memcpy__pvfpst(F(0xE00), a, 100);
    ok = memcmp__fpvz(F(0xD00), a, 100) == 0 && memcmp__pvfz(a, F(0xD00), 100) == 0 &&
         memcmp__fpfz(F(0xD00), F(0xE00), 100) == 0 && memcmp__pvpvz(a, a, 100) == 0;
    far_write_uint8(F(0xE00), 70, 200);
    ok = ok && memcmp__fpfz(F(0xD00), F(0xE00), 100) < 0 && memcmp__fpfz(F(0xE00), F(0xD00), 100) > 0 &&
         memcmp__fpfz(F(0xD00), F(0xE00), 70) == 0 && memcmp__fpvz(F(0xE00), a, 100) > 0 &&
         memcmp__pvfz(a, F(0xE00), 100) < 0;
    return ok;
}

static int t_arrays(void) {
    far_ptr_t base = far_make(BANK4, 0);
    far_array_write_int16(base, 3, -1234);
    far_array_write_int16(base, 20000, 4321);   /* at $4_9C40: past offset 32767 */
    far_array_write_uint8(base, 40003, 0x5A);
    far_array_write_uint32(base, 9, 0x12345678UL);
    return far_array_read_int16(base, 3) == -1234 && far_array_read_int16(base, 20000) == 4321 &&
           far_read_int16(far_make(BANK4, 40000), 0) == 4321 && far_array_read_uint8(base, 40003) == 0x5A &&
           far_array_read_uint32(base, 9) == 0x12345678UL && far_read_uint32(base, 36) == 0x12345678UL;
}

int main(void) {
    OUT[0] = t_memcpy();
    OUT[1] = t_memset_strlen();
    OUT[2] = t_strcpy();
    OUT[3] = t_strcmp();
    OUT[4] = t_strcat();
    OUT[5] = t_memcmp();
    OUT[6] = t_arrays();
    OUT[7] = 0xAA;
    return 0;
}
