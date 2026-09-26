// Phase 101: Performance Profiling Implementation
#include "CompilationProfiler.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace phase101 {

void CompilationProfiler::recordHintApplication(const PerformanceCounter& counter) {
    counters_.push_back(counter);
    totalTimeSpent_ += counter.appliedTime.count();
    totalBytesSaved_ += counter.savedBytes;

    if (counter.estimationAccuracy > 0) {
        accuracyValues_.push_back(counter.estimationAccuracy);
    }
}

void CompilationProfiler::recordCodeGenMetrics(const std::string& hintId,
                                            int originalSize, int optimizedSize,
                                            std::chrono::microseconds genTime) {
    // Find or create counter
    auto it = std::find_if(counters_.begin(), counters_.end(),
        [&](const PerformanceCounter& c) { return c.hintId == hintId; });

    if (it != counters_.end()) {
        it->originalInstructions = originalSize / 3;  // Avg 3 bytes per instruction
        it->optimizedInstructions = optimizedSize / 3;
        it->savedBytes = originalSize - optimizedSize;
        it->generationTime = genTime;
    }
}

void CompilationProfiler::profileHintEffectiveness(
    const std::shared_ptr<phase100::OptimizationHint>& hint,
    int originalSize, int optimizedSize) {
    int saved = originalSize - optimizedSize;

    if (saved > 0) {
        hintEffectiveness_[hint->hintType] += saved;
    }
}

std::vector<HotSpot> CompilationProfiler::identifyHotSpots(const std::string& profileData) {
    std::vector<HotSpot> spots;

    // Parse profile data and identify functions/loops with high call frequency
    // This is a placeholder - in real implementation would parse actual profiling data

    // Example: assume functions with names containing "loop" are hot
    if (profileData.find("inner_loop") != std::string::npos) {
        HotSpot spot;
        spot.functionName = "process_array";
        spot.loopLabel = "inner_loop";
        spot.callFrequency = 10000;  // 10K iterations
        spot.estimatedCycles = 50000;
        spot.cyclePercentage = 35;
        spots.push_back(spot);
    }

    return spots;
}

CompilationProfiler::ComparisonResult CompilationProfiler::comparePerformance() {
    ComparisonResult result;

    // Sum up all metrics
    for (const auto& counter : counters_) {
        result.originalTotalSize += counter.originalInstructions * 3;  // bytes
        result.optimizedTotalSize += counter.optimizedInstructions * 3;
        result.totalSaved += counter.savedBytes;
        result.totalCyclesSaved += counter.cyclesSaved;
    }

    // Calculate improvement percentages
    if (result.originalTotalSize > 0) {
        result.improvementPercent = 100.0 * result.totalSaved / result.originalTotalSize;
    }

    if (result.totalCyclesSaved > 0) {
        // Assume ~1000 total cycles (simplified model)
        result.speedupPercent = 100.0 * result.totalCyclesSaved / 1000;
    }

    return result;
}

std::map<std::pair<std::string, std::string>, double>
CompilationProfiler::analyzeHintCorrelation() {
    std::map<std::pair<std::string, std::string>, double> correlations;

    // Analyze which hint pairs work well together
    for (size_t i = 0; i < counters_.size(); ++i) {
        for (size_t j = i + 1; j < counters_.size(); ++j) {
            const auto& h1 = counters_[i];
            const auto& h2 = counters_[j];

            // Simple correlation: if both saved bytes and target same area
            double correlation = 0.0;
            if (h1.savedBytes > 0 && h2.savedBytes > 0) {
                // Synergy: check if combined benefit exceeds sum
                int expectedSum = h1.savedBytes + h2.savedBytes;
                correlation = 1.0;  // Placeholder - full analysis needed
            }

            if (correlation > 0) {
                correlations[{h1.hintType, h2.hintType}] = correlation;
            }
        }
    }

    return correlations;
}

std::string CompilationProfiler::generateProfilingReport() {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║        Phase 101: Performance Profiling Report                 ║\n";
    report << "║        Hint Application Effectiveness Analysis                ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    // Summary statistics
    report << "SUMMARY STATISTICS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Hints Applied:       " << getTotalHintsApplied() << "\n";
    report << "Total Time Spent:          " << formatDurationMs(std::chrono::microseconds(totalTimeSpent_)) << "\n";
    report << "Total Bytes Saved:         " << formatSizeBytes(totalBytesSaved_) << "\n";
    report << "Average Accuracy:          " << std::fixed << std::setprecision(1)
           << (getAverageAccuracy() * 100.0) << "%\n\n";

    // Performance comparison
    auto comparison = comparePerformance();
    report << "PERFORMANCE COMPARISON\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Original Code Size:        " << formatSizeBytes(comparison.originalTotalSize) << "\n";
    report << "Optimized Code Size:       " << formatSizeBytes(comparison.optimizedTotalSize) << "\n";
    report << "Total Saved:               " << formatSizeBytes(comparison.totalSaved) << "\n";
    report << "Improvement:               " << std::setprecision(1)
           << comparison.improvementPercent << "%\n";
    report << "Cycles Saved (estimated):  " << comparison.totalCyclesSaved << "\n";
    report << "Speedup (estimated):       " << std::setprecision(1)
           << comparison.speedupPercent << "%\n\n";

    // Per-hint breakdown
    report << "PER-HINT EFFECTIVENESS\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Hint Type              Bytes Saved  Avg Time    Count\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    std::map<std::string, int> bytesPerType;
    std::map<std::string, long long> timePerType;
    std::map<std::string, int> countPerType;

    for (const auto& counter : counters_) {
        bytesPerType[counter.hintType] += counter.savedBytes;
        timePerType[counter.hintType] += counter.appliedTime.count();
        countPerType[counter.hintType]++;
    }

    for (const auto& [hintType, bytes] : bytesPerType) {
        long long avgTime = timePerType[hintType] / countPerType[hintType];
        report << std::left << std::setw(20) << hintType
               << "  " << std::setw(10) << formatSizeBytes(bytes)
               << "  " << std::setw(9) << formatDurationMs(std::chrono::microseconds(avgTime))
               << "  " << countPerType[hintType] << "\n";
    }

    report << "\n";
    return report.str();
}

double CompilationProfiler::getAverageAccuracy() const {
    if (accuracyValues_.empty()) return 0.0;

    double sum = std::accumulate(accuracyValues_.begin(), accuracyValues_.end(), 0.0);
    return sum / accuracyValues_.size();
}

std::string CompilationProfiler::exportMetricsAsJSON() {
    std::stringstream json;

    json << "{\n";
    json << "  \"profile\": {\n";
    json << "    \"totalHints\": " << getTotalHintsApplied() << ",\n";
    json << "    \"totalTimeMs\": " << (totalTimeSpent_ / 1000) << ",\n";
    json << "    \"totalBytesSaved\": " << totalBytesSaved_ << ",\n";
    json << "    \"averageAccuracy\": " << std::fixed << std::setprecision(2)
         << (getAverageAccuracy() * 100.0) << ",\n";

    json << "    \"hints\": [\n";

    for (size_t i = 0; i < counters_.size(); ++i) {
        const auto& c = counters_[i];
        json << "      {\n";
        json << "        \"id\": \"" << c.hintId << "\",\n";
        json << "        \"phase\": \"" << c.phase << "\",\n";
        json << "        \"type\": \"" << c.hintType << "\",\n";
        json << "        \"timeUs\": " << c.appliedTime.count() << ",\n";
        json << "        \"bytesSaved\": " << c.savedBytes << ",\n";
        json << "        \"accuracy\": " << std::setprecision(2) << c.estimationAccuracy << "\n";
        json << "      }";
        if (i < counters_.size() - 1) json << ",";
        json << "\n";
    }

    json << "    ]\n";
    json << "  }\n";
    json << "}\n";

    return json.str();
}

std::string CompilationProfiler::exportMetricsAsCSV() {
    std::stringstream csv;

    csv << "HintID,Phase,Type,TimeUs,BytesSaved,Accuracy,Beneficial\n";

    for (const auto& c : counters_) {
        csv << c.hintId << ","
            << c.phase << ","
            << c.hintType << ","
            << c.appliedTime.count() << ","
            << c.savedBytes << ","
            << std::fixed << std::setprecision(2) << c.estimationAccuracy << ","
            << (c.wasBeneficial ? "yes" : "no") << "\n";
    }

    return csv.str();
}

double CompilationProfiler::calculateAccuracy(int estimated, int actual) {
    if (estimated == 0) return 0.0;

    double ratio = (double)actual / estimated;
    return std::min(1.0, ratio);  // Accuracy capped at 100%
}

int CompilationProfiler::estimateCycleImpact(int bytesSaved) {
    // Rough estimate: 3 bytes = 2-3 cycles
    return (bytesSaved / 3) * 2;
}

std::string CompilationProfiler::formatDurationMs(std::chrono::microseconds us) {
    std::stringstream ss;
    if (us.count() < 1000) {
        ss << us.count() << "us";
    } else {
        ss << std::fixed << std::setprecision(2) << (us.count() / 1000.0) << "ms";
    }
    return ss.str();
}

std::string CompilationProfiler::formatSizeBytes(int bytes) {
    std::stringstream ss;
    if (bytes < 1024) {
        ss << bytes << "B";
    } else if (bytes < 1024 * 1024) {
        ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << "KB";
    } else {
        ss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024)) << "MB";
    }
    return ss.str();
}

} // namespace phase101
