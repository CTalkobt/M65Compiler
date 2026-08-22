// Phase 101.2: Hot-Spot Detection & Analysis
// Identifies functions and loops consuming most execution time

#pragma once

#include "CompilationProfiler.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace phase101 {

// Hot-spot ranking entry
struct HotSpotRanking {
    int rank = 0;                   // 1 = hottest
    std::string identifier;         // function or loop name
    int estimatedCycles = 0;
    double percentageOfTotal = 0.0;
    std::vector<std::string> applicableOptimizations;
    int potentialCycleSavings = 0;
    double optimizationPriority = 0.0;  // Higher = should optimize first
};

// Forward declaration
namespace phase100 { struct OptimizationHint; }

// Hot-spot analysis report
class HotSpotProfiler {
public:
    explicit HotSpotProfiler(const CompilationProfiler& profiler)
        : profiler_(profiler) {}

    // Detect hot spots from execution profile
    std::vector<HotSpotRanking> detectHotSpots(int cycleThresholdPercent = 5);

    // Rank hot spots by optimization opportunity
    std::vector<HotSpotRanking> rankByOpportunity(
        const std::vector<HotSpotRanking>& spots);

    // Suggest optimizations for a hot spot
    std::vector<std::string> suggestOptimizations(const HotSpotRanking& spot);

    // Identify optimization gaps (hot spots not yet optimized)
    std::vector<HotSpotRanking> findOptimizationGaps();

    // Generate hot-spot analysis report
    std::string generateHotSpotReport();

    // Calculate optimization ROI for a hot spot
    double calculateROI(const HotSpotRanking& spot);

    // Get hot-spot statistics
    int getTotalIdentifiedSpots() const { return allSpots_.size(); }
    int getTotalOptimizedSpots() const { return optimizedSpots_; }
    double getCoveragePercent() const;

private:
    const CompilationProfiler& profiler_;

    std::vector<HotSpotRanking> allSpots_;
    int optimizedSpots_ = 0;
    int totalHotnessCycles_ = 0;

    // Helper methods
    bool isHotSpotApplicable(const HotSpotRanking& spot,
                            const std::string& optimization);
    int estimateCycleSavings(const HotSpotRanking& spot,
                            const std::string& optimization);
    std::string formatHotSpotRow(const HotSpotRanking& spot);
};

} // namespace phase101
