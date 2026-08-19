#include "OptimizationController.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

void OptimizationController::setLevel(int level) {
    if (level < 0 || level > 9) {
        std::cerr << "Warning: Invalid optimization level " << level
                  << " (valid: 0-9), using level 2\n";
        level = 2;
    }
    level_ = level;
    applyLevelDefaults();
}

void OptimizationController::enableOptimization(const std::string& name) {
    enabledOptimizations_.insert(name);
    disabledOptimizations_.erase(name);
}

void OptimizationController::disableOptimization(const std::string& name) {
    disabledOptimizations_.insert(name);
    enabledOptimizations_.erase(name);
}

bool OptimizationController::isEnabled(const std::string& name) const {
    // Check explicit disable first
    if (disabledOptimizations_.count(name)) {
        return false;
    }

    // Check explicit enable
    if (enabledOptimizations_.count(name)) {
        return true;
    }

    // Check if enabled at current level
    return OptimizationCatalog::isEnabledAtLevel(name, level_);
}

std::set<std::string> OptimizationController::getEnabledOptimizations() const {
    std::set<std::string> result;

    // Get all optimizations at current level
    auto levelOpts = OptimizationCatalog::getOptimizationsAtLevel(level_);
    for (const auto& opt : levelOpts) {
        if (!disabledOptimizations_.count(opt.name)) {
            result.insert(opt.name);
        }
    }

    // Add explicitly enabled optimizations
    for (const auto& name : enabledOptimizations_) {
        result.insert(name);
    }

    return result;
}

bool OptimizationController::parseOption(const std::string& option) {
    if (option.length() < 2) return false;

    // Handle -O<level>
    if (option[0] == '-' && option[1] == 'O') {
        if (option.length() == 3 && std::isdigit(option[2])) {
            // -O0 through -O9
            int level = option[2] - '0';
            setLevel(level);
            return true;
        }

        if (option.length() > 2 && option[2] == 'N' && option[3] == 'o') {
            // -PNo<Name> or -ONo<Name> - disable optimization
            std::string name = option.substr(4);
            auto opt = OptimizationCatalog::getOptimization(name);
            if (opt) {
                disableOptimization(name);
                return true;
            }
            return false;
        }

        // -O<Name> - enable specific optimization
        std::string name = option.substr(2);
        auto opt = OptimizationCatalog::getOptimization(name);
        if (opt) {
            enableOptimization(name);
            return true;
        }
        return false;
    }

    // Handle -P<Name> (same as -O<Name>, backward compatible)
    if (option[0] == '-' && option[1] == 'P') {
        if (option.length() > 2 && option[2] == 'N' && option[3] == 'o') {
            // -PNo<Name> - disable optimization
            std::string name = option.substr(4);
            auto opt = OptimizationCatalog::getOptimization(name);
            if (opt) {
                disableOptimization(name);
                return true;
            }
            return false;
        }

        // -P<Name> - enable specific optimization
        std::string name = option.substr(2);
        auto opt = OptimizationCatalog::getOptimization(name);
        if (opt) {
            enableOptimization(name);
            return true;
        }
        return false;
    }

    return false;
}

void OptimizationController::applyLevelDefaults() {
    enabledOptimizations_.clear();
    disabledOptimizations_.clear();

    // All optimizations enabled at their respective levels
    // will be automatically enabled by isEnabled() checking level_
}

void OptimizationController::setFunctionMode(const std::string& funcName,
                                            FunctionOptimizationMode mode) {
    functionModes_[funcName] = mode;
}

OptimizationController::FunctionOptimizationMode
OptimizationController::getFunctionMode(const std::string& funcName) const {
    auto it = functionModes_.find(funcName);
    if (it != functionModes_.end()) {
        return it->second;
    }
    return DEFAULT;
}

std::string OptimizationController::getDocumentation() const {
    return OptimizationCatalog::getDocumentation();
}
