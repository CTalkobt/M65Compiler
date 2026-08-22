#include "MathLibraryOptimization.hpp"
#include <cmath>
#include <limits>

// Define supported math functions with their evaluators
const std::map<std::string, MathLibraryOptimization::MathFunction>
MathLibraryOptimization::MATH_FUNCTIONS = {
    {"sqrtf", {.name = "sqrtf", .argCount = 1, .evaluator = std::sqrt, .evaluator2 = nullptr}},
    {"sqrt", {.name = "sqrt", .argCount = 1, .evaluator = std::sqrt, .evaluator2 = nullptr}},
    {"sinf", {.name = "sinf", .argCount = 1, .evaluator = std::sin, .evaluator2 = nullptr}},
    {"sin", {.name = "sin", .argCount = 1, .evaluator = std::sin, .evaluator2 = nullptr}},
    {"cosf", {.name = "cosf", .argCount = 1, .evaluator = std::cos, .evaluator2 = nullptr}},
    {"cos", {.name = "cos", .argCount = 1, .evaluator = std::cos, .evaluator2 = nullptr}},
    {"tanf", {.name = "tanf", .argCount = 1, .evaluator = std::tan, .evaluator2 = nullptr}},
    {"tan", {.name = "tan", .argCount = 1, .evaluator = std::tan, .evaluator2 = nullptr}},
    {"logf", {.name = "logf", .argCount = 1, .evaluator = std::log, .evaluator2 = nullptr}},
    {"log", {.name = "log", .argCount = 1, .evaluator = std::log, .evaluator2 = nullptr}},
    {"log10f", {.name = "log10f", .argCount = 1, .evaluator = std::log10, .evaluator2 = nullptr}},
    {"log10", {.name = "log10", .argCount = 1, .evaluator = std::log10, .evaluator2 = nullptr}},
    {"expf", {.name = "expf", .argCount = 1, .evaluator = std::exp, .evaluator2 = nullptr}},
    {"exp", {.name = "exp", .argCount = 1, .evaluator = std::exp, .evaluator2 = nullptr}},
    {"absf", {.name = "absf", .argCount = 1, .evaluator = std::fabs, .evaluator2 = nullptr}},
    {"fabsf", {.name = "fabsf", .argCount = 1, .evaluator = std::fabs, .evaluator2 = nullptr}},
};

MathLibraryOptimization::MathLibraryOptimization()
    : callsOptimized_(0), bytesReduced_(0) {
    metrics_.optimizationName = "Math Library Constant Folding";
    metrics_.type = OptimizationType::MATH_LIBRARY_OPTIMIZATION;
}

MathLibraryOptimization::~MathLibraryOptimization() = default;

void MathLibraryOptimization::apply(TranslationUnit& /* ast */) {
    // AST-level math optimization
    // TODO: Walk AST and identify constant math function calls
    // For now, optimization primarily happens at IR level
}

void MathLibraryOptimization::apply(ir::Module& /* irModule */) {
    // IR-level math constant folding
    // TODO: Walk IR module for CALL ops to math functions
    // When call has constant arguments, replace with precomputed literal
    // Track optimization metrics
}

bool MathLibraryOptimization::isMathFunction(const std::string& funcName) const {
    return MATH_FUNCTIONS.find(funcName) != MATH_FUNCTIONS.end();
}

bool MathLibraryOptimization::canEvaluate(const std::string& funcName) const {
    return isMathFunction(funcName);
}

double MathLibraryOptimization::evaluateMathCall(
    const std::string& funcName, double arg1, double arg2) {

    auto it = MATH_FUNCTIONS.find(funcName);
    if (it == MATH_FUNCTIONS.end()) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    const auto& func = it->second;

    if (func.argCount == 1 && func.evaluator) {
        // Clamp domain checks: sqrt requires arg >= 0, log requires arg > 0
        if ((funcName.find("sqrt") != std::string::npos && arg1 < 0.0) ||
            (funcName.find("log") != std::string::npos && arg1 <= 0.0)) {
            return std::numeric_limits<double>::quiet_NaN();
        }
        return func.evaluator(arg1);
    }

    // Two-argument functions (if implemented)
    if (func.argCount == 2 && func.evaluator2) {
        return func.evaluator2(arg1, arg2);
    }

    return std::numeric_limits<double>::quiet_NaN();
}
