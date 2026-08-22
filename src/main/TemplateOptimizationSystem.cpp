#include "TemplateOptimizationSystem.hpp"
#include "MathLibraryOptimization.hpp"
#include "ConditionalChainFusion.hpp"
#include "RegisterResidentLoops.hpp"
#include "PointerCachingOptimization.hpp"
#include "TableDrivenDispatch.hpp"
#include "BitManipulationReduction.hpp"
#include "DMAOperationTemplate.hpp"
#include "ZeroPagePressureRelief.hpp"
#include "AdaptiveLearnerIntegration.hpp"
#include <iostream>
#include <sstream>

TemplateOptimizationSystem::TemplateOptimizationSystem()
    : learner_(nullptr), verbose_(false), initialized_(false),
      totalCodeReductionBytes_(0), totalInstructionsOptimized_(0) {
    registry_ = std::make_unique<TemplateRegistry>();
}

TemplateOptimizationSystem::~TemplateOptimizationSystem() = default;

void TemplateOptimizationSystem::initialize(bool verbose) {
    verbose_ = verbose;
    registerAllOptimizations();
    initialized_ = true;

    if (verbose) {
        std::cout << "Template Optimization System initialized" << std::endl;
        std::cout << registry_->generateReport();
    }
}

void TemplateOptimizationSystem::registerAllOptimizations() {
    if (verbose_) {
        std::cout << "  Registering optimization templates..." << std::endl;
    }

    // Register Math Library Constant Folding
    registry_->registerOptimization(std::make_unique<MathLibraryOptimization>());

    // Register Conditional Chain Fusion
    registry_->registerOptimization(std::make_unique<ConditionalChainFusion>());

    // Register Register-Resident Loop Optimization
    registry_->registerOptimization(std::make_unique<RegisterResidentLoops>());

    // Register Pointer Chasing Optimization
    registry_->registerOptimization(std::make_unique<PointerCachingOptimization>());

    // Register Table-Driven Dispatch
    registry_->registerOptimization(std::make_unique<TableDrivenDispatch>());

    // Register Bit Manipulation Strength Reduction
    registry_->registerOptimization(std::make_unique<BitManipulationReduction>());

    // Register DMA Operation Template
    registry_->registerOptimization(std::make_unique<DMAOperationTemplate>());

    // Register Zero-Page Pressure Relief
    registry_->registerOptimization(std::make_unique<ZeroPagePressureRelief>());

    if (verbose_) {
        std::cout << "  Registered " << registry_->getOptimizationCount() << " optimization templates" << std::endl;
    }
}

void TemplateOptimizationSystem::applyToAST(TranslationUnit& ast) {
    if (!initialized_) return;

    auto enabledOptimizations = registry_->getEnabledOptimizations();
    for (auto optimization : enabledOptimizations) {
        if (verbose_) {
            std::cout << "  Applying " << optimization->getName() << "..." << std::endl;
        }
        optimization->apply(ast);
    }

    collectMetrics();
}

void TemplateOptimizationSystem::applyToIR(ir::Module& irModule) {
    if (!initialized_) return;

    auto enabledOptimizations = registry_->getEnabledOptimizations();
    for (auto optimization : enabledOptimizations) {
        if (verbose_) {
            std::cout << "  Applying " << optimization->getName() << " to IR..." << std::endl;
        }
        optimization->apply(irModule);
    }

    collectMetrics();
}

void TemplateOptimizationSystem::applyPostCodeGen(const std::string& /* assembly */) {
    if (!initialized_) return;

    // Assembly-level optimizations applied here
    // For now, just a placeholder

    if (verbose_) {
        std::cout << "  Post-code generation optimization complete" << std::endl;
    }
}

void TemplateOptimizationSystem::loadConfiguration(const std::string& configFile) {
    if (registry_) {
        registry_->loadConfiguration(configFile);
    }
}

void TemplateOptimizationSystem::setExecutionStrategy(TemplateRegistry::ExecutionStrategy strategy) {
    if (registry_) {
        registry_->setStrategy(strategy);
    }
}

void TemplateOptimizationSystem::enableOptimization(TemplateOptimizationPass::OptimizationType type, bool enabled) {
    if (registry_) {
        registry_->enableOptimization(type, enabled);
    }
}

bool TemplateOptimizationSystem::isOptimizationEnabled(TemplateOptimizationPass::OptimizationType type) const {
    if (registry_) {
        return registry_->isOptimizationEnabled(type);
    }
    return false;
}

std::string TemplateOptimizationSystem::generateReport() const {
    std::stringstream ss;
    ss << "=== Template Optimization Report ===\n\n";

    if (registry_) {
        ss << registry_->generateReport() << "\n";
    }

    ss << "Total Code Reduction: " << totalCodeReductionBytes_ << " bytes\n";
    ss << "Total Instructions Optimized: " << totalInstructionsOptimized_ << "\n";

    if (!allMetrics_.empty()) {
        ss << "\nPer-Optimization Metrics:\n";
        ss << "--------------------------\n";
        for (const auto& metric : allMetrics_) {
            ss << "- " << metric.optimizationName << ": " << metric.codeReductionBytes
               << " bytes, " << metric.instructionsOptimized << " instructions\n";
        }
    }

    return ss.str();
}

std::vector<TemplateOptimizationPass::OptimizationMetrics> TemplateOptimizationSystem::getMetrics() const {
    return allMetrics_;
}

int TemplateOptimizationSystem::getTotalCodeReduction() const {
    return totalCodeReductionBytes_;
}

double TemplateOptimizationSystem::getEstimatedSpeedup() const {
    // TODO: Calculate based on instruction optimizations and type
    return 0.0;
}

bool TemplateOptimizationSystem::hasOptimization(TemplateOptimizationPass::OptimizationType type) const {
    if (registry_) {
        return registry_->getOptimization(type) != nullptr;
    }
    return false;
}

void TemplateOptimizationSystem::collectMetrics() {
    allMetrics_ = registry_->getAllMetrics();
    totalCodeReductionBytes_ = 0;
    totalInstructionsOptimized_ = 0;

    for (const auto& metric : allMetrics_) {
        totalCodeReductionBytes_ += metric.codeReductionBytes;
        totalInstructionsOptimized_ += metric.instructionsOptimized;
    }
}

void TemplateOptimizationSystem::reportToLearner() {
    if (!learner_) return;

    // TODO: Report effectiveness metrics to learning system
    // This allows the learning system to understand which optimizations work best in different contexts
}
