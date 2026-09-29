#include "TemplateOptimizationPass.hpp"
#include <sstream>
#include <iomanip>

std::string TemplateOptimizationPass::generateReport() const {
    std::stringstream ss;
    ss << "=== " << metrics_.optimizationName << " Report ===\n";
    ss << "Type: ";
    switch (metrics_.type) {
        case OptimizationType::MATH_LIBRARY_OPTIMIZATION:
            ss << "Math Library Constant Folding\n"; break;
        case OptimizationType::CONDITIONAL_CHAIN_FUSION:
            ss << "Conditional Chain Fusion\n"; break;
        case OptimizationType::REGISTER_RESIDENT_LOOPS:
            ss << "Register-Resident Loop Counters\n"; break;
        case OptimizationType::POINTER_CHASING_CACHE:
            ss << "Pointer Chasing Optimization\n"; break;
        case OptimizationType::TABLE_DRIVEN_DISPATCH:
            ss << "Table-Driven Dispatch (Switch→JMP)\n"; break;
        case OptimizationType::BIT_MANIPULATION_REDUCTION:
            ss << "Bit Manipulation Strength Reduction\n"; break;
        case OptimizationType::DMA_AUTO_INVOKE:
            ss << "DMA Auto-Invocation for Bulk Ops\n"; break;
        case OptimizationType::ZEROPAGE_PRESSURE_RELIEF:
            ss << "Zero-Page Pressure Relief\n"; break;
    }
    ss << "Enabled: " << (metrics_.enabled ? "Yes" : "No") << "\n";
    ss << "Code Reduction: " << metrics_.codeReductionBytes << " bytes\n";
    ss << "Instructions Optimized: " << metrics_.instructionsOptimized << "\n";
    ss << "Speed Improvement: " << std::fixed << std::setprecision(1)
       << metrics_.speedupPercentage << "%\n";
    ss << "Execution Time: " << metrics_.executionTimeMs << "ms\n";
    ss << "Estimated Benefit: " << std::fixed << std::setprecision(2)
       << (metrics_.estimatedBenefit * 100.0) << "%\n";
    return ss.str();
}
