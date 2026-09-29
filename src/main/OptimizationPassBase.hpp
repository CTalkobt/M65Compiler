#pragma once
#include "TemplateOptimizationPass.hpp"
#include <string>

namespace ir { struct Module; }
class TranslationUnit;

// Common base class for all optimization passes
// Consolidates shared initialization and metric management logic
// Derived passes inherit common getMetrics/getType/getName implementations
// and define their own apply() methods for specific pattern detection and transformation
class OptimizationPassBase : public TemplateOptimizationPass {
public:
    virtual ~OptimizationPassBase() = default;

    // These are implemented by this base class (final - no override needed)
    OptimizationMetrics getMetrics() const override final { return metrics_; }
    OptimizationType getType() const override final { return metrics_.type; }
    std::string getName() const override final { return metrics_.optimizationName; }

    // Derived classes implement these with pass-specific logic
    void apply(TranslationUnit& ast) override = 0;
    void apply(ir::Module& irModule) override = 0;

protected:
    // Constructor for derived classes to initialize metrics
    explicit OptimizationPassBase(OptimizationType optType, const std::string& name)
        : TemplateOptimizationPass() {
        metrics_.type = optType;
        metrics_.optimizationName = name;
    }

    // Helper to initialize pass-specific member variables
    // Derived classes can call this in their constructor for uniform initialization
    void initializeCounters(int& operationsCounter, int& bytesCounter) {
        // This is just a helper to ensure consistent patterns across passes
        // Each pass initializes its own counters in its member initializer list
    }
};
