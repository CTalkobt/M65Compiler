#include "OptimizationEffectivenessCollector.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>

OptimizationEffectivenessCollector::OptimizationEffectivenessCollector()
    : totalOptimizationApplications_(0) {
}

OptimizationEffectivenessCollector::~OptimizationEffectivenessCollector() = default;

void OptimizationEffectivenessCollector::startSession(
    const std::string& sourceFile,
    long fileSize) {

    currentRecord_.sourceFile = sourceFile;
    currentRecord_.fileSize = fileSize;
    currentRecord_.timestamp = std::chrono::system_clock::now();
    currentRecord_.collectedSignals.clear();
    currentRecord_.decisions.clear();
}

void OptimizationEffectivenessCollector::recordSignalAndDecision(
    const CompilationSignal& signal,
    const HookDecision& decision) {

    currentRecord_.collectedSignals.push_back(signal);
    currentRecord_.decisions.push_back(decision);
}

void OptimizationEffectivenessCollector::recordOptimizationApplication(
    const OptimizationApplication& application) {

    applicationHistory_.push_back(application);
    totalOptimizationApplications_++;

    if (application.applied) {
        currentRecord_.enabledOptimizations.push_back(
            application.optimizationName);
    }
}

void OptimizationEffectivenessCollector::finializeCompilation(
    int finalAssemblySize,
    double compilationTimeMs) {

    currentRecord_.finalAssemblySize = finalAssemblySize;
    currentRecord_.compilationTimeMs = compilationTimeMs;

    compilationHistory_.push_back(currentRecord_);
}

OptimizationEffectiveness OptimizationEffectivenessCollector::getEffectiveness(
    const std::string& optimizationName) const {

    return computeEffectiveness(optimizationName);
}

std::vector<OptimizationEffectiveness>
OptimizationEffectivenessCollector::getAllEffectiveness() const {

    std::vector<OptimizationEffectiveness> allEffectiveness;
    std::map<std::string, int> optimizationSet;

    // Collect all unique optimization names
    for (const auto& app : applicationHistory_) {
        optimizationSet[app.optimizationName]++;
    }

    // Compute effectiveness for each
    for (const auto& entry : optimizationSet) {
        allEffectiveness.push_back(
            computeEffectiveness(entry.first));
    }

    return allEffectiveness;
}

OptimizationEffectiveness OptimizationEffectivenessCollector::computeEffectiveness(
    const std::string& optimizationName) const {

    OptimizationEffectiveness result;
    result.optimizationName = optimizationName;
    result.applicationCount = 0;
    result.successCount = 0;
    result.avgSpeedup = 0.0;
    result.avgSizeReduction = 0.0;
    result.avgCost = 0.0;

    if (applicationHistory_.empty()) {
        result.successRate = 0.0;
        result.benefitScore = 0.0;
        return result;
    }

    double totalSpeedup = 0.0;
    double totalReduction = 0.0;
    double totalCost = 0.0;

    for (const auto& app : applicationHistory_) {
        if (app.optimizationName == optimizationName) {
            result.applicationCount++;

            if (app.successful) {
                result.successCount++;
            }

            totalSpeedup += app.actualSpeedup;
            totalReduction += app.actualSizeReduction;
            totalCost += app.compilationCost;
        }
    }

    if (result.applicationCount > 0) {
        result.avgSpeedup = totalSpeedup / result.applicationCount;
        result.avgSizeReduction = totalReduction / result.applicationCount;
        result.avgCost = totalCost / result.applicationCount;
        result.successRate = static_cast<double>(result.successCount) /
                            result.applicationCount;

        // Calculate benefit score
        if (result.avgCost > 0.0) {
            result.benefitScore = (result.avgSpeedup +
                                  result.avgSizeReduction) /
                                  result.avgCost;
        } else {
            // Free optimization
            result.benefitScore =
                result.avgSpeedup + result.avgSizeReduction;
        }
    }

    return result;
}

void OptimizationEffectivenessCollector::saveToDatabase(
    const std::string& dbPath) {

    std::ofstream file(dbPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open database file: " << dbPath
                  << std::endl;
        return;
    }

    // Write header
    file << "{\n";
    file << "  \"compilationCount\": " << compilationHistory_.size() << ",\n";
    file << "  \"totalApplications\": "
         << totalOptimizationApplications_ << ",\n";
    file << "  \"compilations\": [\n";

    // Write each compilation record
    for (size_t i = 0; i < compilationHistory_.size(); i++) {
        file << recordToJson(compilationHistory_[i]);
        if (i < compilationHistory_.size() - 1) {
            file << ",\n";
        }
    }

    file << "  ]\n";
    file << "}\n";
    file.close();
}

void OptimizationEffectivenessCollector::loadFromDatabase(
    const std::string& dbPath) {

    // Placeholder: would implement JSON parsing
    std::cout << "[Phase109] Database loading not yet implemented: "
              << dbPath << std::endl;
}

std::string OptimizationEffectivenessCollector::recordToJson(
    const CompilationRecord& record) const {

    std::string json = "    {\n";
    json += "      \"file\": \"" + record.sourceFile + "\",\n";
    json += "      \"size\": " + std::to_string(record.fileSize) + ",\n";
    json += "      \"functions\": "
         + std::to_string(record.functionCount) + ",\n";
    json += "      \"loops\": "
         + std::to_string(record.loopCount) + ",\n";
    json += "      \"branchDensity\": "
         + std::to_string(record.branchDensity) + ",\n";
    json += "      \"compilationTime\": "
         + std::to_string(record.compilationTimeMs) + ",\n";
    json += "      \"assemblySize\": "
         + std::to_string(record.finalAssemblySize) + ",\n";
    json += "      \"optimizationCount\": "
         + std::to_string(record.enabledOptimizations.size()) + "\n";
    json += "    }";

    return json;
}

CompilationRecord OptimizationEffectivenessCollector::recordFromJson(
    const std::string& json) const {

    // Placeholder: would implement JSON parsing
    return CompilationRecord{};
}
