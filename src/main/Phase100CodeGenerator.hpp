// Phase 100.5: Link-Time Code Generation
// Generates optimized code applying coordinated hints from all phases

#pragma once

#include "Phase100Coordinator.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase100 {

// Code generation decision for a single hint application
struct HintCodeGenDecision {
    std::shared_ptr<OptimizationHint> hint;
    std::string assemblyPrefix;      // Setup code before target
    std::string assemblyInline;      // Inline substitution
    std::string assemblySuffix;      // Cleanup code after target
    std::string debugComment;        // Hint application comment
    bool isApplied = false;
    std::string appliedLocation;     // File/line where applied
};

// Code generation context for a function or loop
struct CodeGenContext {
    std::string targetName;          // Function or loop label
    std::string targetType;          // "function" or "loop"
    std::vector<HintCodeGenDecision> decisions;
    std::string generatedCode;
    int estimatedSize = 0;
    int estimatedBenefit = 0;
};

// Link-time code generator
class Phase100CodeGenerator {
public:
    explicit Phase100CodeGenerator(const LinkTimeCoordinator& coordinator)
        : coordinator_(coordinator) {}

    // Generate code for coordinated hints
    std::string generateOptimizedCode(const CoordinationPlan& plan);

    // Apply IPO specialization code
    std::string applySpecializationCode(const std::shared_ptr<OptimizationHint>& hint);

    // Apply field caching code
    std::string applyCachingCode(const std::shared_ptr<OptimizationHint>& hint);

    // Apply bank setup hoisting code
    std::string applyBankHoistingCode(const std::shared_ptr<OptimizationHint>& hint);

    // Generate dispatch/routing code for multiple specializations
    std::string generateDispatchCode(const std::vector<std::shared_ptr<OptimizationHint>>& hints);

    // Emit code with debug comments
    std::string emitWithComments(const std::string& code,
                                const std::vector<std::shared_ptr<OptimizationHint>>& appliedHints);

    // Generate cache check/update sequences
    std::string generateCacheCheckSequence(const std::shared_ptr<OptimizationHint>& cachingHint);

    // Generate bank setup sequences
    std::string generateBankSetupSequence(const std::shared_ptr<OptimizationHint>& bankHint);

    // Statistics
    int getTotalGeneratedSize() const { return totalGeneratedSize_; }
    int getTotalAppliedHints() const { return appliedHintsCount_; }
    int getCodeReductionAchieved() const { return codeReductionAchieved_; }

private:
    const LinkTimeCoordinator& coordinator_;

    int totalGeneratedSize_ = 0;
    int appliedHintsCount_ = 0;
    int codeReductionAchieved_ = 0;

    // Helper methods
    std::string formatDebugComment(const std::shared_ptr<OptimizationHint>& hint,
                                   const std::string& action);
    std::string generateSpecializationSelector(const std::vector<std::shared_ptr<OptimizationHint>>& specs);
    std::string generateInlineCode(const std::shared_ptr<OptimizationHint>& hint);
    std::string generateCallSiteAdapter(const std::shared_ptr<OptimizationHint>& hint);
};

} // namespace phase100
