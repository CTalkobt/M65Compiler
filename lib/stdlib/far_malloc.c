#include <far_ptr.h>
#include <far_ops.h>
#include <string.h>
#include <stdlib.h>

/* ============================================================================
 * Far memory allocator
 *
 * Allocations live in the chip RAM banks no program or ROM uses: banks 4
 * and 5 ($40000-$5FFFF), and in attic RAM when the program enables it
 * (far_attic_enable: pointers with banks $80-$FF). Bank 0 holds the program, bank 1 BASIC 65's
 * variables, banks 2-3 the ROM; 6-7 are not chip RAM. Other banks get no
 * memory (far_malloc returns the null far pointer). The chip RAM heap
 * (far_malloc, far_free, far_size_long, far_reserve, bank_reset,
 * bank_reset_all) is stdlib_asm/far_heap.s45: first fit over up to 32
 * records a bank, in address order; ranges of banks 4 and 5 a program keeps
 * for itself (far_reserve) and the code overlays' store are never handed
 * out. Attic RAM is far_attic.c's buddy allocator, reached through the
 * __far_attic_* hooks far_attic_hook sets (far_heap.s45 reaches attic RAM
 * through far_attic_hook.c's routes). This file has the rest of the API.
 * ========================================================================== */

/* far_heap.s45: $FF00 less the bytes allocated and kept in bank 4 or 5 */
__fastcall__ size_t __far_heap_avail(bank_t bank);

/* Attic RAM (far_attic.c, set by far_attic_init; null until then) */
far_ptr_t (*__far_attic_alloc)(unsigned long size);
unsigned long (*__far_attic_size)(unsigned long at);
void (*__far_attic_free)(unsigned long at);
unsigned long (*__far_attic_avail)(void);

/* far_malloc_current / far_calloc_current choose their bank by the policy:
 * the selected bank (bank_set, far_policy_fixed_bank; the default, bank 4),
 * first fit (bank 4, then 5) or round robin (alternating, the other bank
 * when one is full) */
#define POLICY_FIXED       0
#define POLICY_FIRST_FIT   1
#define POLICY_ROUND_ROBIN 2
static bank_t g_current_bank = BANK4;
static unsigned char g_allocation_policy = POLICY_FIXED;
static bank_t g_next_bank = BANK4;   /* round robin: the next bank to try */

/* ============================================================================
 * Bank Management Implementation
 * ========================================================================== */

/* The bank far_malloc_current / far_calloc_current use (it maps nothing:
 * far pointers carry their bank) */
void bank_set(bank_t bank) {
    if (bank < 8 || bank == BANK_ATTIC) {
        g_current_bank = bank;
        g_allocation_policy = POLICY_FIXED;
    }
}

bank_t bank_get(void) {
    return g_current_bank;
}

size_t bank_available(bank_t bank) {
    if (bank == BANK_ATTIC) {   /* (at most 65535: see far_attic_available) */
        unsigned long n = __far_attic_avail ? __far_attic_avail() : 0;
        return n > 0xFFFFUL ? 0xFFFF : (size_t)n;
    }
    return __far_heap_avail(bank);
}

/* ============================================================================
 * Allocation Implementation
 * ========================================================================== */

far_ptr_t far_malloc_long(unsigned long size, bank_t bank) {
    if (bank & 0x80)
        return __far_attic_alloc && size ? __far_attic_alloc(size) : far_make(BANK0, 0);
    if (size > 0xFFFFUL) return far_make(BANK0, 0);
    return far_malloc((size_t)size, bank);
}

far_ptr_t far_calloc(size_t count, size_t size, bank_t bank) {
    size_t total = count * size;
    far_ptr_t ptr = far_malloc(total, bank);

    if (!far_is_null(ptr)) far_memzero(ptr, total);
    return ptr;
}

far_ptr_t far_realloc(far_ptr_t ptr, size_t new_size, bank_t new_bank) {
    if (far_is_null(ptr)) {
        /* Equivalent to malloc */
        return far_malloc(new_size, new_bank);
    }

    bank_t old_bank = far_bank(ptr);
    unsigned long old_size = far_size_long(ptr);

    if (old_size == new_size && old_bank == new_bank) {
        /* No change needed */
        return ptr;
    }

    /* Allocate new block */
    far_ptr_t new_ptr = far_malloc(new_size, new_bank);
    if (far_is_null(new_ptr)) {
        return new_ptr;
    }

    /* Copy the old data (any banks) */
    if (old_size > 0)
        far_memcpy(new_ptr, ptr, (old_size < new_size) ? (size_t)old_size : new_size);

    /* Free old block */
    far_free(ptr);

    return new_ptr;
}

/* ============================================================================
 * Current Bank Allocation Variants
 * ========================================================================== */

far_ptr_t far_malloc_current(size_t size) {
    far_ptr_t p;
    bank_t first, second;
    if (g_allocation_policy == POLICY_FIXED) return far_malloc(size, g_current_bank);
    if (g_allocation_policy == POLICY_FIRST_FIT) {
        first = BANK4;
    } else {
        first = g_next_bank;
        g_next_bank = (first == BANK4) ? BANK5 : BANK4;
    }
    second = (first == BANK4) ? BANK5 : BANK4;
    p = far_malloc(size, first);
    if (far_is_null(p)) p = far_malloc(size, second);
    /* (attic RAM, when enabled, after the chip RAM banks) */
    if (far_is_null(p) && __far_attic_alloc) p = __far_attic_alloc(size);
    return p;
}

far_ptr_t far_calloc_current(size_t count, size_t size) {
    size_t total = count * size;
    far_ptr_t ptr = far_malloc_current(total);
    if (!far_is_null(ptr)) far_memzero(ptr, total);
    return ptr;
}

/* ============================================================================
 * Allocation Metadata
 * ========================================================================== */

/* (a block of attic RAM of 64 KB or more: 65535; far_size_long) */
size_t far_size(far_ptr_t ptr) {
    unsigned long n = far_size_long(ptr);
    return n > 0xFFFFUL ? 0xFFFF : (size_t)n;
}

bank_t far_alloc_bank(far_ptr_t ptr) {
    return far_bank(ptr);
}

int far_is_valid(far_ptr_t ptr) {
    return far_size(ptr) > 0;
}

/* ============================================================================
 * Allocation Policies
 * ========================================================================== */

void far_policy_round_robin(void) {
    g_allocation_policy = POLICY_ROUND_ROBIN;
    g_next_bank = BANK4;
}

/* The same as bank_set */
void far_policy_fixed_bank(bank_t bank) {
    bank_set(bank);
}

void far_policy_first_fit(void) {
    g_allocation_policy = POLICY_FIRST_FIT;
}
