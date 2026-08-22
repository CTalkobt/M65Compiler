#pragma once
#include "TemplateOptimizationPass.hpp"
#include <map>
#include <string>

namespace ir { struct Module; }
class TranslationUnit;

// DMA Operation Template Optimization
// Recognizes memory transfer patterns and auto-invokes DMA where beneficial
class DMAOperationTemplate : public TemplateOptimizationPass {
public:
    DMAOperationTemplate();
    ~DMAOperationTemplate() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

    OptimizationMetrics getMetrics() const override { return metrics_; }
    std::string getName() const override { return "DMA Operation Template"; }
    OptimizationType getType() const override { return OptimizationType::DMA_OPERATION_TEMPLATE; }

private:
    struct DMACandidate {
        std::string operation;      // memcpy, memset, memmove
        unsigned int sourceAddr;    // Source address or register
        unsigned int destAddr;      // Destination address
        unsigned int length;        // Byte count
        bool isConstantLength;      // Can be optimized with DMA
        bool isDMAWorthwhile;       // Breaks even at ~20 bytes
        int bytesReduced;          // Estimated savings
    };

    int operationsOptimized_;
    int bytesReduced_;

    // Pattern detection
    void detectDMAPatterns(TranslationUnit& ast);
    void detectDMAPatternsIR(ir::Module& irModule);

    // Heuristics
    bool isDMABeneficial(const DMACandidate& candidate) const;
    int estimateDMABenefit(unsigned int length) const;

    // Code generation
    void applyDMAOptimization(TranslationUnit& ast);
    void applyDMAOptimizationIR(ir::Module& irModule);
};
