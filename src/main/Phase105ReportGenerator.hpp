// Phase 105.5: Report Generator
// Comprehensive benchmark report generation and output

#pragma once

#include "Phase105ComparativeAnalyzer.hpp"
#include "Phase104BenchmarkSuite.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <memory>

namespace phase105 {

// Comprehensive benchmark report
struct BenchmarkReport {
    std::string title;
    std::string timestamp;
    std::string summary;

    // Report sections
    std::string executionOverview;
    std::string metricsAnalysis;
    std::string strategyComparison;
    std::string bestPractices;
    std::string recommendations;

    // Appendices
    std::string detailedResults;
    std::string regressionAnalysis;
};

// Report configuration
struct ReportConfig {
    bool includeDetailedResults = true;
    bool includeRegressionAnalysis = true;
    bool includePerOptimizationStats = true;
    bool generateHTML = false;
    bool generateMarkdown = true;
    std::string outputDirectory = "./benchmark_reports";
};

// Report generator coordinator
class Phase105ReportGenerator {
public:
    Phase105ReportGenerator() = default;

    // Generate comprehensive benchmark report
    BenchmarkReport generateReport(
        const std::vector<ExecutionResult>& results,
        const AggregateMetrics& metrics,
        const std::vector<StrategyComparison>& comparisons);

    // Generate executive summary
    std::string generateExecutiveSummary(
        const AggregateMetrics& metrics,
        const std::vector<StrategyComparison>& comparisons);

    // Generate best practices section
    std::string generateBestPractices(
        const std::vector<Phase105ComparativeAnalyzer::BenchmarkBestStrategy>& best);

    // Generate recommendations section
    std::string generateRecommendations(
        const std::vector<Regression>& regressions,
        const std::vector<StrategyComparison>& comparisons);

    // Save report to file
    bool saveReport(const BenchmarkReport& report,
                   const ReportConfig& config);

    // Generate HTML report
    std::string generateHTMLReport(const BenchmarkReport& report);

    // Generate Markdown report
    std::string generateMarkdownReport(const BenchmarkReport& report);

    // Export metrics to CSV
    bool exportMetricsCSV(const std::vector<ExecutionResult>& results,
                         const std::string& filename);

    // Get report statistics
    int getTotalReportsGenerated() const { return reportsGenerated_; }
    int getTotalReportsSaved() const { return reportsSaved_; }

private:
    int reportsGenerated_ = 0;
    int reportsSaved_ = 0;

    // Helper methods
    std::string getCurrentTimestamp();
    std::string formatSection(const std::string& title,
                            const std::string& content);
    std::string generateMetricsTable(const AggregateMetrics& metrics);
    std::string generateStrategyTable(const std::vector<StrategyComparison>& comps);
    std::string escapeHTML(const std::string& text);
};

} // namespace phase105
