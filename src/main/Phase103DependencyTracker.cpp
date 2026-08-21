// Phase 103.2: Dependency Tracking Implementation
#include "Phase103DependencyTracker.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <numeric>

namespace phase103 {

DependencyGraph Phase103DependencyTracker::buildDependencyGraph(
    const std::vector<std::string>& moduleNames) {
    DependencyGraph graph;
    graph.modules = moduleNames;

    // Simulate building dependency edges
    for (size_t i = 0; i < moduleNames.size(); ++i) {
        for (size_t j = 0; j < moduleNames.size(); ++j) {
            if (i == j) continue;

            DependencyEdge edge;
            edge.sourceModule = moduleNames[i];
            edge.targetModule = moduleNames[j];
            edge.weight = (i + 1) * (j + 1) % 5 + 1;  // Simulated weight
            edge.estimatedCallCount = 50 + (i * 10);
            edge.isHotEdge = edge.estimatedCallCount > 100;
            edge.callsites.push_back("func_" + std::to_string(i) + "_" + std::to_string(j));

            graph.edges.push_back(edge);
            graph.adjacency[edge.sourceModule].push_back(edge.targetModule);
            graph.edgeWeights[{edge.sourceModule, edge.targetModule}] = edge.weight;
            totalEdges_++;
        }
    }

    analyzeDependencies(graph);
    return graph;
}

void Phase103DependencyTracker::analyzeDependencies(DependencyGraph& graph) {
    // Compute edge weights and properties
    for (auto& edge : graph.edges) {
        // Weight based on call frequency
        edge.weight = std::min(10, edge.estimatedCallCount / 50);
    }
}

std::vector<ModuleCluster> Phase103DependencyTracker::identifyModuleClusters(
    const DependencyGraph& graph) {
    std::vector<ModuleCluster> clusters;
    std::set<std::string> visited;

    for (const auto& module : graph.modules) {
        if (visited.count(module)) continue;

        std::vector<std::string> component;
        dfs(module, visited, graph, component);

        if (component.empty()) continue;

        ModuleCluster cluster;
        cluster.clusterId = "mc_" + std::to_string(identifiedClusters_++);
        cluster.members = component;

        cluster.internalDependencies = countInternalDependencies(cluster, graph);
        cluster.externalDependencies = countExternalDependencies(cluster, graph);
        cluster.cohesion = calculateCohesion(cluster, graph);
        cluster.canOptimizeTogether = cluster.cohesion > 0.6;

        if (!cluster.members.empty()) {
            clusters.push_back(cluster);
            cohesionValues_.push_back(cluster.cohesion);
        }
    }

    return clusters;
}

double Phase103DependencyTracker::calculateCohesion(
    const ModuleCluster& cluster,
    const DependencyGraph& graph) {
    int internal = cluster.internalDependencies;
    int external = cluster.externalDependencies;

    if (internal + external == 0) return 0.0;

    return (double)internal / (internal + external);
}

std::vector<ModuleCluster>
Phase103DependencyTracker::findOptimizationBoundaries(
    const DependencyGraph& graph) {
    auto clusters = identifyModuleClusters(graph);

    // Filter to clusters that can optimize together
    std::vector<ModuleCluster> optimizable;
    for (const auto& cluster : clusters) {
        if (cluster.canOptimizeTogether) {
            optimizable.push_back(cluster);
        }
    }

    return optimizable;
}

std::string Phase103DependencyTracker::generateOptimizationStrategy(
    const std::vector<ModuleCluster>& clusters) {
    std::stringstream strategy;

    strategy << "CROSS-MODULE OPTIMIZATION STRATEGY\n";
    strategy << "====================================\n\n";

    for (size_t i = 0; i < clusters.size(); ++i) {
        const auto& cluster = clusters[i];
        strategy << "Cluster " << (i + 1) << ": " << cluster.clusterId << "\n";
        strategy << "  Modules: " << cluster.members.size() << "\n";
        strategy << "  Cohesion: " << std::fixed << std::setprecision(2)
                 << cluster.cohesion << "\n";
        strategy << "  Internal Dependencies: " << cluster.internalDependencies << "\n";
        strategy << "  External Dependencies: " << cluster.externalDependencies << "\n";
        strategy << "  Optimization: " << (cluster.canOptimizeTogether ? "YES" : "NO") << "\n\n";
    }

    return strategy.str();
}

std::string Phase103DependencyTracker::generateDependencyReport(
    const DependencyGraph& graph) {
    std::stringstream report;

    report << "╔════════════════════════════════════════════════════════════════╗\n";
    report << "║     Phase 103.2: Module Dependency Analysis Report             ║\n";
    report << "║     Cross-Module Optimization Opportunities                    ║\n";
    report << "╚════════════════════════════════════════════════════════════════╝\n\n";

    report << "DEPENDENCY SUMMARY\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Total Modules:              " << graph.modules.size() << "\n";
    report << "Total Dependencies:         " << graph.edges.size() << "\n";
    report << "Identified Clusters:        " << identifiedClusters_ << "\n";
    report << "Average Cohesion:           " << std::fixed << std::setprecision(2)
           << getAverageCohesion() << "\n\n";

    report << "DEPENDENCY EDGES\n";
    report << "──────────────────────────────────────────────────────────────────\n";
    report << "Source           Target           Weight  Calls   Hot\n";
    report << "──────────────────────────────────────────────────────────────────\n";

    for (const auto& edge : graph.edges) {
        report << formatEdgeRow(edge);
    }

    report << "\n";
    return report.str();
}

double Phase103DependencyTracker::getAverageCohesion() const {
    if (cohesionValues_.empty()) return 0.0;

    double sum = std::accumulate(cohesionValues_.begin(),
                                cohesionValues_.end(), 0.0);
    return sum / cohesionValues_.size();
}

void Phase103DependencyTracker::dfs(
    const std::string& node,
    std::set<std::string>& visited,
    const DependencyGraph& graph,
    std::vector<std::string>& component) {
    visited.insert(node);
    component.push_back(node);

    auto it = graph.adjacency.find(node);
    if (it != graph.adjacency.end()) {
        for (const auto& neighbor : it->second) {
            if (!visited.count(neighbor)) {
                dfs(neighbor, visited, graph, component);
            }
        }
    }
}

int Phase103DependencyTracker::countInternalDependencies(
    const ModuleCluster& cluster,
    const DependencyGraph& graph) {
    int count = 0;
    std::set<std::string> members(cluster.members.begin(), cluster.members.end());

    for (const auto& edge : graph.edges) {
        if (members.count(edge.sourceModule) &&
            members.count(edge.targetModule)) {
            count += edge.weight;
        }
    }

    return count;
}

int Phase103DependencyTracker::countExternalDependencies(
    const ModuleCluster& cluster,
    const DependencyGraph& graph) {
    int count = 0;
    std::set<std::string> members(cluster.members.begin(), cluster.members.end());

    for (const auto& edge : graph.edges) {
        if ((members.count(edge.sourceModule) &&
             !members.count(edge.targetModule)) ||
            (!members.count(edge.sourceModule) &&
             members.count(edge.targetModule))) {
            count += edge.weight;
        }
    }

    return count;
}

std::string Phase103DependencyTracker::formatEdgeRow(const DependencyEdge& edge) {
    std::stringstream row;

    row << std::left << std::setw(16) << edge.sourceModule
        << "  " << std::setw(16) << edge.targetModule
        << "  " << std::right << std::setw(6) << edge.weight
        << "  " << std::setw(6) << edge.estimatedCallCount
        << "  " << (edge.isHotEdge ? "Yes" : "No") << "\n";

    return row.str();
}

} // namespace phase103
