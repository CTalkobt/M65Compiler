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

void OptimizationController::setFunctionOptimization(const std::string& funcName,
                                                    const std::string& optName,
                                                    bool enable) {
    // Convert pragma name to internal name if needed
    std::string internalName = pragmaNameToInternal(optName);

    // Validate that this is a real optimization
    auto opt = OptimizationCatalog::getOptimization(internalName);
    if (!opt) {
        std::cerr << "Warning: Unknown optimization '" << optName << "'\n";
        return;
    }

    functionOptimizations_[funcName][internalName] = enable;
}

bool OptimizationController::isFunctionOptimizationEnabled(const std::string& funcName,
                                                          const std::string& optName) const {
    // Check if there's a function-level override
    auto funcIt = functionOptimizations_.find(funcName);
    if (funcIt != functionOptimizations_.end()) {
        auto optIt = funcIt->second.find(optName);
        if (optIt != funcIt->second.end()) {
            return optIt->second;
        }
    }

    // Fall back to global settings
    return isEnabled(optName);
}

bool OptimizationController::hasFunctionOptimizationOverride(const std::string& funcName,
                                                            const std::string& optName) const {
    auto funcIt = functionOptimizations_.find(funcName);
    if (funcIt != functionOptimizations_.end()) {
        return funcIt->second.find(optName) != funcIt->second.end();
    }
    return false;
}

std::string OptimizationController::pragmaNameToInternal(const std::string& pragmaName) const {
    // Convert snake_case to CamelCase
    // Example: "loop_unrolling" -> "LoopUnrolling"
    std::string result;
    bool capitalize = true;

    for (char c : pragmaName) {
        if (c == '_') {
            capitalize = true;
        } else if (capitalize) {
            result += std::toupper(c);
            capitalize = false;
        } else {
            result += c;
        }
    }

    return result;
}

std::string OptimizationController::internalNameToPragma(const std::string& internalName) const {
    // Convert CamelCase to snake_case
    // Example: "LoopUnrolling" -> "loop_unrolling"
    std::string result;

    for (size_t i = 0; i < internalName.length(); ++i) {
        char c = internalName[i];
        if (i > 0 && std::isupper(c)) {
            result += '_';
            result += std::tolower(c);
        } else {
            result += std::tolower(c);
        }
    }

    return result;
}

std::string OptimizationController::getDocumentation() const {
    return OptimizationCatalog::getDocumentation();
}
