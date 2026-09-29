#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Phase 96.5.3: Cross-Module Caching Analysis
// Analyze register pressure, lifetimes, and cost-benefit for field caching

namespace phase96_5 {

// Register availability tracking
struct RegisterAvailability {
    int registerNum;                    // ZP register number ($60-$70)
    std::string occupantField;          // Currently cached field (if any)
    int pressureLevel;                  // 0=available, 1=contested, 2=unavailable
    std::set<std::string> competingFields;  // Fields that could use this register
};

// Field pointer lifetime information
struct FieldPointerLifetime {
    std::string fieldName;
    std::string functionName;
    int firstAccessLine;                // Source line of first access
    int lastAccessLine;                 // Source line of last access
    int accessSpan;                     // lastAccessLine - firstAccessLine
    int accessCount;                    // Total accesses in this function
    bool crossesFunctionCall;           // Lifetime spans function call?
    bool crossesMemoryWrite;            // Lifetime spans potential memory write?
    int loopNestingLevel;               // Deepest loop nesting
};

// Cache invalidation event
struct InvalidationEvent {
    enum class Type {
        FunctionCall,                   // JSR instruction
        MemoryWrite,                    // Store to memory
        GlobalWrite,                    // Global variable write
        ConditionalBranch,              // Branch that could modify memory
    };

    Type type;
    int lineNumber;
    std::string description;
    bool isConservative;                // Conservative invalidation?
};

// Caching decision for a single field
struct CachingDecision {
    std::string fieldName;
    std::string structName;
    bool shouldCache;                   // True if caching is beneficial
    int suggestedRegister;              // ZP register ($60-$70)
    double estimatedSavings;            // Bytes saved by caching
    double implementationCost;          // Bytes added by caching logic
    double netBenefit;                  // estimatedSavings - implementationCost

    std::vector<InvalidationEvent> invalidationEvents;
    std::string cachingStrategy;        // "function-level", "loop-level", "conservative"
    int recommendedCacheDuration;       // Lines of code cache is valid
};

// Cross-module caching analysis engine
class FieldCachingAnalyzer {
public:
    FieldCachingAnalyzer();
    ~FieldCachingAnalyzer() = default;

    // Register pressure analysis phase
    void analyzeRegisterPressure(const std::vector<std::string>& fieldCandidates,
                                 const std::vector<std::string>& compilingFunctions);

    // Lifetime analysis phase
    void analyzeFieldLifetimes(const std::string& functionName,
                               const std::vector<FieldPointerLifetime>& lifetimes);

    // Invalidation strategy planning
    void planInvalidationStrategy(const std::string& functionName,
                                 const std::vector<InvalidationEvent>& events);

    // Cost-benefit analysis phase
    void analyzeCostBenefit();

    // Query and decision making
    std::vector<CachingDecision> getOptimalCachingDecisions();
    std::vector<CachingDecision> getCachingDecisionsByFunction(const std::string& funcName);

    RegisterAvailability getRegisterAvailability(int registerNum) const;
    std::vector<RegisterAvailability> getAvailableRegisters() const;

    // Validation and diagnostics
    bool validateCachingDecision(const CachingDecision& decision) const;
    double getTotalEstimatedSavings() const;
    int getRegisterAllocationScore() const;

    // Debug output
    void printAnalysisReport(std::ostream& out) const;
    void printRegisterAllocationMap(std::ostream& out) const;
    void printInvalidationStrategy(std::ostream& out) const;

    void clear();

private:
    // Analysis data
    std::vector<RegisterAvailability> registers;
    std::map<std::string, std::vector<FieldPointerLifetime>> fieldLifetimes;
    std::map<std::string, std::vector<InvalidationEvent>> functionInvalidationEvents;
    std::vector<CachingDecision> cachingDecisions;

    // Helper methods
    void initializeRegisters();

    // Register pressure methods
    double computeRegisterPressure(const std::string& fieldName) const;
    int computeConflictScore(const std::string& field1, const std::string& field2) const;
    int allocateOptimalRegister(const std::string& fieldName);

    // Lifetime analysis methods
    int computeLifetimeScore(const FieldPointerLifetime& lifetime) const;
    bool lifetimeSpansFunctionCall(const FieldPointerLifetime& lifetime,
                                   const std::vector<InvalidationEvent>& events) const;

    // Invalidation strategy methods
    std::string determineInvalidationStrategy(const FieldPointerLifetime& lifetime,
                                             const std::vector<InvalidationEvent>& events) const;
    void planConservativeInvalidation(const std::string& functionName);
    void planAggressiveInvalidation(const std::string& functionName);

    // Cost-benefit methods
    double computeImplementationCost(const std::string& fieldName,
                                    const std::string& cachingStrategy) const;
    double computeNetBenefit(double savings, double cost) const;
};

} // namespace phase96_5
