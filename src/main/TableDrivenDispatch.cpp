#include "TableDrivenDispatch.hpp"
#include <algorithm>
#include <cmath>

TableDrivenDispatch::TableDrivenDispatch()
    : switchesOptimized_(0), bytesReduced_(0) {
    metrics_.optimizationName = "Table-Driven Dispatch";
    metrics_.type = OptimizationType::TABLE_DRIVEN_DISPATCH;
}

TableDrivenDispatch::~TableDrivenDispatch() = default;

void TableDrivenDispatch::apply(TranslationUnit& ast) {
    // AST-level switch statement analysis
    analyzeSwitchStatements(ast);

    // Filter to table-worthy switches
    switchCandidates_.erase(
        std::remove_if(switchCandidates_.begin(), switchCandidates_.end(),
            [this](const SwitchAnalysis& analysis) {
                return !shouldUseTable(analysis);
            }),
        switchCandidates_.end()
    );

    // Generate jump tables for qualifying switches
    generateJumpTables(ast);

    // Update metrics
    for (const auto& sw : switchCandidates_) {
        if (sw.isDense) {
            switchesOptimized_++;
            bytesReduced_ += sw.bytesReduced;
        }
    }

    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = switchesOptimized_;
}

void TableDrivenDispatch::apply(ir::Module& /* irModule */) {
    // IR-level switch table generation
    // TODO: Identify SWITCH ops with dense case ranges
    // TODO: Generate jump table data
    // TODO: Replace SWITCH with table lookup + indirect JMP
}

void TableDrivenDispatch::analyzeSwitchStatements(TranslationUnit& /* ast */) {
    // Pattern detection: find switch statements
    // TODO: Walk AST for SwitchStatement nodes
    // For each switch:
    //   - Extract case values
    //   - Calculate min/max/range
    //   - Determine density (gaps vs. cases)
    //   - Create SwitchAnalysis record
}

void TableDrivenDispatch::analyzeSwitchStatementsIR(ir::Module& /* irModule */) {
    // IR-level switch analysis
    // TODO: Walk IR for SWITCH ops
    // TODO: Extract case labels and values
}

bool TableDrivenDispatch::shouldUseTable(const SwitchAnalysis& analysis) const {
    // Decision criteria for table-driven dispatch
    // Requirements:
    //   1. Minimum cases: at least 5 (threshold for tables)
    //   2. Density: at least 60% of range filled
    //   3. Range: not too large (reasonable table size)
    //   4. Benefit: estimated savings justify overhead

    if (analysis.caseCount < 5) return false;  // Too few cases
    if (analysis.maxCase - analysis.minCase > 256) return false;  // Too sparse

    int density = calculateDensity(analysis);
    return density >= 60;  // At least 60% dense
}

int TableDrivenDispatch::calculateDensity(const SwitchAnalysis& analysis) const {
    // Calculate case density as percentage
    // Density = (cases / range) * 100
    int range = analysis.maxCase - analysis.minCase + 1;
    if (range == 0) return 0;
    return (analysis.caseCount * 100) / range;
}

int TableDrivenDispatch::estimateTableBenefit(const SwitchAnalysis& analysis) const {
    // Estimate bytes saved by using jump table
    //
    // Before (cascading comparisons):
    //   cmp #val1, beq @l1 (4 bytes per case)
    //   cmp #val2, beq @l2 (4 bytes per case)
    //   ... repeated for N cases
    // Cost: N * 4 bytes (at least)
    //
    // After (jump table):
    //   cmp #minVal, blt @default (5 bytes for bounds check)
    //   cmp #maxVal, bgt @default (5 bytes)
    //   sec                         (1 byte)
    //   sbc #minVal                 (2 bytes - offset to table base)
    //   asl                         (1 byte - convert to word offset)
    //   jmp (@table, x)            (3 bytes - indirect jump)
    //   .word @case0, @case1, ... (2 bytes per case)
    // Cost: ~20 bytes + (2 * cases)
    //
    // Break-even: N * 4 = 20 + 2N → 2N = 20 → N = 10 cases
    // Benefit becomes positive at ~10+ cases

    int cascadeCost = analysis.caseCount * 4;  // Rough estimate
    int tableSize = 20 + (analysis.caseCount * 2);  // Table overhead
    return cascadeCost - tableSize;
}

void TableDrivenDispatch::generateJumpTables(TranslationUnit& /* ast */) {
    // AST transformation: generate jump tables
    // For each switch to be converted:
    //   1. Build jump table data section
    //   2. Generate bounds checking code
    //   3. Replace cascading comparisons with table lookup
    //   4. Generate indirect jump instruction
    // TODO: Implement table generation and code replacement
}

void TableDrivenDispatch::generateJumpTablesIR(ir::Module& /* irModule */) {
    // IR transformation: generate table data and dispatch
    // TODO: Create table data in IR
    // TODO: Generate lookup and indirect jump
}

std::vector<TableDrivenDispatch::JumpTableEntry>
TableDrivenDispatch::buildJumpTable(const SwitchAnalysis& /* analysis */) {
    std::vector<JumpTableEntry> table;
    // TODO: Build array of entries from minCase to maxCase
    // Entries: {caseValue, targetLabel}
    // For missing cases: fill with default label
    return table;
}

int TableDrivenDispatch::calculateTableSize(int minCase, int maxCase) const {
    // Calculate jump table size in bytes
    // Each entry is 2 bytes (word address for 6502 JMP indirect)
    return (maxCase - minCase + 1) * 2;
}
