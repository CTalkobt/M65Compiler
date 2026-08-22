#pragma once
#include "OptimizationPassBase.hpp"
#include <vector>
#include <string>

namespace ir { struct Module; }
class TranslationUnit;

// Conditional Jump-Chain Fusion Optimization
// Fuses adjacent conditional branches with identical conditions
// Eliminates redundant condition evaluation
class ConditionalChainFusion : public OptimizationPassBase {
public:
    ConditionalChainFusion();
    ~ConditionalChainFusion() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

private:
    struct ConditionalChain {
        std::string conditionVreg;           // The vreg being tested
        std::vector<std::string> branchLabels;  // Labels in sequence
        int bytesReduced;
    };

    int chainsOptimized_;
    int bytesReduced_;
    std::vector<ConditionalChain> detectedChains_;

    // Pattern detection
    void detectChains(ir::Module& irModule);

    // Chain analysis
    bool isChainable(const std::string& cond1, const std::string& cond2) const;

    // Optimization application
    void fuseChains(ir::Module& irModule);
};
