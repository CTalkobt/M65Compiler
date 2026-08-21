#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

// Phase 96.5.2: Inter-TU Pattern Detection
// Detect pointer field access patterns across module boundaries

namespace phase96_5 {

// Pattern types detected by inter-TU analysis
enum class PatternType {
    SequentialFieldAccess,      // access field1 then field2
    LoopNestedFieldAccess,      // field accessed in loops
    FieldReuse,                 // same field accessed multiple times
    PointerDereference,         // load pointer then dereference
    AliasChain,                 // pointer aliased across functions
};

// A detected pattern instance
struct DetectedPattern {
    PatternType type;
    std::string description;
    std::set<std::string> involvingFunctions;  // Functions where pattern occurs
    std::set<std::string> involvingStructs;    // Structs involved
    std::set<std::string> involvingFields;     // Field names
    double costWithoutOptimization = 0.0;      // Estimated bytes without opt
    double costWithOptimization = 0.0;         // Estimated bytes with opt
    double savingsEstimate = 0.0;              // Potential savings
    int frequency = 0;                         // How often pattern occurs
};

// Inter-TU pattern detector
class InterTUPatternDetector {
public:
    InterTUPatternDetector();
    ~InterTUPatternDetector() = default;

    // Recording phase: Record field accesses during compilation
    void recordFieldAccess(const std::string& functionName,
                          const std::string& structName,
                          const std::string& fieldName,
                          bool isLoad,
                          bool isDereference);

    void recordFunctionCall(const std::string& caller,
                            const std::string& callee);

    void recordFieldAliasing(const std::string& field1,
                             const std::string& field2);

    // Analysis phase: Detect patterns across all recorded accesses
    void detectPatterns();

    // Query phase: Get detected patterns
    std::vector<DetectedPattern> getDetectedPatterns() const;
    std::vector<DetectedPattern> getPatternsByType(PatternType type) const;
    std::vector<DetectedPattern> getHighValuePatterns(double minSavings = 5.0) const;

    // Debug/reporting
    void printReport(std::ostream& out) const;
    int getTotalPatternsDetected() const { return detectedPatterns.size(); }
    double getTotalEstimatedSavings() const;

    // Clear detector (for testing)
    void clear();

private:
    // Recorded data
    std::map<std::string, std::set<std::pair<std::string, std::string>>> functionFieldAccesses;
    std::map<std::string, std::vector<std::string>> fieldAccessSequences;
    std::map<std::string, std::set<std::string>> aliasGroups;
    std::map<std::string, std::set<std::string>> callGraph;

    // Detected patterns storage
    std::vector<DetectedPattern> detectedPatterns;

    // Detection methods (called by detectPatterns())
    void detectSequentialFieldAccess();
    void detectLoopNestedAccess();
    void detectFieldReuse();
    void detectPointerDereference();
    void detectAliasChains();

    // Helper methods
    double computeSequentialAccessCost(const std::string& field1,
                                       const std::string& field2) const;
    double computeLoopAccessCost(const std::string& fieldName,
                                 int loopDepth) const;
    double computeFieldReuseCost(const std::string& fieldName,
                                 int reuseCount) const;
};

} // namespace phase96_5
