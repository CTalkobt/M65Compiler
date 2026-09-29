// Phase 101: Performance Profiling & Metrics Collection
// Profiles hint application effectiveness and collects detailed optimization metrics

#pragma once

#include "LinkTimeOptimizationValidator.hpp"
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>

namespace phase101 {

// Performance counter for a specific hint application
struct PerformanceCounter {
    std::string hintId;              // Unique hint identifier
    std::string phase;               // "91", "96.5", "99", "100"
    std::string hintType;

    // Timing
    std::chrono::microseconds appliedTime{0};    // Time to apply hint
    std::chrono::microseconds generationTime{0}; // Time to generate code
    std::chrono::microseconds verificationTime{0};

    // Code metrics
    int originalInstructions = 0;
    int optimizedInstructions = 0;
    int savedInstructions = 0;
    int savedBytes = 0;

    // Call frequency metrics
    int callSites = 0;
    int estimatedCallFrequency = 0;

    // Performance impact
    int cyclesPerCallOriginal = 0;
    int cyclesPerCallOptimized = 0;
    int cyclesSaved = 0;

    // Accuracy metrics
    double estimationAccuracy = 0.0;  // % match with actual
    bool wasBeneficial = false;       // Did it actually help?
};

// Hot-spot information
struct HotSpot {
    std::string functionName;
    std::string loopLabel;           // Empty if function-level
    int callFrequency = 0;
    int estimatedCycles = 0;
    int cyclePercentage = 0;         // % of total program cycles
    std::vector<std::shared_ptr<phase100::OptimizationHint>> applicableHints;
    bool isOptimized = false;
};

// Performance profiling engine
class CompilationProfiler {
public:
    CompilationProfiler() = default;

    // Record hint application timing
    void recordHintApplication(const PerformanceCounter& counter);

    // Record code generation metrics
    void recordCodeGenMetrics(const std::string& hintId,
                             int originalSize, int optimizedSize,
                             std::chrono::microseconds genTime);

    // Profile hint effectiveness
    void profileHintEffectiveness(const std::shared_ptr<phase100::OptimizationHint>& hint,
                                  int originalSize, int optimizedSize);

    // Identify hot spots in code
    std::vector<HotSpot> identifyHotSpots(const std::string& profileData);

    // Compare performance before/after optimizations
    struct ComparisonResult {
        int originalTotalSize = 0;
        int optimizedTotalSize = 0;
        int totalSaved = 0;
        double improvementPercent = 0.0;
        int totalCyclesSaved = 0;
        double speedupPercent = 0.0;
    };

    ComparisonResult comparePerformance();

    // Analyze hint correlation (which hints work well together)
    std::map<std::pair<std::string, std::string>, double> analyzeHintCorrelation();

    // Generate detailed profiling report
    std::string generateProfilingReport();

    // Get profiling statistics
    int getTotalHintsApplied() const { return counters_.size(); }
    long long getTotalTimeSpent() const { return totalTimeSpent_; }
    int getTotalBytesSaved() const { return totalBytesSaved_; }
    double getAverageAccuracy() const;

    // Export metrics for analysis
    std::string exportMetricsAsJSON();
    std::string exportMetricsAsCSV();

private:
    std::vector<PerformanceCounter> counters_;
    std::map<std::string, std::vector<HotSpot>> hotSpotsByFunction_;
    std::map<std::string, int> hintEffectiveness_;  // hintId -> net bytes saved

    long long totalTimeSpent_ = 0;
    int totalBytesSaved_ = 0;
    std::vector<double> accuracyValues_;

    // Helper methods
    double calculateAccuracy(int estimated, int actual);
    int estimateCycleImpact(int bytesSaved);
    std::string formatDurationMs(std::chrono::microseconds us);
    std::string formatSizeBytes(int bytes);
};

} // namespace phase101
