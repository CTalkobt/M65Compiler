/* test_far_attic_xemu.c — far_malloc from attic RAM (Issue #305, #306): off
 * by default; far_attic_enable / far_attic_init check the memory is there
 * and add it after banks 4 and 5, as a buddy allocator (power-of-two blocks
 * aligned to their size); far_reserve keeps ranges of banks 4 and 5
 *
 * Results at $C0C0 (01 = pass):
 *   [0] off: far_malloc(.., BANK_ATTIC) is null, and with banks 4 and 5 full
 *       first fit hands out nothing more (nothing outside banks 4-5)
 *   [1] far_attic_enable over all of it: 8 MB (xemu has attic RAM)
 *   [2] past the 128 KB of chip RAM: first fit continues in attic RAM,
 *       10000 bytes a 16 KB block at $8000000; written and read back
 *   [3] far_malloc_long(512 KB): aligned to 512 KB, its size; far_add
 *       carries into the next attic bank; bytes 4 banks apart
 *   [4] far_free (merged): the attic RAM free again; 16 bytes take the
 *       first granule
 *   [5] far_realloc from chip RAM to attic RAM keeps the data
 *   [6] far_memcpy between chip RAM and attic RAM
 *   [7] 256 slabs of 32 KB (all of it), each written and read back; every
 *       other one freed: 4 MB free but no 64 KB block (no buddies free);
 *       the rest freed: one 8 MB block again
 *   [8] far_attic_init with 32 KB granules and the program's 128-byte table,
 *       a range not on granules (rounded in); a table too small refused
 *   [9] far_reserve: banks 4 and 5 never hand out the reserved ranges
 *   [10] far_attic_malloc / far_attic_size / far_attic_free (no chip RAM
 *        heap): the same blocks as far_malloc(.., BANK_ATTIC)
 *   [11] $AA sentinel
 */
#include <far_ptr.h>
#include <far_ops.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static far_ptr_t slab[256];
static unsigned char meta[FAR_ATTIC_META_SIZE(15)];

static unsigned long attic_off(far_ptr_t p) { return far_to_linear(p) - 0x8000000UL; }

/* [lo, lo+n) outside every reserved range */
static unsigned char clear_of(unsigned long lo, unsigned long n) {
    unsigned long hi = lo + n - 1;
    return (hi < 0x40800UL || lo > 0x40FFFUL) && (hi < 0x41800UL || lo > 0x41BFFUL) &&
           (hi < 0x5FE00UL || lo > 0x5FFFFUL);
}

int main(void) {
    far_ptr_t a4, a5, x, y, c, r, t;
    unsigned char ok, i, cnt4, cnt5;
    unsigned int k, v;
    unsigned long n;

    far_policy_first_fit();
    a4 = far_malloc(60000u, BANK4);
    a5 = far_malloc(60000u, BANK5);
    x = far_malloc(10000u, BANK_ATTIC);
    y = far_malloc_current(10000u);
    OUT[0] = !far_is_null(a4) && !far_is_null(a5) && far_is_null(x) && far_is_null(y);

    n = far_attic_enable(0x8000000UL, 0x87FFFFFUL);
    OUT[1] = n == 0x800000UL && far_attic_available() == 0x800000UL;

    x = far_malloc_current(10000u);              /* more than banks 4-5 have left */
    ok = !far_is_null(x) && far_bank(x) == 0x80 && far_offset(x) == 0 && far_size(x) == 16384u;
    for (i = 0; i < 10; i++) far_write_uint8(x, i * 1000u, (uint8_t)(i * 7 + 1));
    for (i = 0; i < 10; i++) ok = ok && far_read_uint8(x, i * 1000u) == (uint8_t)(i * 7 + 1);
    OUT[2] = ok;

    y = far_malloc_long(0x80000UL, BANK_ATTIC);
    ok = !far_is_null(y) && (attic_off(y) & 0x7FFFFUL) == 0 && far_size_long(y) == 0x80000UL &&
         far_size(y) == 0xFFFF;
    t = far_add(far_add(far_add(y, 0x6000), 0x6000), 0x4010);   /* (signed offsets) into the next bank */
    ok = ok && far_bank(t) == far_bank(y) + 1 && far_offset(t) == 0x10;
    t = far_of_linear(far_to_linear(y) + 0x4D2F0UL);   /* 4 banks on */
    ok = ok && far_bank(t) == far_bank(y) + 4;
    far_write_uint16(y, 0, 0x1234);
    far_write_uint16(t, 0, 0xBEEF);
    ok = ok && far_read_uint16(y, 0) == 0x1234 && far_read_uint16(t, 0) == 0xBEEF;
    OUT[3] = ok;

    far_free(y);
    far_free(x);
    ok = far_attic_available() == 0x800000UL && far_size(x) == 0;
    x = far_malloc(16u, BANK_ATTIC);
    ok = ok && far_bank(x) == 0x80 && far_offset(x) == 0 && far_size(x) == 4096u;
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
        t = far_malloc(64u, BANK_ATTIC);
        for (i = 0; i < 64; i++) src[i] = (unsigned char)(i ^ 0x5A);
        far_memcpy_from_local(t, src, 64);
        far_memcpy(far_add(r, 1000), t, 64);       /* attic to attic */
        far_memcpy_to_local(back, far_add(r, 1000), 64);
        ok = 1;
        for (i = 0; i < 64; i++) ok = ok && back[i] == (unsigned char)(i ^ 0x5A);
        OUT[6] = ok;
        far_free(t);
    }
    far_free(r);

    ok = far_attic_available() == 0x800000UL;
    for (k = 0; k < 256; k++) {
        slab[k] = far_malloc(32768u, BANK_ATTIC);
        if (far_is_null(slab[k]) || (attic_off(slab[k]) & 0x7FFFUL)) ok = 0;
        else {
            far_write_uint16(slab[k], 0, k);
            far_write_uint16(slab[k], 32766u, k ^ 0xFFFF);
        }
    }
    ok = ok && far_is_null(far_malloc(16u, BANK_ATTIC)) && far_attic_available() == 0;
    for (k = 0; k < 256 && ok; k++) {
        v = far_read_uint16(slab[k], 0);
        ok = v == k && far_read_uint16(slab[k], 32766u) == (k ^ 0xFFFF);
    }
    for (k = 0; k < 256; k += 2) far_free(slab[k]);
    ok = ok && far_attic_available() == 0x400000UL;
    ok = ok && far_is_null(far_malloc_long(65536UL, BANK_ATTIC));
    x = far_malloc(32768u, BANK_ATTIC);
    ok = ok && !far_is_null(x) && far_read_uint16(x, 0) % 2 == 0;   /* a freed one */
    far_free(x);
    for (k = 1; k < 256; k += 2) far_free(slab[k]);
    x = far_malloc_long(0x800000UL, BANK_ATTIC);
    ok = ok && far_bank(x) == 0x80 && far_offset(x) == 0 && far_attic_available() == 0;
    far_free(x);
    OUT[7] = ok;

    ok = far_attic_init(0x8000000UL, 0x87FFFFFUL, 15, meta, sizeof meta - 1) == 0;
    n = far_attic_init(0x8001000UL, 0x803FFFFUL, 15, meta, sizeof meta);   /* (still hooked) */
    ok = ok && n == 0x38000UL && far_attic_available() == 0x38000UL;
    x = far_malloc(100u, BANK_ATTIC);
    ok = ok && far_size(x) == 32768u && attic_off(x) == 0x8000UL;
    y = far_malloc_long(0x20000UL, BANK_ATTIC);      /* $20000-$3FFFF, the one 128 KB block */
    ok = ok && attic_off(y) == 0x20000UL && far_is_null(far_malloc_long(0x20000UL, BANK_ATTIC));
    far_free(x);
    far_free(y);
    ok = ok && far_attic_available() == 0x38000UL;
    OUT[8] = ok;

    ok = far_reserve(0x30000UL, 0x3FFFFUL) == 0;
    bank_reset_all();
    ok = ok && far_reserve(0x40800UL, 0x40FFFUL) && far_reserve(0x41800UL, 0x41BFFUL) &&
         far_reserve(0x5FE00UL, 0x5FFFFUL);
    cnt4 = cnt5 = 0;
    while (!far_is_null(x = far_malloc(4096u, BANK4))) {
        if (!clear_of(far_to_linear(x), 4096u)) ok = 0;
        cnt4++;
    }
    while (!far_is_null(x = far_malloc(4096u, BANK5))) {
        if (!clear_of(far_to_linear(x), 4096u)) ok = 0;
        cnt5++;
    }
    /* bank 4: $41C00-$4FFFF holds 14 (the gaps below are too small);
     * bank 5: $50100-$5FDFF 15 */
    OUT[9] = ok && cnt4 == 14 && cnt5 == 15;

    n = far_attic_available();                   /* (granules of 32 KB: [8]) */
    x = far_attic_malloc(1u);
    y = far_attic_malloc(0x10000UL);
    ok = attic_off(x) == 0x8000UL && far_attic_size(x) == 32768UL && attic_off(y) == 0x10000UL &&
         far_attic_size(y) == 0x10000UL && far_attic_available() == n - 0x18000UL;
    ok = ok && far_attic_size(far_add(x, 1)) == 0;   /* not a block's start */
    far_attic_free(y);
    far_attic_free(x);
    ok = ok && far_attic_available() == n && far_is_null(far_attic_malloc(0x40000UL));
    OUT[10] = ok;
    OUT[11] = 0xAA;
    return 0;
}
