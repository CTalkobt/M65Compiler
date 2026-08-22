#pragma once
#include "TemplateOptimizationPass.hpp"
#include <map>
#include <string>
#include <cmath>

namespace ir { struct Module; }
class TranslationUnit;

// Math Library Constant Folding Optimization
// Detects calls to sqrt, sin, cos, etc. with constant arguments
// Pre-computes results at compile time and replaces with constant literals
class MathLibraryOptimization : public TemplateOptimizationPass {
public:
    MathLibraryOptimization();
    ~MathLibraryOptimization() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

    OptimizationMetrics getMetrics() const override { return metrics_; }
    std::string getName() const override { return "Math Library Constant Folding"; }
    OptimizationType getType() const override { return OptimizationType::MATH_LIBRARY_OPTIMIZATION; }

private:
    struct MathFunction {
        std::string name;
        int argCount;
        double (*evaluator)(double);  // Single-argument evaluator
        double (*evaluator2)(double, double);  // Two-argument evaluator
    };

    static const std::map<std::string, MathFunction> MATH_FUNCTIONS;

    int callsOptimized_;
    int bytesReduced_;

    // Evaluation functions
    double evaluateMathCall(const std::string& funcName, double arg1, double arg2 = 0.0);
    bool canEvaluate(const std::string& funcName) const;
    bool isMathFunction(const std::string& funcName) const;
};
