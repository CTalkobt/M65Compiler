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
    : OptimizationPassBase(OptimizationType::MATH_LIBRARY_OPTIMIZATION,
                          "Math Library Constant Folding"),
      callsOptimized_(0), bytesReduced_(0) {
}

MathLibraryOptimization::~MathLibraryOptimization() = default;

void MathLibraryOptimization::apply(TranslationUnit& /* ast */) {
    // AST-level math optimization
    // Constant folding of math functions is deferred to compile-time evaluation
    // IR-level optimization is more effective after semantic analysis
}

void MathLibraryOptimization::apply(ir::Module& irModule) {
    // IR-level math constant folding (Phase C5.2)
    // Walk IR module for CALL ops to math functions with constant arguments
    // When call has constant arguments, replace with precomputed literal

    for (auto& func : irModule.functions) {
        for (auto& block : func.blocks) {
            // Track which instructions to replace (can't modify during iteration)
            std::vector<std::pair<size_t, ir::Inst>> replacements;

            for (size_t i = 0; i < block.insts.size(); ++i) {
                auto& inst = block.insts[i];

                // Look for CALL instructions
                if (inst.op != ir::Op::CALL) {
                    continue;
                }

                // Extract function name from operand
                if (inst.src1.kind != ir::OperandKind::GLOBAL) {
                    continue;
                }

                const std::string& funcName = inst.src1.name;

                // Check if this is a math function we can optimize
                if (!isMathFunction(funcName)) {
                    continue;
                }

                // Check if all arguments are constants (Phase C5.2)
                if (inst.args.empty()) {
                    continue;
                }

                // Verify all arguments are immediate values
                bool allConstant = true;
                std::vector<double> argValues;

                for (const auto& arg : inst.args) {
                    if (arg.kind != ir::OperandKind::IMM) {
                        allConstant = false;
                        break;
                    }
                    argValues.push_back(static_cast<double>(arg.immVal));
                }

                if (!allConstant || argValues.empty()) {
                    continue;
                }

                // Evaluate math function at compile time
                double result = evaluateMathCall(funcName, argValues[0],
                                                 argValues.size() > 1 ? argValues[1] : 0.0);

                // Skip if evaluation failed (domain error, etc.)
                if (std::isnan(result)) {
                    continue;
                }

                // Create replacement CONST instruction
                ir::Inst replacement;
                replacement.op = ir::Op::CONST;
                replacement.dest = inst.dest;
                replacement.resultType = inst.resultType;

                // Convert float result to int64 for storage
                // For float types, store bit representation
                if (inst.resultType == ir::Type::F32) {
                    // CBM 40-bit float: store as integer representation
                    replacement.src1 = ir::Operand::imm(
                        *reinterpret_cast<int64_t*>(&result), ir::Type::I32);
                } else {
                    // Integer result: direct conversion
                    replacement.src1 = ir::Operand::imm(
                        static_cast<int64_t>(result), inst.resultType);
                }
                replacement.loc = inst.loc;

                // Queue replacement
                replacements.push_back({i, replacement});

                // Update metrics
                callsOptimized_++;
                bytesReduced_ += 6;  // Estimate: CALL is ~6 bytes, CONST load is ~2-3 bytes
            }

            // Apply queued replacements
            for (const auto& [index, replacement] : replacements) {
                block.insts[index] = replacement;
            }
        }
    }

    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = callsOptimized_;
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
        // Domain checks: sqrt requires arg >= 0, log requires arg > 0
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
