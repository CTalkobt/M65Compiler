#include "AdaptiveLearnerIntegration.hpp"
#include <iostream>
#include <sstream>

AdaptiveLearnerIntegration::AdaptiveLearnerIntegration()
    : verboseLogging_(false), compilationCount_(0), updateCount_(0) {

    // Initialize all sub-components
    collector_ = std::make_shared<OptimizationEffectivenessCollector>();
    database_ = std::make_shared<OptimizationProfileDatabase>();
    adjuster_ = std::make_shared<AdaptiveThresholdAdjuster>();
    analyzer_ = std::make_shared<OptimizationPatternAnalyzer>();
    selector_ = std::make_shared<LearnerBasedOptimizationSelector>();
}

AdaptiveLearnerIntegration::~AdaptiveLearnerIntegration() = default;

void AdaptiveLearnerIntegration::initialize(bool enableVerboseLogging) {
    verboseLogging_ = enableVerboseLogging;

    if (verboseLogging_) {
        logMessage("[Phase109] Initializing adaptive optimization system");
    }

    // Wire up component relationships
    adjuster_->setProfileDatabase(database_);
    analyzer_->setProfileDatabase(database_);
    analyzer_->setThresholdAdjuster(adjuster_);
    selector_->setProfileDatabase(database_);
    selector_->setThresholdAdjuster(adjuster_);
    selector_->setPatternAnalyzer(analyzer_);

    // Load any persistent data
    loadPersistentData();

    if (verboseLogging_) {
        logMessage("[Phase109] Initialization complete");
    }
}

OptimizationStrategy AdaptiveLearnerIntegration::getCompilationStrategy(
    const std::string& sourceFile,
    long fileSize,
    int functionCount,
    int loopCount,
    int branchDensity) {

    compilationCount_++;

    if (verboseLogging_) {
        std::stringstream ss;
        ss << "[Phase109] Analyzing file: " << sourceFile << " ("
           << fileSize << " bytes)";
        logMessage(ss.str());
    }

    // Get recommendation from selector
    auto strategy = selector_->getOptimizationStrategy(
        fileSize, functionCount, loopCount, branchDensity);

    if (verboseLogging_) {
        logMessage(strategy.summary);
    }

    return strategy;
}

void AdaptiveLearnerIntegration::recordHookSignal(
    const CompilationSignal& signal,
    const HookDecision& decision) {

    if (collector_) {
        collector_->recordSignalAndDecision(signal, decision);
    }
}

void AdaptiveLearnerIntegration::recordOptimizationApplication(
    const OptimizationApplication& application) {

    if (collector_) {
        collector_->recordOptimizationApplication(application);
    }
}

void AdaptiveLearnerIntegration::finalizeCompilation(
    const std::string& sourceFile,
    long fileSize,
    int functionCount,
    int loopCount,
    int branchDensity,
    int finalAssemblySize,
    double compilationTimeMs) {

    if (!collector_) {
        return;
    }

    collector_->finializeCompilation(finalAssemblySize, compilationTimeMs);

    // Build optimization record and update database
    auto currentRecord = collector_->getHistory().back();
    std::vector<OptimizationApplication> apps;

    // Collect all recorded applications
    for (size_t i = 0; i < currentRecord.enabledOptimizations.size(); i++) {
        OptimizationApplication app;
        app.optimizationName = currentRecord.enabledOptimizations[i];
        app.applied = true;
        app.estimatedSpeedup = 0.0;
        app.estimatedSizeReduction = 0.0;
        app.actualSpeedup = 0.0;
        app.actualSizeReduction = 0.0;
        app.compilationCost = 0.0;
        app.successful = (finalAssemblySize < 10000);  // Heuristic
        apps.push_back(app);
    }

    database_->recordCompilation(currentRecord, apps);

    if (verboseLogging_) {
        std::stringstream ss;
        ss << "[Phase109] Compilation finalized: " << finalAssemblySize
           << " bytes in " << compilationTimeMs << "ms";
        logMessage(ss.str());
    }
}

void AdaptiveLearnerIntegration::updateProfiles() {
    if (verboseLogging_) {
        logMessage("[Phase109] Updating profiles and analyzing trends");
    }

    updateCount_++;

    // Analyze trends and patterns
    if (adjuster_) {
        adjuster_->analyzeTrends();
    }

    if (analyzer_) {
        analyzer_->analyzePatterns();
    }

    if (verboseLogging_) {
        std::stringstream ss;
        ss << "[Phase109] Updated " << updateCount_ << " profile analyses";
        logMessage(ss.str());
    }
}

void AdaptiveLearnerIntegration::generateAdaptiveRecommendations() {
    if (verboseLogging_) {
        logMessage("[Phase109] Generating adaptive recommendations");
    }

    // Generate recommendations (happens in getCompilationStrategy)
    // This is a hook point for future enhancements
}

std::string AdaptiveLearnerIntegration::generateFullReport() const {
    std::stringstream ss;

    ss << "=== Phase 109 Full Integration Report ===\n\n";

    ss << "Compilation Sessions: " << compilationCount_ << "\n";
    ss << "Profile Updates: " << updateCount_ << "\n";
    ss << "Optimizations Tracked: "
       << getTotalOptimizationsApplied() << "\n\n";

    if (collector_) {
        ss << "Collector Report:\n";
        ss << "-----------------\n";
        ss << "Total Compilations: "
           << collector_->getTotalCompilations() << "\n\n";
    }

    if (database_) {
        ss << "Database Report:\n";
        ss << "----------------\n";
        ss << "Optimizations: " << database_->getTotalOptimizations()
           << "\n\n";
    }

    if (adjuster_) {
        ss << "Threshold Adjuster Report:\n";
        ss << "--------------------------\n";
        ss << "Applied Adjustments: " << adjuster_->getAppliedAdjustments()
           << "\n\n";
    }

    if (analyzer_) {
        ss << "Pattern Analyzer Report:\n";
        ss << "-----------------------\n";
        ss << "Patterns Discovered: "
           << analyzer_->getTotalPatternsDiscovered() << "\n\n";
    }

    if (selector_) {
        ss << "Adaptive Selector Report:\n";
        ss << "------------------------\n";
        ss << "Recommendation Accuracy: "
           << (selector_->getRecommendationAccuracy() * 100.0) << "%\n\n";
    }

    return ss.str();
}

void AdaptiveLearnerIntegration::logMessage(const std::string& message) const {
    if (verboseLogging_) {
        std::cout << message << std::endl;
    }
}

void AdaptiveLearnerIntegration::loadPersistentData() {
    // Placeholder: Would load from files
    // database_->loadFromFile("~/.config/m65/phase109.db");
}

void AdaptiveLearnerIntegration::savePersistentData() {
    // Placeholder: Would save to files
    // database_->saveToFile("~/.config/m65/phase109.db");
}
