#include "TableDrivenDispatch.hpp"
#include "IR.hpp"
#include <algorithm>
#include <cmath>
#include <map>

TableDrivenDispatch::TableDrivenDispatch()
    : OptimizationPassBase(OptimizationType::TABLE_DRIVEN_DISPATCH,
                          "Table-Driven Dispatch"),
      switchesOptimized_(0), bytesReduced_(0) {
}

TableDrivenDispatch::~TableDrivenDispatch() = default;

void TableDrivenDispatch::apply(TranslationUnit& /* ast */) {
    // AST-level switch statement analysis deferred to IR phase
    // IR representation provides better structure for jump table generation
}

void TableDrivenDispatch::apply(ir::Module& irModule) {
    // IR-level switch table generation (Phase C5.3)
    // Walk SWITCH ops and identify candidates for table-driven dispatch
    // Dense switches (10+ cases, 60%+ fill) benefit from jump tables

    for (auto& func : irModule.functions) {
        analyzeSwitchStatementsIR(irModule);

        for (auto& block : func.blocks) {
            // Track which instructions to replace
            std::vector<std::pair<size_t, ir::Inst>> replacements;

            for (size_t i = 0; i < block.insts.size(); ++i) {
                auto& inst = block.insts[i];

                // Look for SWITCH instructions
                if (inst.op != ir::Op::SWITCH) {
                    continue;
                }

                // Extract case information
                SwitchAnalysis analysis;
                analysis.switchVarName = "switch_var";  // Placeholder; actual var from inst

                if (inst.switchCases.empty()) {
                    continue;
                }

                // Analyze cases (Phase C5.3)
                analysis.caseCount = inst.switchCases.size();
                analysis.minCase = static_cast<int>(inst.switchCases[0].first);
                analysis.maxCase = static_cast<int>(inst.switchCases[0].first);

                for (const auto& [caseVal, label] : inst.switchCases) {
                    int val = static_cast<int>(caseVal);
                    analysis.minCase = std::min(analysis.minCase, val);
                    analysis.maxCase = std::max(analysis.maxCase, val);
                }

                // Store case information for buildJumpTable
                // Note: Default label from inst.src2.name would be used by buildJumpTable

                // Check if this switch should use table-driven dispatch
                if (!shouldUseTable(analysis)) {
                    continue;
                }

                // Build jump table
                auto jumpTable = buildJumpTable(analysis);
                if (jumpTable.empty()) {
                    continue;
                }

                // Create table label for data section
                std::string tableLabel = "__switch_table_" + std::to_string(i);

                // Record metrics
                switchesOptimized_++;
                bytesReduced_ += estimateTableBenefit(analysis);

                // Note: Actual code generation for the table lookup and indirect jump
                // would be handled in CodeGenerator phase. Here we mark the switch
                // as optimizable and attach metadata.
                //
                // For Phase C5.3, we track that the switch was analyzed and
                // a table would be generated, enabling the backend to emit the
                // table and optimized dispatch code.

                inst.asmText = tableLabel;  // Store table label for backend
            }
        }
    }

    metrics_.codeReductionBytes = bytesReduced_;
    metrics_.instructionsOptimized = switchesOptimized_;
}

void TableDrivenDispatch::analyzeSwitchStatements(TranslationUnit& /* ast */) {
    // AST-level switch analysis deferred to IR phase
    // Pattern detection framework provides:
    // - Switch statement identification
    // - Case extraction
    // - Range and density analysis
}

void TableDrivenDispatch::analyzeSwitchStatementsIR(ir::Module& /* irModule */) {
    // IR-level switch analysis
    // Deferred: Walk IR for SWITCH ops and extract case information
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
    // Deferred: Implementation would transform SwitchStatement nodes to use table lookup
}

void TableDrivenDispatch::generateJumpTablesIR(ir::Module& /* irModule */) {
    // IR transformation: generate table data and dispatch
    // Framework provides: density analysis, benefit estimation, table structure
    // Deferred: Implementation would generate SWITCH → TABLE_LOOKUP + INDIRECT_JMP
}

std::vector<TableDrivenDispatch::JumpTableEntry>
TableDrivenDispatch::buildJumpTable(const SwitchAnalysis& analysis) {
    std::vector<JumpTableEntry> table;

    // Build jump table array from minCase to maxCase
    // Each entry is either a case label or the default label (Phase C5.3)
    //
    // Note: In Phase C5.3, this is a framework function.
    // Actual case-to-label mapping would come from the SWITCH instruction
    // in the apply() method and passed via a helper structure.

    // For now, create placeholder entries
    // (Actual labels would be populated from SWITCH instruction data)
    for (int i = analysis.minCase; i <= analysis.maxCase; ++i) {
        JumpTableEntry entry;
        entry.caseValue = i;
        entry.targetLabel = "@default";  // Placeholder; actual labels from SWITCH
        table.push_back(entry);
    }

    return table;
}

int TableDrivenDispatch::calculateTableSize(int minCase, int maxCase) const {
    // Calculate jump table size in bytes
    // Each entry is 2 bytes (word address for 6502 JMP indirect)
    return (maxCase - minCase + 1) * 2;
}
