/* test_far_ops_xemu.c — far pointers (far_ptr.h / far_ops.h) and the
 * dma_controller.h transfers, all through DMA at 24/28-bit addresses
 *
 * Results at $C0C0 (01 = pass):
 *   [0] far_write/far_read of uint8, int16, uint32, float in bank 4
 *   [1] far_memset / far_memcpy / far_memcpy_to_local / _from_local
 *   [2] strings: far_strcpy_from_local, far_strlen, far_strcmp,
 *       far_strcpy_to_local (truncating)
 *   [3] far_add carries into the next bank ($4FFFF + 1 = $50000);
 *       far_diff, far_cmp across banks
 *   [4] far_malloc: bank 0 (the program) gives the null pointer; bank 4
 *       blocks; far_calloc zeroes; far_free then reuse (coalescing)
 *   [5] far_realloc from bank 4 to bank 5 keeps the data
 *   [6] dma_controller.h: dma_fill, dma_transfer_smart, dma_execute
 *   [7] $AA sentinel
 */
#include <far_ptr.h>
#include <far_ops.h>
#include <dma_controller.h>
#include <string.h>
#define OUT ((volatile unsigned char *)0xC0C0)

static unsigned char buf[64];

int main(void) {
    far_ptr_t p = far_make(BANK4, 0x1000), q, r;
    unsigned char ok;
    unsigned int i;
    float f;
    char s[16];

    /* [0] */
    far_write_uint8(p, 0, 0xA5);
    far_write_int16(p, 1, -1234);
    far_write_uint32(p, 3, 0x12345678UL);
    far_write_float(p, 7, 2.5f);
    f = far_read_float(p, 7);
    OUT[0] = far_read_uint8(p, 0) == 0xA5 && far_read_int16(p, 1) == -1234 &&
             far_read_uint32(p, 3) == 0x12345678UL && f == 2.5f;

    /* [1] */
    far_memset(p, 0x3C, 40);
    q = far_make(BANK5, 0x2000);
    far_memcpy(q, p, 40);
    memset(buf, 0, sizeof buf);
    far_memcpy_to_local(buf, q, 41);
    ok = 1; for (i = 0; i < 40; i++) if (buf[i] != 0x3C) ok = 0;
    for (i = 0; i < 64; i++) buf[i] = (unsigned char)i;
    far_memcpy_from_local(q, buf, 64);
    OUT[1] = ok && far_read_uint8(q, 63) == 63 && far_read_uint8(q, 10) == 10;

    /* [2] */
    far_strcpy_from_local(p, "hello far world", 64);
    far_strcpy_to_local(s, p, 6);
    OUT[2] = far_strlen(p) == 15 && far_strcmp(p, "hello far world") == 0 &&
             far_strcmp(p, "hello") > 0 && strcmp(s, "hello") == 0;

    /* [3] */
    q = far_add(far_make(BANK4, 0xFFFF), 1);
    r = far_sub(q, 2);
    OUT[3] = far_bank(q) == BANK5 && far_offset(q) == 0 && far_bank(r) == BANK4 && far_offset(r) == 0xFFFE &&
             far_diff(q, r) == 2 && far_lt(r, q) && far_cmp(q, far_make(BANK5, 0)) == 0;

    /* [4] */
    {
        far_ptr_t a, b, c, z = far_malloc(16, BANK0);
        a = far_malloc(100, BANK4);
        b = far_calloc(10, 10, BANK4);
        ok = far_is_null(z) && !far_is_null(a) && !far_is_null(b) && far_bank(a) == BANK4 &&
             far_size(a) == 100 && far_size(b) == 100 && far_offset(b) == far_offset(a) + 100;
        for (i = 0; i < 100; i++) if (far_read_uint8(b, i) != 0) ok = 0;
        far_free(a);
        c = far_malloc(60, BANK4);            /* fits in a's place */
        OUT[4] = ok && far_offset(c) == far_offset(a) && !far_is_valid(a) == 0;
    }

    /* [5] */
    {
        far_ptr_t a = far_malloc(20, BANK4), b;
        far_strcpy_from_local(a, "moved", 20);
        b = far_realloc(a, 40, BANK5);
        far_strcpy_to_local(s, b, 16);
        OUT[5] = far_bank(b) == BANK5 && strcmp(s, "moved") == 0 && far_size(b) == 40;
    }

    /* [6] */
    {
        dma_request_t req;
        dma_fill(5, 0x3000, 0x77, 10);
        dma_transfer_smart(4, 0x3000, 5, 0x3000, 10);
        req.src_bank = 4; req.src_addr = 0x3000; req.dst_bank = 0;
        req.dst_addr = (uint16_t)(unsigned int)buf; req.length = 10;
        buf[10] = 0x55;
        dma_execute(&req);
        ok = 1; for (i = 0; i < 10; i++) if (buf[i] != 0x77) ok = 0;
        OUT[6] = ok && buf[10] == 0x55 && !dma_is_busy() && dma_available();
    }

    OUT[7] = 0xAA;
    return 0;
}
