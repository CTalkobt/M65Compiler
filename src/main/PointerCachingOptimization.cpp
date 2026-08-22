#include "PointerCachingOptimization.hpp"
#include <algorithm>
#include <sstream>

PointerCachingOptimization::PointerCachingOptimization()
    : pointersOptimized_(0), bytesReduced_(0) {
    metrics_.optimizationName = "Pointer Chasing Cache";
    metrics_.type = OptimizationType::POINTER_CHASING_CACHE;
}

PointerCachingOptimization::~PointerCachingOptimization() = default;

void PointerCachingOptimization::apply(TranslationUnit& ast) {
    // AST-level pointer usage analysis
    analyzePointerUsage(ast);

    // Filter to cacheable pointers (dereferenced > 2 times)
    pointerUsages_.erase(
        std::remove_if(pointerUsages_.begin(), pointerUsages_.end(),
            [this](const PointerUsage& usage) {
                return !shouldCache(usage);
            }),
        pointerUsages_.end()
    );

    // Allocate cache slots for qualified pointers
    allocateCacheSlots(ast);

    // Update metrics
    for (const auto& usage : pointerUsages_) {
        if (usage.isCacheable) {
            pointersOptimized_++;
            bytesReduced_ += usage.bytesReduced;
        }
    }

    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = pointersOptimized_;
}

void PointerCachingOptimization::apply(ir::Module& /* irModule */) {
    // IR-level pointer caching
    // TODO: Identify DEREF ops on pointer vreg
    // TODO: Count dereferencing frequency per loop
    // TODO: Allocate ZP cache slots for hot pointers
}

void PointerCachingOptimization::analyzePointerUsage(TranslationUnit& /* ast */) {
    // Pattern detection: find pointer dereferences in loops
    // TODO: Walk AST for:
    //   - MemberAccess (p->field)
    //   - ArrayAccess (arr[i])
    //   - UnaryOp (dereference: *p)
    // Count per pointer, track loop context
    // Create PointerUsage records
}

void PointerCachingOptimization::analyzePointerUsageIR(ir::Module& /* irModule */) {
    // IR-level dereference counting
    // TODO: Walk IR for LOAD/DEREF ops with pointer operands
}

bool PointerCachingOptimization::shouldCache(const PointerUsage& usage) const {
    // Cache decision: dereference count threshold
    // Heuristic: cache if dereferenced > 2 times in loop
    // Trade-off: cache load/store vs repeated dereferences
    return usage.dereferenceCount > 2 && usage.isCacheable;
}

int PointerCachingOptimization::estimateCacheBenefit(const PointerUsage& usage) const {
    // Estimate bytes saved by caching
    // Cost: 1 cache load per setup = 3-4 bytes
    // Benefit: each dereference without load = 2-3 bytes saved
    // Net: (dereferenceCount - 1) * 2 - 4
    // Example: 3 dereferences = (3-1)*2 - 4 = 0 (break-even)
    //          4 dereferences = (4-1)*2 - 4 = 2 bytes saved
    //          5+ dereferences = 4+ bytes saved
    if (usage.dereferenceCount < 3) return 0;
    return (usage.dereferenceCount - 1) * 2 - 4;
}

void PointerCachingOptimization::allocateCacheSlots(TranslationUnit& /* ast */) {
    // AST transformation: allocate cache variables and rewrites
    // For each cacheable pointer:
    //   1. Create cache variable: __zp_cache_<name>
    //   2. Initialize cache at loop start: __zp_cache_p = p;
    //   3. Replace dereferences with cache: p->field becomes __zp_cache_p->field
    //   4. Update metrics
    // TODO: Implement cache slot allocation and AST rewriting
}

void PointerCachingOptimization::allocateCacheSlotsIR(ir::Module& /* irModule */) {
    // IR transformation: mark pointer vreg for caching
    // TODO: Create cache allocation hints for code generator
}

std::string PointerCachingOptimization::generateCacheName(const std::string& pointerName) {
    // Generate ZP cache variable name from pointer name
    // Examples:
    //   p -> __zp_cache_p
    //   node -> __zp_cache_node
    //   ptr -> __zp_cache_ptr
    std::stringstream ss;
    ss << "__zp_cache_" << pointerName;
    return ss.str();
}
