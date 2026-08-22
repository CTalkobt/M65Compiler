#pragma once
#include "TemplateRegistry.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace ir { struct Module; }
class TranslationUnit;
class AdaptiveLearningSystem;

// Template-based Code Optimization System
// Integrates 8 optimization templates into the compilation pipeline
class TemplateOptimizationSystem {
public:
    TemplateOptimizationSystem();
    ~TemplateOptimizationSystem();

    // Initialize for compilation session
    void initialize(bool verbose = false);

    // Apply templates at appropriate points in compilation pipeline
    void applyToAST(TranslationUnit& ast);           // For AST-level optimizations
    void applyToIR(ir::Module& irModule);            // For IR-level optimizations
    void applyPostCodeGen(const std::string& assembly);  // For assembly-level optimizations

    // Configuration
    void loadConfiguration(const std::string& configFile);
    void setExecutionStrategy(TemplateRegistry::ExecutionStrategy strategy);

    // Hook into adaptive learning system for effectiveness tracking
    void linkLearningSystem(AdaptiveLearningSystem* learner) { learner_ = learner; }

    // Control individual optimizations
    void enableOptimization(TemplateOptimizationPass::OptimizationType type, bool enabled);
    bool isOptimizationEnabled(TemplateOptimizationPass::OptimizationType type) const;

    // Reporting and metrics
    std::string generateReport() const;
    std::vector<TemplateOptimizationPass::OptimizationMetrics> getMetrics() const;
    int getTotalCodeReduction() const;
    double getEstimatedSpeedup() const;

    // Optimization availability check
    bool hasOptimization(TemplateOptimizationPass::OptimizationType type) const;

    // Get registry for direct access
    TemplateRegistry* getRegistry() { return registry_.get(); }

private:
    std::unique_ptr<TemplateRegistry> registry_;
    AdaptiveLearningSystem* learner_;  // Link to learning system for effectiveness tracking
    bool verbose_;
    bool initialized_;

    // Statistics
    int totalCodeReductionBytes_;
    int totalInstructionsOptimized_;
    std::vector<TemplateOptimizationPass::OptimizationMetrics> allMetrics_;

    // Helper methods
    void registerAllOptimizations();
    void collectMetrics();
    void reportToLearner();
};
