#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>

// Phase 96.5: Cross-Module Pointer Field Analysis
// Global database tracking pointer field usage across translation units

namespace phase96_5 {

// Access type for pointer field accesses
enum class AccessType {
    Read,      // Load pointer field
    Write,     // Store to pointer field
    Address,   // Take address of pointer field
};

// Pattern of accessing a pointer field
struct AccessPattern {
    size_t readCount = 0;          // Number of times field read
    size_t writeCount = 0;         // Number of times field written
    bool isSequential = false;      // Fields accessed in order
    bool isNested = false;          // Accessed in loops
    int commonSuccessor = -1;       // Next field usually accessed
    int loopDepth = 0;              // Typical nesting depth
    size_t cacheOpportunities = 0;  // Estimated cache hits if cached
};

// Profile of a single pointer field across all TUs
struct PointerFieldProfile {
    std::string fieldName;                           // e.g., "vertices"
    std::string structName;                          // e.g., "Mesh"
    size_t fieldOffset = 0;                          // Byte offset in struct
    size_t fieldSize = 2;                            // Always 2 bytes for pointer

    // Cross-module usage patterns
    std::set<std::string> accessingFunctions;        // Functions that access field
    std::map<std::string, AccessPattern> patterns;   // Per-function patterns

    // Usage statistics
    size_t totalAccesses = 0;                        // Total access count across TUs
    std::set<std::string> modulesAccessing;          // Which TUs use this field

    // Optimization hints
    bool isCacheCandidate = false;                   // Can benefit from caching
    int suggestedCacheRegister = -1;                 // Suggested ZP register for cache
    std::set<std::string> aliasingFields;            // Other fields accessed together
    double estimatedSavings = 0.0;                   // Estimated bytes saved by caching
};

// Profile of a struct type's pointer fields
struct StructPointerFieldSet {
    std::string structName;
    std::vector<std::shared_ptr<PointerFieldProfile>> fields;
    size_t totalAccessCount = 0;
    bool hasCacheableFields = false;                // Any fields worth caching
};

// Global database singleton
class GlobalPointerFieldDatabase {
public:
    static GlobalPointerFieldDatabase& instance();

    // Recording phase: Collect field access information during compilation
    void recordFieldAccess(const std::string& functionName,
                          const std::string& structName,
                          const std::string& fieldName,
                          size_t fieldOffset,
                          AccessType accessType);

    void recordFieldInLoop(const std::string& functionName,
                           const std::string& structName,
                           const std::string& fieldName,
                           int loopDepth);

    void recordSequentialAccess(const std::string& functionName,
                                const std::string& structName,
                                const std::string& field1,
                                const std::string& field2);

    // Analysis phase: Analyze patterns and compute optimization decisions
    void analyzePatterns();

    // Query phase: Get optimization candidates
    std::vector<std::shared_ptr<PointerFieldProfile>> getOptimizationCandidates() const;
    std::vector<std::shared_ptr<PointerFieldProfile>> getFieldsByStruct(const std::string& structName) const;
    std::shared_ptr<PointerFieldProfile> getFieldProfile(const std::string& structName,
                                                         const std::string& fieldName) const;

    // Cross-module analysis
    std::vector<std::string> getHotStructs() const;  // Structs accessed in >3 TUs
    std::vector<std::string> getGlobalAliasChains() const;

    // Clear database (for testing or multiple compilations)
    void clear();

private:
    GlobalPointerFieldDatabase() = default;
    ~GlobalPointerFieldDatabase() = default;

    // Prevent copying
    GlobalPointerFieldDatabase(const GlobalPointerFieldDatabase&) = delete;
    GlobalPointerFieldDatabase& operator=(const GlobalPointerFieldDatabase&) = delete;

    // Database storage
    std::map<std::string, std::shared_ptr<StructPointerFieldSet>> structFields;
    std::map<std::string, std::shared_ptr<PointerFieldProfile>> fieldProfiles;
    std::map<std::string, std::vector<std::string>> fieldSequences; // field -> [next fields]

    // Helper methods
    std::shared_ptr<PointerFieldProfile> getOrCreateFieldProfile(const std::string& structName,
                                                                  const std::string& fieldName,
                                                                  size_t fieldOffset);
    void computeCacheCandidates();
    void computeAliasingPatterns();
    double computeSavingsEstimate(const PointerFieldProfile& profile) const;
};

} // namespace phase96_5
