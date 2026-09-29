// Phase 106.2: Optimization Learner
// Learns from benchmark results to improve optimization strategy selection

#pragma once

#include "IterationManager.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase106 {

// Learned optimization pattern
struct OptimizationPattern {
    std::string patternName;
    std::string description;

    std::vector<std::string> applicableOptimizations;
    std::vector<std::string> benchmarkTypes;  // "loops", "structs", "memory", "mixed"

    double expectedSpeedup = 0.0;
    double expectedSizeReduction = 0.0;
    int observationCount = 0;
    double confidence = 0.0;  // 0.0-1.0 based on consistency
};

// Workload classifier for benchmark programs
struct WorkloadCharacterization {
    std::string benchmarkName;
    std::string primaryType;  // "loop-heavy", "struct-intensive", "memory-bound"
    double loopIntensity = 0.0;
    double memoryIntensity = 0.0;
    double structIntensity = 0.0;
};

// Optimization learner coordinator
class OptimizationLearner {
public:
    OptimizationLearner() = default;

    // Learn patterns from iteration results
    void learnFromResults(const std::vector<IterationResult>& results);

    // Classify workload characteristics
    WorkloadCharacterization classifyWorkload(
        const phase104::BenchmarkProgram& program);

    // Predict best optimization for a workload
    std::string predictBestOptimization(
        const WorkloadCharacterization& workload);

    // Compute pattern confidence
    double computePatternConfidence(const OptimizationPattern& pattern);

    // Get learned patterns
    const std::vector<OptimizationPattern>& getLearnedPatterns() const {
        return learnedPatterns_;
    }

    // Extract optimization heuristics
    struct OptimizationHeuristic {
        std::string condition;      // e.g., "loop_intensive"
        std::string recommendation; // e.g., "use loop-unrolling"
        double successRate = 0.0;
    };

    std::vector<OptimizationHeuristic> extractHeuristics();

    // Generate learning report
    std::string generateLearningReport();

    // Get learner statistics
    int getTotalPatternsLearned() const { return learnedPatterns_.size(); }
    double getAveragePatternConfidence() const { return avgPatternConfidence_; }

private:
    std::vector<OptimizationPattern> learnedPatterns_;
    std::map<std::string, WorkloadCharacterization> characterizations_;
    double avgPatternConfidence_ = 0.0;

    // Helper methods
    void extractPatternsFromIteration(const IterationResult& result);
    void updatePatternStatistics();
    std::string formatPatternRow(const OptimizationPattern& pattern);
};

} // namespace phase106
