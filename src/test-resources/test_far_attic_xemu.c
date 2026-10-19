/* test_far_attic_xemu.c — far_malloc from attic RAM (Issue #305): off by
 * default; far_attic_enable checks the memory is there and adds it after
 * banks 4 and 5
 *
 * Results at $C0C0 (01 = pass):
 *   [0] off: far_malloc(.., BANK_ATTIC) is null, and with banks 4 and 5 full
 *       first fit hands out nothing more (nothing outside banks 4-5)
 *   [1] far_attic_enable over all of it: 8 MB (xemu has attic RAM)
 *   [2] past the 128 KB of chip RAM: first fit continues in attic RAM
 *       (bank $80 and up), written and read back
 *   [3] an allocation across a 64 KB boundary of attic RAM: its bytes on
 *       both sides; far_add carries into the next attic bank
 *   [4] far_size, far_free (merged): the attic RAM free again
 *   [5] far_realloc from chip RAM to attic RAM keeps the data
 *   [6] far_memcpy between chip RAM and attic RAM
 *   [7] $AA sentinel
 */
#include <far_ptr.h>
#include <far_ops.h>
#define OUT ((volatile unsigned char *)0xC0C0)

int main(void) {
    far_ptr_t a4, a5, x, y, z, c, r;
    unsigned char ok, i;
    unsigned long n;

    far_policy_first_fit();
    a4 = far_malloc(60000u, BANK4);
    a5 = far_malloc(60000u, BANK5);
    x = far_malloc(10000u, BANK_ATTIC);
    y = far_malloc_current(10000u);
    OUT[0] = !far_is_null(a4) && !far_is_null(a5) && far_is_null(x) && far_is_null(y);

    n = far_attic_enable(0x8000000UL, 0x87FFFFFUL);
    OUT[1] = n == 0x800000UL && far_attic_available() == 0x800000UL;

    x = far_malloc_current(10000u);            /* more than banks 4-5 have left */
    ok = !far_is_null(x) && far_bank(x) >= 0x80;
    for (i = 0; i < 10; i++) far_write_uint8(x, i * 100, (uint8_t)(i * 7 + 1));
    for (i = 0; i < 10; i++) ok = ok && far_read_uint8(x, i * 100) == (uint8_t)(i * 7 + 1);
    OUT[2] = ok;

    /* x: 10000 bytes from $8000000; y: 54000 more; z then starts at
     * $800FA00 and runs past $8010000 */
    y = far_malloc(54000u, BANK_ATTIC);
    z = far_malloc(2000u, BANK_ATTIC);
    ok = !far_is_null(y) && !far_is_null(z) && far_bank(z) == 0x80;
    far_write_uint16(z, 0, 0x1234);
    far_write_uint16(z, 1990, 0xBEEF);           /* in bank $81 */
    ok = ok && far_read_uint16(z, 0) == 0x1234 && far_read_uint16(z, 1990) == 0xBEEF;
    ok = ok && far_bank(far_add(z, 1990)) == 0x81;
    OUT[3] = ok;

    ok = far_size(z) == 2000u && far_size(y) == 54000u && far_size(x) == 10000u;
    far_free(y);
    far_free(x);
    far_free(z);
    ok = ok && far_attic_available() == 0x800000UL;
    x = far_malloc(16u, BANK_ATTIC);
    ok = ok && far_bank(x) == 0x80 && far_offset(x) == 0;   /* the first byte again */
    far_free(x);
    OUT[4] = ok;

    c = far_malloc(100u, BANK4);
    for (i = 0; i < 100; i++) far_write_uint8(c, i, i);
    far_free(a4);
    far_free(a5);
    a4 = far_malloc(65000u, BANK4);
    a5 = far_malloc(65000u, BANK5);               /* chip RAM full again */
    r = far_realloc(c, 30000u, BANK_ATTIC);
    ok = !far_is_null(r) && far_bank(r) >= 0x80;
    for (i = 0; i < 100; i++) ok = ok && far_read_uint8(r, i) == i;
    OUT[5] = ok;

    {
        static unsigned char src[64], back[64];
        far_ptr_t t = far_malloc(64u, BANK_ATTIC);
        for (i = 0; i < 64; i++) src[i] = (unsigned char)(i ^ 0x5A);
        far_memcpy_from_local(t, src, 64);
        far_memcpy(far_add(r, 1000), t, 64);       /* attic to attic */
        far_memcpy_to_local(back, far_add(r, 1000), 64);
        ok = 1;
        for (i = 0; i < 64; i++) ok = ok && back[i] == (unsigned char)(i ^ 0x5A);
        OUT[6] = ok;
    }
    OUT[7] = 0xAA;
    return 0;
}
