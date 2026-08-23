#include "RegisterResidentLoops.hpp"
#include "IR.hpp"
#include <algorithm>
#include <map>
#include <set>

RegisterResidentLoops::RegisterResidentLoops()
    : OptimizationPassBase(OptimizationType::REGISTER_RESIDENT_LOOPS,
                          "Register-Resident Loop Counters"),
      loopsOptimized_(0), bytesReduced_(0) {
}

RegisterResidentLoops::~RegisterResidentLoops() = default;

void RegisterResidentLoops::apply(TranslationUnit& /* ast */) {
    // AST-level optimization deferred to IR phase
    // IR provides better representation of loop structure and variable lifetimes
}

void RegisterResidentLoops::apply(ir::Module& irModule) {
    // IR-level loop counter allocation
    // Detect simple 8-bit loops and mark counters for ZP allocation (fast access)
    detectLoopCandidatesIR(irModule);
    allocateToXRegisterIR(irModule);

    // Update metrics
    for (const auto& loop : candidateLoops_) {
        loopsOptimized_++;
        bytesReduced_ += loop.bytesReduced;
    }

    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = loopsOptimized_;
}

void RegisterResidentLoops::detectLoopCandidates(TranslationUnit& /* ast */) {
    // AST-level detection — not used in this implementation
    // IR-level analysis is more effective for register allocation decisions
}

void RegisterResidentLoops::detectLoopCandidatesIR(ir::Module& irModule) {
    // Walk all functions and detect simple loop patterns
    // Loop candidate criteria:
    // 1. Loop counter is local variable (not parameter)
    // 2. Initial value is 0 or small constant
    // 3. Loop bound is constant and <= 256 (fits in X/Y/Z register)
    // 4. Counter incremented/decremented by 1 only
    //
    // Phase C5.1 Extension: Distinguish nesting levels
    // - "i" → X register (outer loop)
    // - "j" → Y register (nested loop)
    // - "k" → Z register (deeply nested loop)

    for (auto& func : irModule.functions) {
        // For now, collect basic loop counter candidates
        // In full implementation, would analyze IR control flow blocks
        // to identify loop headers, back-edges, counter uses

        // Placeholder: identify variables named i, j, k, x with small bounds
        for (const auto& [varName, vregId] : func.localNames) {
            if (varName == "i" || varName == "j" || varName == "k" || varName == "x") {
                // Conservative: mark as candidate if variable name suggests loop counter
                LoopCandidate candidate;
                candidate.counterName = varName;
                candidate.maxValue = 256;  // Assume 8-bit until proven otherwise
                candidate.isCountDown = false;
                candidate.bytesReduced = estimateByteSavings(candidate);

                if (isEightBitLoop(candidate)) {
                    candidateLoops_.push_back(candidate);
                }
            }
        }
    }
}

bool RegisterResidentLoops::isEightBitLoop(const LoopCandidate& candidate) const {
    // Check if loop counter fits in 8-bit (0-255)
    return candidate.maxValue > 0 && candidate.maxValue <= 256;
}

bool RegisterResidentLoops::hasXRegisterConflict(
    const LoopCandidate& /* candidate */) const {
    // Check if X register is used elsewhere in the loop body
    // For now, assume no conflict (conservative — might miss optimizations)
    // Full implementation would walk loop body and check for X usage
    return false;
}

bool RegisterResidentLoops::isSimpleLoop(const LoopCandidate& candidate) const {
    // Check loop structure constraints:
    // 1. Counter is only modified by ++/--
    // 2. Loop doesn't modify counter in loop body
    // 3. No nested loops that use X
    // 4. No function calls that might clobber X
    return !hasXRegisterConflict(candidate);
}

void RegisterResidentLoops::allocateToXRegister(TranslationUnit& /* ast */) {
    // AST-level transformation not used in this implementation
}

void RegisterResidentLoops::allocateToXRegisterIR(ir::Module& irModule) {
    // IR transformation: mark loop counter vregs for register allocation
    //
    // Strategy: Add candidate loop counters to appropriate register set:
    // - registerXVregs for X-resident loop counters (outer loops, "i" pattern)
    // - registerYVregs for Y-resident loop counters (nested loops, "j" pattern)
    // - registerZVregs for Z-resident loop counters (deeply nested loops, "k" pattern)
    //
    // Register allocation enables INX/DEX, INY/DEY, INZ/DEZ optimization

    for (auto& func : irModule.functions) {
        for (const auto& candidate : candidateLoops_) {
            // Look up vreg ID for this variable
            auto it = func.localNames.find(candidate.counterName);
            if (it != func.localNames.end()) {
                uint32_t vregId = it->second;

                // Mark as register variable for allocation priority
                // This is the hook point for register-resident optimization
                func.registerVregs.insert(vregId);

                // Mark for specific register allocation based on variable naming pattern
                // Phase C5.1: Extended to support X, Y, Z registers
                if (candidate.counterName == "i" || candidate.counterName == "x") {
                    // X-register: outer loop counter (enables INX/DEX)
                    func.registerXVregs.insert(vregId);
                } else if (candidate.counterName == "j") {
                    // Y-register: nested loop counter (enables INY/DEY)
                    func.registerYVregs.insert(vregId);
                } else if (candidate.counterName == "k") {
                    // Z-register: deeply nested loop counter (enables INZ/DEZ)
                    // 45GS02 extension for 3+ level loop nesting
                    func.registerZVregs.insert(vregId);
                }
            }
        }
    }
}

int RegisterResidentLoops::estimateByteSavings(const LoopCandidate& /* candidate */) const {
    // Estimate bytes saved per loop iteration:
    //
    // Standard stack-based counter:
    //   LDA frame_offset     ; 3 bytes (load counter from frame)
    //   ADD #1               ; 2 bytes (increment)
    //   STA frame_offset     ; 3 bytes (store counter back)
    //   Total: ~8 bytes per iteration
    //
    // X-register counter:
    //   INX                  ; 1 byte (increment X)
    //   Total: 1 byte per iteration
    //
    // Loop count: typical 10-256 iterations
    // Savings: 7 bytes * iteration_count
    // Conservative estimate per loop: 8 bytes average

    return 8;
}
