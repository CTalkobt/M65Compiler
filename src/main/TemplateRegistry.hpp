#pragma once
#include "TemplateOptimizationPass.hpp"
#include <map>
#include <memory>
#include <vector>
#include <functional>

// Registry for all template-based code optimization passes
class TemplateRegistry {
public:
    TemplateRegistry();
    ~TemplateRegistry();

    // Register an optimization template
    void registerOptimization(std::unique_ptr<TemplateOptimizationPass> optimization);

    // Get optimization by type
    TemplateOptimizationPass* getOptimization(TemplateOptimizationPass::OptimizationType type);

    // Get all optimizations
    std::vector<TemplateOptimizationPass*> getAllOptimizations();

    // Get enabled optimizations
    std::vector<TemplateOptimizationPass*> getEnabledOptimizations();

    // Control optimization execution
    void enableOptimization(TemplateOptimizationPass::OptimizationType type, bool enabled);
    bool isOptimizationEnabled(TemplateOptimizationPass::OptimizationType type) const;

    // Get optimization count
    size_t getOptimizationCount() const { return optimizations_.size(); }

    // Get optimization by name
    TemplateOptimizationPass* getOptimizationByName(const std::string& name);

    // Configuration loading
    void loadConfiguration(const std::string& configFile);
    void setExecutionOrder(const std::vector<TemplateOptimizationPass::OptimizationType>& order);

    // Reporting
    std::string generateReport() const;
    std::vector<TemplateOptimizationPass::OptimizationMetrics> getAllMetrics() const;

    // Strategy for optimization ordering and selection
    enum class ExecutionStrategy {
        ALL,                    // Run all enabled optimizations
        GREEDY_BENEFIT,         // Run by estimated benefit (highest first)
        BUDGET_AWARE,           // Run within time budget
        DEPENDENCY_ORDERED      // Run respecting dependencies
    };

    void setStrategy(ExecutionStrategy strategy) { strategy_ = strategy; }
    ExecutionStrategy getStrategy() const { return strategy_; }

private:
    std::map<TemplateOptimizationPass::OptimizationType, std::unique_ptr<TemplateOptimizationPass>> optimizations_;
    std::map<std::string, TemplateOptimizationPass*> optimizationsByName_;
    std::vector<TemplateOptimizationPass::OptimizationType> executionOrder_;
    ExecutionStrategy strategy_;
    bool verbose_;

    // Helper for automatic optimization ordering
    void computeOptimalOrder();
};
