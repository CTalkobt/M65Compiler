#ifndef FAR_PTR_H
#define FAR_PTR_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Phase 26: Far Pointer Infrastructure
 *
 * 24-bit far pointers for MEGA65 extended memory (8 banks × 64KB each)
 * Format: [BANK:8][OFFSET:16]
 * ========================================================================== */

/* 24-bit far pointer type (3 bytes: bank + offset) */
struct __far_ptr {
    unsigned char bytes[3];  /* [0]=bank, [1]=offset_low, [2]=offset_high */
};

typedef struct __far_ptr far_ptr_t;
typedef unsigned int uint24_t;  /* 24-bit unsigned integer (3 bytes) */

/* Bank enumeration (0-7 for MEGA65 extended memory) */
typedef enum {
    BANK0 = 0,
    BANK1 = 1,
    BANK2 = 2,
    BANK3 = 3,
    BANK4 = 4,
    BANK5 = 5,
    BANK6 = 6,
    BANK7 = 7,
} bank_t;

/* ============================================================================
 * Far Pointer Manipulation (Inline Helpers)
 * ========================================================================== */

/* Create a far pointer from bank and offset */
static inline far_ptr_t far_make(bank_t bank, uint16_t offset) {
    return ((far_ptr_t)bank << 16) | offset;
}

/* Extract bank from far pointer (upper 8 bits) */
static inline bank_t far_bank(far_ptr_t ptr) {
    return (bank_t)((ptr >> 16) & 0xFF);
}

/* Extract offset from far pointer (lower 16 bits) */
static inline uint16_t far_offset(far_ptr_t ptr) {
    return ptr & 0xFFFF;
}

/* Check if far pointer is NULL */
static inline int far_is_null(far_ptr_t ptr) {
    return ptr == 0;
}

/* ============================================================================
 * Bank Management
 * ========================================================================== */

/* Set current memory bank (affects local memory access) */
void bank_set(bank_t bank);

/* Get current memory bank */
bank_t bank_get(void);

/* Get available free space in a bank (in bytes) */
size_t bank_available(bank_t bank);

/* Reset all bank allocators (clear all far allocations) */
void bank_reset_all(void);

/* Reset allocator for specific bank */
void bank_reset(bank_t bank);

/* ============================================================================
 * Far Memory Allocation
 * ========================================================================== */

/* Allocate memory in specified bank */
far_ptr_t far_malloc(size_t size, bank_t bank);

/* Allocate and zero-initialize in specified bank */
far_ptr_t far_calloc(size_t count, size_t size, bank_t bank);

/* Resize allocation (may move to different location/bank if necessary) */
far_ptr_t far_realloc(far_ptr_t ptr, size_t new_size, bank_t new_bank);

/* Free allocation */
void far_free(far_ptr_t ptr);

/* ============================================================================
 * Bank Allocation Variants (Use Current Bank)
 * ========================================================================== */

/* Allocate in current bank (user responsible for bank context) */
far_ptr_t far_malloc_current(size_t size);

/* Allocate and zero-initialize in current bank */
far_ptr_t far_calloc_current(size_t count, size_t size);

/* ============================================================================
 * Allocation Metadata
 * ========================================================================== */

/* Get size of allocation (or 0 if not allocated) */
size_t far_size(far_ptr_t ptr);

/* Get bank of allocation */
bank_t far_alloc_bank(far_ptr_t ptr);

/* Check if pointer is valid (allocated) */
int far_is_valid(far_ptr_t ptr);

/* ============================================================================
 * Bank Allocation Policy
 * ========================================================================== */

/* Use round-robin bank allocation (default) */
void far_policy_round_robin(void);

/* Use specific bank for new allocations */
void far_policy_fixed_bank(bank_t bank);

/* Use first-fit across all banks */
void far_policy_first_fit(void);

#endif
