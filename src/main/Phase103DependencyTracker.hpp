// Phase 103.2: Inter-Module Dependency Tracking
// Tracks dependencies between modules for optimization coordination

#pragma once

#include "Phase103CrossModuleEnhancer.hpp"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

namespace phase103 {

// Module dependency edge
struct DependencyEdge {
    std::string sourceModule;
    std::string targetModule;
    int weight = 1;                // Strength of dependency
    std::vector<std::string> callsites;  // Function names
    bool isHotEdge = false;        // Frequent calls
    int estimatedCallCount = 0;
};

// Module dependency graph
struct DependencyGraph {
    std::vector<std::string> modules;
    std::vector<DependencyEdge> edges;
    std::map<std::string, std::vector<std::string>> adjacency;
    std::map<std::pair<std::string, std::string>, int> edgeWeights;
};

// Module cluster for optimization
struct ModuleCluster {
    std::string clusterId;
    std::vector<std::string> members;
    int internalDependencies = 0;
    int externalDependencies = 0;
    double cohesion = 0.0;        // Internal vs external ratio
    bool canOptimizeTogether = false;
};

// Dependency tracker for cross-module optimization
class Phase103DependencyTracker {
public:
    explicit Phase103DependencyTracker(
        const Phase103CrossModuleEnhancer& enhancer)
        : enhancer_(enhancer) {}

    // Build dependency graph from modules
    DependencyGraph buildDependencyGraph(
        const std::vector<std::string>& moduleNames);

    // Analyze module dependencies
    void analyzeDependencies(DependencyGraph& graph);

    // Identify strongly connected components (clusters)
    std::vector<ModuleCluster> identifyModuleClusters(
        const DependencyGraph& graph);

    // Calculate module cohesion
    double calculateCohesion(const ModuleCluster& cluster,
                            const DependencyGraph& graph);

    // Find optimization boundaries
    std::vector<ModuleCluster> findOptimizationBoundaries(
        const DependencyGraph& graph);

    // Generate coordinated optimization strategy
    std::string generateOptimizationStrategy(
        const std::vector<ModuleCluster>& clusters);

    // Generate dependency report
    std::string generateDependencyReport(const DependencyGraph& graph);

    // Get tracking statistics
    int getTotalEdges() const { return totalEdges_; }
    int getIdentifiedClusters() const { return identifiedClusters_; }
    double getAverageCohesion() const;

private:
    const Phase103CrossModuleEnhancer& enhancer_;

    int totalEdges_ = 0;
    int identifiedClusters_ = 0;
    std::vector<double> cohesionValues_;

    // Helper methods
    void dfs(const std::string& node,
            std::set<std::string>& visited,
            const DependencyGraph& graph,
            std::vector<std::string>& component);
    int countInternalDependencies(const ModuleCluster& cluster,
                                 const DependencyGraph& graph);
    int countExternalDependencies(const ModuleCluster& cluster,
                                 const DependencyGraph& graph);
    std::string formatEdgeRow(const DependencyEdge& edge);
};

} // namespace phase103
