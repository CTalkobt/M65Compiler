#pragma once
#include "Phase108HookPoints.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

// Forward declarations
class OptimizationResult;

// Phase 109.1: Effectiveness Collector
// Collects signal-decision pairs and compilation results for adaptive tuning

struct CompilationRecord {
    // Source characteristics
    std::string sourceFile;
    long fileSize;
    int functionCount;
    int loopCount;
    int branchDensity;

    // Compilation context
    std::chrono::system_clock::time_point timestamp;
    int optimizationLevel;  // -O0 to -O9
    double budgetMs;

    // Hook data
    std::vector<CompilationSignal> collectedSignals;
    std::vector<HookDecision> decisions;

    // Results
    int finalAssemblySize;
    double compilationTimeMs;
    std::vector<std::string> enabledOptimizations;
};

struct OptimizationApplication {
    std::string optimizationName;
    bool applied;
    double estimatedSpeedup;      // Expected % speedup
    double estimatedSizeReduction; // Expected % reduction
    double actualSpeedup;          // Observed % speedup
    double actualSizeReduction;    // Observed % reduction
    double compilationCost;        // ms overhead
    bool successful;               // Was it beneficial?
};

struct OptimizationEffectiveness {
    std::string optimizationName;
    int applicationCount;
    int successCount;
    double successRate;            // successful / applicationCount
    double avgSpeedup;             // Average observed speedup
    double avgSizeReduction;       // Average observed reduction
    double avgCost;                // Average compilation cost
    double benefitScore;           // (speedup + reduction) / cost
};

// Collector class
class OptimizationEffectivenessCollector {
public:
    OptimizationEffectivenessCollector();
    ~OptimizationEffectivenessCollector();

    // Initialize collection for a compilation session
    void startSession(const std::string& sourceFile, long fileSize);

    // Record signal and decision during compilation
    void recordSignalAndDecision(
        const CompilationSignal& signal,
        const HookDecision& decision);

    // Record optimization application
    void recordOptimizationApplication(
        const OptimizationApplication& application);

    // Finalize compilation record
    void finializeCompilation(
        int finalAssemblySize,
        double compilationTimeMs);

    // Query effectiveness of optimization
    OptimizationEffectiveness getEffectiveness(
        const std::string& optimizationName) const;

    // Get all optimization effectiveness profiles
    std::vector<OptimizationEffectiveness> getAllEffectiveness() const;

    // Get compilation record history
    const std::vector<CompilationRecord>& getHistory() const {
        return compilationHistory_;
    }

    // Statistics
    int getTotalCompilations() const {
        return compilationHistory_.size();
    }

    int getTotalOptimizationApplications() const {
        return totalOptimizationApplications_;
    }

    // Persistence
    void saveToDatabase(const std::string& dbPath);
    void loadFromDatabase(const std::string& dbPath);

    // Reset for testing
    void clearHistory() { compilationHistory_.clear(); }

private:
    CompilationRecord currentRecord_;
    std::vector<CompilationRecord> compilationHistory_;
    std::vector<OptimizationApplication> applicationHistory_;

    int totalOptimizationApplications_;

    // Helper: compute effectiveness from applications
    OptimizationEffectiveness computeEffectiveness(
        const std::string& optimizationName) const;

    // Helper: write JSON record
    std::string recordToJson(const CompilationRecord& record) const;

    // Helper: read JSON record
    CompilationRecord recordFromJson(const std::string& json) const;
};
