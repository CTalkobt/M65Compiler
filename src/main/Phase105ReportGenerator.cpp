// Phase 105.5: Report Generator Implementation
#include "Phase105ReportGenerator.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <filesystem>

namespace phase105 {

BenchmarkReport Phase105ReportGenerator::generateReport(
    const std::vector<ExecutionResult>& results,
    const AggregateMetrics& metrics,
    const std::vector<StrategyComparison>& comparisons) {

    BenchmarkReport report;
    report.title = "Phase 105: Automated Benchmarking & Profiling Report";
    report.timestamp = getCurrentTimestamp();

    // Generate sections
    report.executionOverview = "Total Executions: " + std::to_string(metrics.totalExecutions) +
                              "\nSuccessful: " + std::to_string(metrics.successfulExecutions) +
                              "\nSuccess Rate: " + std::to_string(metrics.successRate * 100.0) + "%\n";

    report.metricsAnalysis = generateMetricsTable(metrics);
    report.strategyComparison = generateStrategyTable(comparisons);

    report.summary = generateExecutiveSummary(metrics, comparisons);

    // Identify best strategies
    std::map<std::string, std::vector<ExecutionResult>> byBench;
    for (const auto& result : results) {
        byBench[result.benchmarkName].push_back(result);
    }

    std::vector<Phase105ComparativeAnalyzer::BenchmarkBestStrategy> best;
    for (const auto& [benchName, benchResults] : byBench) {
        Phase105ComparativeAnalyzer::BenchmarkBestStrategy b;
        b.benchmarkName = benchName;
        double maxSpeedup = -1.0;
        for (const auto& result : benchResults) {
            if (result.succeeded && result.actualSpeedup > maxSpeedup) {
                maxSpeedup = result.actualSpeedup;
                b.bestStrategy = result.strategy.strategyName;
                b.achievedSpeedup = result.actualSpeedup;
                b.achievedSizeReduction = result.actualSizeReduction;
            }
        }
        if (!b.bestStrategy.empty()) {
            best.push_back(b);
        }
    }

    report.bestPractices = generateBestPractices(best);

    // Detect regressions
    std::vector<Regression> regressions;
    auto it = byBench.find("baseline");
    if (it != byBench.end()) {
        for (const auto& result : results) {
            if (!result.strategy.isBaseline && result.succeeded) {
                bool foundBaseline = false;
                for (const auto& baselineResult : it->second) {
                    if (baselineResult.benchmarkName == result.benchmarkName) {
                        if (result.actualSpeedup < baselineResult.actualSpeedup - 5.0) {
                            Regression reg;
                            reg.strategy = result.strategy.strategyName;
                            reg.benchmark = result.benchmarkName;
                            reg.expectedPerformance = baselineResult.actualSpeedup;
                            reg.actualPerformance = result.actualSpeedup;
                            reg.regressionPercent = result.actualSpeedup - baselineResult.actualSpeedup;
                            regressions.push_back(reg);
                        }
                        foundBaseline = true;
                        break;
                    }
                }
            }
        }
    }

    report.recommendations = generateRecommendations(regressions, comparisons);

    reportsGenerated_++;
    return report;
}

std::string Phase105ReportGenerator::generateExecutiveSummary(
    const AggregateMetrics& metrics,
    const std::vector<StrategyComparison>& comparisons) {

    std::stringstream summary;

    summary << "EXECUTIVE SUMMARY\n";
    summary << "═════════════════════════════════════════════════════════════════\n";
    summary << "The automated benchmarking suite ran " << metrics.totalExecutions
            << " optimization scenarios across multiple benchmark programs.\n\n";

    summary << "KEY FINDINGS:\n";
    summary << "  • Average speedup achieved: " << std::fixed << std::setprecision(1)
            << metrics.avgSpeedup << "% faster\n";
    summary << "  • Average code size reduction: " << metrics.avgSizeReduction << "%\n";
    summary << "  • Success rate: " << (metrics.successRate * 100.0) << "%\n\n";

    summary << "OPTIMIZATION EFFECTIVENESS:\n";
    summary << "  • Best speedup: " << metrics.maxSpeedup << "%\n";
    summary << "  • Median speedup: " << metrics.medianSpeedup << "%\n";
    summary << "  • Speedup volatility (std dev): "
            << (metrics.maxSpeedup - metrics.minSpeedup) / 4.0 << "%\n\n";

    summary << "COMPILATION COSTS:\n";
    summary << "  • Average compile time: " << std::fixed << std::setprecision(0)
            << metrics.avgCompileTimeMs << " ms\n";
    summary << "  • Average binary size: " << metrics.avgBinarySizeBytes << " bytes\n";

    return summary.str();
}

std::string Phase105ReportGenerator::generateBestPractices(
    const std::vector<Phase105ComparativeAnalyzer::BenchmarkBestStrategy>& best) {

    std::stringstream practices;

    practices << "\nBEST PRACTICES\n";
    practices << "═════════════════════════════════════════════════════════════════\n\n";

    // Group by strategy
    std::map<std::string, std::vector<std::string>> strategyBenches;
    double strategySpeedup;
    for (const auto& b : best) {
        strategyBenches[b.bestStrategy].push_back(b.benchmarkName);
    }

    for (const auto& [strat, benches] : strategyBenches) {
        practices << "Strategy: " << strat << "\n";
        practices << "  Optimal for: ";
        for (size_t i = 0; i < benches.size(); ++i) {
            practices << benches[i];
            if (i < benches.size() - 1) practices << ", ";
        }
        practices << "\n";
        practices << "  Use case: Recommended for programs matching above patterns\n\n";
    }

    return practices.str();
}

std::string Phase105ReportGenerator::generateRecommendations(
    const std::vector<Regression>& regressions,
    const std::vector<StrategyComparison>& comparisons) {

    std::stringstream recommendations;

    recommendations << "\nRECOMMENDATIONS\n";
    recommendations << "═════════════════════════════════════════════════════════════════\n\n";

    if (regressions.empty()) {
        recommendations << "✓ No significant regressions detected.\n\n";
    } else {
        recommendations << "⚠ REGRESSIONS DETECTED:\n";
        for (const auto& reg : regressions) {
            recommendations << "  • " << reg.strategy << " on " << reg.benchmark
                           << ": " << std::fixed << std::setprecision(1)
                           << reg.regressionPercent << "% slower\n";
        }
        recommendations << "\n";
    }

    recommendations << "STRATEGY SELECTION GUIDANCE:\n";
    for (const auto& comp : comparisons) {
        if (!comp.recommendation.empty()) {
            recommendations << "  • " << comp.recommendation << "\n";
        }
    }

    recommendations << "\nFUTURE OPTIMIZATION OPPORTUNITIES:\n";
    recommendations << "  • Profile loop-heavy benchmarks with loop-optimized strategy\n";
    recommendations << "  • Evaluate memory access patterns for bank optimization\n";
    recommendations << "  • Consider struct array striping for data-heavy workloads\n";
    recommendations << "  • Monitor cross-module inlining benefits in larger programs\n";

    return recommendations.str();
}

bool Phase105ReportGenerator::saveReport(const BenchmarkReport& report,
                                        const ReportConfig& config) {

    try {
        // Create output directory if needed
        std::filesystem::create_directories(config.outputDirectory);

        // Generate filename
        std::string filename = config.outputDirectory + "/benchmark_report_" +
                             report.timestamp + ".txt";

        // Open file
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        // Write report
        file << "╔════════════════════════════════════════════════════════════════╗\n";
        file << "║ " << report.title << "\n";
        file << "║ Generated: " << report.timestamp << "\n";
        file << "╚════════════════════════════════════════════════════════════════╝\n\n";

        file << report.summary << "\n\n";
        file << report.executionOverview << "\n\n";
        file << report.metricsAnalysis << "\n\n";
        file << report.strategyComparison << "\n\n";
        file << report.bestPractices << "\n\n";
        file << report.recommendations << "\n\n";

        if (config.includeDetailedResults) {
            file << report.detailedResults << "\n\n";
        }

        if (config.includeRegressionAnalysis) {
            file << report.regressionAnalysis << "\n\n";
        }

        file.close();
        reportsSaved_++;
        return true;

    } catch (...) {
        return false;
    }
}

std::string Phase105ReportGenerator::generateHTMLReport(
    const BenchmarkReport& report) {

    std::stringstream html;

    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "  <title>" << report.title << "</title>\n";
    html << "  <style>\n";
    html << "    body { font-family: monospace; margin: 20px; }\n";
    html << "    h1 { color: #333; }\n";
    html << "    .section { margin: 20px 0; padding: 10px; border-left: 4px solid #0066cc; }\n";
    html << "  </style>\n";
    html << "</head>\n<body>\n";

    html << "<h1>" << report.title << "</h1>\n";
    html << "<p>Generated: " << report.timestamp << "</p>\n";
    html << "<div class=\"section\"><pre>" << escapeHTML(report.summary)
         << "</pre></div>\n";
    html << "<div class=\"section\"><pre>" << escapeHTML(report.metricsAnalysis)
         << "</pre></div>\n";
    html << "<div class=\"section\"><pre>" << escapeHTML(report.strategyComparison)
         << "</pre></div>\n";

    html << "</body>\n</html>";

    return html.str();
}

std::string Phase105ReportGenerator::generateMarkdownReport(
    const BenchmarkReport& report) {

    std::stringstream md;

    md << "# " << report.title << "\n\n";
    md << "**Generated:** " << report.timestamp << "\n\n";

    md << "## Executive Summary\n\n";
    md << "```\n" << report.summary << "\n```\n\n";

    md << "## Execution Overview\n\n";
    md << "```\n" << report.executionOverview << "\n```\n\n";

    md << "## Metrics Analysis\n\n";
    md << "```\n" << report.metricsAnalysis << "\n```\n\n";

    md << "## Strategy Comparison\n\n";
    md << "```\n" << report.strategyComparison << "\n```\n\n";

    md << "## Best Practices\n\n";
    md << "```\n" << report.bestPractices << "\n```\n\n";

    md << "## Recommendations\n\n";
    md << "```\n" << report.recommendations << "\n```\n\n";

    return md.str();
}

bool Phase105ReportGenerator::exportMetricsCSV(
    const std::vector<ExecutionResult>& results,
    const std::string& filename) {

    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        // Write header
        file << "Benchmark,Strategy,BinarySize,Speedup,SizeReduction,CompileTime,Success\n";

        // Write rows
        for (const auto& result : results) {
            file << result.benchmarkName << ","
                 << result.strategy.strategyName << ","
                 << result.metrics.binarySize << ","
                 << std::fixed << std::setprecision(2) << result.actualSpeedup << ","
                 << result.actualSizeReduction << ","
                 << result.metrics.totalCompileTime.count() << ","
                 << (result.succeeded ? "true" : "false") << "\n";
        }

        file.close();
        return true;

    } catch (...) {
        return false;
    }
}

std::string Phase105ReportGenerator::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm* tm = std::localtime(&time);

    std::stringstream ss;
    ss << std::put_time(tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

std::string Phase105ReportGenerator::formatSection(const std::string& title,
                                                   const std::string& content) {

    std::stringstream section;
    section << title << "\n";
    section << std::string(title.length(), '=') << "\n\n";
    section << content << "\n\n";
    return section.str();
}

std::string Phase105ReportGenerator::generateMetricsTable(
    const AggregateMetrics& metrics) {

    std::stringstream table;
    table << "METRICS SUMMARY\n";
    table << "───────────────────────────────────────────────────────────────────\n";
    table << "Total Executions:     " << metrics.totalExecutions << "\n";
    table << "Successful:           " << metrics.successfulExecutions << "\n";
    table << "Avg Speedup:          " << std::fixed << std::setprecision(1)
          << metrics.avgSpeedup << "%\n";
    table << "Avg Size Reduction:   " << metrics.avgSizeReduction << "%\n";

    return table.str();
}

std::string Phase105ReportGenerator::generateStrategyTable(
    const std::vector<StrategyComparison>& comps) {

    std::stringstream table;
    table << "STRATEGY COMPARISONS\n";
    table << "───────────────────────────────────────────────────────────────────\n";

    for (const auto& comp : comps) {
        table << comp.baselineStrategy << " vs " << comp.optimizedStrategy << ": ";
        table << std::fixed << std::setprecision(1) << comp.speedupDifference
              << "% speedup difference\n";
    }

    return table.str();
}

std::string Phase105ReportGenerator::escapeHTML(const std::string& text) {
    std::stringstream ss;
    for (char c : text) {
        switch (c) {
            case '&': ss << "&amp;"; break;
            case '<': ss << "&lt;"; break;
            case '>': ss << "&gt;"; break;
            case '"': ss << "&quot;"; break;
            default: ss << c;
        }
    }
    return ss.str();
}

} // namespace phase105
