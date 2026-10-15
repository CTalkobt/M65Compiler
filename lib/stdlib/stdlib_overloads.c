/* stdlib_overloads.c — memory and string functions for any mix of local and
 * far pointers (declared in <far_string.h>)
 *
 * The suffix names the parameter types, in order: pv void *, pc char *,
 * fp far_ptr_t, i int, st / z size_t. A function whose destination is far
 * returns the far pointer. Strings have no length limit: far data moves by
 * DMA (far_ops.c), compared in 32-byte chunks.
 */

#pragma cc45 signed_default

#include <far_ptr.h>
#include <far_ops.h>
#include <far_string.h>
#include <string.h>
#include <stdlib.h>

#define CHUNK 32

/* The linear address of p + off, as a far pointer (off may pass 32767) */
static far_ptr_t far_at(far_ptr_t p, unsigned long off) {
    unsigned long a = ((unsigned long)far_bank(p) << 16) + far_offset(p) + off;
    return far_make((bank_t)(unsigned char)(a >> 16), (uint16_t)a);
}

/* ============================================================================
 * memcpy
 * ========================================================================== */

void *memcpy__pvpvst(void *dst, const void *src, size_t len) {
    return memcpy(dst, (void *)src, len);
}

void *memcpy__fpvst(void *dst, far_ptr_t src, size_t len) {
    far_memcpy_to_local(dst, src, len);
    return dst;
}

far_ptr_t memcpy__pvfpst(far_ptr_t dst, const void *src, size_t len) {
    far_memcpy_from_local(dst, src, len);
    return dst;
}

far_ptr_t memcpy__fpfpst(far_ptr_t dst, far_ptr_t src, size_t len) {
    far_memcpy(dst, src, len);
    return dst;
}

/* ============================================================================
 * memset
 * ========================================================================== */

void *memset__pvist(void *ptr, int val, size_t len) {
    return memset(ptr, val, len);
}

far_ptr_t memset__fpist(far_ptr_t ptr, int val, size_t len) {
    far_memset(ptr, val, len);
    return ptr;
}

/* ============================================================================
 * strlen
 * ========================================================================== */

size_t strlen__pc(const char *str) {
    return strlen((char *)str);
}

size_t strlen__fp(far_ptr_t str) {
    return far_strlen(str);
}

/* ============================================================================
 * strcpy (the terminating 0 included)
 * ========================================================================== */

char *strcpy__pcpc(char *dst, const char *src) {
    return strcpy(dst, (char *)src);
}

char *strcpy__pcfp(char *dst, far_ptr_t src) {
    far_memcpy_to_local(dst, src, far_strlen(src) + 1);
    return dst;
}

far_ptr_t strcpy__fppc(far_ptr_t dst, const char *src) {
    far_memcpy_from_local(dst, src, strlen((char *)src) + 1);
    return dst;
}

far_ptr_t strcpy__fpfp(far_ptr_t dst, far_ptr_t src) {
    far_memcpy(dst, src, far_strlen(src) + 1);
    return dst;
}

/* ============================================================================
 * strcmp (<0, 0, >0 by the first differing unsigned byte)
 * ========================================================================== */

int strcmp__pcpc(const char *str1, const char *str2) {
    return strcmp((char *)str1, (char *)str2);
}

int strcmp__fppc(far_ptr_t str1, const char *str2) {
    return far_strcmp(str1, str2);
}

int strcmp__pcfp(const char *str1, far_ptr_t str2) {
    return -far_strcmp(str2, str1);
}

int strcmp__fpfp(far_ptr_t str1, far_ptr_t str2) {
    unsigned char a[CHUNK], b[CHUNK];
    unsigned long pos = 0;
    for (;;) {
        unsigned char i;
        far_memcpy_to_local(a, far_at(str1, pos), CHUNK);
        far_memcpy_to_local(b, far_at(str2, pos), CHUNK);
        for (i = 0; i < CHUNK; i++) {
            if (a[i] != b[i]) return (int)a[i] - (int)b[i];
            if (a[i] == 0) return 0;
        }
        pos += CHUNK;
    }
}

/* ============================================================================
 * strcat
 * ========================================================================== */

char *strcat__pcpc(char *dst, const char *src) {
    return strcat(dst, (char *)src);
}

char *strcat__pcfp(char *dst, far_ptr_t src) {
    far_memcpy_to_local(dst + strlen(dst), src, far_strlen(src) + 1);
    return dst;
}

far_ptr_t strcat__fppc(far_ptr_t dst, const char *src) {
    far_memcpy_from_local(far_at(dst, far_strlen(dst)), src, strlen((char *)src) + 1);
    return dst;
}

far_ptr_t strcat__fpfp(far_ptr_t dst, far_ptr_t src) {
    far_memcpy(far_at(dst, far_strlen(dst)), src, far_strlen(src) + 1);
    return dst;
}

/* ============================================================================
 * memcmp
 * ========================================================================== */

int memcmp__pvpvz(const void *ptr1, const void *ptr2, size_t len) {
    return memcmp((void *)ptr1, (void *)ptr2, len);
}

/* Compare len bytes; the far ones are read in chunks (a NULL local side is
 * the other far one) */
static int cmp_chunks(far_ptr_t f1, const unsigned char *l1, far_ptr_t f2, const unsigned char *l2,
                      size_t len) {
    unsigned char a[CHUNK], b[CHUNK];
    size_t off = 0;
    while (off < len) {
        size_t n = len - off > CHUNK ? CHUNK : len - off;
        const unsigned char *p = l1 ? l1 + off : a;
        const unsigned char *q = l2 ? l2 + off : b;
        size_t i;
        if (!l1) far_memcpy_to_local(a, far_at(f1, off), n);
        if (!l2) far_memcpy_to_local(b, far_at(f2, off), n);
        for (i = 0; i < n; i++)
            if (p[i] != q[i]) return (int)p[i] - (int)q[i];
        off += n;
    }
    return 0;
}

int memcmp__fpvz(far_ptr_t ptr1, const void *ptr2, size_t len) {
    return cmp_chunks(ptr1, 0, far_make(BANK0, 0), (const unsigned char *)ptr2, len);
}

int memcmp__pvfz(const void *ptr1, far_ptr_t ptr2, size_t len) {
    return cmp_chunks(far_make(BANK0, 0), (const unsigned char *)ptr1, ptr2, 0, len);
}

int memcmp__fpfz(far_ptr_t ptr1, far_ptr_t ptr2, size_t len) {
    return cmp_chunks(ptr1, 0, ptr2, 0, len);
}

/* ============================================================================
 * Typed far array elements (any index: the address is computed in 32 bits)
 * ========================================================================== */

int16_t far_array_read_int16(far_ptr_t base, int index) {
    return far_read_int16(far_at(base, (unsigned long)(unsigned int)index * 2), 0);
}

void far_array_write_int16(far_ptr_t base, int index, int16_t val) {
    far_write_int16(far_at(base, (unsigned long)(unsigned int)index * 2), 0, val);
}

uint8_t far_array_read_uint8(far_ptr_t base, int index) {
    return far_read_uint8(far_at(base, (unsigned long)(unsigned int)index), 0);
}

void far_array_write_uint8(far_ptr_t base, int index, uint8_t val) {
    far_write_uint8(far_at(base, (unsigned long)(unsigned int)index), 0, val);
}

uint32_t far_array_read_uint32(far_ptr_t base, int index) {
    return far_read_uint32(far_at(base, (unsigned long)(unsigned int)index * 4), 0);
}

void far_array_write_uint32(far_ptr_t base, int index, uint32_t val) {
    far_write_uint32(far_at(base, (unsigned long)(unsigned int)index * 4), 0, val);
}
