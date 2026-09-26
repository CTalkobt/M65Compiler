#include <far_ptr.h>
#include <far_ops.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * Phase 28: Standard Library Overloads
 *
 * Function variants supporting both local and far pointers.
 * Compiler selects correct variant based on parameter types.
 * ========================================================================== */

/* ============================================================================
 * memcpy Overloads (4 variants)
 * ========================================================================== */

/* Original: local to local */
void* memcpy__pvpvst(void* dst, const void* src, size_t len) {
    return memcpy(dst, src, len);
}

/* Far source to local destination: memcpy(void*, far_ptr_t, size_t) */
void* memcpy__fpvst(void* dst, far_ptr_t src, size_t len) {
    far_memcpy_to_local(dst, src, len);
    return dst;
}

/* Local source to far destination: memcpy(far_ptr_t, void*, size_t) */
void* memcpy__pvfpst(far_ptr_t dst, const void* src, size_t len) {
    far_memcpy_from_local(dst, src, len);
    return dst;
}

/* Far to far: memcpy(far_ptr_t, far_ptr_t, size_t) */
void* memcpy__fpfpst(far_ptr_t dst, far_ptr_t src, size_t len) {
    far_memcpy(dst, src, len);
    return dst;
}

/* ============================================================================
 * memset Overloads (2 variants)
 * ========================================================================== */

/* Original: local memory */
void* memset__pvist(void* ptr, int val, size_t len) {
    return memset(ptr, val, len);
}

/* Far memory: memset(far_ptr_t, int, size_t) */
void* memset__fpist(far_ptr_t ptr, int val, size_t len) {
    far_memset(ptr, val, len);
    return ptr;  /* Return as void* for compatibility, but ptr is far_ptr_t */
}

/* ============================================================================
 * strlen Overloads (2 variants)
 * ========================================================================== */

/* Original: local string */
size_t strlen__pc(const char* str) {
    return strlen(str);
}

/* Far string: strlen(far_ptr_t) */
size_t strlen__fp(far_ptr_t str) {
    return far_strlen(str);
}

/* ============================================================================
 * strcpy Overloads (4 variants)
 * ========================================================================== */

/* Original: local to local */
char* strcpy__pcpc(char* dst, const char* src) {
    return strcpy(dst, src);
}

/* Far source to local destination: strcpy(char*, far_ptr_t) */
char* strcpy__pcfp(char* dst, far_ptr_t src) {
    far_strcpy_to_local(dst, src, 256);  /* Assume 256-byte max */
    return dst;
}

/* Local source to far destination: strcpy(far_ptr_t, const char*) */
char* strcpy__fppc(far_ptr_t dst, const char* src) {
    far_strcpy_from_local(dst, src, 256);  /* Assume 256-byte max */
    return dst;  /* Return as char* for compatibility */
}

/* Far to far: strcpy(far_ptr_t, far_ptr_t) */
char* strcpy__fpfp(far_ptr_t dst, far_ptr_t src) {
    /* Copy via local temporary */
    char temp[256];
    far_strcpy_to_local(temp, src, 256);
    far_strcpy_from_local(dst, temp, 256);
    return dst;
}

/* ============================================================================
 * strcmp Overloads (4 variants)
 * ========================================================================== */

/* Original: local to local */
int strcmp__pcpc(const char* str1, const char* str2) {
    return strcmp(str1, str2);
}

/* Far first string: strcmp(far_ptr_t, const char*) */
int strcmp__fppc(far_ptr_t str1, const char* str2) {
    return far_strcmp(str1, str2);
}

/* Local first, far second: strcmp(const char*, far_ptr_t) */
int strcmp__pcfp(const char* str1, far_ptr_t str2) {
    /* Load far string into temp and compare */
    char temp[256];
    far_strcpy_to_local(temp, str2, 256);
    return strcmp(str1, temp);
}

/* Far to far: strcmp(far_ptr_t, far_ptr_t) */
int strcmp__fpfp(far_ptr_t str1, far_ptr_t str2) {
    /* Load both strings into temps and compare */
    char temp1[256];
    char temp2[256];
    far_strcpy_to_local(temp1, str1, 256);
    far_strcpy_to_local(temp2, str2, 256);
    return strcmp(temp1, temp2);
}

/* ============================================================================
 * strcat Overloads (4 variants)
 * ========================================================================== */

/* Original: local to local */
char* strcat__pcpc(char* dst, const char* src) {
    return strcat(dst, src);
}

/* Far source to local destination: strcat(char*, far_ptr_t) */
char* strcat__pcfp(char* dst, far_ptr_t src) {
    /* Load far string and concatenate */
    char temp[256];
    far_strcpy_to_local(temp, src, 256);
    return strcat(dst, temp);
}

/* Local source to far destination: strcat(far_ptr_t, const char*) */
char* strcat__fppc(far_ptr_t dst, const char* src) {
    /* Load far string, concatenate in temp, write back */
    char temp[512];
    far_strcpy_to_local(temp, dst, 256);
    strcat(temp, src);
    far_strcpy_from_local(dst, temp, 512);
    return dst;
}

/* Far to far: strcat(far_ptr_t, far_ptr_t) */
char* strcat__fpfp(far_ptr_t dst, far_ptr_t src) {
    /* Load both strings, concatenate, write back */
    char temp[512];
    char src_temp[256];
    far_strcpy_to_local(temp, dst, 256);
    far_strcpy_to_local(src_temp, src, 256);
    strcat(temp, src_temp);
    far_strcpy_from_local(dst, temp, 512);
    return dst;
}

/* ============================================================================
 * memcmp Overloads (4 variants)
 * ========================================================================== */

/* Original: local to local */
int memcmp__pvpvz(const void* ptr1, const void* ptr2, size_t len) {
    return memcmp(ptr1, ptr2, len);
}

/* Far first buffer: memcmp(far_ptr_t, const void*, size_t) */
int memcmp__fpvz(far_ptr_t ptr1, const void* ptr2, size_t len) {
    /* Load far buffer into temp and compare */
    uint8_t temp[256];
    size_t cmp_len = (len > 256) ? 256 : len;
    far_memcpy_to_local(temp, ptr1, cmp_len);

    int result = memcmp(temp, ptr2, cmp_len);

    /* If buffers match for first chunk but len > 256, continue */
    if (result == 0 && len > 256) {
        int i;
    for (i = 256; i < len; i += 256) {
            size_t chunk = (len - i > 256) ? 256 : (len - i);
            far_memcpy_to_local(temp, far_add(ptr1, i), chunk);
            result = memcmp(temp, (uint8_t*)ptr2 + i, chunk);
            if (result != 0) break;
        }
    }

    return result;
}

/* Local first, far second: memcmp(const void*, far_ptr_t, size_t) */
int memcmp__pvfz(const void* ptr1, far_ptr_t ptr2, size_t len) {
    /* Load far buffer and compare */
    uint8_t temp[256];
    size_t cmp_len = (len > 256) ? 256 : len;
    far_memcpy_to_local(temp, ptr2, cmp_len);

    int result = memcmp(ptr1, temp, cmp_len);

    if (result == 0 && len > 256) {
        int i;
    for (i = 256; i < len; i += 256) {
            size_t chunk = (len - i > 256) ? 256 : (len - i);
            far_memcpy_to_local(temp, far_add(ptr2, i), chunk);
            result = memcmp((uint8_t*)ptr1 + i, temp, chunk);
            if (result != 0) break;
        }
    }

    return result;
}

/* Far to far: memcmp(far_ptr_t, far_ptr_t, size_t) */
int memcmp__fpfz(far_ptr_t ptr1, far_ptr_t ptr2, size_t len) {
    /* Load both buffers in chunks and compare */
    uint8_t temp1[256];
    uint8_t temp2[256];
    size_t offset = 0;

    while (offset < len) {
        size_t chunk = (len - offset > 256) ? 256 : (len - offset);

        far_memcpy_to_local(temp1, far_add(ptr1, offset), chunk);
        far_memcpy_to_local(temp2, far_add(ptr2, offset), chunk);

        int result = memcmp(temp1, temp2, chunk);
        if (result != 0) {
            return result;
        }

        offset += chunk;
    }

    return 0;
}

/* ============================================================================
 * Array Access Helpers (Overloaded for convenience)
 * ========================================================================== */

/* Read int16_t from far array */
int16_t far_array_read_int16(far_ptr_t base, int index) {
    return far_read_int16(base, index * 2);
}

/* Write int16_t to far array */
void far_array_write_int16(far_ptr_t base, int index, int16_t val) {
    far_write_int16(base, index * 2, val);
}

/* Read uint8_t from far array */
uint8_t far_array_read_uint8(far_ptr_t base, int index) {
    return far_read_uint8(base, index);
}

/* Write uint8_t to far array */
void far_array_write_uint8(far_ptr_t base, int index, uint8_t val) {
    far_write_uint8(base, index, val);
}

/* Read uint32_t from far array */
uint32_t far_array_read_uint32(far_ptr_t base, int index) {
    return far_read_uint32(base, index * 4);
}

/* Write uint32_t to far array */
void far_array_write_uint32(far_ptr_t base, int index, uint32_t val) {
    far_write_uint32(base, index * 4, val);
}
