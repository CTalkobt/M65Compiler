#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Phase 96.5.4: Linker-Level Field Optimization
// Coordinate cross-module field caching at linking time

namespace phase96_5 {

// Field caching directive for assembly
struct FieldCachingDirective {
    std::string fieldName;                  // "mesh.vertices"
    std::string structName;                 // "Mesh"
    int suggestedRegister;                  // ZP register ($60-$70)
    std::string cachingStrategy;            // "function-level", "loop-level", etc.
    double estimatedSavings;                // Bytes saved
    std::vector<std::string> invalidationPoints;  // Where to invalidate
};

// Cross-module field profile merged from all object files
struct MergedFieldProfile {
    std::string fieldName;
    std::string structName;
    std::set<std::string> accessingModules;  // Which TUs access this field
    std::set<std::string> accessingFunctions; // All accessing functions
    size_t totalAccessCount;                 // Total accesses across TUs
    double globalSavingsEstimate;            // Savings across all TUs
    bool shouldCacheGlobally;                // Beneficial across TUs?
};

// Dispatcher for field access (extends Phase 91.4)
struct FieldDispatcher {
    std::string dispatcherName;              // "_field_cache_dispatch_vertices"
    std::string fieldName;
    std::vector<std::string> cachedVersions; // Functions with cached access
    std::vector<std::string> uncachedVersions; // Functions without cache
    int dispatcherSize;                     // Bytes of dispatcher code
};

// Assembly hint for field caching optimization
struct AssemblyHint {
    enum class Type {
        CacheLoad,                          // Load pointer into cache register
        CacheReuse,                         // Reuse cached pointer
        CacheInvalidate,                    // Invalidate cache
        CacheCheck,                         // Check if cache valid
        RegisterAllocation,                 // Allocate register
    };

    Type type;
    std::string fieldName;
    int lineNumber;                         // In assembly
    std::string description;
    int estimatedByteSavings;
};

// Linker-level field optimization coordinator
class LinkerFieldOptimizer {
public:
    LinkerFieldOptimizer();
    ~LinkerFieldOptimizer() = default;

    // Phase 1: Merge field profiles from all object files
    void mergeFieldProfiles(const std::vector<std::string>& objectFiles);

    void addFieldProfile(const std::string& structName,
                        const std::string& fieldName,
                        const std::string& sourceModule,
                        const std::vector<std::string>& accessingFunctions);

    // Phase 2: Analyze cross-module opportunities
    void analyzeGlobalOptimizationOpportunities();

    // Phase 3: Generate dispatchers for multi-version fields
    void generateFieldDispatchers();

    // Phase 4: Emit assembly caching hints
    void emitCachingDirectives();

    // Phase 5: Coordinate with assembler
    void coordinateWithAssembler();

    // Query methods
    std::vector<MergedFieldProfile> getGlobalCachingCandidates() const;
    std::vector<FieldCachingDirective> getCachingDirectives() const;
    std::vector<FieldDispatcher> getFieldDispatchers() const;
    std::vector<AssemblyHint> getAssemblyHints() const;

    // Statistics and reporting
    double getTotalGlobalSavings() const;
    int getFieldDispatcherCount() const;
    int getTotalCachingHints() const;
    std::set<std::string> getOptimizedStructs() const;

    // Output generation
    void generateLinkerScript(const std::string& outputFile);
    void generateAssemblyHints(const std::string& outputFile);
    void printOptimizationReport(std::ostream& out) const;
    void printDispatcherMap(std::ostream& out) const;

    void clear();

private:
    // Cross-module data
    std::map<std::string, MergedFieldProfile> mergedProfiles;
    std::vector<FieldCachingDirective> cachingDirectives;
    std::vector<FieldDispatcher> dispatchers;
    std::vector<AssemblyHint> assemblyHints;

    // Helper methods
    void mergeProfilesFromObjectFile(const std::string& objectFile);
    void computeGlobalSavings();
    void identifyDispatcherOpportunities();
    void generateDispatcherCode(const std::string& fieldName);
    void emitHintsForField(const std::string& fieldName,
                          const MergedFieldProfile& profile);

    // Register allocation coordination
    int allocateGlobalRegister(const std::string& fieldName,
                               double savingsEstimate);
    void validateRegisterAllocation();
};

// Assembler coordinator for Phase 96.5.4
class AssemblerCoordinator {
public:
    AssemblerCoordinator();
    ~AssemblerCoordinator() = default;

    // Register linker directives with assembler
    void registerCachingDirective(const FieldCachingDirective& directive);

    // Apply field caching during assembly
    void applyFieldCachingOptimization(const std::string& fieldName,
                                       const std::string& assemblyCode);

    // Validate assembly output
    bool validateCachingImplementation();

    // Query optimization state
    bool isFieldCached(const std::string& fieldName) const;
    int getCacheRegister(const std::string& fieldName) const;
    std::string getCachingStrategy(const std::string& fieldName) const;

    // Report optimization results
    void printOptimizationResults(std::ostream& out) const;

    void clear();

private:
    std::map<std::string, FieldCachingDirective> activeDirectives;
    std::map<std::string, int> registerAllocation;
    std::map<std::string, std::vector<AssemblyHint>> appliedHints;
};

} // namespace phase96_5
