#include "CallPatternAnalyzer.hpp"
#include <iostream>

void CallPatternAnalyzer::analyze(TranslationUnit& /* tu */, const std::string& moduleName) {
    currentModule_ = moduleName;

    // Phase 91.2 placeholder: Detailed AST analysis deferred to future iteration
    // This will be implemented to:
    // 1. Walk AST and extract function definitions
    // 2. Count local variables per function
    // 3. Track function calls and their argument patterns
    // 4. Identify constant vs variable arguments
    // 5. Mark functions as leaf/no-calls as applicable

    // For now, analysis is done at IR level instead (see IRBuilder integration)
}

FunctionInfo* CallPatternAnalyzer::getFunctionInfo(const std::string& name) {
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        return &it->second;
    }
    return nullptr;
}

const FunctionInfo* CallPatternAnalyzer::getFunctionInfo(const std::string& name) const {
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::unordered_map<std::string, FunctionInfo>& CallPatternAnalyzer::getAllFunctions() {
    return functions_;
}

const std::unordered_map<std::string, FunctionInfo>& CallPatternAnalyzer::getAllFunctions() const {
    return functions_;
}

void CallPatternAnalyzer::printResults() const {
    std::cerr << "\n=== Call Pattern Analysis Results ===\n";
    std::cerr << "Module: " << currentModule_ << "\n";
    std::cerr << "Total functions: " << functions_.size() << "\n";

    int leafCount = 0;
    int noCallsCount = 0;

    for (const auto& [name, info] : functions_) {
        if (info.hasNoCalls) noCallsCount++;
        if (info.hasNoCalls && info.hasNoLocals) leafCount++;
    }

    std::cerr << "\nAnalysis Summary:\n";
    std::cerr << "  Functions with no calls: " << noCallsCount << "\n";
    std::cerr << "  Leaf functions: " << leafCount << "\n";
    std::cerr << "\n";
}
