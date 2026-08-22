// Phase 102: Pattern Analysis Implementation
#include "PatternRecognitionEngine.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace phase102 {

CodePattern PatternRecognitionEngine::analyzeCodePattern(
    const std::string& targetName,
    const std::string& patternType) {
    CodePattern pattern;
    pattern.targetName = targetName;
    pattern.patternType = patternType;

    // Simulate pattern analysis (real implementation would parse IR/AST)
    if (patternType == "loop") {
        pattern.estimatedIterations = 1000;  // Placeholder
        pattern.isHotPath = true;
        pattern.estimatedCodeSize = 150;
    } else if (patternType == "function") {
        pattern.callDepth = 2;
        pattern.isLeaf = false;
        pattern.estimatedCodeSize = 500;
    } else if (patternType == "struct_access") {
        pattern.fieldAccessCount = 500;
        pattern.accessFrequency = 500;
        pattern.estimatedCodeSize = 200;
    }

    // Analyze memory and control flow patterns
    analyzeMemoryPatterns(pattern);
    analyzeControlFlow(pattern);

    // Detect pattern type
    pattern.patternName = detectPatternType(pattern);
    patternsAnalyzed_++;
    patternsByType_[pattern.patternName].push_back(pattern);

    return pattern;
}

std::string PatternRecognitionEngine::detectPatternType(const CodePattern& pattern) {
    // Heuristic pattern detection
    if (isLoopPattern(pattern)) {
        return "tight_loop";
    } else if (isStructHeavyPattern(pattern)) {
        return "struct_heavy";
    } else if (isRecursivePattern(pattern)) {
        return "recursive";
    } else if (isLeafFunctionPattern(pattern)) {
        return "leaf_function";
    } else {
        return "general_code";
    }
}

void PatternRecognitionEngine::analyzeMemoryPatterns(CodePattern& pattern) {
    // Simulate memory pattern analysis
    if (pattern.patternType == "loop") {
        pattern.memoryAccessCount = 50;
        pattern.arrayAccessCount = 40;
        pattern.pointerDereferenceCount = 10;
    } else if (pattern.patternType == "struct_access") {
        pattern.fieldAccessCount = 100;
        pattern.pointerDereferenceCount = 50;
        pattern.memoryAccessCount = 150;
    } else {
        pattern.memoryAccessCount = 30;
        pattern.fieldAccessCount = 10;
    }
}

void PatternRecognitionEngine::analyzeControlFlow(CodePattern& pattern) {
    // Simulate control flow analysis
    if (pattern.patternType == "loop") {
        pattern.isHotPath = pattern.estimatedIterations > 100;
    }

    if (pattern.patternType == "function") {
        pattern.isLeaf = pattern.callDepth == 0;
        pattern.isRecursive = (pattern.targetName.find("recurs") != std::string::npos);
    }
}

std::vector<OptimizationRecommendation>
PatternRecognitionEngine::recommendOptimizations(const CodePattern& pattern) {
    std::vector<OptimizationRecommendation> recommendations;

    // Recommend based on pattern type
    if (pattern.patternName == "tight_loop") {
        // Loop-specific optimizations
        auto hint1 = std::make_shared<phase100::OptimizationHint>();
        hint1->phase = "100";
        hint1->hintType = "loop_unrolling";
        hint1->targetLoop = pattern.targetName;
        hint1->estimatedBenefit = 150;
        hint1->priority = 10;

        OptimizationRecommendation rec1;
        rec1.hint = hint1;
        rec1.confidenceScore = 0.95;
        rec1.expectedBenefit = 20.0;
        rec1.reasoning.push_back("Loop has high iteration count (>100)");
        rec1.reasoning.push_back("Code size within unrolling threshold");
        rec1.isApplicable = true;

        if (rec1.confidenceScore > 0.8) highConfidenceCount_++;
        recommendations.push_back(rec1);

        // Co-location for array accesses
        if (pattern.arrayAccessCount > 20) {
            auto hint2 = std::make_shared<phase100::OptimizationHint>();
            hint2->phase = "99";
            hint2->hintType = "co_location";
            hint2->targetLoop = pattern.targetName;
            hint2->estimatedBenefit = 100;
            hint2->priority = 8;

            OptimizationRecommendation rec2;
            rec2.hint = hint2;
            rec2.confidenceScore = 0.85;
            rec2.expectedBenefit = 15.0;
            rec2.reasoning.push_back("Detected " + std::to_string(pattern.arrayAccessCount) +
                                    " array accesses");
            rec2.reasoning.push_back("Co-location beneficial for cache locality");
            rec2.isApplicable = true;

            if (rec2.confidenceScore > 0.8) highConfidenceCount_++;
            recommendations.push_back(rec2);
        }

    } else if (pattern.patternName == "struct_heavy") {
        // Struct access optimizations
        auto hint = std::make_shared<phase100::OptimizationHint>();
        hint->phase = "96.5";
        hint->hintType = "field_caching";
        hint->targetFunction = pattern.targetName;
        hint->estimatedBenefit = 120;
        hint->priority = 9;

        OptimizationRecommendation rec;
        rec.hint = hint;
        rec.confidenceScore = 0.90;
        rec.expectedBenefit = 18.0;
        rec.reasoning.push_back("Detected " + std::to_string(pattern.fieldAccessCount) +
                               " field accesses");
        rec.reasoning.push_back("Field caching reduces memory traffic");
        rec.isApplicable = true;

        if (rec.confidenceScore > 0.8) highConfidenceCount_++;
        recommendations.push_back(rec);

    } else if (pattern.patternName == "leaf_function") {
        // Leaf function optimizations
        auto hint = std::make_shared<phase100::OptimizationHint>();
        hint->phase = "91";
        hint->hintType = "inline";
        hint->targetFunction = pattern.targetName;
        hint->estimatedBenefit = 80;
        hint->priority = 7;

        OptimizationRecommendation rec;
        rec.hint = hint;
        rec.confidenceScore = 0.92;
        rec.expectedBenefit = 12.0;
        rec.reasoning.push_back("Leaf function with code size " +
                               std::to_string(pattern.estimatedCodeSize) + " bytes");
        rec.reasoning.push_back("Inlining eliminates call overhead");
        rec.isApplicable = true;

        if (rec.confidenceScore > 0.8) highConfidenceCount_++;
        recommendations.push_back(rec);
    }

    return recommendations;
}

double PatternRecognitionEngine::scoreHintForPattern(
    const std::shared_ptr<phase100::OptimizationHint>& hint,
    const CodePattern& pattern) {
    double score = 0.0;

    // Score based on hint type and pattern
    if (hint->hintType == "loop_unrolling" && pattern.patternName == "tight_loop") {
        score = 0.95;
    } else if (hint->hintType == "field_caching" && pattern.patternName == "struct_heavy") {
        score = 0.90;
    } else if (hint->hintType == "inline" && pattern.patternName == "leaf_function") {
        score = 0.92;
    } else if (hint->hintType == "co_location" && pattern.arrayAccessCount > 10) {
        score = 0.85;
    } else if (hint->hintType == "specialization" && pattern.isHotPath) {
        score = 0.88;
    } else {
        score = 0.50;
    }

    // Reduce score if hint has explicit conflicts
    if (hint->conflictsWith.count(hint->hintType) > 0) {
        score *= 0.5;
    }

    return score;
}

std::string PatternRecognitionEngine::generatePatternReport(
    const CodePattern& pattern) {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║        Phase 102: Code Pattern Analysis Report                 ║\n";
    report << "║        Adaptive Optimization Recommendation                    ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "PATTERN ANALYSIS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Target:                   " << pattern.targetName << "\n";
    report << "Pattern Type:             " << pattern.patternType << "\n";
    report << "Detected Pattern:         " << pattern.patternName << "\n";
    report << "Is Hot Path:              " << (pattern.isHotPath ? "Yes" : "No") << "\n";
    report << "Estimated Code Size:      " << pattern.estimatedCodeSize << " bytes\n\n";

    report << "METRICS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Loop Iterations:          " << pattern.estimatedIterations << "\n";
    report << "Field Accesses:           " << pattern.fieldAccessCount << "\n";
    report << "Array Accesses:           " << pattern.arrayAccessCount << "\n";
    report << "Pointer Dereferences:     " << pattern.pointerDereferenceCount << "\n";
    report << "Memory Accesses:          " << pattern.memoryAccessCount << "\n";
    report << "Call Depth:               " << pattern.callDepth << "\n";
    report << "Is Recursive:             " << (pattern.isRecursive ? "Yes" : "No") << "\n";
    report << "Is Leaf Function:         " << (pattern.isLeaf ? "Yes" : "No") << "\n\n";

    // Recommendations
    auto recommendations = recommendOptimizations(pattern);
    report << "OPTIMIZATION RECOMMENDATIONS\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (size_t i = 0; i < recommendations.size(); ++i) {
        const auto& rec = recommendations[i];
        report << "\n" << (i + 1) << ". " << rec.hint->hintType
               << " (Confidence: " << std::fixed << std::setprecision(0)
               << (rec.confidenceScore * 100.0) << "%)\n";
        report << "   Expected Benefit: " << std::setprecision(1)
               << rec.expectedBenefit << "%\n";
        report << "   Reasoning:\n";
        for (const auto& reason : rec.reasoning) {
            report << "     • " << reason << "\n";
        }
    }

    report << "\n";
    return report.str();
}

bool PatternRecognitionEngine::isLoopPattern(const CodePattern& pattern) {
    return pattern.patternType == "loop" && pattern.estimatedIterations > 100;
}

bool PatternRecognitionEngine::isStructHeavyPattern(const CodePattern& pattern) {
    return pattern.fieldAccessCount > 50 || pattern.pointerDereferenceCount > 30;
}

bool PatternRecognitionEngine::isRecursivePattern(const CodePattern& pattern) {
    return pattern.isRecursive;
}

bool PatternRecognitionEngine::isLeafFunctionPattern(const CodePattern& pattern) {
    return pattern.isLeaf && pattern.estimatedCodeSize < 300;
}

double PatternRecognitionEngine::calculatePatternHotness(const CodePattern& pattern) {
    if (!pattern.isHotPath) return 0.0;

    // Hotness = iteration count or access frequency
    if (pattern.patternType == "loop") {
        return std::min(1.0, pattern.estimatedIterations / 10000.0);
    } else {
        return std::min(1.0, pattern.accessFrequency / 1000.0);
    }
}

std::string PatternRecognitionEngine::formatPatternRow(const CodePattern& pattern) {
    std::stringstream row;

    row << std::left << std::setw(30) << pattern.targetName
        << "  " << std::setw(15) << pattern.patternName
        << "  " << std::setw(8) << (pattern.isHotPath ? "Hot" : "Cold")
        << "  " << std::setw(5) << pattern.estimatedCodeSize << "B\n";

    return row.str();
}

} // namespace phase102
