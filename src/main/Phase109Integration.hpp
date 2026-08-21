#pragma once
#include "Phase109EffectivenessCollector.hpp"
#include "Phase109ProfileDatabase.hpp"
#include "Phase109ThresholdAdjuster.hpp"
#include "Phase109PatternAnalyzer.hpp"
#include "Phase109AdaptiveSelector.hpp"
#include <string>
#include <memory>

// Phase 109.6: Integration Coordinator
// Orchestrates all Phase 109 components and integrates with cc45_main
class Phase109Integration {
public:
    Phase109Integration();
    ~Phase109Integration();

    // Initialize all sub-components
    void initialize(bool enableVerboseLogging = false);

    // Get compilation strategy before compilation starts
    OptimizationStrategy getCompilationStrategy(
        const std::string& sourceFile,
        long fileSize,
        int functionCount,
        int loopCount,
        int branchDensity);

    // Record compilation signal (from Phase 108 hooks)
    void recordHookSignal(
        const CompilationSignal& signal,
        const HookDecision& decision);

    // Record optimization application
    void recordOptimizationApplication(
        const OptimizationApplication& application);

    // Finalize compilation and update database
    void finalizeCompilation(
        const std::string& sourceFile,
        long fileSize,
        int functionCount,
        int loopCount,
        int branchDensity,
        int finalAssemblySize,
        double compilationTimeMs);

    // Analyze collected data and update profiles
    void updateProfiles();

    // Generate adaptive recommendations for next compilation
    void generateAdaptiveRecommendations();

    // Statistics
    int getTotalCompilationsProcessed() const {
        if (collector_) {
            return collector_->getTotalCompilations();
        }
        return 0;
    }

    int getTotalOptimizationsApplied() const {
        if (collector_) {
            return collector_->getTotalOptimizationApplications();
        }
        return 0;
    }

    // Report generation
    std::string generateFullReport() const;

    // Verbosity control
    void setVerboseLogging(bool enable) {
        verboseLogging_ = enable;
    }

    // Component access (for testing)
    std::shared_ptr<Phase109EffectivenessCollector> getCollector() {
        return collector_;
    }

    std::shared_ptr<Phase109ProfileDatabase> getDatabase() {
        return database_;
    }

    std::shared_ptr<Phase109ThresholdAdjuster> getAdjuster() {
        return adjuster_;
    }

    std::shared_ptr<Phase109PatternAnalyzer> getAnalyzer() {
        return analyzer_;
    }

    std::shared_ptr<Phase109AdaptiveSelector> getSelector() {
        return selector_;
    }

private:
    // Sub-components
    std::shared_ptr<Phase109EffectivenessCollector> collector_;
    std::shared_ptr<Phase109ProfileDatabase> database_;
    std::shared_ptr<Phase109ThresholdAdjuster> adjuster_;
    std::shared_ptr<Phase109PatternAnalyzer> analyzer_;
    std::shared_ptr<Phase109AdaptiveSelector> selector_;

    // State
    bool verboseLogging_;
    int compilationCount_;
    int updateCount_;

    // Helper: Log message if verbose enabled
    void logMessage(const std::string& message) const;

    // Helper: Initialize database from files if available
    void loadPersistentData();

    // Helper: Save current database to files
    void savePersistentData();
};
