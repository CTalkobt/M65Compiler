// Phase 106.2: Optimization Learner Implementation
#include "OptimizationLearner.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <cmath>

using namespace phase105;

namespace phase106 {

void OptimizationLearner::learnFromResults(
    const std::vector<IterationResult>& results) {

    for (const auto& result : results) {
        extractPatternsFromIteration(result);
    }

    updatePatternStatistics();
}

void OptimizationLearner::extractPatternsFromIteration(
    const IterationResult& result) {

    // Extract patterns from strategy comparisons
    std::map<std::string, std::vector<double>> strategySpeedups;

    for (const auto& benchResult : result.benchmarkResults) {
        strategySpeedups[benchResult.strategy.strategyName].push_back(
            benchResult.actualSpeedup);
    }

    // Create patterns for high-performing strategies
    for (const auto& [stratName, speedups] : strategySpeedups) {
        double avgSpeedup = std::accumulate(speedups.begin(),
                                           speedups.end(), 0.0) / speedups.size();

        if (avgSpeedup > 10.0) {  // Only record patterns with meaningful speedup
            OptimizationPattern pattern;
            pattern.patternName = "pattern_" + stratName + "_iter" +
                                 std::to_string(result.iterationNumber);
            pattern.description = stratName + " strategy iteration " +
                                 std::to_string(result.iterationNumber);
            pattern.applicableOptimizations.push_back(stratName);
            pattern.expectedSpeedup = avgSpeedup;
            pattern.expectedSizeReduction = result.avgSizeReduction;
            pattern.observationCount = 1;

            learnedPatterns_.push_back(pattern);
        }
    }
}

WorkloadCharacterization OptimizationLearner::classifyWorkload(
    const phase104::BenchmarkProgram& program) {

    WorkloadCharacterization workload;
    workload.benchmarkName = program.name;

    // Classify based on program name and optimization phases
    if (program.category == "loops") {
        workload.primaryType = "loop-heavy";
        workload.loopIntensity = 0.8;
        workload.memoryIntensity = 0.3;
        workload.structIntensity = 0.1;
    } else if (program.category == "structs") {
        workload.primaryType = "struct-intensive";
        workload.loopIntensity = 0.4;
        workload.memoryIntensity = 0.6;
        workload.structIntensity = 0.9;
    } else if (program.category == "memory") {
        workload.primaryType = "memory-bound";
        workload.loopIntensity = 0.5;
        workload.memoryIntensity = 0.9;
        workload.structIntensity = 0.3;
    } else {
        workload.primaryType = "mixed";
        workload.loopIntensity = 0.5;
        workload.memoryIntensity = 0.5;
        workload.structIntensity = 0.5;
    }

    characterizations_[program.name] = workload;
    return workload;
}

std::string OptimizationLearner::predictBestOptimization(
    const WorkloadCharacterization& workload) {

    // Heuristic: match optimization to workload type
    if (workload.primaryType == "loop-heavy") {
        return "loop-optimized";
    } else if (workload.primaryType == "struct-intensive") {
        return "struct-optimized";
    } else if (workload.primaryType == "memory-bound") {
        return "memory-optimized";
    } else {
        return "IPO-optimized";
    }
}

double OptimizationLearner::computePatternConfidence(
    const OptimizationPattern& pattern) {

    // Confidence based on observation count and consistency
    double countConfidence = std::min(1.0, pattern.observationCount / 10.0);
    double consistencyBonus = (pattern.expectedSpeedup > 15.0) ? 0.2 : 0.0;

    return std::min(1.0, countConfidence + consistencyBonus);
}

std::vector<OptimizationLearner::OptimizationHeuristic>
OptimizationLearner::extractHeuristics() {

    std::vector<OptimizationHeuristic> heuristics;

    // Generate heuristics from learned patterns
    std::map<std::string, std::vector<OptimizationPattern>> patternsByType;

    for (const auto& pattern : learnedPatterns_) {
        for (const auto& benchType : pattern.benchmarkTypes) {
            patternsByType[benchType].push_back(pattern);
        }
    }

    for (const auto& [benchType, patterns] : patternsByType) {
        if (!patterns.empty()) {
            OptimizationHeuristic heuristic;
            heuristic.condition = benchType;

            // Find best optimization for this type
            double maxSpeedup = 0.0;
            std::string bestOpt;
            for (const auto& pattern : patterns) {
                if (pattern.expectedSpeedup > maxSpeedup) {
                    maxSpeedup = pattern.expectedSpeedup;
                    if (!pattern.applicableOptimizations.empty()) {
                        bestOpt = pattern.applicableOptimizations[0];
                    }
                }
            }

            heuristic.recommendation = bestOpt;
            heuristic.successRate = computePatternConfidence(patterns[0]);
            heuristics.push_back(heuristic);
        }
    }

    return heuristics;
}

std::string OptimizationLearner::generateLearningReport() {

    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 106.2: Optimization Learning Report                  ║\n";
    report << "║     Pattern Recognition & Heuristic Extraction                ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "LEARNED PATTERNS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Patterns:             " << getTotalPatternsLearned() << "\n";
    report << "Average Confidence:         " << std::fixed << std::setprecision(2)
           << (getAveragePatternConfidence() * 100.0) << "%\n\n";

    report << "Pattern Details\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Pattern                  Speedup  Size Red.  Obs  Confidence\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& pattern : learnedPatterns_) {
        report << formatPatternRow(pattern);
    }

    report << "\n";
    report << "EXTRACTED HEURISTICS\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    auto heuristics = extractHeuristics();
    report << "Condition                Recommendation         Success Rate\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& heuristic : heuristics) {
        report << std::left << std::setw(24) << heuristic.condition
               << "  " << std::setw(22) << heuristic.recommendation
               << "  " << std::right << std::setw(6) << std::fixed << std::setprecision(1)
               << (heuristic.successRate * 100.0) << "%\n";
    }

    report << "\nWORKLOAD CHARACTERIZATIONS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Benchmark            Type               Loop  Memory  Struct\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& [benchName, workload] : characterizations_) {
        report << std::left << std::setw(20) << benchName
               << "  " << std::setw(18) << workload.primaryType
               << "  " << std::right << std::setw(4) << std::fixed << std::setprecision(1)
               << (workload.loopIntensity * 100.0) << "%"
               << "  " << std::setw(6) << (workload.memoryIntensity * 100.0) << "%"
               << "  " << std::setw(6) << (workload.structIntensity * 100.0) << "%\n";
    }

    report << "\n";
    return report.str();
}

void OptimizationLearner::updatePatternStatistics() {
    double sumConfidence = 0.0;
    for (auto& pattern : learnedPatterns_) {
        pattern.confidence = computePatternConfidence(pattern);
        sumConfidence += pattern.confidence;
    }

    if (!learnedPatterns_.empty()) {
        avgPatternConfidence_ = sumConfidence / learnedPatterns_.size();
    }
}

std::string OptimizationLearner::formatPatternRow(
    const OptimizationPattern& pattern) {

    std::stringstream row;

    std::string patternShort = pattern.patternName;
    if (patternShort.length() > 22) {
        patternShort = patternShort.substr(0, 19) + "...";
    }

    row << std::left << std::setw(23) << patternShort
        << "  " << std::right << std::setw(7) << std::fixed << std::setprecision(1)
        << pattern.expectedSpeedup << "%"
        << "  " << std::setw(9) << pattern.expectedSizeReduction << "%"
        << "  " << std::setw(3) << pattern.observationCount
        << "  " << std::setw(10) << (pattern.confidence * 100.0) << "%\n";

    return row.str();
}

} // namespace phase106
