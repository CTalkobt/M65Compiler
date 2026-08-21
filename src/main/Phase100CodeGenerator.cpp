// Phase 100.5: Link-Time Code Generation Implementation
#include "Phase100CodeGenerator.hpp"
#include <sstream>
#include <algorithm>

namespace phase100 {

std::string Phase100CodeGenerator::generateOptimizedCode(const CoordinationPlan& plan) {
    std::stringstream code;
    appliedHintsCount_ = 0;
    totalGeneratedSize_ = 0;
    codeReductionAchieved_ = 0;

    // Generate code for each hint in application order
    for (const auto& hint : plan.hintsToApply) {
        std::string hintCode;

        if (hint->phase == "91") {
            // IPO hint
            hintCode = applySpecializationCode(hint);
        } else if (hint->phase == "96.5") {
            // Field caching hint
            hintCode = applyCachingCode(hint);
        } else if (hint->phase == "99") {
            // Bank hoisting hint
            hintCode = applyBankHoistingCode(hint);
        }

        if (!hintCode.empty()) {
            code << formatDebugComment(hint, "apply");
            code << hintCode << "\n";
            appliedHintsCount_++;
            totalGeneratedSize_ += hintCode.size();
            codeReductionAchieved_ += hint->estimatedBenefit;
        }
    }

    // Add summary comment
    code << "; Phase 100: Applied " << appliedHintsCount_ << " coordinated hints\n";
    code << "; Estimated benefit: " << codeReductionAchieved_ << " bytes\n";

    return code.str();
}

std::string Phase100CodeGenerator::applySpecializationCode(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::stringstream code;

    // IPO specialization code
    // Check for constant pattern and dispatch to specialized variant
    if (hint->hintType == "inline") {
        code << generateInlineCode(hint);
    } else if (hint->hintType == "specialization") {
        code << generateSpecializationSelector({hint});
    } else if (hint->hintType == "dead_code_elimination") {
        // DCE is handled at linker level - no assembly needed
        code << "; Dead code eliminated: " << hint->targetFunction << "\n";
    }

    return code.str();
}

std::string Phase100CodeGenerator::applyCachingCode(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::stringstream code;

    // Field caching code - generate cache check and setup
    if (hint->hintType == "field_cache_setup") {
        code << generateCacheCheckSequence(hint);
    } else if (hint->hintType == "global_variable_cache") {
        // Global variable caching - setup pointer in ZP
        code << "; Cache global: " << hint->targetVariable << "\n";
        code << "lda #<" << hint->targetVariable << "\n";
        code << "sta $20\n";
        code << "lda #>" << hint->targetVariable << "\n";
        code << "sta $21\n";
    }

    return code.str();
}

std::string Phase100CodeGenerator::applyBankHoistingCode(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::stringstream code;

    // Bank setup hoisting code
    if (hint->hintType == "bank_setup") {
        code << generateBankSetupSequence(hint);
    } else if (hint->hintType == "co_location") {
        // Co-location is handled at linker level - emit informational comment
        code << "; Co-locate with: " << hint->targetVariable << "\n";
    } else if (hint->hintType == "loop_bank_hoist") {
        // Setup hoisting for loops - emit before loop
        code << "; Bank setup hoisted before loop\n";
        code << "lda #$01\n";
        code << "sta $FFD30000\n";  // MEGA65 bank register
    }

    return code.str();
}

std::string Phase100CodeGenerator::generateDispatchCode(
    const std::vector<std::shared_ptr<OptimizationHint>>& hints) {
    std::stringstream code;

    if (hints.empty()) {
        return "";
    }

    // Generate dispatcher for multiple specializations
    code << "; IPO Specialization Dispatcher\n";

    for (size_t i = 0; i < hints.size(); ++i) {
        const auto& hint = hints[i];
        std::string variantLabel = hint->targetFunction + "_variant_" + std::to_string(i);

        code << "cmp #" << i << "\n";
        code << "beq @" << variantLabel << "\n";
    }

    code << "jmp " << hints[0]->targetFunction << "_default\n";

    return code.str();
}

std::string Phase100CodeGenerator::emitWithComments(
    const std::string& code,
    const std::vector<std::shared_ptr<OptimizationHint>>& appliedHints) {
    std::stringstream result;

    // Emit hint application header
    result << "; ============================================\n";
    result << "; Phase 100: Link-Time Optimizations Applied\n";
    result << "; ============================================\n";

    // List applied hints
    for (const auto& hint : appliedHints) {
        result << "; Hint: " << hint->phase << " - " << hint->hintType;
        if (!hint->targetFunction.empty()) {
            result << " (func: " << hint->targetFunction << ")";
        }
        if (!hint->targetLoop.empty()) {
            result << " (loop: " << hint->targetLoop << ")";
        }
        result << " [benefit: " << hint->estimatedBenefit << "]\n";
    }

    result << "; ============================================\n";
    result << code;

    return result.str();
}

std::string Phase100CodeGenerator::generateCacheCheckSequence(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::stringstream code;

    // Cache check pattern: LDA cached_value; CMP current; BEQ skip_setup
    code << "; Cache check for: " << hint->targetVariable << "\n";
    code << "lda $20                   ; Load cached value\n";
    code << "cmp " << hint->targetVariable << "\n";
    code << "beq @cache_hit\n";

    // Cache miss - perform setup
    code << "; Cache miss - setup required\n";
    code << "lda " << hint->targetVariable << "\n";
    code << "sta $20                   ; Update cache\n";

    code << "@cache_hit:\n";
    code << "; Cache hit or updated - continue\n";

    return code.str();
}

std::string Phase100CodeGenerator::generateBankSetupSequence(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::stringstream code;

    // Bank setup for MEGA65 extended memory
    code << "; Bank setup for variable group\n";

    // Determine bank number from hint properties
    std::string bankNum = "01";  // Default to bank 1
    auto bankIt = hint->properties.find("bank_number");
    if (bankIt != hint->properties.end()) {
        bankNum = bankIt->second;
    }

    code << "lda #$" << bankNum << "\n";
    code << "sta $FFD30000            ; Set MEGA65 bank\n";

    return code.str();
}

std::string Phase100CodeGenerator::formatDebugComment(
    const std::shared_ptr<OptimizationHint>& hint,
    const std::string& action) {
    std::stringstream comment;

    comment << "; [Phase " << hint->phase << "] ";
    comment << action << " " << hint->hintType;

    if (!hint->targetFunction.empty()) {
        comment << " on " << hint->targetFunction;
    }
    if (!hint->targetLoop.empty()) {
        comment << " in loop " << hint->targetLoop;
    }

    comment << " (benefit: " << hint->estimatedBenefit << ")\n";

    return comment.str();
}

std::string Phase100CodeGenerator::generateSpecializationSelector(
    const std::vector<std::shared_ptr<OptimizationHint>>& specs) {
    std::stringstream code;

    if (specs.empty()) {
        return "";
    }

    // Generate comparison tree for specialization selector
    code << "; IPO Specialization Selector\n";
    code << "ldx #<arg_constant\n";
    code << "ldy #>arg_constant\n";

    for (size_t i = 0; i < specs.size(); ++i) {
        const auto& spec = specs[i];
        std::string variantLabel = spec->targetFunction + "_spec" + std::to_string(i);

        code << "cmp #$" << std::hex << (i * 16) << std::dec << "\n";
        code << "beq @" << variantLabel << "\n";
    }

    code << "jmp " << specs[0]->targetFunction << "_generic\n";

    return code.str();
}

std::string Phase100CodeGenerator::generateInlineCode(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::stringstream code;

    // Generate inlined function body
    code << "; Inlined: " << hint->targetFunction << "\n";
    code << "lda #0x00              ; Function body placeholder\n";
    code << "; (Actual function body would be inserted here)\n";

    return code.str();
}

std::string Phase100CodeGenerator::generateCallSiteAdapter(
    const std::shared_ptr<OptimizationHint>& hint) {
    std::stringstream code;

    // Generate adapter for cross-convention or specialized call
    code << "; Call site adapter for: " << hint->targetFunction << "\n";
    code << "jsr " << hint->targetFunction << "_optimized\n";

    return code.str();
}

} // namespace phase100
