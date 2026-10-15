#include <far_ops.h>
#include <far_ptr.h>
#include <dma.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * Far pointer operations
 *
 * A far pointer is a bank (address bits 16-23) and a 16-bit offset: the
 * linear address (bank << 16) | offset. Every access is an enhanced DMA job
 * (dma_copy_far / dma_fill_far, lib/stdlib/dma_far.c): the CPU waits while
 * it runs, so a read or write is done when the call returns. Offsets add to
 * the linear address, carrying into the bank.
 * ========================================================================== */

/* The linear address of ptr + offset */
static unsigned long far_linear(far_ptr_t ptr, long offset) {
    unsigned long a = ((unsigned long)far_bank(ptr) << 16) | far_offset(ptr);
    return a + (unsigned long)offset;
}

/* A local (bank 0) address as a linear one */
#define LOCAL(p) ((unsigned long)(unsigned int)(p))

static far_ptr_t far_from_linear(unsigned long a) {
    return far_make((bank_t)(unsigned char)(a >> 16), (uint16_t)a);
}

/* ============================================================================
 * Element-wise access
 * ========================================================================== */

uint8_t far_read_uint8(far_ptr_t ptr, int offset) {
    uint8_t v;
    dma_copy_far(LOCAL(&v), far_linear(ptr, offset), 1);
    return v;
}

int8_t far_read_int8(far_ptr_t ptr, int offset) {
    return (int8_t)far_read_uint8(ptr, offset);
}

uint16_t far_read_uint16(far_ptr_t ptr, int offset) {
    uint16_t v;
    dma_copy_far(LOCAL(&v), far_linear(ptr, offset), sizeof v);
    return v;
}

int16_t far_read_int16(far_ptr_t ptr, int offset) {
    return (int16_t)far_read_uint16(ptr, offset);
}

uint32_t far_read_uint32(far_ptr_t ptr, int offset) {
    uint32_t v;
    dma_copy_far(LOCAL(&v), far_linear(ptr, offset), sizeof v);
    return v;
}

int32_t far_read_int32(far_ptr_t ptr, int offset) {
    return (int32_t)far_read_uint32(ptr, offset);
}

/* float and double are cc45's 5-byte CBM format */
float far_read_float(far_ptr_t ptr, int offset) {
    float v;
    dma_copy_far(LOCAL(&v), far_linear(ptr, offset), sizeof v);
    return v;
}

double far_read_double(far_ptr_t ptr, int offset) {
    double v;
    dma_copy_far(LOCAL(&v), far_linear(ptr, offset), sizeof v);
    return v;
}

void far_write_uint8(far_ptr_t ptr, int offset, uint8_t val) {
    dma_copy_far(far_linear(ptr, offset), LOCAL(&val), 1);
}

void far_write_int8(far_ptr_t ptr, int offset, int8_t val) {
    far_write_uint8(ptr, offset, (uint8_t)val);
}

void far_write_uint16(far_ptr_t ptr, int offset, uint16_t val) {
    dma_copy_far(far_linear(ptr, offset), LOCAL(&val), sizeof val);
}

void far_write_int16(far_ptr_t ptr, int offset, int16_t val) {
    far_write_uint16(ptr, offset, (uint16_t)val);
}

void far_write_uint32(far_ptr_t ptr, int offset, uint32_t val) {
    dma_copy_far(far_linear(ptr, offset), LOCAL(&val), sizeof val);
}

void far_write_int32(far_ptr_t ptr, int offset, int32_t val) {
    far_write_uint32(ptr, offset, (uint32_t)val);
}

void far_write_float(far_ptr_t ptr, int offset, float val) {
    dma_copy_far(far_linear(ptr, offset), LOCAL(&val), sizeof val);
}

void far_write_double(far_ptr_t ptr, int offset, double val) {
    dma_copy_far(far_linear(ptr, offset), LOCAL(&val), sizeof val);
}

/* ============================================================================
 * Bulk memory operations
 * ========================================================================== */

void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    dma_copy_far(far_linear(dst, 0), far_linear(src, 0), len);
}

void far_memcpy_to_local(void* dst, far_ptr_t src, size_t len) {
    dma_copy_far(LOCAL(dst), far_linear(src, 0), len);
}

void far_memcpy_from_local(far_ptr_t dst, const void* src, size_t len) {
    dma_copy_far(far_linear(dst, 0), LOCAL(src), len);
}

void far_memset(far_ptr_t ptr, int val, size_t len) {
    dma_fill_far(far_linear(ptr, 0), len, (unsigned char)val);
}

void far_memzero(far_ptr_t ptr, size_t len) {
    far_memset(ptr, 0, len);
}

void far_memcpy_smart(far_ptr_t dst, far_ptr_t src, size_t len) {
    far_memcpy(dst, src, len);
}

void far_memcpy_stream(far_ptr_t dst, far_ptr_t src, size_t len) {
    far_memcpy(dst, src, len);
}

/* ============================================================================
 * Pointer arithmetic and comparison (on the linear address)
 * ========================================================================== */

far_ptr_t far_add(far_ptr_t ptr, int offset) {
    return far_from_linear(far_linear(ptr, offset));
}

far_ptr_t far_sub(far_ptr_t ptr, int offset) {
    return far_from_linear(far_linear(ptr, -(long)offset));
}

int far_diff(far_ptr_t ptr1, far_ptr_t ptr2) {
    return (int)(far_linear(ptr1, 0) - far_linear(ptr2, 0));
}

far_ptr_t far_inc(far_ptr_t ptr) {
    return far_add(ptr, 1);
}

far_ptr_t far_dec(far_ptr_t ptr) {
    return far_sub(ptr, 1);
}

int far_cmp(far_ptr_t ptr1, far_ptr_t ptr2) {
    unsigned long a = far_linear(ptr1, 0), b = far_linear(ptr2, 0);
    return a < b ? -1 : a > b ? 1 : 0;
}

int far_eq(far_ptr_t ptr1, far_ptr_t ptr2) {
    return far_cmp(ptr1, ptr2) == 0;
}

int far_lt(far_ptr_t ptr1, far_ptr_t ptr2) {
    return far_cmp(ptr1, ptr2) < 0;
}

int far_gt(far_ptr_t ptr1, far_ptr_t ptr2) {
    return far_cmp(ptr1, ptr2) > 0;
}

/* ============================================================================
 * Arrays
 * ========================================================================== */

far_ptr_t far_array_element(far_ptr_t base, int index, size_t element_size) {
    return far_from_linear(far_linear(base, (long)index * (long)element_size));
}

void far_array_read(far_ptr_t base, int index, size_t element_size,
                    void* dst, size_t dst_size) {
    if (dst_size < element_size) return;
    far_memcpy_to_local(dst, far_array_element(base, index, element_size), element_size);
}

void far_array_write(far_ptr_t base, int index, size_t element_size,
                     const void* src, size_t src_size) {
    if (src_size < element_size) return;
    far_memcpy_from_local(far_array_element(base, index, element_size), src, element_size);
}

/* ============================================================================
 * Strings (read in 32-byte chunks)
 * ========================================================================== */

#define CHUNK 32

size_t far_strlen(far_ptr_t str) {
    unsigned char buf[CHUNK];
    size_t len = 0;
    for (;;) {
        unsigned char i;
        dma_copy_far(LOCAL(buf), far_linear(str, (long)len), CHUNK);
        for (i = 0; i < CHUNK; i++) {
            if (buf[i] == 0) return len;
            len++;
        }
    }
}

void far_strcpy_to_local(char* dst, far_ptr_t src, size_t max_len) {
    size_t n;
    if (max_len == 0) return;
    n = far_strlen(src);
    if (n > max_len - 1) n = max_len - 1;
    far_memcpy_to_local(dst, src, n);
    dst[n] = 0;
}

void far_strcpy_from_local(far_ptr_t dst, const char* src, size_t max_len) {
    size_t n;
    char z = 0;
    if (max_len == 0) return;
    n = strlen(src);
    if (n > max_len - 1) n = max_len - 1;
    far_memcpy_from_local(dst, src, n);
    dma_copy_far(far_linear(dst, (long)n), LOCAL(&z), 1);
}

int far_strcmp(far_ptr_t str1, const char* str2) {
    unsigned char buf[CHUNK];
    size_t pos = 0;
    for (;;) {
        unsigned char i;
        dma_copy_far(LOCAL(buf), far_linear(str1, (long)pos), CHUNK);
        for (i = 0; i < CHUNK; i++) {
            unsigned char c1 = buf[i], c2 = (unsigned char)str2[pos];
            if (c1 != c2) return (int)c1 - (int)c2;
            if (c1 == 0) return 0;
            pos++;
        }
    }
}
