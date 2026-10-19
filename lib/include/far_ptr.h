#ifndef FAR_PTR_H
#define FAR_PTR_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * Phase 26: Far Pointer Infrastructure
 *
 * 24-bit far pointers: a bank and a 16-bit offset. Banks $00-$7F are chip
 * RAM, the linear address (bank << 16) | offset; banks $80-$FF are attic RAM
 * (the 8 MB HyperRAM at $8000000): bank $80 + n is $8000000 + n * 64 KB.
 * far_ops.h reads and writes through DMA. The allocator uses banks 4 and 5
 * ($40000-$5FFFF), the chip RAM no program or ROM uses (but the ranges a
 * program keeps, far_reserve), and attic RAM when the program asks for it
 * (far_attic_init / far_attic_enable: power-of-two blocks).
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
    BANK_ATTIC = 0x80,   /* far_malloc: anywhere in attic RAM (far_attic_init) */
} bank_t;

/* ============================================================================
 * Far Pointer Manipulation (Inline Helpers)
 * ========================================================================== */

/* Create a far pointer from bank and offset */
static inline far_ptr_t far_make(bank_t bank, uint16_t offset) {
    far_ptr_t p;
    p.bytes[0] = (unsigned char)bank;
    p.bytes[1] = (unsigned char)offset;
    p.bytes[2] = (unsigned char)(offset >> 8);
    return p;
}

/* Extract bank from far pointer */
static inline bank_t far_bank(far_ptr_t ptr) {
    return (bank_t)ptr.bytes[0];
}

/* Extract offset from far pointer (lower 16 bits) */
static inline uint16_t far_offset(far_ptr_t ptr) {
    return (uint16_t)(ptr.bytes[1] | ((uint16_t)ptr.bytes[2] << 8));
}

/* The linear (28-bit) address of a far pointer, and back: banks $80-$FF
 * are attic RAM */
static inline unsigned long far_to_linear(far_ptr_t ptr) {
    unsigned long off = (unsigned long)ptr.bytes[1] | ((unsigned long)ptr.bytes[2] << 8);
    if (ptr.bytes[0] & 0x80) return 0x8000000UL + ((unsigned long)(ptr.bytes[0] & 0x7F) << 16) + off;
    return ((unsigned long)ptr.bytes[0] << 16) | off;
}
static inline far_ptr_t far_of_linear(unsigned long a) {
    far_ptr_t p;
    p.bytes[0] = a >= 0x8000000UL ? (unsigned char)(0x80 | ((a - 0x8000000UL) >> 16)) : (unsigned char)(a >> 16);
    p.bytes[1] = (unsigned char)a;
    p.bytes[2] = (unsigned char)(a >> 8);
    return p;
}

/* Check if far pointer is NULL (bank 0, offset 0) */
static inline int far_is_null(far_ptr_t ptr) {
    return (ptr.bytes[0] | ptr.bytes[1] | ptr.bytes[2]) == 0;
}

/* ============================================================================
 * Bank Management
 * ========================================================================== */

/* Select the bank far_malloc_current / far_calloc_current allocate in
 * (bank 4 at first; it maps nothing: far pointers carry their bank) */
void bank_set(bank_t bank);

/* Get current memory bank */
bank_t bank_get(void);

/* Get available free space in a bank (in bytes) */
size_t bank_available(bank_t bank);

/* Reset all bank allocators (clear all far allocations) */
void bank_reset_all(void);

/* Reset allocator for specific bank (frees all; far_reserve ranges stay) */
void bank_reset(bank_t bank);

/* ============================================================================
 * Far Memory Allocation
 * ========================================================================== */

/* Allocate memory in specified bank: BANK4, BANK5, or BANK_ATTIC (a
 * power-of-two block; far_malloc_long for more than 64 KB) */
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

/* Get size of allocation (or 0 if not allocated); in attic RAM the block's
 * size, at most 65535 (far_size_long) */
size_t far_size(far_ptr_t ptr);

/* Get bank of allocation */
bank_t far_alloc_bank(far_ptr_t ptr);

/* Check if pointer is valid (allocated) */
int far_is_valid(far_ptr_t ptr);

/* ============================================================================
 * Bank Allocation Policy
 * ========================================================================== */

/* How far_malloc_current / far_calloc_current choose a bank (far_malloc
 * and far_calloc take theirs as an argument). The default is the bank
 * bank_set selected, bank 4 at first. */

/* Alternate between banks 4 and 5 (the other one when a bank is full) */
void far_policy_round_robin(void);

/* Always the given bank (the same as bank_set) */
void far_policy_fixed_bank(bank_t bank);

/* Bank 4, then bank 5 when bank 4 is full */
void far_policy_first_fit(void);

/* ============================================================================
 * Attic RAM (off until a program asks for it)
 * ========================================================================== */

/* Let the allocator also use attic RAM from first to last (linear
 * addresses, within $8000000-$87FFFFF): far_malloc(size, BANK_ATTIC),
 * far_malloc_long, and after banks 4 and 5 for far_malloc_current with the
 * first-fit and round-robin policies. It checks that the memory is there
 * (not every board or emulator has it) and returns the bytes it took, 0 when
 * there is none. Attic RAM is slower than chip RAM and reached only through
 * far_* / DMA.
 *
 * A buddy allocator: a block is a power of two, at least a granule of
 * 2^order bytes, and aligned to its size (one of 64 KB or less never crosses
 * an attic bank); a free block merges with its buddy. far_size gives the
 * block's size. far_attic_init takes the granule's order (9: 512 bytes ..
 * 20: 1 MB) and memory for its table, FAR_ATTIC_META_SIZE(order) bytes
 * (1 KB for 4 KB granules, 128 bytes for 32 KB) that must stay valid;
 * far_attic_enable is far_attic_init with 4 KB granules and a table of its
 * own. A program that calls neither links no attic code. */
#define FAR_ATTIC_MIN_ORDER 9
#define FAR_ATTIC_MAX_ORDER 20
#define FAR_ATTIC_META_SIZE(order) (1u << (22 - (order)))
unsigned long far_attic_init(unsigned long first, unsigned long last, unsigned char order,
                             unsigned char *meta, unsigned int size);
unsigned long far_attic_enable(unsigned long first, unsigned long last);

/* Bytes of attic RAM not handed out (0 when not enabled) */
unsigned long far_attic_available(void);

/* Attic RAM directly (far_malloc(size, BANK_ATTIC), far_free, far_size_long
 * without the chip RAM heap's code): a program that uses only attic RAM
 * links just the attic allocator */
far_ptr_t far_attic_malloc(unsigned long size);
void far_attic_free(far_ptr_t p);
unsigned long far_attic_size(far_ptr_t p);

/* far_malloc for more than 64 KB: attic RAM (BANK_ATTIC) up to all of it,
 * chip RAM banks up to 65535 bytes. far_size_long gives its size (far_size
 * at most 65535). */
far_ptr_t far_malloc_long(unsigned long size, bank_t bank);
unsigned long far_size_long(far_ptr_t ptr);

/* Keep first..last (linear addresses in banks 4 and 5, $40000-$5FFFF) from
 * the far heap: a program's own buffers at fixed addresses. Call it before
 * the first allocation there (what is allocated already stays so);
 * bank_reset keeps it. At most 8 ranges; 0 when full or outside banks 4-5. */
int far_reserve(unsigned long first, unsigned long last);

#endif
