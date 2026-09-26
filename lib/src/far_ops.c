#include <far_ops.h>
#include <far_ptr.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ============================================================================
 * Phase 27: Far Pointer Operations Implementation
 * ========================================================================== */

/* Bank register for switching memory context */
static volatile uint8_t* const g_bank_reg = (volatile uint8_t*)0x01;

/* Helper: Switch to bank, execute operation, restore bank */
static inline bank_t save_bank(void) {
    return *g_bank_reg & 0x07;
}

static inline void restore_bank(bank_t bank) {
    *g_bank_reg = bank;
}

/* ============================================================================
 * Element-Wise Access Implementation
 * ========================================================================== */

uint8_t far_read_uint8(far_ptr_t ptr, int offset) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    uint8_t val = *(volatile uint8_t*)addr;

    restore_bank(saved);
    return val;
}

int8_t far_read_int8(far_ptr_t ptr, int offset) {
    return (int8_t)far_read_uint8(ptr, offset);
}

uint16_t far_read_uint16(far_ptr_t ptr, int offset) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    uint16_t val = *(volatile uint16_t*)addr;

    restore_bank(saved);
    return val;
}

int16_t far_read_int16(far_ptr_t ptr, int offset) {
    return (int16_t)far_read_uint16(ptr, offset);
}

uint32_t far_read_uint32(far_ptr_t ptr, int offset) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    uint32_t val = *(volatile uint32_t*)addr;

    restore_bank(saved);
    return val;
}

int32_t far_read_int32(far_ptr_t ptr, int offset) {
    return (int32_t)far_read_uint32(ptr, offset);
}

float far_read_float(far_ptr_t ptr, int offset) {
    uint32_t bits = far_read_uint32(ptr, offset);
    return *(float*)&bits;
}

double far_read_double(far_ptr_t ptr, int offset) {
    /* Double is 5 bytes (MEGA65 CBM float format) */
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    /* Read 5 bytes (assuming double is stored as 5 bytes) */
    uint8_t bytes[5];
    for (int i = 0; i < 5; ++i) {
        bytes[i] = *(volatile uint8_t*)(addr + i);
    }

    restore_bank(saved);

    /* Return as double (implementation assumes CBM format) */
    double d = *(double*)bytes;
    return d;
}

void far_write_uint8(far_ptr_t ptr, int offset, uint8_t val) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    *(volatile uint8_t*)addr = val;

    restore_bank(saved);
}

void far_write_int8(far_ptr_t ptr, int offset, int8_t val) {
    far_write_uint8(ptr, offset, (uint8_t)val);
}

void far_write_uint16(far_ptr_t ptr, int offset, uint16_t val) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    *(volatile uint16_t*)addr = val;

    restore_bank(saved);
}

void far_write_int16(far_ptr_t ptr, int offset, int16_t val) {
    far_write_uint16(ptr, offset, (uint16_t)val);
}

void far_write_uint32(far_ptr_t ptr, int offset, uint32_t val) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    *(volatile uint32_t*)addr = val;

    restore_bank(saved);
}

void far_write_int32(far_ptr_t ptr, int offset, int32_t val) {
    far_write_uint32(ptr, offset, (uint32_t)val);
}

void far_write_float(far_ptr_t ptr, int offset, float val) {
    uint32_t bits = *(uint32_t*)&val;
    far_write_uint32(ptr, offset, bits);
}

void far_write_double(far_ptr_t ptr, int offset, double val) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    uint8_t* bytes = (uint8_t*)&val;
    for (int i = 0; i < 5; ++i) {
        *(volatile uint8_t*)(addr + i) = bytes[i];
    }

    restore_bank(saved);
}

/* ============================================================================
 * Bulk Memory Operations Implementation
 * ========================================================================== */

void far_memcpy(far_ptr_t dst, far_ptr_t src, size_t len) {
    if (len == 0) return;

    bank_t dst_bank = far_bank(dst);
    bank_t src_bank = far_bank(src);
    uint16_t dst_offset = far_offset(dst);
    uint16_t src_offset = far_offset(src);

    if (dst_bank == src_bank) {
        /* Same bank: direct memcpy */
        bank_t saved = save_bank();
        *g_bank_reg = src_bank;

        memcpy((void*)dst_offset, (void*)src_offset, len);

        restore_bank(saved);
    } else {
        /* Cross-bank: byte-by-byte with bank switching */
        for (size_t i = 0; i < len; ++i) {
            uint8_t byte = far_read_uint8(src, i);
            far_write_uint8(dst, i, byte);
        }
    }
}

void far_memcpy_to_local(void* dst, far_ptr_t src, size_t len) {
    if (len == 0) return;

    bank_t src_bank = far_bank(src);
    uint16_t src_offset = far_offset(src);

    bank_t saved = save_bank();
    *g_bank_reg = src_bank;

    memcpy(dst, (void*)src_offset, len);

    restore_bank(saved);
}

void far_memcpy_from_local(far_ptr_t dst, const void* src, size_t len) {
    if (len == 0) return;

    bank_t dst_bank = far_bank(dst);
    uint16_t dst_offset = far_offset(dst);

    bank_t saved = save_bank();
    *g_bank_reg = dst_bank;

    memcpy((void*)dst_offset, src, len);

    restore_bank(saved);
}

void far_memset(far_ptr_t ptr, int val, size_t len) {
    if (len == 0) return;

    bank_t bank = far_bank(ptr);
    uint16_t offset = far_offset(ptr);

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    memset((void*)offset, val, len);

    restore_bank(saved);
}

void far_memzero(far_ptr_t ptr, size_t len) {
    far_memset(ptr, 0, len);
}

/* ============================================================================
 * Pointer Arithmetic Implementation
 * ========================================================================== */

far_ptr_t far_add(far_ptr_t ptr, int offset) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) + offset;

    /* Note: Could wrap to next bank if offset crosses boundary
       For now, simple addition within bank */
    return far_make(bank, addr);
}

far_ptr_t far_sub(far_ptr_t ptr, int offset) {
    bank_t bank = far_bank(ptr);
    uint16_t addr = far_offset(ptr) - offset;

    return far_make(bank, addr);
}

int far_diff(far_ptr_t ptr1, far_ptr_t ptr2) {
    /* Only valid if same bank */
    assert(far_bank(ptr1) == far_bank(ptr2));

    uint16_t off1 = far_offset(ptr1);
    uint16_t off2 = far_offset(ptr2);

    return (int)off1 - (int)off2;
}

far_ptr_t far_inc(far_ptr_t ptr) {
    return far_add(ptr, 1);
}

far_ptr_t far_dec(far_ptr_t ptr) {
    return far_sub(ptr, 1);
}

/* ============================================================================
 * Comparison Implementation
 * ========================================================================== */

int far_cmp(far_ptr_t ptr1, far_ptr_t ptr2) {
    /* Compare bank first, then offset */
    bank_t b1 = far_bank(ptr1);
    bank_t b2 = far_bank(ptr2);

    if (b1 != b2) {
        return (int)b1 - (int)b2;
    }

    uint16_t o1 = far_offset(ptr1);
    uint16_t o2 = far_offset(ptr2);

    return (int)o1 - (int)o2;
}

int far_eq(far_ptr_t ptr1, far_ptr_t ptr2) {
    return ptr1 == ptr2;
}

int far_lt(far_ptr_t ptr1, far_ptr_t ptr2) {
    return far_cmp(ptr1, ptr2) < 0;
}

int far_gt(far_ptr_t ptr1, far_ptr_t ptr2) {
    return far_cmp(ptr1, ptr2) > 0;
}

/* ============================================================================
 * Array Access Helpers Implementation
 * ========================================================================== */

far_ptr_t far_array_element(far_ptr_t base, int index, size_t element_size) {
    return far_add(base, index * element_size);
}

void far_array_read(far_ptr_t base, int index, size_t element_size,
                    void* dst, size_t dst_size) {
    assert(dst_size >= element_size);

    far_ptr_t elem = far_array_element(base, index, element_size);
    far_memcpy_to_local(dst, elem, element_size);
}

void far_array_write(far_ptr_t base, int index, size_t element_size,
                     const void* src, size_t src_size) {
    assert(src_size >= element_size);

    far_ptr_t elem = far_array_element(base, index, element_size);
    far_memcpy_from_local(elem, src, element_size);
}

/* ============================================================================
 * String Operations Implementation
 * ========================================================================== */

size_t far_strlen(far_ptr_t str) {
    size_t len = 0;

    bank_t bank = far_bank(str);
    uint16_t offset = far_offset(str);

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    while (*(volatile uint8_t*)(offset + len) != 0) {
        len++;
    }

    restore_bank(saved);
    return len;
}

void far_strcpy_to_local(char* dst, far_ptr_t src, size_t max_len) {
    size_t len = 0;

    bank_t bank = far_bank(src);
    uint16_t offset = far_offset(src);

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    while (len < max_len - 1) {
        uint8_t ch = *(volatile uint8_t*)(offset + len);
        dst[len] = ch;

        if (ch == 0) break;
        len++;
    }

    dst[len] = 0;  /* Null terminate */
    restore_bank(saved);
}

void far_strcpy_from_local(far_ptr_t dst, const char* src, size_t max_len) {
    size_t len = 0;

    bank_t bank = far_bank(dst);
    uint16_t offset = far_offset(dst);

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    while (len < max_len - 1 && src[len] != 0) {
        *(volatile uint8_t*)(offset + len) = src[len];
        len++;
    }

    *(volatile uint8_t*)(offset + len) = 0;  /* Null terminate */
    restore_bank(saved);
}

int far_strcmp(far_ptr_t str1, const char* str2) {
    size_t i = 0;

    bank_t bank = far_bank(str1);
    uint16_t offset = far_offset(str1);

    bank_t saved = save_bank();
    *g_bank_reg = bank;

    while (1) {
        uint8_t ch1 = *(volatile uint8_t*)(offset + i);
        uint8_t ch2 = str2[i];

        if (ch1 != ch2) {
            restore_bank(saved);
            return (int)ch1 - (int)ch2;
        }

        if (ch1 == 0) break;
        i++;
    }

    restore_bank(saved);
    return 0;
}

/* ============================================================================
 * Smart Transfer Variants Implementation
 * ========================================================================== */

void far_memcpy_smart(far_ptr_t dst, far_ptr_t src, size_t len) {
    /* If both in same bank, use fast path */
    if (far_bank(dst) == far_bank(src)) {
        bank_t bank = far_bank(src);
        uint16_t dst_offset = far_offset(dst);
        uint16_t src_offset = far_offset(src);

        bank_t saved = save_bank();
        *g_bank_reg = bank;

        memcpy((void*)dst_offset, (void*)src_offset, len);

        restore_bank(saved);
    } else {
        /* Cross-bank: use byte-by-byte */
        far_memcpy(dst, src, len);
    }
}

void far_memcpy_stream(far_ptr_t dst, far_ptr_t src, size_t len) {
    /* Streaming copy: currently same as smart, but hints for DMA optimization */
    far_memcpy_smart(dst, src, len);
}
