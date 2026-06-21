#include "IR.hpp"
#include <iomanip>

namespace ir {

const char* opName(Op op) {
    switch (op) {
        case Op::CONST:        return "const";
        case Op::ADD:          return "add";
        case Op::SUB:          return "sub";
        case Op::MUL:          return "mul";
        case Op::MUL_U:        return "mul_u";
        case Op::DIV:          return "div";
        case Op::DIV_U:        return "div_u";
        case Op::MOD:          return "mod";
        case Op::MOD_U:        return "mod_u";
        case Op::AND:          return "and";
        case Op::OR:           return "or";
        case Op::XOR:          return "xor";
        case Op::LSL:          return "lsl";
        case Op::LSR:          return "lsr";
        case Op::ASR:          return "asr";
        case Op::SHL:          return "shl";
        case Op::SHR:          return "shr";
        case Op::NEG:          return "neg";
        case Op::NOT:          return "not";
        case Op::CMP_EQ:       return "cmp_eq";
        case Op::CMP_NE:       return "cmp_ne";
        case Op::CMP_LT:       return "cmp_lt";
        case Op::CMP_LE:       return "cmp_le";
        case Op::CMP_GT:       return "cmp_gt";
        case Op::CMP_GE:       return "cmp_ge";
        case Op::CMP_LTU:      return "cmp_ltu";
        case Op::CMP_LEU:      return "cmp_leu";
        case Op::CMP_GTU:      return "cmp_gtu";
        case Op::CMP_GEU:      return "cmp_geu";
        case Op::LOAD:         return "load";
        case Op::STORE:        return "store";
        case Op::LOAD_ZP:      return "load_zp";
        case Op::STORE_ZP:     return "store_zp";
        case Op::ADDR_GLOBAL:  return "addr_global";
        case Op::ADDR_LOCAL:   return "addr_local";
        case Op::ADDR_LABEL:   return "addr_label";
        case Op::ADDR_UPLEVEL: return "addr_uplevel";
        case Op::ADDR_ELEM:    return "addr_elem";
        case Op::SEXT:         return "sext";
        case Op::ZEXT:         return "zext";
        case Op::TRUNC:        return "trunc";
        case Op::BFEXT:        return "bfext";
        case Op::BFINS:        return "bfins";
        case Op::COPY:         return "copy";
        case Op::BR:           return "br";
        case Op::BR_COND:      return "br_cond";
        case Op::BR_INDIRECT:  return "br_indirect";
        case Op::SWITCH:       return "switch";
        case Op::RET:          return "ret";
        case Op::RET_VOID:     return "ret";
        case Op::CALL:         return "call";
        case Op::CALL_INDIRECT: return "call_indirect";
        case Op::CALL_VOID:    return "call_void";
        case Op::CPU_REG_READ: return "cpu_reg_read";
        case Op::CPU_REG_WRITE: return "cpu_reg_write";
        case Op::CPU_FLAG_READ: return "cpu_flag_read";
        case Op::CPU_FLAG_WRITE: return "cpu_flag_write";
        case Op::ASM_INLINE:   return "asm";
        case Op::VA_START:     return "va_start";
        case Op::TRAMPOLINE:   return "trampoline";
        case Op::PHI:          return "phi";
        case Op::GET_FP:       return "get_fp";
        case Op::NOP:          return "nop";
    }
    return "unknown";
}

const char* typeName(Type t) {
    switch (t) {
        case Type::VOID: return "void";
        case Type::I8:   return "i8";
        case Type::I16:  return "i16";
        case Type::I32:  return "i32";
        case Type::PTR:  return "ptr";
    }
    return "?";
}

std::string Printer::operandStr(const Operand& op) {
    switch (op.kind) {
        case OperandKind::NONE:   return "_";
        case OperandKind::VREG:   return "%" + std::to_string(op.vregId);
        case OperandKind::IMM:    return std::to_string(op.immVal);
        case OperandKind::GLOBAL: return "@" + op.name;
        case OperandKind::LABEL:  return "@" + op.name;
        case OperandKind::FRAME_RELATIVE: return "fp+" + std::to_string(op.immVal);
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

        case Op::BR_INDIRECT:
            out << "br_indirect " << operandStr(inst.src1) << locComment << "\n";
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

        case Op::CALL_INDIRECT:
            out << operandStr(inst.dest) << " = call_indirect " << operandStr(inst.src1) << "(";
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

        case Op::ADDR_GLOBAL:
        case Op::ADDR_LOCAL:
        case Op::ADDR_LABEL:
            out << operandStr(inst.dest) << " = " << opName(inst.op) << " " << operandStr(inst.src1) << locComment << "\n";
            return;

        case Op::ADDR_UPLEVEL:
            out << operandStr(inst.dest) << " = addr_uplevel levels=" << operandStr(inst.args[1]) 
                << ", target=" << inst.args[0].name << "(v" << inst.args[0].vregId << ")" << locComment << "\n";
            return;

        case Op::ADDR_ELEM:
            out << operandStr(inst.dest) << " = addr_elem " << operandStr(inst.src1)
                << ", " << operandStr(inst.src2) << ", " << operandStr(inst.args[0]) << locComment << "\n";
            return;

        case Op::GET_FP:
            out << operandStr(inst.dest) << " = get_fp" << locComment << "\n";
            return;

        case Op::LOAD_ZP:
            out << operandStr(inst.dest) << " = load_zp " << operandStr(inst.src1) << locComment << "\n";
            return;

        case Op::TRAMPOLINE:
            out << operandStr(inst.dest) << " = trampoline target=" << operandStr(inst.src1)
                << ", link=" << operandStr(inst.src2) << ", buf=" << operandStr(inst.args[0]) << locComment << "\n";
            return;

        case Op::PHI:
            out << operandStr(inst.dest) << " = phi ";
            for (size_t i = 0; i < inst.phiIncoming.size(); i++) {
                if (i > 0) out << ", ";
                out << "[" << operandStr(inst.phiIncoming[i].first) << ", " << inst.phiIncoming[i].second << "]";
            }
            out << locComment << "\n";
            return;

        default:
            if (inst.dest.isVreg()) {
                out << operandStr(inst.dest) << " = ";
            }
            out << opName(inst.op) << " " << typeName(inst.resultType) << " "
                << operandStr(inst.src1);
            if (!inst.src2.isNone()) {
                out << ", " << operandStr(inst.src2);
            }
            out << locComment << "\n";
            return;
    }
}

void Printer::print(std::ostream& out, const Function& fn) {
    out << "fn " << fn.name << "(";
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        if (i > 0) out << ", ";
        out << typeName(fn.paramTypes[i]) << " " << fn.paramNames[i];
    }
    out << ") -> " << typeName(fn.returnType);
    if (fn.conv == CallConv::ZP) out << " [fastcall]";
    out << " {\n";

    for (const auto& block : fn.blocks) {
        out << "@" << block.label << ":\n";
        for (const auto& inst : block.insts) {
            print(out, inst);
        }
    }
    out << "}\n\n";
}

void Printer::print(std::ostream& out, const Module& mod) {
    out << "; Module: " << mod.sourceFile << "\n\n";
    for (const auto& fn : mod.functions) {
        print(out, fn);
    }
}

} // namespace ir
