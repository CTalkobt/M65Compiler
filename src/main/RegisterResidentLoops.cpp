#include "RegisterResidentLoops.hpp"
#include <algorithm>

RegisterResidentLoops::RegisterResidentLoops()
    : OptimizationPassBase(OptimizationType::REGISTER_RESIDENT_LOOPS,
                          "Register-Resident Loop Counters"),
      loopsOptimized_(0), bytesReduced_(0) {
}

RegisterResidentLoops::~RegisterResidentLoops() = default;

void RegisterResidentLoops::apply(TranslationUnit& ast) {
    // AST-level loop detection and analysis
    // Detect simple for-loops with 8-bit counters
    detectLoopCandidates(ast);

    // Filter candidates based on allocation constraints
    candidateLoops_.erase(
        std::remove_if(candidateLoops_.begin(), candidateLoops_.end(),
            [this](const LoopCandidate& loop) {
                return !isEightBitLoop(loop) || !isSimpleLoop(loop);
            }),
        candidateLoops_.end()
    );

    // Apply X register allocation
    allocateToXRegister(ast);

    // Update metrics
    for (const auto& loop : candidateLoops_) {
        loopsOptimized_++;
        bytesReduced_ += loop.bytesReduced;
    }

    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = loopsOptimized_;
}

void RegisterResidentLoops::apply(ir::Module& /* irModule */) {
    // IR-level loop counter allocation
    // TODO: Detect FOR_LOOP IR nodes with 8-bit counter vreg
    // TODO: Allocate counter directly to X register (virtual reg mapping)
    // TODO: Generate optimized increment/decrement instructions
}

void RegisterResidentLoops::detectLoopCandidates(TranslationUnit& /* ast */) {
    // Pattern detection: find simple for-loops
    // TODO: Walk AST for ForLoop nodes with:
    //   - Initial value (constant or zero)
    //   - Increment/decrement by 1
    //   - Bound check (i < N, i <= N, etc.)
    //   - Bound is constant and <= 256
    // Create LoopCandidate records
}

void RegisterResidentLoops::detectLoopCandidatesIR(ir::Module& /* irModule */) {
    // IR-level loop detection
    // TODO: Walk IR for FOR_LOOP ops with constant bounds
}

bool RegisterResidentLoops::isEightBitLoop(const LoopCandidate& candidate) const {
    // Check if loop counter fits in 8-bit (0-255)
    return candidate.maxValue > 0 && candidate.maxValue <= 256;
}

bool RegisterResidentLoops::hasXRegisterConflict(
    const LoopCandidate& /* candidate */) const {
    // Check if X register is used elsewhere in the loop body
    // TODO: Analyze loop body for other X register uses
    // Return true if conflict found
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
    // AST transformation: allocate loop counters to X register
    // TODO: For each candidate loop:
    //   - Replace stack frame references to counter with X register
    //   - Generate INX/DEX instructions instead of LDA/ADD/STA
    //   - Update CPX #limit comparisons
}

void RegisterResidentLoops::allocateToXRegisterIR(ir::Module& /* irModule */) {
    // IR transformation: mark loop counter vreg as X-resident
    // TODO: Set vreg allocation hint to X register
}

int RegisterResidentLoops::estimateByteSavings(const LoopCandidate& /* candidate */) const {
    // Estimate bytes saved per loop iteration:
    // Normal: LDA frame, ADD #1, STA frame = ~7 bytes + address overhead
    // X-register: INX = 1 byte
    // Savings per iteration: ~6+ bytes
    // For N iterations: ~6N bytes, but can merge with loop body
    // Conservative estimate: 5-10 bytes per loop depending on iterations
    return 8;
}
