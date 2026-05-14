#include "IRCodeGen.hpp"
#include <iomanip>
#include <sstream>

IRCodeGen::IRCodeGen(std::ostream& out) : out_(out) {}

void IRCodeGen::emit(const std::string& line) {
    out_ << "    " << line << "\n";
}

void IRCodeGen::emitLabel(const std::string& label) {
    out_ << label << ":\n";
}

void IRCodeGen::emitComment(const std::string& text) {
    out_ << "; " << text << "\n";
}

void IRCodeGen::emitBlank() {
    out_ << "\n";
}

// ============================================================================
// Frame management
// ============================================================================

void IRCodeGen::resetFrame() {
    vregOffset_.clear();
    vregType_.clear();
    frameSize_ = 0;
}

int IRCodeGen::allocSlot(uint32_t vregId, ir::Type type) {
    auto it = vregOffset_.find(vregId);
    if (it != vregOffset_.end()) return it->second;
    int size = ir::typeSize(type);
    if (size < 2) size = 2; // minimum 2 bytes per slot for stack alignment
    int offset = frameSize_;
    frameSize_ += size;
    vregOffset_[vregId] = offset;
    vregType_[vregId] = type;
    return offset;
}

int IRCodeGen::slotOf(uint32_t vregId) {
    auto it = vregOffset_.find(vregId);
    if (it != vregOffset_.end()) return it->second;
    return allocSlot(vregId, ir::Type::I16);
}

void IRCodeGen::prescanFunction(const ir::Function& fn) {
    resetFrame();
    // Allocate slots for parameters first
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        allocSlot((uint32_t)i, fn.paramTypes[i]);
    }
    // Scan all instructions for vReg definitions
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block.insts) {
            if (inst.dest.isVreg()) {
                allocSlot(inst.dest.vregId, inst.resultType != ir::Type::VOID ? inst.resultType : ir::Type::I16);
            }
        }
    }
}

// ============================================================================
// Load/store vRegs via frame pointer
// ============================================================================

void IRCodeGen::loadVreg(uint32_t vregId) {
    auto alloc = alloc_.getAlloc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            // Already in A:X — check if it's still there
            if (alloc_.isInAX(vregId, currentInstIdx_)) return; // no-op!
            // Fell through from A:X to somewhere — treat as frame
            if (vregOffset_.count(vregId)) {
                emit("ldax.fp " + std::to_string(vregOffset_[vregId]));
            }
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            std::string zpAddr = ss.str();
            if (alloc.type == ir::Type::I8) {
                emit("lda " + zpAddr);
            } else {
                emit("lda " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (alloc.offset + 1);
                emit("ldx " + ss.str());
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            int off = slotOf(vregId);
            emit("ldax.fp " + std::to_string(off));
            break;
        }
    }
}

void IRCodeGen::storeVreg(uint32_t vregId) {
    auto alloc = alloc_.getAlloc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            // Value stays in A:X — no store needed!
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            std::string zpAddr = ss.str();
            if (alloc.type == ir::Type::I8) {
                emit("sta " + zpAddr);
            } else {
                emit("sta " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (alloc.offset + 1);
                emit("stx " + ss.str());
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            int off = slotOf(vregId);
            emit("stax.fp " + std::to_string(off));
            break;
        }
    }
}

void IRCodeGen::loadOperand(const ir::Operand& op) {
    switch (op.kind) {
        case ir::OperandKind::VREG:
            loadVreg(op.vregId);
            break;
        case ir::OperandKind::IMM:
            emit("lda #" + std::to_string((int)(op.immVal & 0xFF)));
            emit("ldx #" + std::to_string((int)((op.immVal >> 8) & 0xFF)));
            break;
        case ir::OperandKind::GLOBAL:
            emit("ldax #" + op.name);
            break;
        default:
            emit("lda #0");
            emit("ldx #0");
            break;
    }
}

// ============================================================================
// Module-level emission
// ============================================================================

void IRCodeGen::generate(const ir::Module& mod, bool relocMode, bool zpCallMode) {
    relocMode_ = relocMode;
    zpCallMode_ = zpCallMode;

    if (relocMode) {
        emit(".o45");
        emit(".extern __sp_base");
        emitBlank();
    }

    // Emit global declarations
    emitGlobals(mod, relocMode);

    // Emit functions
    for (const auto& fn : mod.functions) {
        emitFunction(fn, relocMode);
    }

    // Emit string data
    emitStrings();
}

void IRCodeGen::emitGlobals(const ir::Module& mod, bool relocMode) {
    // Emit .global directives for non-static globals and functions
    if (relocMode) {
        for (const auto& g : mod.globals) {
            if (!g.isStatic) emit(".global " + g.name);
        }
        for (const auto& fn : mod.functions) {
            if (!fn.isStatic) emit(".global " + fn.name);
        }
        emitBlank();
    }

    // Emit data section for initialized globals
    bool hasData = false;
    for (const auto& g : mod.globals) {
        if (!hasData) {
            emit(".segment \"data\"");
            hasData = true;
        }
        emitLabel(g.name);
        emit(".res " + std::to_string(g.size > 0 ? g.size : ir::typeSize(g.type)));
    }
    if (hasData) emitBlank();

    // Switch to code segment
    if (relocMode) {
        emit(".segment \"code\"");
    } else {
        emit(".segment \"code\"");
    }
    emitBlank();
}

void IRCodeGen::emitStrings() {
    if (stringPool_.empty()) return;
    emitBlank();
    emit(".segment \"data\"");
    for (const auto& [text, label] : stringPool_) {
        emitLabel(label);
        emit(".text \"" + text + "\"");
        emit(".byte 0");
    }
}

// ============================================================================
// Function emission
// ============================================================================

void IRCodeGen::emitFunction(const ir::Function& fn, bool relocMode) {
    emitComment("function " + fn.name);

    // Run register allocator
    alloc_.analyze(fn);

    // Prescan for frame-allocated vRegs only
    prescanFunction(fn);

    // Override: only allocate frame slots for vRegs assigned to frame by allocator
    resetFrame();
    for (const auto& lr : alloc_.liveRanges()) {
        auto alloc = alloc_.getAlloc(lr.vregId);
        if (alloc.loc == VRegAllocator::IN_FRAME) {
            allocSlot(lr.vregId, lr.type);
        }
    }

    currentInstIdx_ = 0;

    // proc directive with parameter specs
    std::string procLine = "proc " + fn.name;
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        procLine += ", ";
        int pSize = ir::typeSize(fn.paramTypes[i]);
        if (pSize == 1) procLine += "B";
        else if (pSize == 4) procLine += "D";
        else procLine += "W";
        // Parameter name
        if (i < fn.paramNames.size() && !fn.paramNames[i].empty()) {
            procLine += "#_p_" + fn.paramNames[i];
        } else {
            procLine += "#_p_" + std::to_string(i);
        }
    }
    emit(procLine);

    // Frame pointer variable
    emit(".var _fp = 0");

    // Allocate frame only for frame-allocated vRegs
    int localFrameSize = frameSize_;
    if (localFrameSize > 0) {
        emitComment("frame: " + std::to_string(localFrameSize) + " bytes (frame-allocated vRegs only)");
        // Allocate in 2-byte pairs
        for (int i = 0; i < localFrameSize; i += 2) {
            emit("phw #0");
            emit(".var _fp = _fp + 2");
        }
    }

    // Emit .local for frame-allocated vRegs only
    for (const auto& [vregId, offset] : vregOffset_) {
        auto alloc = alloc_.getAlloc(vregId);
        if (alloc.loc == VRegAllocator::IN_FRAME) {
            emit(".local __vr" + std::to_string(vregId) + " = " + std::to_string(offset));
        }
    }

    // Emit parameter .var directives (past frame + return addr)
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        int pSize = ir::typeSize(fn.paramTypes[i]);
        if (pSize < 2) pSize = 2;
        std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
            ? fn.paramNames[i] : std::to_string(i);
        emit(".var _p_" + pName + " = _fp + 2");
        emit(".var _fp = _fp + " + std::to_string(pSize));
    }

    emitBlank();

    // Copy params from stack into vReg frame slots
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
            ? fn.paramNames[i] : std::to_string(i);
        emit("ldax.fp _p_" + pName);
        storeVreg((uint32_t)i);
    }

    // Emit blocks (with instruction index tracking for allocator queries)
    currentInstIdx_ = 0;
    for (const auto& block : fn.blocks) {
        emitLabel("@" + block.label);
        for (const auto& inst : block.insts) {
            emitInst(inst);
            currentInstIdx_++;
        }
    }

    // Epilogue
    if (localFrameSize > 0) {
        emitComment("frame cleanup");
        // Preserve return value in AX, clean up frame
        emit("tay"); // save A
        for (int i = 0; i < localFrameSize; i++) {
            emit("pla");
        }
        emit("tya"); // restore A
    }

    // Function attribute directives
    if (zpCallMode_) {
        emit(".func_flags zp_call");
    } else {
        emit(".func_flags stack_call");
    }
    emit(".reg_clobbers A, X, Y, Z");
    emit(".flag_clobbers C, N, Z, V");

    emit("endproc");
    emitBlank();
}

// ============================================================================
// Instruction emission
// ============================================================================

void IRCodeGen::emitInst(const ir::Inst& inst) {
    switch (inst.op) {
        case ir::Op::NOP:
            break;

        case ir::Op::CONST: {
            int val = (int)inst.src1.immVal;
            emit("lda #" + std::to_string(val & 0xFF));
            emit("ldx #" + std::to_string((val >> 8) & 0xFF));
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADD:
        case ir::Op::SUB: {
            loadOperand(inst.src1);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId); // temp store src1
            loadOperand(inst.src1); // reload
            std::string op16 = (inst.op == ir::Op::ADD) ? "add.16" : "sub.16";
            // src2: if immediate, use immediate form; otherwise load from vReg
            if (inst.src2.isImm()) {
                emit(op16 + " .AX, #" + std::to_string((int)inst.src2.immVal));
            } else if (inst.src2.isVreg()) {
                int off2 = slotOf(inst.src2.vregId);
                // Use frame-relative operand
                emit(op16 + " .AX, __vr" + std::to_string(inst.src2.vregId) + ", s");
            } else {
                emit(op16 + " .AX, #0");
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::AND:
        case ir::Op::OR:
        case ir::Op::XOR: {
            loadOperand(inst.src1);
            std::string mn;
            if (inst.op == ir::Op::AND) mn = "and.16";
            else if (inst.op == ir::Op::OR) mn = "ora.16";
            else mn = "eor.16";
            if (inst.src2.isImm()) {
                emit(mn + " .AX, #" + std::to_string((int)inst.src2.immVal));
            } else if (inst.src2.isVreg()) {
                emit(mn + " .AX, __vr" + std::to_string(inst.src2.vregId) + ", s");
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::NEG: {
            loadOperand(inst.src1);
            emit("neg.16 .AX");
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::NOT: {
            loadOperand(inst.src1);
            emit("not.16 .AX");
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::SHL: {
            loadOperand(inst.src1);
            emit("lsl.16 .AX, #" + std::to_string((int)inst.src2.immVal));
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::SHR:
        case ir::Op::ASR: {
            loadOperand(inst.src1);
            std::string mn = (inst.op == ir::Op::ASR) ? "asr.16" : "lsr.16";
            emit(mn + " .AX, #" + std::to_string((int)inst.src2.immVal));
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MUL: {
            loadOperand(inst.src1);
            if (inst.src2.isImm()) {
                emit("mul.16 .AX, #" + std::to_string((int)inst.src2.immVal));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::DIV: {
            loadOperand(inst.src1);
            if (inst.src2.isImm()) {
                emit("div.16 .AX, #" + std::to_string((int)inst.src2.immVal));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MOD: {
            loadOperand(inst.src1);
            if (inst.src2.isImm()) {
                emit("mod.16 .AX, #" + std::to_string((int)inst.src2.immVal));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CMP_EQ:
        case ir::Op::CMP_NE:
        case ir::Op::CMP_LT:
        case ir::Op::CMP_LE:
        case ir::Op::CMP_GT:
        case ir::Op::CMP_GE:
        case ir::Op::CMP_LTU:
        case ir::Op::CMP_LEU:
        case ir::Op::CMP_GTU:
        case ir::Op::CMP_GEU: {
            // Load src1, compare with src2, set result to 0 or 1
            loadOperand(inst.src1);
            if (inst.src2.isImm()) {
                emit("cmp.16 .AX, #" + std::to_string((int)inst.src2.immVal));
            } else if (inst.src2.isVreg()) {
                emit("cmp.16 .AX, __vr" + std::to_string(inst.src2.vregId) + ", s");
            }
            // Set A to 0 or 1 based on flags
            emit("lda #0");
            emit("ldx #0");
            std::string branch;
            switch (inst.op) {
                case ir::Op::CMP_EQ:  branch = "bne"; break; // skip set if not equal
                case ir::Op::CMP_NE:  branch = "beq"; break;
                case ir::Op::CMP_LT:
                case ir::Op::CMP_LTU: branch = "bcs"; break; // skip if carry set (>=)
                case ir::Op::CMP_GE:
                case ir::Op::CMP_GEU: branch = "bcc"; break;
                default: branch = "bne"; break;
            }
            std::string skipLabel = "@__cmp_skip_" + std::to_string(stringCount_++);
            emit(branch + " " + skipLabel);
            emit("lda #1");
            emitLabel(skipLabel);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::LOAD: {
            // Load from address in src1
            loadOperand(inst.src1);
            // TODO: proper indirect load — for now, assume address in AX
            // This needs ZP indirect: store addr to ZP, lda (ZP),Y
            emitComment("TODO: indirect load");
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::STORE: {
            // Store src1 (value) to address in src2
            // For vReg-to-vReg stores, this is just a frame copy
            if (inst.src2.isVreg() && inst.src1.isVreg()) {
                loadVreg(inst.src1.vregId);
                storeVreg(inst.src2.vregId);
            } else {
                loadOperand(inst.src1);
                emitComment("TODO: store to computed address");
            }
            break;
        }

        case ir::Op::ADDR_GLOBAL: {
            emit("ldax #" + inst.src1.name);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_LOCAL: {
            int offset = (int)inst.src1.immVal;
            emit("leax.fp " + std::to_string(offset));
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BR: {
            if (inst.src1.kind == ir::OperandKind::LABEL) {
                emit("bra @" + inst.src1.name);
            }
            break;
        }

        case ir::Op::BR_COND: {
            // src1 = condition, dest = true label, src2 = false label
            loadOperand(inst.src1);
            emit("chknonzero.8 .A");
            if (inst.dest.kind == ir::OperandKind::LABEL) {
                emit("bne @" + inst.dest.name);
            }
            if (inst.src2.kind == ir::OperandKind::LABEL) {
                emit("bra @" + inst.src2.name);
            }
            break;
        }

        case ir::Op::RET: {
            loadOperand(inst.src1);
            // Return value in A:X — frame cleanup handled by function epilogue
            emit("rtn #0");
            break;
        }

        case ir::Op::RET_VOID: {
            emit("rtn #0");
            break;
        }

        case ir::Op::CALL:
        case ir::Op::CALL_VOID: {
            // Push arguments in left-to-right order (cdecl: caller cleanup)
            for (auto it = inst.args.rbegin(); it != inst.args.rend(); ++it) {
                loadOperand(*it);
                emit("phx");
                emit("pha");
            }
            // JSR
            if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                emit("jsr " + inst.src1.name);
            }
            // Cleanup args
            int argBytes = (int)inst.args.size() * 2;
            if (argBytes > 0) {
                // Stack cleanup: TSX; TXA; CLC; ADC #N; TAX; TXS
                emit("tsx");
                emit("txa");
                emit("clc");
                emit("adc #" + std::to_string(argBytes));
                emit("tax");
                emit("txs");
            }
            // Store return value
            if (inst.op == ir::Op::CALL && inst.dest.isVreg()) {
                storeVreg(inst.dest.vregId);
            }
            break;
        }

        case ir::Op::CALL_INDIRECT: {
            // Push arguments
            for (auto it = inst.args.rbegin(); it != inst.args.rend(); ++it) {
                loadOperand(*it);
                emit("phx");
                emit("pha");
            }
            // Load function pointer and call
            loadOperand(inst.src1);
            emitComment("TODO: indirect call via JSR ($ZP)");
            // Cleanup
            int argBytes = (int)inst.args.size() * 2;
            if (argBytes > 0) {
                emit("tsx");
                emit("txa");
                emit("clc");
                emit("adc #" + std::to_string(argBytes));
                emit("tax");
                emit("txs");
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::SEXT: {
            loadOperand(inst.src1);
            if (inst.src1.type == ir::Type::I8 && inst.resultType == ir::Type::I16) {
                emit("sxt.8");
            } else if (inst.src1.type == ir::Type::I16 && inst.resultType == ir::Type::I32) {
                emit("sxt.16");
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ZEXT: {
            loadOperand(inst.src1);
            if (inst.src1.type == ir::Type::I8) {
                emit("ldx #0"); // zero-extend: clear high byte
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::TRUNC: {
            loadOperand(inst.src1);
            // Truncation: just keep the low byte(s) — already in A
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ASM_INLINE: {
            emit(inst.asmText);
            break;
        }

        case ir::Op::PHI: {
            // PHI nodes are not directly executable in linear code.
            // For now, skip — the IRBuilder's control flow already sets
            // values before branching.
            break;
        }

        default:
            emitComment("TODO: unimplemented IR op");
            break;
    }
}
