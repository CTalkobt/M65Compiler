#pragma once
#include "OptimizationPassBase.hpp"
#include <map>
#include <string>
#include <vector>

namespace ir { struct Module; }
class TranslationUnit;

// Pointer Chasing Optimization
// Caches frequently-dereferenced pointers in local variables
// Avoids repeated memory loads in tight loops
class PointerCachingOptimization : public OptimizationPassBase {
public:
    PointerCachingOptimization();
    ~PointerCachingOptimization() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

private:
    struct PointerUsage {
        std::string pointerName;        // Variable name: p, node, ptr, etc.
        std::string structType;         // Type: struct Node, struct Point, etc.
        int dereferenceCount;           // Times dereferenced in loop
        int loopDepth;                  // Which loop (outer/inner)
        bool isCacheable;              // Should be cached
        int bytesReduced;              // Estimated savings
    };

    struct CacheSlot {
        std::string pointerName;
        std::string cacheVarName;      // __zp_cache_p, __zp_cache_node, etc.
        int accessCount;
    };

    int pointersOptimized_;
    int bytesReduced_;
    std::vector<PointerUsage> pointerUsages_;
    std::vector<CacheSlot> allocatedCaches_;

    // Analysis
    void analyzePointerUsage(TranslationUnit& ast);
    void analyzePointerUsageIR(ir::Module& irModule);

    // Decision making
    bool shouldCache(const PointerUsage& usage) const;
    int estimateCacheBenefit(const PointerUsage& usage) const;

    // Optimization
    void allocateCacheSlots(TranslationUnit& ast);
    void allocateCacheSlotsIR(ir::Module& irModule);

    // Cache variable naming
    std::string generateCacheName(const std::string& pointerName);
};
