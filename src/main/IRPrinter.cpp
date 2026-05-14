#include "IR.hpp"
#include <iomanip>
#include <sstream>

namespace ir {

static const char* opName(Op op) {
    switch (op) {
        case Op::CONST:        return "const";
        case Op::ADD:          return "add";
        case Op::SUB:          return "sub";
        case Op::MUL:          return "mul";
        case Op::DIV:          return "div";
        case Op::MOD:          return "mod";
        case Op::NEG:          return "neg";
        case Op::AND:          return "and";
        case Op::OR:           return "or";
        case Op::XOR:          return "xor";
        case Op::NOT:          return "not";
        case Op::SHL:          return "shl";
        case Op::SHR:          return "shr";
        case Op::ASR:          return "asr";
        case Op::CMP_EQ:      return "cmp_eq";
        case Op::CMP_NE:      return "cmp_ne";
        case Op::CMP_LT:      return "cmp_lt";
        case Op::CMP_LE:      return "cmp_le";
        case Op::CMP_GT:      return "cmp_gt";
        case Op::CMP_GE:      return "cmp_ge";
        case Op::CMP_LTU:     return "cmp_ltu";
        case Op::CMP_LEU:     return "cmp_leu";
        case Op::CMP_GTU:     return "cmp_gtu";
        case Op::CMP_GEU:     return "cmp_geu";
        case Op::LOAD:         return "load";
        case Op::STORE:        return "store";
        case Op::LOAD_ZP:      return "load_zp";
        case Op::STORE_ZP:     return "store_zp";
        case Op::ADDR_GLOBAL:  return "addr_global";
        case Op::ADDR_LOCAL:   return "addr_local";
        case Op::ADDR_ELEM:    return "addr_elem";
        case Op::SEXT:         return "sext";
        case Op::ZEXT:         return "zext";
        case Op::TRUNC:        return "trunc";
        case Op::BFEXT:        return "bfext";
        case Op::BFINS:        return "bfins";
        case Op::BR:           return "br";
        case Op::BR_COND:      return "br_cond";
        case Op::SWITCH:       return "switch";
        case Op::RET:          return "ret";
        case Op::RET_VOID:     return "ret";
        case Op::CALL:         return "call";
        case Op::CALL_INDIRECT: return "call_indirect";
        case Op::CALL_VOID:    return "call_void";
        case Op::ASM_INLINE:   return "asm";
        case Op::PHI:          return "phi";
        case Op::NOP:          return "nop";
    }
    return "???";
}

std::string Printer::operandStr(const Operand& op) {
    switch (op.kind) {
        case OperandKind::NONE:   return "_";
        case OperandKind::VREG:   return "%" + std::to_string(op.vregId);
        case OperandKind::IMM:    return std::to_string(op.immVal);
        case OperandKind::GLOBAL: return "@" + op.name;
        case OperandKind::LABEL:  return "." + op.name;
    }
    return "?";
}

void Printer::print(std::ostream& out, const Inst& inst) {
    out << "  ";

    // Source location comment
    std::string locComment;
    if (inst.loc.valid()) {
        locComment = " ; " + inst.loc.file + ":" + std::to_string(inst.loc.line);
    }

    switch (inst.op) {
        case Op::CONST:
            out << operandStr(inst.dest) << " = const " << typeName(inst.resultType)
                << " " << inst.src1.immVal << locComment << "\n";
            return;

        case Op::BR:
            out << "br " << operandStr(inst.src1) << locComment << "\n";
            return;

        case Op::BR_COND:
            out << "br_cond " << operandStr(inst.src1) << ", "
                << operandStr(inst.dest) << ", " << operandStr(inst.src2) << locComment << "\n";
            return;

        case Op::RET:
            out << "ret " << operandStr(inst.src1) << locComment << "\n";
            return;

        case Op::RET_VOID:
            out << "ret void" << locComment << "\n";
            return;

        case Op::STORE:
        case Op::STORE_ZP:
            out << opName(inst.op) << " " << typeName(inst.resultType) << " "
                << operandStr(inst.src1) << ", " << operandStr(inst.src2) << locComment << "\n";
            return;

        case Op::CALL:
            out << operandStr(inst.dest) << " = call " << operandStr(inst.src1) << "(";
            for (size_t i = 0; i < inst.args.size(); i++) {
                if (i > 0) out << ", ";
                out << operandStr(inst.args[i]);
            }
            out << ") -> " << typeName(inst.resultType) << locComment << "\n";
            return;

        case Op::CALL_VOID:
            out << "call_void " << operandStr(inst.src1) << "(";
            for (size_t i = 0; i < inst.args.size(); i++) {
                if (i > 0) out << ", ";
                out << operandStr(inst.args[i]);
            }
            out << ")" << locComment << "\n";
            return;

        case Op::CALL_INDIRECT:
            out << operandStr(inst.dest) << " = call_indirect " << operandStr(inst.src1) << "(";
            for (size_t i = 0; i < inst.args.size(); i++) {
                if (i > 0) out << ", ";
                out << operandStr(inst.args[i]);
            }
            out << ") -> " << typeName(inst.resultType) << locComment << "\n";
            return;

        case Op::BFEXT:
            // %d = bfext type src, bitOffset, bitWidth
            out << operandStr(inst.dest) << " = bfext " << typeName(inst.resultType)
                << " " << operandStr(inst.src1);
            if (inst.args.size() >= 2) {
                out << ", " << inst.args[0].immVal << ", " << inst.args[1].immVal;
            }
            out << locComment << "\n";
            return;

        case Op::BFINS:
            // bfins type val, addr, bitOffset, bitWidth
            out << "bfins " << typeName(inst.resultType)
                << " " << operandStr(inst.src1) << ", " << operandStr(inst.src2);
            if (inst.args.size() >= 2) {
                out << ", " << inst.args[0].immVal << ", " << inst.args[1].immVal;
            }
            out << locComment << "\n";
            return;

        case Op::ASM_INLINE:
            out << "asm \"" << inst.asmText << "\"" << locComment << "\n";
            return;

        case Op::SWITCH:
            out << "switch " << operandStr(inst.src1) << ", " << operandStr(inst.src2) << ", [";
            for (size_t i = 0; i < inst.switchCases.size(); i++) {
                if (i > 0) out << ", ";
                out << inst.switchCases[i].first << ": ." << inst.switchCases[i].second;
            }
            out << "]" << locComment << "\n";
            return;

        case Op::PHI:
            out << operandStr(inst.dest) << " = phi ";
            for (size_t i = 0; i < inst.phiIncoming.size(); i++) {
                if (i > 0) out << ", ";
                out << "[" << operandStr(inst.phiIncoming[i].first) << ", ." << inst.phiIncoming[i].second << "]";
            }
            out << locComment << "\n";
            return;

        case Op::SEXT:
        case Op::ZEXT:
        case Op::TRUNC:
            out << operandStr(inst.dest) << " = " << opName(inst.op) << " "
                << typeName(inst.src1.type) << " " << operandStr(inst.src1)
                << " to " << typeName(inst.resultType) << locComment << "\n";
            return;

        case Op::NOP:
            out << "nop" << locComment << "\n";
            return;

        default:
            break;
    }

    // Generic binary/unary format: %d = op type %s1, %s2
    if (!inst.dest.isNone()) {
        out << operandStr(inst.dest) << " = ";
    }
    out << opName(inst.op) << " " << typeName(inst.resultType);
    if (!inst.src1.isNone()) {
        out << " " << operandStr(inst.src1);
    }
    if (!inst.src2.isNone()) {
        out << ", " << operandStr(inst.src2);
    }
    out << locComment << "\n";
}

void Printer::print(std::ostream& out, const Function& fn) {
    out << "func @" << fn.name << "(";
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        if (i > 0) out << ", ";
        out << typeName(fn.paramTypes[i]);
        if (i < fn.paramNames.size() && !fn.paramNames[i].empty()) {
            out << " %" << fn.paramNames[i];
        }
    }
    if (fn.isVariadic) {
        if (!fn.paramTypes.empty()) out << ", ";
        out << "...";
    }
    out << ") -> " << typeName(fn.returnType);
    if (fn.conv == CallConv::ZP) out << " conv=zp";
    out << " {\n";

    for (const auto& block : fn.blocks) {
        out << block.label << ":\n";
        for (const auto& inst : block.insts) {
            print(out, inst);
        }
    }
    out << "}\n";
}

void Printer::print(std::ostream& out, const Module& mod) {
    if (!mod.sourceFile.empty()) {
        out << "; source: " << mod.sourceFile << "\n";
    }
    out << "\n";

    for (const auto& g : mod.globals) {
        out << "global @" << g.name << " : " << typeName(g.type);
        if (g.size > typeSize(g.type)) {
            out << "[" << (g.size / std::max(1, typeSize(g.type))) << "]";
        }
        if (g.isConst) out << " const";
        if (g.isStatic) out << " static";
        out << "\n";
    }
    if (!mod.globals.empty()) out << "\n";

    for (const auto& fn : mod.functions) {
        print(out, fn);
        out << "\n";
    }
}

} // namespace ir
