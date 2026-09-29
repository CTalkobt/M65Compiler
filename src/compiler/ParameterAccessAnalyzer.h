#pragma once
#include <cstdint>
#include <map>
#include <vector>
#include <string>

// Forward declarations
namespace ir {
class Function;
class Block;
class Instruction;
}

// =============================================================================
// Phase 78: ParameterAccessAnalyzer
//
// Analyzes parameter access patterns in function IR to determine SMC eligibility.
// Counts how many times each parameter is accessed and records the instruction
// offsets where those accesses occur.
//
// Used to decide: Should parameter be embedded in instruction immediates (SMC)
// rather than loaded from AR buffer?
//
// Decision logic:
//   int savings = (accessCount - 1) - 3;  // 3-byte call overhead for SMC
//   useSMC = (savings > 0);
//
// Example:
//   Parameter accessed 5 times:
//   - SAC: lda AR_param (3 bytes) × 5 = 15 bytes
//   - SMC: lda #param (2 bytes) × 5 + call overhead (3 bytes) = 13 bytes
//   - Savings: 2 bytes → use SMC
// =============================================================================

namespace compiler {

struct ParameterAccessInfo {
    int paramID = -1;                    // Parameter index (0, 1, 2, ...)
    uint32_t accessCount = 0;            // Total number of accesses
    std::vector<uint32_t> instructionOffsets;  // Code offsets where accessed
    std::vector<uint8_t> accessSizes;    // Size of each access (1=I8, 2=I16, 4=I32)

    bool shouldUseSMC() const {
        // SMC is beneficial only if we save bytes overall
        // Each access saves: 3 bytes (AR load) - 2 bytes (immediate) = 1 byte
        // Call overhead: 3 bytes (sta to patch immediate)
        // Net savings: accessCount * 1 - 3 = accessCount - 3
        return accessCount > 3;  // Only use if > 3 accesses (save 1+ bytes)
    }

    int estimatedSavings() const {
        if (!shouldUseSMC()) return 0;
        return (int)accessCount - 3;  // 1 byte saved per access minus 3-byte call cost
    }
};

class ParameterAccessAnalyzer {
public:
    // Analyze function IR to determine parameter access patterns
    // Returns map of paramID -> ParameterAccessInfo
    static std::map<int, ParameterAccessInfo> analyze(const ir::Function& fn);

private:
    ParameterAccessAnalyzer() = default;

    // Helper to analyze a block and its instructions
    static void analyzeBlock(
        const ir::Block& block,
        std::map<int, ParameterAccessInfo>& accessMap,
        uint32_t& instructionOffset,
        const std::vector<std::string>& paramNames
    );

    // Helper to check if an instruction accesses a parameter
    // Returns paramID if it does, -1 otherwise
    static int extractParameterAccess(
        const ir::Instruction& instr,
        const std::vector<std::string>& paramNames
    );
};

}  // namespace compiler
