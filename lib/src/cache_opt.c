/* cache_opt.c — Cache Optimization Implementation */

#include "cache_opt.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ===== Prefetch Implementation ===== */

void cache_prefetch_read(const void *addr) {
    /* MEGA65 has limited prefetch support; use volatile read as hint */
    volatile const unsigned char *p = (volatile const unsigned char *)addr;
    (void)*p;
}

void cache_prefetch_write(void *addr) {
    /* Write intent prefetch via volatile read-modify-write hint */
    volatile unsigned char *p = (volatile unsigned char *)addr;
    unsigned char val = *p;
    *p = val;
}

void cache_flush_line(void *addr) {
    /* Flush cache line to memory (implementation varies by platform) */
    volatile unsigned char *p = (volatile unsigned char *)addr;
    unsigned char val = *p;
    (void)val;
}

void cache_memory_barrier(void) {
    /* Memory barrier to ensure visibility of prior operations */
    __asm__ volatile("" : : : "memory");
}

/* ===== SoA Layout Implementation ===== */

soa_layout_t soa_create(int num_arrays, int array_element_size, int num_elements) {
    soa_layout_t layout = {0};
    layout.array_count = num_arrays;
    layout.element_size = array_element_size;
    layout.num_elements = num_elements;

    /* Align stride to cache line */
    layout.stride = ALIGN_CACHE_LINE(array_element_size);

    /* Allocate arrays */
    for (int i = 0; i < num_arrays && i < 16; i++) {
        layout.arrays[i] = malloc(layout.stride * num_elements);
    }

    return layout;
}

void *soa_get_array(soa_layout_t *layout, int array_index) {
    if (!layout || array_index < 0 || array_index >= layout->array_count) {
        return NULL;
    }
    return layout->arrays[array_index];
}

void soa_destroy(soa_layout_t *layout) {
    if (!layout) return;

    for (int i = 0; i < layout->array_count && i < 16; i++) {
        free(layout->arrays[i]);
    }

    layout->array_count = 0;
}

/* ===== Access Pattern Analysis ===== */

access_pattern_t analyze_access_pattern(const void *buffer,
                                        int buffer_size,
                                        const int *access_offsets,
                                        int num_accesses) {
    access_pattern_t pattern = {0};

    if (!buffer || !access_offsets || num_accesses == 0) {
        return pattern;
    }

    /* Calculate spatial locality (consecutive accesses close together) */
    int total_distance = 0;
    int max_distance = 0;

    for (int i = 1; i < num_accesses; i++) {
        int distance = access_offsets[i] - access_offsets[i - 1];
        if (distance < 0) distance = -distance;

        total_distance += distance;
        if (distance > max_distance) {
            max_distance = distance;
        }
    }

    /* Spatial locality: 100 = within cache line, 0 = worst case */
    int avg_distance = total_distance / (num_accesses - 1);
    pattern.spatial_locality = 100 - ((avg_distance * 100) / CACHE_LINE_SIZE);
    if (pattern.spatial_locality < 0) pattern.spatial_locality = 0;
    if (pattern.spatial_locality > 100) pattern.spatial_locality = 100;

    /* Temporal locality: assume repeating accesses are likely */
    int repeat_distance = (num_accesses > 0) ? buffer_size / num_accesses : 0;
    pattern.temporal_locality = 100 - ((repeat_distance * 100) / 16384);  /* 16KB working set */
    if (pattern.temporal_locality < 0) pattern.temporal_locality = 0;
    if (pattern.temporal_locality > 100) pattern.temporal_locality = 100;

    /* Stride size */
    pattern.stride_size = (num_accesses > 0) ? avg_distance : 0;

    /* Estimate cache misses (simplified) */
    int expected_miss_rate = (100 - pattern.spatial_locality) / 10;
    pattern.cache_misses_estimated = expected_miss_rate > 0 ? expected_miss_rate : 1;

    return pattern;
}

/* ===== Cache-Aware Algorithms ===== */

void cache_aware_copy(void *dest, const void *src, int size) {
    /* Copy in cache-line sized chunks for better performance */
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;

    int remaining = size;
    while (remaining >= CACHE_LINE_SIZE) {
        /* Prefetch ahead */
        cache_prefetch_read(s + CACHE_LINE_SIZE);

        /* Copy one cache line */
        for (int i = 0; i < CACHE_LINE_SIZE; i++) {
            d[i] = s[i];
        }

        d += CACHE_LINE_SIZE;
        s += CACHE_LINE_SIZE;
        remaining -= CACHE_LINE_SIZE;
    }

    /* Copy remaining bytes */
    while (remaining > 0) {
        *d++ = *s++;
        remaining--;
    }
}

void cache_aware_fill(void *dest, int value, int size) {
    /* Fill in cache-line sized chunks */
    unsigned char *d = (unsigned char *)dest;
    unsigned char v = (unsigned char)(value & 0xFF);

    int remaining = size;
    while (remaining >= CACHE_LINE_SIZE) {
        for (int i = 0; i < CACHE_LINE_SIZE; i++) {
            d[i] = v;
        }

        d += CACHE_LINE_SIZE;
        remaining -= CACHE_LINE_SIZE;
    }

    while (remaining > 0) {
        *d++ = v;
        remaining--;
    }
}

void cache_aware_transpose(int *dest, const int *src, int rows, int cols) {
    /* Transpose with cache-aware block processing */
    #define BLOCK_SIZE 8

    for (int bi = 0; bi < rows; bi += BLOCK_SIZE) {
        for (int bj = 0; bj < cols; bj += BLOCK_SIZE) {
            /* Process block */
            int imax = (bi + BLOCK_SIZE < rows) ? bi + BLOCK_SIZE : rows;
            int jmax = (bj + BLOCK_SIZE < cols) ? bj + BLOCK_SIZE : cols;

            for (int i = bi; i < imax; i++) {
                for (int j = bj; j < jmax; j++) {
                    dest[j * rows + i] = src[i * cols + j];
                }
            }
        }
    }

    #undef BLOCK_SIZE
}

/* ===== Cache Coherency Helpers ===== */

static volatile int *dirty_bitmap = NULL;
static int dirty_bitmap_size = 0;

int cache_line_mark_dirty(void *addr) {
    if (!dirty_bitmap) {
        dirty_bitmap_size = 16384 / CACHE_LINE_SIZE;  /* 16KB / cache line */
        dirty_bitmap = (volatile int *)malloc((dirty_bitmap_size / 32) * sizeof(int));
        if (!dirty_bitmap) return 0;
        memset((void *)dirty_bitmap, 0, (dirty_bitmap_size / 32) * sizeof(int));
    }

    intptr_t line_idx = ((intptr_t)addr / CACHE_LINE_SIZE) % dirty_bitmap_size;
    int word_idx = line_idx / 32;
    int bit_idx = line_idx % 32;

    if (word_idx < 0 || word_idx >= dirty_bitmap_size / 32) return 0;

    dirty_bitmap[word_idx] |= (1 << bit_idx);
    return 1;
}

int cache_line_is_dirty(void *addr) {
    if (!dirty_bitmap) return 0;

    intptr_t line_idx = ((intptr_t)addr / CACHE_LINE_SIZE) % dirty_bitmap_size;
    int word_idx = line_idx / 32;
    int bit_idx = line_idx % 32;

    if (word_idx < 0 || word_idx >= dirty_bitmap_size / 32) return 0;

    return (dirty_bitmap[word_idx] & (1 << bit_idx)) != 0;
}
