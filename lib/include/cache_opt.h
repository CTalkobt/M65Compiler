/* cache_opt.h — Phase 77: Cache Optimization
 *
 * Cache-line aware data layout, prefetching hints, and access pattern
 * optimization for improved memory hierarchy performance.
 */

#ifndef CACHE_OPT_H
#define CACHE_OPT_H

/* Cache line size (typical 64 bytes, but MEGA65 uses 32-byte effective) */
#define CACHE_LINE_SIZE 32

/* Alignment macros */
#define ALIGN_CACHE_LINE(size) \
    (((size) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))

#define DECLARE_ALIGNED(type, name) \
    type name __attribute__((aligned(CACHE_LINE_SIZE)))

/* Hot/cold data markers for layout optimization */
#define HOT_DATA __attribute__((section(".hot_data")))
#define COLD_DATA __attribute__((section(".cold_data")))
#define READ_MOSTLY __attribute__((aligned(CACHE_LINE_SIZE)))

/* SoA (Structure of Arrays) helper for cache coherency */
typedef struct {
    void *arrays[16];           /* Up to 16 arrays */
    int array_count;
    int element_size;           /* Size per logical element across all arrays */
    int num_elements;
    int stride;                 /* Distance between elements (cache-aligned) */
} soa_layout_t;

/* ===== Cache-Line Prefetch API ===== */

/* Software prefetch to L1 cache (read) */
void cache_prefetch_read(const void *addr);

/* Software prefetch to L1 cache (write intent) */
void cache_prefetch_write(void *addr);

/* Flush cache line from CPU cache to memory */
void cache_flush_line(void *addr);

/* Memory barrier (ensure prior operations visible) */
void cache_memory_barrier(void);

/* ===== Data Layout Optimization ===== */

/* Create SoA layout for optimal cache performance */
soa_layout_t soa_create(int num_arrays, int array_element_size, int num_elements);

/* Get pointer to element in SoA */
void *soa_get_array(soa_layout_t *layout, int array_index);

/* Destroy SoA layout */
void soa_destroy(soa_layout_t *layout);

/* ===== Access Pattern Analysis ===== */

typedef struct {
    int spatial_locality;       /* 0-100: how close accesses are */
    int temporal_locality;      /* 0-100: how soon accesses repeat */
    int stride_size;            /* Typical stride in bytes */
    int cache_misses_estimated; /* Estimated misses per 1000 accesses */
} access_pattern_t;

/* Analyze buffer access pattern (compile-time hint) */
access_pattern_t analyze_access_pattern(const void *buffer,
                                        int buffer_size,
                                        const int *access_offsets,
                                        int num_accesses);

/* ===== Cache-Aware Algorithms ===== */

/* Copy memory with cache optimization (larger block copies) */
void cache_aware_copy(void *dest, const void *src, int size);

/* Fill memory with cache optimization */
void cache_aware_fill(void *dest, int value, int size);

/* Cache-aware array transpose (minimize cache misses) */
void cache_aware_transpose(int *dest, const int *src, int rows, int cols);

/* ===== Cache Coherency Helpers ===== */

/* Declare shared data (multi-core safe) */
typedef struct {
    volatile void *data;
    int dirty;
} shared_cache_line_t;

/* Mark cache line as dirty (needs write-back) */
int cache_line_mark_dirty(void *addr);

/* Check if cache line is dirty */
int cache_line_is_dirty(void *addr);

#endif
