#include "CallPatternAnalyzer.hpp"
#include <iostream>

void CallPatternAnalyzer::analyze(TranslationUnit& /* tu */, const std::string& moduleName) {
    currentModule_ = moduleName;
    // Phase 91.1 basic analysis - will be extended in future phases
    // For now, just initialize the module name
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
    std::cerr << "Total functions analyzed: " << functions_.size() << "\n";

    int leafFunctions = 0;
    int functionsWithCalls = 0;

    for (const auto& [name, info] : functions_) {
        if (info.hasNoCalls) leafFunctions++;
        if (!info.hasNoCalls) functionsWithCalls++;
    }

    std::cerr << "Leaf functions (no calls): " << leafFunctions << "\n";
    std::cerr << "Functions with calls: " << functionsWithCalls << "\n";
    std::cerr << "\n";
}
