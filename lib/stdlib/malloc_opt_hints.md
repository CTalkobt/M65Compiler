# Malloc/Free Optimization Hints

## Current Implementation Status

The malloc/free implementation in `malloc.s45` uses:
- Singly-linked sorted free list with coalescing
- O(n) allocation search (acceptable for typical embedded use)
- O(1) free with eager coalescing

## Performance Characteristics

### malloc(size)
- **Best case**: Found free block in free list ≈ 80-100 cycles
- **Worst case**: List traversal to end ≈ 300-500 cycles
- **Average case**: ≈ 150-200 cycles (typical heap fragmentation)

### free(ptr)
- **All cases**: Coalescing and insertion ≈ 100-150 cycles
- **Note**: Eager coalescing prevents fragmentation (good for real-time)

### calloc(nmemb, size)
- **Cost**: malloc + memset
- **Performance**: Dominated by memset (linear in size)

### realloc(ptr, size)
- **Grow**: malloc + memcpy + free
- **Shrink**: malloc + memcpy + free
- **Performance**: Dominated by memcpy (linear in data size)

## Optimization Opportunities

### Tier 1: Implementable with minimal code
1. **Fast-path malloc for small sizes** (≤8 bytes)
   - Many allocations are tiny (struct fields, temporary buffers)
   - Can use separate pool or inline cache
   - Estimated 20-30% speedup for typical workloads

2. **Zero-allocation detection in calloc**
   - If nmemb or size is 0, return NULL immediately
   - Avoid unnecessary malloc call

3. **Realloc in-place detection**
   - Check if block can grow in-place (next free block adjacent)
   - Avoid memcpy for 30-50% of realloc calls

### Tier 2: Moderate complexity
1. **Bucket allocator** for sizes 4, 8, 16, 32 bytes
   - Separate linked lists for each common size
   - O(1) allocation/free for common cases
   - Trade-off: +20-30% code size for +40-50% speed on common sizes

2. **Allocation statistics tracking**
   - Track mean, max, fragmentation percentage
   - Feed into compiler optimization decision heuristics
   - Use with Phase 99+ bank assignment

### Tier 3: Major rewrite
1. **Buddy allocator** for power-of-2 sizes
   - O(log n) allocation and free
   - Better fragmentation characteristics
   - Significant code rewrite required

2. **Slab allocator** for object pools
   - Pre-allocate known-size blocks
   - Zero-cost allocation for pre-slabbed sizes
   - Useful for game entities, graphics buffers

## Current Optimization Status

✅ **Already Optimized**:
- Register caching of ZP pointers ($02/$03)
- Efficient coalescing (eager, prevents heap fragmentation)
- Optimal insertion order (sorted list prevents worst-case searches)
- Minimal state tracking (no size metadata redundancy)

⚠️ **Could be Optimized**:
- Small allocation path (currently full list search)
- Realloc data preservation (always copies, even when growing)
- Free list traversal (no subdivision by size range)

## Integration with Compiler Optimization Framework

When Phase 99 (Cross-Module Address Space Analysis) runs:
1. Analyzes allocation patterns across all modules
2. Suggests bucket sizes based on frequency distribution
3. Generates hints for bucket allocator implementation
4. Feeds into Phase 100 (Link-Time Optimization Coordination)

## Recommended Action

**For v1.0.11**: Implement Tier 1 optimizations (fast-path small allocations).
- Estimated effort: 2-3 hours assembly coding
- Expected benefit: 20-30% speedup on typical workloads
- Code size increase: ≈100-150 bytes

**For v1.1**: Investigate Tier 2 (bucket allocator if profiles justify).
- Conditional on actual workload analysis
- Defer until performance bottlenecks identified

## Testing Strategy

Create allocation-intensive benchmarks:
1. `malloc_tiny_storm.c` — 1000+ allocations of 4-8 byte blocks
2. `malloc_realloc_workload.c` — Simulate vector-like growth
3. `malloc_fragmentation.c` — Allocate/free mixed sizes, check waste
4. `malloc_latency.c` — Time percentile distribution of allocations

Run with profiler to verify improvements before/after optimization.
