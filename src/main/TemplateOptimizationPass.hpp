#pragma once
#include <string>
#include <memory>
#include <vector>

namespace ir { struct Module; }
class TranslationUnit;

// Base class for all template-based code optimization passes
class TemplateOptimizationPass {
public:
    enum class OptimizationType {
        MATH_LIBRARY_OPTIMIZATION,      // Constant folding for sqrt, sin, etc.
        CONDITIONAL_CHAIN_FUSION,       // Fuse adjacent comparisons
        REGISTER_RESIDENT_LOOPS,        // Keep loop counters in registers
        POINTER_CHASING_CACHE,          // Cache pointer dereferences
        TABLE_DRIVEN_DISPATCH,          // Switch→JMP table
        BIT_MANIPULATION_REDUCTION,     // Optimize bit patterns
        DMA_AUTO_INVOKE,                // F018B DMA for bulk ops
        ZEROPAGE_PRESSURE_RELIEF        // Spill ZP temporaries to stack
    };

    struct OptimizationMetrics {
        std::string optimizationName;
        OptimizationType type;
        int codeReductionBytes = 0;
        int instructionsOptimized = 0;
        double speedupPercentage = 0.0;
        double estimatedBenefit = 0.0;  // 0.0-1.0
        bool enabled = true;
        int executionTimeMs = 0;
    };

    virtual ~TemplateOptimizationPass() = default;

    // Execute the template optimization
    virtual void apply(TranslationUnit& ast) = 0;
    virtual void apply(ir::Module& irModule) = 0;

    // Get optimization metadata
    virtual OptimizationMetrics getMetrics() const = 0;
    virtual std::string getName() const = 0;
    virtual OptimizationType getType() const = 0;

    // Control flags
    virtual void setEnabled(bool enabled) { metrics_.enabled = enabled; }
    bool isEnabled() const { return metrics_.enabled; }

    // Reporting
    virtual std::string generateReport() const;

protected:
    OptimizationMetrics metrics_;

    // Helper for derived classes to report metrics
    void reportMetrics(int codeReduction, int instructionsOpt, double speedup) {
        metrics_.codeReductionBytes = codeReduction;
        metrics_.instructionsOptimized = instructionsOpt;
        metrics_.speedupPercentage = speedup;
    }
};
