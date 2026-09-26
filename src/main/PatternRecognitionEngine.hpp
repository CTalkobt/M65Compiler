// Phase 102: Adaptive Optimization Selection
// Analyzes code patterns and selects optimal hints based on characteristics

#pragma once

#include "ComparativePerformanceAnalyzer.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

// Forward declarations
namespace phase101 { class ComparativePerformanceAnalyzer; }

namespace phase102 {

// Code pattern characteristics
struct CodePattern {
    std::string patternName;        // "tight_loop", "struct_heavy", "recursive", etc.
    std::string targetName;         // Function or loop name
    std::string patternType;        // "loop", "function", "struct_access"

    // Pattern metrics
    int estimatedIterations = 0;    // For loops
    int accessFrequency = 0;        // Field/variable accesses
    int callDepth = 0;              // For recursive functions
    bool isRecursive = false;
    bool isLeaf = false;            // No calls to other functions
    bool isHotPath = false;         // In top optimization targets

    // Memory access patterns
    int memoryAccessCount = 0;
    int fieldAccessCount = 0;
    int arrayAccessCount = 0;
    int pointerDereferenceCount = 0;

    // Size characteristics
    int estimatedCodeSize = 0;
    int estimatedStackDepth = 0;
};

// Optimization recommendation
struct OptimizationRecommendation {
    std::shared_ptr<phase100::OptimizationHint> hint;
    double confidenceScore = 0.0;   // 0.0 to 1.0
    double expectedBenefit = 0.0;   // % improvement
    std::vector<std::string> reasoning;  // Why recommended
    bool isApplicable = false;
};

// Pattern analyzer for code characteristics
class PatternRecognitionEngine {
public:
    explicit PatternRecognitionEngine(
        const phase101::ComparativePerformanceAnalyzer& analyzer)
        : analyzer_(analyzer) {}

    // Analyze code patterns in a function/loop
    CodePattern analyzeCodePattern(const std::string& targetName,
                                   const std::string& patternType);

    // Detect pattern type (tight loop, struct-heavy, etc.)
    std::string detectPatternType(const CodePattern& pattern);

    // Analyze memory access patterns
    void analyzeMemoryPatterns(CodePattern& pattern);

    // Analyze control flow patterns
    void analyzeControlFlow(CodePattern& pattern);

    // Generate recommendations based on pattern
    std::vector<OptimizationRecommendation> recommendOptimizations(
        const CodePattern& pattern);

    // Score hint suitability for pattern
    double scoreHintForPattern(
        const std::shared_ptr<phase100::OptimizationHint>& hint,
        const CodePattern& pattern);

    // Generate pattern report
    std::string generatePatternReport(const CodePattern& pattern);

    // Get pattern statistics
    int getTotalPatternsAnalyzed() const { return patternsAnalyzed_; }
    int getHighConfidenceRecommendations() const { return highConfidenceCount_; }

private:
    const phase101::ComparativePerformanceAnalyzer& analyzer_;

    int patternsAnalyzed_ = 0;
    int highConfidenceCount_ = 0;
    std::map<std::string, std::vector<CodePattern>> patternsByType_;

    // Helper methods
    bool isLoopPattern(const CodePattern& pattern);
    bool isStructHeavyPattern(const CodePattern& pattern);
    bool isRecursivePattern(const CodePattern& pattern);
    bool isLeafFunctionPattern(const CodePattern& pattern);
    double calculatePatternHotness(const CodePattern& pattern);
    std::string formatPatternRow(const CodePattern& pattern);
};

} // namespace phase102
