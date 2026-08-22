#pragma once
#include "TemplateOptimizationPass.hpp"
#include <map>
#include <string>
#include <vector>

namespace ir { struct Module; }
class TranslationUnit;

// Register-Resident Loop Optimization
// Allocates simple loop counters directly to X register (8-bit operations)
// Avoids frame stack access for loop increment/decrement
class RegisterResidentLoops : public TemplateOptimizationPass {
public:
    RegisterResidentLoops();
    ~RegisterResidentLoops() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

    OptimizationMetrics getMetrics() const override { return metrics_; }
    std::string getName() const override { return "Register-Resident Loop Counters"; }
    OptimizationType getType() const override { return OptimizationType::REGISTER_RESIDENT_LOOPS; }

private:
    struct LoopCandidate {
        std::string counterName;       // Variable name of loop counter
        int maxValue;                 // Upper bound (0-256 for 8-bit)
        bool isCountDown;            // true for i--, false for i++
        int bytesReduced;            // Estimated bytes saved
    };

    int loopsOptimized_;
    int bytesReduced_;
    std::vector<LoopCandidate> candidateLoops_;

    // Loop detection
    void detectLoopCandidates(TranslationUnit& ast);
    void detectLoopCandidatesIR(ir::Module& irModule);

    // Candidate analysis
    bool isEightBitLoop(const LoopCandidate& candidate) const;
    bool hasXRegisterConflict(const LoopCandidate& candidate) const;
    bool isSimpleLoop(const LoopCandidate& candidate) const;

    // Optimization application
    void allocateToXRegister(TranslationUnit& ast);
    void allocateToXRegisterIR(ir::Module& irModule);

    // Metrics calculation
    int estimateByteSavings(const LoopCandidate& candidate) const;
};
