/* test_far2_xemu.c — far pointer and DMA controller functions not covered by
 * test_far_ops_xemu.c
 *
 * Results at $C0C0 (01 = pass):
 *   [0] far_read/far_write of int8, uint16, int32, double (bank 5)
 *   [1] far_array_element / far_array_read / far_array_write
 *   [2] far_inc / far_dec (carrying across a bank), far_eq / far_gt / far_lt
 *   [3] far_memzero, far_memcpy_smart, far_memcpy_stream
 *   [4] bank_set / bank_get, far_malloc_current / far_calloc_current,
 *       far_alloc_bank, bank_available, bank_reset / bank_reset_all
 *   [5] allocation policies for far_malloc_current: fixed bank, first fit
 *       (bank 4 then 5), round robin (alternating)
 *   [6] dma_controller.h: dma_disable makes the transfers no-ops, dma_enable
 *       restores them; dma_get_status, dma_wait, dma_is_busy,
 *       dma_speed_bytes_per_us, dma_set_interrupt_enable, dma_print_status
 *   [7] memcpy_optimized (sizes 0-40, overlapping nothing)
 *   [8] $AA sentinel
 */
#pragma cc45 signed_default

#include <far_ptr.h>
#include <far_ops.h>
#include <dma_controller.h>
#include <string.h>

#define OUT ((volatile unsigned char *)0xC0C0)

void *memcpy_optimized(void *dest, const void *src, int n);

static unsigned char buf[64];

static int t_rw(void) {
    far_ptr_t p = far_make(BANK5, 0x1000);
    far_write_int8(p, 0, -5);
    far_write_uint16(p, 1, 0xBEEF);
    far_write_int32(p, 3, -100000L);
    far_write_double(p, 7, 2.75);
    return far_read_int8(p, 0) == -5 && far_read_uint16(p, 1) == 0xBEEF &&
           far_read_int32(p, 3) == -100000L && far_read_double(p, 7) == 2.75 &&
           far_read_uint8(p, 0) == 0xFB;
}

static int t_array(void) {
    struct rec { int a; long b; } r, w;
    far_ptr_t base = far_make(BANK5, 0x2000);
    far_ptr_t e3 = far_array_element(base, 3, sizeof(struct rec));
    int i, ok = far_offset(e3) == 0x2000 + 3 * 6 && far_bank(e3) == BANK5;
    for (i = 0; i < 5; i++) {
        w.a = i * 11;
        w.b = 1000L * i;
        far_array_write(base, i, sizeof w, &w, sizeof w);
    }
    far_array_read(base, 3, sizeof r, &r, sizeof r);
    ok = ok && r.a == 33 && r.b == 3000L;
    /* a destination smaller than an element is left alone */
    r.a = 7;
    far_array_read(base, 2, sizeof r, &r, 2);
    return ok && r.a == 7;
}

static int t_arith(void) {
    far_ptr_t a = far_make(BANK4, 0xFFFF);
    far_ptr_t b = far_inc(a);
    far_ptr_t c = far_dec(b);
    return far_bank(b) == BANK5 && far_offset(b) == 0 && far_eq(c, a) && !far_eq(a, b) &&
           far_gt(b, a) && !far_gt(a, b) && far_lt(a, b) && far_offset(far_dec(far_make(BANK5, 0))) == 0xFFFF;
}

static int t_bulk(void) {
    far_ptr_t a = far_make(BANK5, 0x3000);
    far_ptr_t b = far_make(BANK5, 0x3100);
    far_ptr_t c = far_make(BANK4, 0x3200);
    int i, ok = 1;
    for (i = 0; i < 64; i++) buf[i] = (unsigned char)(i + 1);
    far_memcpy_from_local(a, buf, 64);
    far_memcpy_smart(b, a, 64);
    far_memcpy_stream(c, b, 64);
    memset(buf, 0, 64);
    far_memcpy_to_local(buf, c, 64);
    for (i = 0; i < 64; i++) if (buf[i] != i + 1) ok = 0;
    far_memzero(c, 10);
    far_memcpy_to_local(buf, c, 12);
    for (i = 0; i < 10; i++) if (buf[i]) ok = 0;
    return ok && buf[10] == 11 && buf[11] == 12;
}

static int t_banks(void) {
    far_ptr_t p, q;
    size_t avail;
    int ok;
    bank_reset_all();
    ok = bank_get() == BANK4;   /* the default */
    bank_set(BANK5);
    ok = ok && bank_get() == BANK5;
    avail = bank_available(BANK5);
    p = far_malloc_current(100);
    ok = ok && far_alloc_bank(p) == BANK5 && bank_available(BANK5) == avail - 100;
    q = far_calloc_current(10, 3);
    ok = ok && far_alloc_bank(q) == BANK5 && far_read_uint8(q, 29) == 0 && far_size(q) == 30;
    bank_reset(BANK5);
    ok = ok && bank_available(BANK5) == avail && far_size(p) == 0;
    bank_set(BANK0);
    ok = ok && far_is_null(far_malloc_current(10));
    ok = ok && bank_available(BANK0) == 0 && bank_available(BANK6) == 0;
    bank_set(BANK4);
    bank_reset_all();
    return ok;
}

static int t_policies(void) {
    far_ptr_t a, b, c, big;
    int ok;
    bank_reset_all();
    far_policy_fixed_bank(BANK5);
    a = far_malloc_current(16);
    ok = far_alloc_bank(a) == BANK5 && bank_get() == BANK5;
    /* first fit: bank 4, then 5 when bank 4 is full */
    bank_reset_all();
    far_policy_first_fit();
    a = far_malloc_current(16);
    big = far_malloc(bank_available(BANK4), BANK4);
    b = far_malloc_current(16);
    ok = ok && far_alloc_bank(a) == BANK4 && !far_is_null(big) && far_alloc_bank(b) == BANK5;
    /* round robin: alternating */
    bank_reset_all();
    far_policy_round_robin();
    a = far_malloc_current(16);
    b = far_malloc_current(16);
    c = far_malloc_current(16);
    ok = ok && far_alloc_bank(a) != far_alloc_bank(b) && far_alloc_bank(c) == far_alloc_bank(a);
    /* bank_set returns to a fixed bank */
    bank_set(BANK4);
    a = far_malloc_current(16);
    b = far_malloc_current(16);
    ok = ok && far_alloc_bank(a) == BANK4 && far_alloc_bank(b) == BANK4;
    bank_reset_all();
    return ok;
}

static int t_dma(void) {
    int ok;
    memset(buf, 0x11, 16);
    dma_disable();
    dma_fill(0, (unsigned int)buf, 0x22, 8);
    ok = buf[0] == 0x11;
    dma_enable();
    dma_fill(0, (unsigned int)buf, 0x22, 8);
    ok = ok && buf[0] == 0x22 && buf[7] == 0x22 && buf[8] == 0x11;
    dma_set_interrupt_enable(1);
    dma_wait();
    dma_print_status();
    return ok && dma_get_status() == 0 && !dma_is_busy() && dma_speed_bytes_per_us() > 0 && dma_available();
}

static int t_memcpy_opt(void) {
    static unsigned char src[48], dst[48];
    int n, i, ok = 1;
    for (i = 0; i < 48; i++) src[i] = (unsigned char)(i * 3 + 1);
    for (n = 0; n <= 40; n++) {
        memset(dst, 0xEE, sizeof dst);
        if (memcpy_optimized(dst + 1, src, n) != dst + 1) ok = 0;
        if (dst[0] != 0xEE || dst[n + 1] != 0xEE) ok = 0;
        for (i = 0; i < n; i++) if (dst[i + 1] != src[i]) ok = 0;
    }
    return ok;
}

int main(void) {
    OUT[0] = t_rw();
    OUT[1] = t_array();
    OUT[2] = t_arith();
    OUT[3] = t_bulk();
    OUT[4] = t_banks();
    OUT[5] = t_policies();
    OUT[6] = t_dma();
    OUT[7] = t_memcpy_opt();
    OUT[8] = 0xAA;
    return 0;
}
