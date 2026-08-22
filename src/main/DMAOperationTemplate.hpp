#pragma once
#include "OptimizationPassBase.hpp"
#include <map>
#include <string>

namespace ir { struct Module; }
class TranslationUnit;

// DMA Operation Template Optimization
// Recognizes memory transfer patterns and auto-invokes DMA where beneficial
class DMAOperationTemplate : public OptimizationPassBase {
public:
    DMAOperationTemplate();
    ~DMAOperationTemplate() override;

    void apply(TranslationUnit& ast) override;
    void apply(ir::Module& irModule) override;

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
