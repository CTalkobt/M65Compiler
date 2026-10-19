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
 * memory (far_malloc returns the null far pointer). The records are in
 * normal RAM; the memory is reached through far_ops (DMA).
 * ========================================================================== */

static int bank_usable(bank_t bank) {
    return bank == BANK4 || bank == BANK5;
}

/* A program with code overlays keeps them in far memory (ln45
 * --overlay-store, bank 5 by default): ln45 defines where, and that part of
 * its bank is never handed out. Without overlays: bank 0, nothing taken
 * (the weak defaults are in crt0: defined in this file, the assembler would
 * take them for constants). */
extern char __ovl_store_bank[], __ovl_store_first[], __ovl_store_last[];
extern char __ovl_store2_bank[], __ovl_store2_first[], __ovl_store2_last[];

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
} bank_allocator_t;

/* Global allocator state */
static bank_allocator_t g_banks[8];

/* Attic RAM (far_attic_enable): one heap over the range the program gave,
 * linear addresses; the records in address order (coalescing needs it).
 * Off until enabled. */
#define MAX_ATTIC_ALLOCS 32
typedef struct {
    unsigned long at;       /* linear address */
    unsigned long size;
    unsigned char is_free;
} attic_record_t;
static attic_record_t g_attic[MAX_ATTIC_ALLOCS];
static int g_attic_n;       /* records in use; 0: attic RAM not enabled */
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
        unsigned long n = far_attic_available();
        return n > 0xFFFFUL ? 0xFFFF : (size_t)n;
    }
    if (!bank_usable(bank)) return 0;

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
    if (bank >= 8) return;

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
        /* the code overlays' store in this bank (the first file's, or a
         * second bank's): a block that is never free (the records stay in
         * address order) */
        int two = (unsigned int)__ovl_store2_bank == (unsigned int)bank && (unsigned int)__ovl_store2_bank;
        if ((unsigned int)__ovl_store_bank == (unsigned int)bank || two) {
            uint16_t first = (uint16_t)(unsigned int)(two ? __ovl_store2_first : __ovl_store_first);
            uint16_t last = (uint16_t)(unsigned int)(two ? __ovl_store2_last : __ovl_store_last);
            int n = 0;
            if (first < HEAP_START) first = HEAP_START;
            if (last >= first) {
                if (first > HEAP_START) {
                    allocator->allocs[n].offset = HEAP_START;
                    allocator->allocs[n].size = first - HEAP_START;
                    allocator->allocs[n].is_free = 1;
                    n++;
                }
                allocator->allocs[n].offset = first;
                allocator->allocs[n].size = last - first + 1;
                allocator->allocs[n].is_free = 0;
                n++;
                if (last != 0xFFFF) {
                    allocator->allocs[n].offset = last + 1;
                    allocator->allocs[n].size = 0xFFFF - last;
                    allocator->allocs[n].is_free = 1;
                    n++;
                }
                allocator->num_allocs = n;
            }
        }
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

/* ============================================================================
 * Attic RAM
 * ========================================================================== */

/* Is there memory at a? (a write read back, a second address not the same
 * byte: no RAM reads back what was written, or mirrors) */
static int attic_probe(unsigned long first, unsigned long last) {
    unsigned char a = 0x5A, b = 0xA5, r = 0;
    far_ptr_t pf = far_of_linear(first), pl = far_of_linear(last);
    far_memcpy_from_local(pf, &a, 1);
    far_memcpy_from_local(pl, &b, 1);
    far_memcpy_to_local(&r, pf, 1);
    if (r != 0x5A) return 0;
    far_memcpy_to_local(&r, pl, 1);
    if (r != 0xA5) return 0;
    a = 0xC3;
    far_memcpy_from_local(pf, &a, 1);
    far_memcpy_to_local(&r, pl, 1);
    return r == 0xA5;
}

unsigned long far_attic_enable(unsigned long first, unsigned long last) {
    if (first < 0x8000000UL) first = 0x8000000UL;
    if (last > 0x87FFFFFUL) last = 0x87FFFFFUL;
    g_attic_n = 0;
    if (last <= first || !attic_probe(first, last)) return 0;
    g_attic[0].at = first;
    g_attic[0].size = last - first + 1;
    g_attic[0].is_free = 1;
    g_attic_n = 1;
    return g_attic[0].size;
}

unsigned long far_attic_available(void) {
    unsigned long n = 0;
    for (int i = 0; i < g_attic_n; ++i)
        if (g_attic[i].is_free) n += g_attic[i].size;
    return n;
}

static far_ptr_t attic_malloc(size_t size) {
    for (int i = 0; i < g_attic_n; ++i) {
        attic_record_t* r = &g_attic[i];
        if (!r->is_free || r->size < size) continue;
        if (r->size > size) {
            /* the rest: a free record after it */
            if (g_attic_n >= MAX_ATTIC_ALLOCS) return far_make(BANK0, 0);
            for (int j = g_attic_n; j > i + 1; --j) g_attic[j] = g_attic[j - 1];
            g_attic[i + 1].at = r->at + size;
            g_attic[i + 1].size = r->size - size;
            g_attic[i + 1].is_free = 1;
            g_attic_n++;
            r->size = size;
        }
        r->is_free = 0;
        return far_of_linear(r->at);
    }
    return far_make(BANK0, 0);
}

/* The record of an allocation in attic RAM, or -1 */
static int attic_find(far_ptr_t ptr) {
    unsigned long a = far_to_linear(ptr);
    for (int i = 0; i < g_attic_n; ++i)
        if (g_attic[i].at == a && !g_attic[i].is_free) return i;
    return -1;
}

static void attic_free(int i) {
    g_attic[i].is_free = 1;
    /* merge with the free neighbours */
    if (i + 1 < g_attic_n && g_attic[i + 1].is_free) {
        g_attic[i].size += g_attic[i + 1].size;
        for (int j = i + 1; j < g_attic_n - 1; ++j) g_attic[j] = g_attic[j + 1];
        g_attic_n--;
    }
    if (i > 0 && g_attic[i - 1].is_free) {
        g_attic[i - 1].size += g_attic[i].size;
        for (int j = i; j < g_attic_n - 1; ++j) g_attic[j] = g_attic[j + 1];
        g_attic_n--;
    }
}

far_ptr_t far_malloc(size_t size, bank_t bank) {
    if ((bank & 0x80) && size) return attic_malloc(size);
    if (!bank_usable(bank) || size == 0) return far_make(BANK0, 0);

    initialize_bank(bank);

    bank_allocator_t* allocator = &g_banks[bank];

    /* Find first free block large enough */
    int block_idx = find_free_block(bank, size);
    if (block_idx < 0) {
        /* No space available */
        return far_make(BANK0, 0);
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
            return far_make(BANK0, 0);
        }

        /* The remainder: a free block right after it (the records stay in
         * address order, which coalescing needs) */
        for (int j = allocator->num_allocs; j > block_idx + 1; --j)
            allocator->allocs[j] = allocator->allocs[j - 1];
        alloc_record_t* new_block = &allocator->allocs[block_idx + 1];
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

    if (!far_is_null(ptr)) far_memzero(ptr, total);
    return ptr;
}

far_ptr_t far_realloc(far_ptr_t ptr, size_t new_size, bank_t new_bank) {
    if (far_is_null(ptr)) {
        /* Equivalent to malloc */
        return far_malloc(new_size, new_bank);
    }

    bank_t old_bank = far_bank(ptr);
    size_t old_size = far_size(ptr);

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
        far_memcpy(new_ptr, ptr, (old_size < new_size) ? old_size : new_size);

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

    if (bank & 0x80) {
        int i = attic_find(ptr);
        if (i >= 0) attic_free(i);
        return;
    }
    if (!bank_usable(bank)) return;

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
    if (far_is_null(p) && g_attic_n) p = attic_malloc(size);
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

size_t far_size(far_ptr_t ptr) {
    if (far_is_null(ptr)) {
        return 0;
    }

    bank_t bank = far_bank(ptr);
    uint16_t offset = far_offset(ptr);

    if (bank & 0x80) {
        int i = attic_find(ptr);
        return i < 0 ? 0 : (size_t)g_attic[i].size;
    }
    if (!bank_usable(bank)) return 0;
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
