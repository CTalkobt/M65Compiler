#include <far_ptr.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ============================================================================
 * Phase 26: Far Memory Allocator Implementation
 * ========================================================================== */

/* Maximum allocations per bank */
#define MAX_ALLOCS_PER_BANK 32

/* Heap start in each bank (reserve first 256 bytes for metadata) */
#define HEAP_START 0x0100
#define HEAP_SIZE (0x10000 - HEAP_START)  /* 64KB - 256 bytes */

/* Allocation record */
typedef struct {
    uint16_t offset;        /* Start offset in bank */
    uint16_t size;          /* Size in bytes */
    int is_free;            /* 1 if free, 0 if allocated */
} alloc_record_t;

/* Per-bank allocator state */
typedef struct {
    alloc_record_t allocs[MAX_ALLOCS_PER_BANK];
    int num_allocs;
    int next_bank;          /* For round-robin policy */
} bank_allocator_t;

/* Global allocator state */
static bank_allocator_t g_banks[8];
static bank_t g_current_bank = BANK0;
static int g_allocation_policy = 0;  /* 0=round-robin, 1=fixed, 2=first-fit */
static bank_t g_fixed_bank = BANK0;

/* Current bank register access (via MMU at $01) */
static volatile uint8_t* g_bank_register = (volatile uint8_t*)0x01;

/* ============================================================================
 * Bank Management Implementation
 * ========================================================================== */

void bank_set(bank_t bank) {
    assert(bank >= 0 && bank < 8);
    g_current_bank = bank;
    *g_bank_register = bank;
}

bank_t bank_get(void) {
    return g_current_bank;
}

size_t bank_available(bank_t bank) {
    assert(bank >= 0 && bank < 8);

    bank_allocator_t* allocator = &g_banks[bank];
    uint16_t used = 0;

    /* Sum all allocated blocks */
    for (int i = 0; i < allocator->num_allocs; ++i) {
        if (!allocator->allocs[i].is_free) {
            used += allocator->allocs[i].size;
        }
    }

    return HEAP_SIZE - used;
}

void bank_reset(bank_t bank) {
    assert(bank >= 0 && bank < 8);

    bank_allocator_t* allocator = &g_banks[bank];
    allocator->num_allocs = 0;
}

void bank_reset_all(void) {
    for (int i = 0; i < 8; ++i) {
        bank_reset((bank_t)i);
    }
}

/* ============================================================================
 * Allocator Initialization
 * ========================================================================== */

static void initialize_bank(bank_t bank) {
    bank_allocator_t* allocator = &g_banks[bank];

    if (allocator->num_allocs == 0) {
        /* First allocation: create initial free block */
        allocator->allocs[0].offset = HEAP_START;
        allocator->allocs[0].size = HEAP_SIZE;
        allocator->allocs[0].is_free = 1;
        allocator->num_allocs = 1;
    }
}

/* ============================================================================
 * Free List Management
 * ========================================================================== */

/* Find a free block large enough for size */
static int find_free_block(bank_t bank, size_t size) {
    bank_allocator_t* allocator = &g_banks[bank];

    for (int i = 0; i < allocator->num_allocs; ++i) {
        if (allocator->allocs[i].is_free &&
            allocator->allocs[i].size >= size) {
            return i;
        }
    }
    return -1;  /* No free block */
}

/* Coalesce adjacent free blocks */
static void coalesce_free_blocks(bank_t bank) {
    bank_allocator_t* allocator = &g_banks[bank];

    int i = 0;
    while (i < allocator->num_allocs - 1) {
        alloc_record_t* curr = &allocator->allocs[i];
        alloc_record_t* next = &allocator->allocs[i + 1];

        /* Check if both free and adjacent */
        if (curr->is_free && next->is_free &&
            curr->offset + curr->size == next->offset) {

            /* Merge */
            curr->size += next->size;

            /* Remove next */
            for (int j = i + 1; j < allocator->num_allocs - 1; ++j) {
                allocator->allocs[j] = allocator->allocs[j + 1];
            }
            allocator->num_allocs--;
        } else {
            i++;
        }
    }
}

/* ============================================================================
 * Allocation Implementation
 * ========================================================================== */

far_ptr_t far_malloc(size_t size, bank_t bank) {
    assert(bank >= 0 && bank < 8);
    assert(size > 0);

    initialize_bank(bank);

    bank_allocator_t* allocator = &g_banks[bank];

    /* Find first free block large enough */
    int block_idx = find_free_block(bank, size);
    if (block_idx < 0) {
        /* No space available */
        return 0;
    }

    alloc_record_t* block = &allocator->allocs[block_idx];
    uint16_t alloc_offset = block->offset;
    uint16_t remaining = block->size - size;

    if (remaining == 0) {
        /* Exact fit: just mark as allocated */
        block->size = size;
        block->is_free = 0;
    } else {
        /* Split block */
        if (allocator->num_allocs >= MAX_ALLOCS_PER_BANK) {
            /* Out of allocation slots */
            return 0;
        }

        /* Create new free block for remainder */
        alloc_record_t* new_block = &allocator->allocs[allocator->num_allocs];
        new_block->offset = block->offset + size;
        new_block->size = remaining;
        new_block->is_free = 1;
        allocator->num_allocs++;

        /* Mark original as allocated */
        block->size = size;
        block->is_free = 0;
    }

    /* Return far pointer: (bank << 16) | offset */
    return far_make(bank, alloc_offset);
}

far_ptr_t far_calloc(size_t count, size_t size, bank_t bank) {
    size_t total = count * size;
    far_ptr_t ptr = far_malloc(total, bank);

    if (far_is_null(ptr)) {
        return 0;
    }

    /* Zero-initialize: switch to bank and memset */
    bank_t saved_bank = bank_get();
    bank_set(bank);
    memset((void*)far_offset(ptr), 0, total);
    bank_set(saved_bank);

    return ptr;
}

far_ptr_t far_realloc(far_ptr_t ptr, size_t new_size, bank_t new_bank) {
    if (far_is_null(ptr)) {
        /* Equivalent to malloc */
        return far_malloc(new_size, new_bank);
    }

    bank_t old_bank = far_bank(ptr);
    uint16_t old_offset = far_offset(ptr);
    size_t old_size = far_size(ptr);

    if (old_size == new_size && old_bank == new_bank) {
        /* No change needed */
        return ptr;
    }

    /* Allocate new block */
    far_ptr_t new_ptr = far_malloc(new_size, new_bank);
    if (far_is_null(new_ptr)) {
        return 0;
    }

    /* Copy old data (if any) */
    if (old_size > 0) {
        size_t copy_size = (old_size < new_size) ? old_size : new_size;
        /* TODO: far_memcpy implementation in Phase 27 */
        /* For now, require same bank for copy */
        if (old_bank == new_bank) {
            bank_t saved = bank_get();
            bank_set(old_bank);
            memcpy((void*)far_offset(new_ptr),
                   (void*)old_offset, copy_size);
            bank_set(saved);
        }
    }

    /* Free old block */
    far_free(ptr);

    return new_ptr;
}

void far_free(far_ptr_t ptr) {
    if (far_is_null(ptr)) {
        return;
    }

    bank_t bank = far_bank(ptr);
    uint16_t offset = far_offset(ptr);

    assert(bank >= 0 && bank < 8);

    bank_allocator_t* allocator = &g_banks[bank];

    /* Find allocation record */
    for (int i = 0; i < allocator->num_allocs; ++i) {
        if (allocator->allocs[i].offset == offset &&
            !allocator->allocs[i].is_free) {

            /* Mark as free */
            allocator->allocs[i].is_free = 1;

            /* Coalesce with adjacent free blocks */
            coalesce_free_blocks(bank);
            return;
        }
    }

    /* Allocation not found (already freed or invalid) */
}

/* ============================================================================
 * Current Bank Allocation Variants
 * ========================================================================== */

far_ptr_t far_malloc_current(size_t size) {
    return far_malloc(size, g_current_bank);
}

far_ptr_t far_calloc_current(size_t count, size_t size) {
    return far_calloc(count, size, g_current_bank);
}

/* ============================================================================
 * Allocation Metadata
 * ========================================================================== */

size_t far_size(far_ptr_t ptr) {
    if (far_is_null(ptr)) {
        return 0;
    }

    bank_t bank = far_bank(ptr);
    uint16_t offset = far_offset(ptr);

    bank_allocator_t* allocator = &g_banks[bank];

    for (int i = 0; i < allocator->num_allocs; ++i) {
        if (allocator->allocs[i].offset == offset &&
            !allocator->allocs[i].is_free) {
            return allocator->allocs[i].size;
        }
    }

    return 0;
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
    g_allocation_policy = 0;
    g_banks[0].next_bank = 0;
}

void far_policy_fixed_bank(bank_t bank) {
    assert(bank >= 0 && bank < 8);
    g_allocation_policy = 1;
    g_fixed_bank = bank;
}

void far_policy_first_fit(void) {
    g_allocation_policy = 2;
}
