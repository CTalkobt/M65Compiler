#include "TemplateRegistry.hpp"
#include <algorithm>
#include <sstream>

TemplateRegistry::TemplateRegistry()
    : strategy_(ExecutionStrategy::GREEDY_BENEFIT), verbose_(false) {
}

TemplateRegistry::~TemplateRegistry() = default;

void TemplateRegistry::registerOptimization(std::unique_ptr<TemplateOptimizationPass> optimization) {
    if (!optimization) return;

    auto type = optimization->getType();
    auto name = optimization->getName();

    optimizations_[type] = std::move(optimization);
    optimizationsByName_[name] = optimizations_[type].get();
}

TemplateOptimizationPass* TemplateRegistry::getOptimization(TemplateOptimizationPass::OptimizationType type) {
    auto it = optimizations_.find(type);
    if (it != optimizations_.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<TemplateOptimizationPass*> TemplateRegistry::getAllOptimizations() {
    std::vector<TemplateOptimizationPass*> result;
    for (auto& entry : optimizations_) {
        result.push_back(entry.second.get());
    }
    return result;
}

std::vector<TemplateOptimizationPass*> TemplateRegistry::getEnabledOptimizations() {
    std::vector<TemplateOptimizationPass*> result;
    for (auto& entry : optimizations_) {
        if (entry.second->isEnabled()) {
            result.push_back(entry.second.get());
        }
    }

    // Sort by execution order if available
    if (!executionOrder_.empty()) {
        std::sort(result.begin(), result.end(),
            [this](TemplateOptimizationPass* a, TemplateOptimizationPass* b) {
                auto aType = a->getType();
                auto bType = b->getType();
                auto aIt = std::find(executionOrder_.begin(), executionOrder_.end(), aType);
                auto bIt = std::find(executionOrder_.begin(), executionOrder_.end(), bType);
                return aIt < bIt;
            });
    }

    return result;
}

void TemplateRegistry::enableOptimization(TemplateOptimizationPass::OptimizationType type, bool enabled) {
    auto optimization = getOptimization(type);
    if (optimization) {
        optimization->setEnabled(enabled);
    }
}

bool TemplateRegistry::isOptimizationEnabled(TemplateOptimizationPass::OptimizationType type) const {
    auto it = optimizations_.find(type);
    if (it != optimizations_.end()) {
        return it->second->isEnabled();
    }
    return false;
}

TemplateOptimizationPass* TemplateRegistry::getOptimizationByName(const std::string& name) {
    auto it = optimizationsByName_.find(name);
    if (it != optimizationsByName_.end()) {
        return it->second;
    }
    return nullptr;
}

void TemplateRegistry::loadConfiguration(const std::string& /* configFile */) {
    // TODO: Implement configuration loading from file
}

void TemplateRegistry::setExecutionOrder(const std::vector<TemplateOptimizationPass::OptimizationType>& order) {
    executionOrder_ = order;
}

std::string TemplateRegistry::generateReport() const {
    std::stringstream ss;
    ss << "=== Template Optimization Registry Report ===\n\n";
    ss << "Total Registered Optimizations: " << optimizations_.size() << "\n";

    // Count enabled optimizations
    int enabledCount = 0;
    for (const auto& entry : optimizations_) {
        if (entry.second->isEnabled()) enabledCount++;
    }
    ss << "Enabled Optimizations: " << enabledCount << "\n";
    ss << "Execution Strategy: ";

    switch (strategy_) {
        case ExecutionStrategy::ALL:
            ss << "All Enabled Optimizations\n"; break;
        case ExecutionStrategy::GREEDY_BENEFIT:
            ss << "Greedy (Highest Benefit First)\n"; break;
        case ExecutionStrategy::BUDGET_AWARE:
            ss << "Budget-Aware\n"; break;
        case ExecutionStrategy::DEPENDENCY_ORDERED:
            ss << "Dependency-Ordered\n"; break;
    }

    ss << "\nRegistered Optimizations:\n";
    ss << "--------------------------\n";

    for (const auto& entry : optimizations_) {
        auto optimization = entry.second.get();
        ss << "- " << optimization->getName() << " ("
           << (optimization->isEnabled() ? "ENABLED" : "DISABLED") << ")\n";
    }

    return ss.str();
}

std::vector<TemplateOptimizationPass::OptimizationMetrics> TemplateRegistry::getAllMetrics() const {
    std::vector<TemplateOptimizationPass::OptimizationMetrics> result;
    for (const auto& entry : optimizations_) {
        if (entry.second->isEnabled()) {
            result.push_back(entry.second->getMetrics());
        }
    }
    return result;
}

void TemplateRegistry::computeOptimalOrder() {
    // TODO: Implement dependency analysis and optimal ordering
    // For now, use default order
    executionOrder_ = {
        TemplateOptimizationPass::OptimizationType::MATH_LIBRARY_OPTIMIZATION,
        TemplateOptimizationPass::OptimizationType::CONDITIONAL_CHAIN_FUSION,
        TemplateOptimizationPass::OptimizationType::REGISTER_RESIDENT_LOOPS,
        TemplateOptimizationPass::OptimizationType::POINTER_CHASING_CACHE,
        TemplateOptimizationPass::OptimizationType::TABLE_DRIVEN_DISPATCH,
        TemplateOptimizationPass::OptimizationType::BIT_MANIPULATION_REDUCTION,
        TemplateOptimizationPass::OptimizationType::DMA_AUTO_INVOKE,
        TemplateOptimizationPass::OptimizationType::ZEROPAGE_PRESSURE_RELIEF
    };
}
