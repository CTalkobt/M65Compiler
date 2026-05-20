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
    // Check for array override size
    auto sit = vregSizes_.find(vregId);
    if (sit != vregSizes_.end()) size = sit->second;
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
                emit("ldx #0");
            } else if (alloc.type == ir::Type::I32) {
                emit("lda " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit("ldx " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 2);
                emit("ldy " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 3);
                emit("ldz " + ss.str());
            } else {
                emit("lda " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit("ldx " + ss.str());
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            std::string sym = "__vr" + std::to_string(vregId);
            if (alloc.type == ir::Type::I32) {
                emit("ldaxyz.fp " + sym);
            } else {
                emit("ldax.fp " + sym);
            }
            break;
        }
    }
}

void IRCodeGen::storeVreg(uint32_t vregId) {
    auto alloc = alloc_.getAlloc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            std::string zpAddr = ss.str();
            if (alloc.type == ir::Type::I8) {
                emit("sta " + zpAddr);
            } else if (alloc.type == ir::Type::I32) {
                emit("sta " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit("stx " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 2);
                emit("sty " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 3);
                emit("stz " + ss.str());
            } else {
                emit("sta " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit("stx " + ss.str());
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            std::string sym = "__vr" + std::to_string(vregId);
            if (alloc.type == ir::Type::I32) {
                emit("staxyz.fp " + sym);
            } else {
                emit("stax.fp " + sym);
            }
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
            if (op.type == ir::Type::I32) {
                emit("ldy #" + std::to_string((int)((op.immVal >> 16) & 0xFF)));
                emit("ldz #" + std::to_string((int)((op.immVal >> 24) & 0xFF)));
            }
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

std::string IRCodeGen::src2MemOperand(const ir::Operand& op) {
    if (op.isImm()) {
        return "#" + std::to_string((int)op.immVal);
    }
    if (op.isVreg()) {
        auto alloc = alloc_.getAlloc(op.vregId);
        switch (alloc.loc) {
            case VRegAllocator::IN_ZP: {
                std::stringstream ss;
                ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
                return ss.str();
            }
            case VRegAllocator::IN_FRAME:
                // Frame vRegs use ldax.fp which internally uses __zp_scratch.
                // Spill to __zp_scratch2 to avoid conflict when loadOperand(src1)
                // also needs __zp_scratch for its own ldax.fp.
                loadVreg(op.vregId);
                emit("sta __zp_scratch2");
                emit("stx __zp_scratch2+1");
                return "__zp_scratch2";
            case VRegAllocator::IN_AX:
                // Value is in A:X — spill to __zp_scratch2 (not __zp_scratch,
                // because loadOperand for the other operand may use ldax.fp
                // which clobbers __zp_scratch internally).
                emit("sta __zp_scratch2");
                emit("stx __zp_scratch2+1");
                return "__zp_scratch2";
        }
    }
    return "#0";
}

// ============================================================================
// Module-level emission
// ============================================================================

void IRCodeGen::generate(const ir::Module& mod, uint32_t zpStart, bool relocMode, bool zpCallMode) {
    relocMode_ = relocMode;
    zpCallMode_ = zpCallMode;
    zeroPageStart_ = zpStart;

    if (relocMode) {
        emit(".o45");
        emit(".extern __sp_base");
    } else {
        // Standalone PRG mode: emit startup stub
        bool hasMain = false;
        for (const auto& fn : mod.functions) {
            if (fn.name == "_main") { hasMain = true; break; }
        }
        out_ << "* = $2000\n";
        emit("__sp_base = $0101");

        // --- ZP Save ---
        if (mod.saveZP) {
            emitComment("Save ZP $08-$FF to BSS buffer");
            emit("ldx #0");
            emitLabel("@__zp_save_loop");
            emit("lda $08,x");
            emit("sta __zp_save_buf,x");
            emit("inx");
            emit("cpx #248");
            emit("bne @__zp_save_loop");
        }

        if (hasMain) {
            emit("jsr _main");
        }
        
        // Halt or exit
        if (mod.saveZP) {
            emitComment("Restore ZP $08-$FF from BSS buffer");
            emit("ldx #0");
            emitLabel("@__zp_restore_loop");
            emit("lda __zp_save_buf,x");
            emit("sta $08,x");
            emit("inx");
            emit("cpx #248");
            emit("bne @__zp_restore_loop");
        }
        emitLabel("__halt");
        emit("jmp __halt");
    }

    auto hex8 = [](uint32_t val) {
        std::stringstream ss;
        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (val & 0xFF);
        return ss.str();
    };

    emit("__zp_scratch = " + hex8(zeroPageStart_));
    emit("__zp_scratch2 = " + hex8(zeroPageStart_ + 2));
    emit("__zp_scratch3 = " + hex8(zeroPageStart_ + 4));
    emit("__zp_scratch4 = " + hex8(zeroPageStart_ + 6));
    emitBlank();

    // Emit extern declarations
    for (const auto& ext : mod.externs) {
        emit(".extern " + ext);
    }
    if (!mod.externs.empty()) emitBlank();

    // Emit global declarations (segment switching only in reloc mode)
    emitGlobals(mod, relocMode);

    // Emit functions
    for (const auto& fn : mod.functions) {
        emitFunction(fn, relocMode);
    }

    // Emit string data
    emitStrings(mod);

    // Emit ZP save buffer at end of program (BSS)
    if (mod.saveZP) {
        emitBlank();
        emitLabel("__zp_save_buf");
        emit(".res 248");
    }
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
            if (relocMode) emit(".segment \"data\"");
            hasData = true;
        }
        emitLabel(g.name);
        if (g.hasInitValue) {
            if (!g.initList.empty()) {
                // Array initializer
                for (size_t i = 0; i < g.initList.size(); i++) {
                    if (g.type == ir::Type::I8) emit(".byte " + std::to_string((int)(g.initList[i] & 0xFF)));
                    else if (g.type == ir::Type::I32) emit(".dword " + std::to_string((int)g.initList[i]));
                    else emit(".word " + std::to_string((int)(g.initList[i] & 0xFFFF)));
                }
                // Handle partial initialization: padding with zeros
                int bytesEmitted = (int)g.initList.size() * ir::typeSize(g.type);
                if (bytesEmitted < g.size) {
                    emit(".res " + std::to_string(g.size - bytesEmitted));
                }
            } else {
                // Scalar initializer
                if (g.type == ir::Type::I8) {
                    emit(".byte " + std::to_string((int)(g.initValue & 0xFF)));
                } else if (g.type == ir::Type::I32) {
                    emit(".dword " + std::to_string((int)g.initValue));
                } else {
                    emit(".word " + std::to_string((int)(g.initValue & 0xFFFF)));
                }
                // Scalar might be larger than its type if it's an array init to zero? 
                // e.g. char a[10] = 0;
                int typeSz = ir::typeSize(g.type);
                if (typeSz < g.size) {
                    emit(".res " + std::to_string(g.size - typeSz));
                }
            }
        } else {
            emit(".res " + std::to_string(g.size > 0 ? g.size : ir::typeSize(g.type)));
        }
    }
    if (hasData) emitBlank();

    // Switch to code segment (only in reloc mode; PRG mode is linear)
    if (relocMode) {
        emit(".segment \"code\"");
    }
    emitBlank();
}

void IRCodeGen::emitStrings(const ir::Module& mod) {
    if (mod.strings.empty()) return;
    emitBlank();
    if (relocMode_) emit(".segment \"data\"");
    for (const auto& sl : mod.strings) {
        emitLabel(sl.label);
        if (sl.isAscii) {
            emit(".ascii \"" + sl.value + "\"");
        } else {
            emit(".text \"" + sl.value + "\"");
        }
        emit(".byte 0");
    }
}

// ============================================================================
// Function emission
// ============================================================================

// ============================================================================
// Per-function clobber analysis
// ============================================================================

IRCodeGen::FuncClobbers IRCodeGen::computeFuncClobbers(const ir::Function& fn) {
    // Register bit constants
    constexpr uint8_t A = 0x01, X = 0x02, Y = 0x04, Z = 0x08;
    constexpr uint8_t C = 0x01, N = 0x02, ZF = 0x04, V = 0x08;

    FuncClobbers fc;

    for (const auto& block : fn.blocks) {
        for (const auto& inst : block.insts) {
            switch (inst.op) {
                case ir::Op::NOP:
                    break;

                case ir::Op::CONST:
                    fc.regs |= A | X;    // lda #imm; ldx #imm
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::ADD:
                case ir::Op::SUB:
                    fc.regs |= A | X;    // simulated add.16/sub.16 uses AX
                    fc.flags |= C | N | ZF | V;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::MUL:
                case ir::Op::DIV:
                case ir::Op::MOD:
                    fc.regs |= A | X;    // hardware math, result in AX
                    fc.flags |= C | N | ZF | V;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::NEG:
                case ir::Op::NOT:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::AND:
                case ir::Op::OR:
                case ir::Op::XOR:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::SHL:
                case ir::Op::SHR:
                case ir::Op::ASR:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::CMP_EQ: case ir::Op::CMP_NE:
                case ir::Op::CMP_LT: case ir::Op::CMP_LE:
                case ir::Op::CMP_GT: case ir::Op::CMP_GE:
                case ir::Op::CMP_LTU: case ir::Op::CMP_LEU:
                case ir::Op::CMP_GTU: case ir::Op::CMP_GEU:
                    fc.regs |= A | X;    // cmp + set result in AX
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::LOAD:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    if (inst.src1.kind != ir::OperandKind::GLOBAL) fc.regs |= Y; // ldy #0 for indirect
                    break;

                case ir::Op::STORE:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    if (inst.src2.kind != ir::OperandKind::GLOBAL) fc.regs |= Y; // ldy for indirect
                    break;

                case ir::Op::LOAD_ZP:
                case ir::Op::STORE_ZP:
                    fc.regs |= A;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::ADDR_GLOBAL:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::ADDR_LOCAL:
                    fc.regs |= A | X;    // leax.fp uses AX
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::ADDR_ELEM:
                    fc.regs |= A | X;    // mul.16 + add.16
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::SEXT:
                    fc.regs |= A | X;    // sxt.8/sxt.16 modify AX
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::ZEXT:
                    fc.regs |= A | X;    // ldx #0
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::TRUNC:
                    fc.regs |= A | X;    // load into AX, keep low
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::BFEXT:
                case ir::Op::BFINS:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::COPY:
                    fc.regs |= A | X;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::BR:
                case ir::Op::PHI:
                    break; // no register effects

                case ir::Op::BR_COND:
                    fc.regs |= A;        // chknonzero.8 tests A
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::RET:
                    fc.regs |= A | X;    // return value in AX
                    fc.flags |= N | ZF;
                    if (inst.src1.type == ir::Type::I32) fc.regs |= Y | Z;
                    break;

                case ir::Op::RET_VOID:
                    break;

                case ir::Op::CALL:
                case ir::Op::CALL_VOID:
                case ir::Op::CALL_INDIRECT:
                    // Calls conservatively clobber everything
                    // (Phase 2 will refine this using callee's .reg_clobbers)
                    fc.regs |= A | X | Y | Z;
                    fc.flags |= C | N | ZF | V;
                    fc.isLeaf = false;
                    break;

                case ir::Op::ASM_INLINE:
                    // Inline asm — conservatively assume everything clobbered
                    fc.regs |= A | X | Y | Z;
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::VA_START:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::SWITCH:
                    fc.regs |= A | X;
                    fc.flags |= C | N | ZF;
                    break;
            }
        }
    }

    return fc;
}

void IRCodeGen::emitFunction(const ir::Function& fn, bool relocMode, bool isMainWithZPSave) {
    emitComment("function " + fn.name);

    // Run register allocator
    alloc_.analyze(fn);

    // Copy local slot info from IR function
    localSlotVregs_ = fn.localSlotVregs;
    vregSizes_ = fn.vregSizes;

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
            procLine += "#@_p_" + fn.paramNames[i];
        } else {
            procLine += "#@_p_" + std::to_string(i);
        }
    }
    emit(procLine);

    // Frame pointer variable
    emit(".var _fp = 0");

    // Allocate frame only for frame-allocated vRegs
    int localFrameSize = frameSize_;
    localFrameSize_ = localFrameSize;  // save for RET cleanup
    if (localFrameSize > 0) {
        emitComment("frame: " + std::to_string(localFrameSize) + " bytes (frame-allocated vRegs only)");
        // Push frame slots physically — do NOT bump _fp (stays at 0).
        // ldax.fp computes __sp_base + _fp + offset + SPL.
        // With _fp=0, SPL accounts for the physical pushes correctly.
        for (int i = 0; i < localFrameSize; i += 2) {
            emit("phw #0");
        }
    }

    // Emit .local for frame-allocated vRegs only
    for (const auto& [vregId, offset] : vregOffset_) {
        auto alloc = alloc_.getAlloc(vregId);
        if (alloc.loc == VRegAllocator::IN_FRAME) {
            emit(".local __vr" + std::to_string(vregId) + " = " + std::to_string(offset));
        }
    }

    // Identify which localNames are parameters
    std::set<std::string> paramNamesSet(fn.paramNames.begin(), fn.paramNames.end());

    // Emit .local/.var aliases for named variables (to support inline asm)
    for (const auto& [name, vregId] : fn.localNames) {
        bool isParam = paramNamesSet.count(name) > 0;
        if (isParam) continue; // Parameters are handled by .var overrides later

        std::string prefix = "@_l_";

        if (vregOffset_.count(vregId)) {
            // It's a local/param on the stack
            emit(".local " + prefix + name + " = " + std::to_string(vregOffset_.at(vregId)));
        } else {
            // It might be in ZP?
            auto alloc = alloc_.getAlloc(vregId);
            if (alloc.loc == VRegAllocator::IN_ZP) {
                std::stringstream ss;
                ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)alloc.offset;
                emit(".var " + prefix + name + " = " + ss.str());
            }
        }
    }

    // Param .var overrides
    if (!zpCallMode_) {
        // Stack convention: args pushed right-to-left, so first param is
        // closest to SP (smallest offset past frame + return address).
        int pOff = localFrameSize + 2;
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            int ps = ir::typeSize(fn.paramTypes[i]);
            if (ps < 2) ps = 2;
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            emit(".var @_p_" + pName + " = " + std::to_string(pOff));
            pOff += ps;
        }
    } else {
        // ZP call convention: params in ZP block past scratch area
        auto hex8 = [](uint32_t val) {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (val & 0xFF);
            return ss.str();
        };
        int zpOff = (int)zeroPageStart_ + 8;
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            int ps = ir::typeSize(fn.paramTypes[i]);
            if (ps < 2) ps = 2;
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            emit(".var @_p_" + pName + " = " + hex8(zpOff));
            zpOff += ps;
        }
    }

    emitBlank();

    // Copy params into vReg frame slots
    if (!zpCallMode_) {
        // Stack convention: params are on the stack, copy to ZP temps
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            if (fn.paramTypes[i] == ir::Type::I32) {
                emit("ldaxyz.fp @_p_" + pName);
            } else {
                emit("ldax.fp @_p_" + pName);
            }
            storeVreg((uint32_t)i);
        }
    } else {
        // ZP call convention: params already in ZP block, copy to vReg slots
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            // ZP params are at $10 + offset; load from the .var @_p_ location
            emit("lda @_p_" + pName);
            emit("ldx @_p_" + pName + "+1");
            storeVreg((uint32_t)i);
        }
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

    // Common return point — clean up frame, then endproc emits rts
    emitLabel("@__return");
    if (localFrameSize > 0) {
        // Preserve return value in A,X,Y,Z while popping frame
        emit("sta __zp_scratch");
        emit("stx __zp_scratch+1");
        emit("sty __zp_scratch3");
        emit("stz __zp_scratch3+1");
        for (int i = 0; i < localFrameSize; i++) emit("pla");
        emit("lda __zp_scratch");
        emit("ldx __zp_scratch+1");
        emit("ldy __zp_scratch3");
        emit("ldz __zp_scratch3+1");
    }

    // Function attribute directives with per-function clobber analysis
    auto fc = computeFuncClobbers(fn);
    {
        std::string funcFlags = zpCallMode_ ? "zp_call" : "stack_call";
        if (fc.isLeaf) funcFlags += ", leaf";
        emit(".func_flags " + funcFlags);
    }
    {
        std::string regs;
        if (fc.regs & 0x01) regs += "A, ";
        if (fc.regs & 0x02) regs += "X, ";
        if (fc.regs & 0x04) regs += "Y, ";
        if (fc.regs & 0x08) regs += "Z, ";
        if (!regs.empty()) { regs.pop_back(); regs.pop_back(); emit(".reg_clobbers " + regs); }
    }
    {
        std::string flags;
        if (fc.flags & 0x01) flags += "C, ";
        if (fc.flags & 0x02) flags += "N, ";
        if (fc.flags & 0x04) flags += "Z, ";
        if (fc.flags & 0x08) flags += "V, ";
        if (!flags.empty()) { flags.pop_back(); flags.pop_back(); emit(".flag_clobbers " + flags); }
    }

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
            // For commutative ADD: if src1 is in frame (would clobber scratch),
            // swap operands — load src1 as memory, src2 into AX
            bool src1InFrame = inst.src1.isVreg() &&
                alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME;
            bool src2InFrame = inst.src2.isVreg() &&
                alloc_.getAlloc(inst.src2.vregId).loc == VRegAllocator::IN_FRAME;
            
            std::string mn;
            if (inst.resultType == ir::Type::I32) {
                mn = (inst.op == ir::Op::ADD) ? "add.32" : "sub.32";
            } else {
                mn = (inst.op == ir::Op::ADD) ? "add.16" : "sub.16";
            }
            std::string reg = (inst.resultType == ir::Type::I32) ? ".AXYZ" : ".AX";

            if (inst.op == ir::Op::ADD && src2InFrame && !src1InFrame) {
                // Swap: load src2 into AX, use src1 as memory operand
                std::string src1mem = src2MemOperand(inst.src1);
                loadOperand(inst.src2);
                emit(mn + " " + reg + ", " + src1mem);
            } else {
                std::string src2mem = src2MemOperand(inst.src2);
                loadOperand(inst.src1);
                emit(mn + " " + reg + ", " + src2mem);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::AND:
        case ir::Op::OR:
        case ir::Op::XOR: {
            // Commutative: if src2 is in frame, swap to avoid scratch conflict
            bool src2InFrame = inst.src2.isVreg() &&
                alloc_.getAlloc(inst.src2.vregId).loc == VRegAllocator::IN_FRAME;
            bool src1InFrame = inst.src1.isVreg() &&
                alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME;
            
            std::string mn;
            if (inst.resultType == ir::Type::I32) {
                if (inst.op == ir::Op::AND) mn = "and.32";
                else if (inst.op == ir::Op::OR) mn = "ora.32";
                else mn = "eor.32";
            } else {
                if (inst.op == ir::Op::AND) mn = "and.16";
                else if (inst.op == ir::Op::OR) mn = "ora.16";
                else mn = "eor.16";
            }
            std::string reg = (inst.resultType == ir::Type::I32) ? ".AXYZ" : ".AX";

            if (src2InFrame && !src1InFrame) {
                std::string src1mem = src2MemOperand(inst.src1);
                loadOperand(inst.src2);
                emit(mn + " " + reg + ", " + src1mem);
            } else {
                std::string src2mem = src2MemOperand(inst.src2);
                loadOperand(inst.src1);
                emit(mn + " " + reg + ", " + src2mem);
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
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            emit("lsl.16 .AX, " + src2);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::SHR:
        case ir::Op::ASR: {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            std::string mn = (inst.op == ir::Op::ASR) ? "asr.16" : "lsr.16";
            emit(mn + " .AX, " + src2);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MUL: {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            if (inst.resultType == ir::Type::I32) {
                emit("mul.32 .AXYZ, " + src2);
            } else {
                emit("mul.16 .AX, " + src2);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::DIV: {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            if (inst.resultType == ir::Type::I32) {
                emit("div.32 .AXYZ, " + src2);
            } else {
                emit("div.16 .AX, " + src2);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MOD: {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            if (inst.resultType == ir::Type::I32) {
                emit("mod.32 .AXYZ, " + src2);
            } else {
                emit("mod.16 .AX, " + src2);
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
            {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            
            bool isSigned = (inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_LE ||
                             inst.op == ir::Op::CMP_GT || inst.op == ir::Op::CMP_GE);
            
            if (inst.src1.type == ir::Type::I32) {
                emit(std::string(isSigned ? "cmp.s32" : "cmp.32") + " .AXYZ, " + src2);
            } else if (inst.src1.type == ir::Type::I8) {
                emit("cmp " + src2);
            } else {
                emit(std::string(isSigned ? "cmp.s16" : "cmp.16") + " .AX, " + src2);
            }
            } // end src2/signed scope
            // Branch IMMEDIATELY (flags live), set result to 0 or 1
            {
            std::string setLabel = "@__cmp_set_" + std::to_string(stringCount_);
            std::string zeroLabel = "@__cmp_zero_" + std::to_string(stringCount_);
            std::string doneLabel = "@__cmp_done_" + std::to_string(stringCount_++);
            switch (inst.op) {
                case ir::Op::CMP_EQ:
                    emit("beq " + setLabel);
                    emit("bra " + zeroLabel);
                    break;
                case ir::Op::CMP_NE:
                    emit("bne " + setLabel);
                    emit("bra " + zeroLabel);
                    break;
                case ir::Op::CMP_LT:
                case ir::Op::CMP_LTU:
                    emit("bcc " + setLabel);
                    emit("bra " + zeroLabel);
                    break;
                case ir::Op::CMP_LE:
                case ir::Op::CMP_LEU:
                    emit("bcc " + setLabel);
                    emit("beq " + setLabel);
                    emit("bra " + zeroLabel);
                    break;
                case ir::Op::CMP_GT:
                case ir::Op::CMP_GTU:
                    emit("beq " + zeroLabel);
                    emit("bcs " + setLabel);
                    emit("bra " + zeroLabel);
                    break;
                case ir::Op::CMP_GE:
                case ir::Op::CMP_GEU:
                    emit("bcs " + setLabel);
                    emit("bra " + zeroLabel);
                    break;
                default:
                    emit("beq " + setLabel);
                    emit("bra " + zeroLabel);
                    break;
            }
            emitLabel(setLabel);
            emit("lda #1");
            emit("ldx #0");
            emit("bra " + doneLabel);
            emitLabel(zeroLabel);
            emit("lda #0");
            emit("ldx #0");
            emitLabel(doneLabel);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::LOAD: {
            if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                // Load directly from global address
                emit("lda " + inst.src1.name);
                if (inst.resultType == ir::Type::I32) {
                    emit("ldx " + inst.src1.name + "+1");
                    emit("ldy " + inst.src1.name + "+2");
                    emit("ldz " + inst.src1.name + "+3");
                } else if (inst.resultType != ir::Type::I8) {
                    emit("ldx " + inst.src1.name + "+1");
                } else {
                    emit("ldx #0");
                }
            } else if (inst.src1.kind != ir::OperandKind::NONE) {
                // Load value from address held in an operand via (ZP),Y
                std::string zpPair;
                if (inst.src1.isVreg()) {
                    auto addrAlloc = alloc_.getAlloc(inst.src1.vregId);
                    if (addrAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << addrAlloc.offset;
                        zpPair = ss.str();
                    } else {
                        loadVreg(inst.src1.vregId);
                        emit("sta __zp_scratch");
                        emit("stx __zp_scratch+1");
                        zpPair = "__zp_scratch";
                    }
                } else {
                    loadOperand(inst.src1);
                    emit("sta __zp_scratch");
                    emit("stx __zp_scratch+1");
                    zpPair = "__zp_scratch";
                }
                
                emit("ldy #0");
                if (inst.resultType == ir::Type::I8) {
                    emit("lda (" + zpPair + "),y");
                    emit("ldx #0");
                } else {
                    emit("lda (" + zpPair + "),y"); // lo
                    emit("pha");
                    emit("iny");
                    emit("lda (" + zpPair + "),y"); // hi
                    emit("tax");
                    emit("pla");
                }
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::STORE: {
            if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                // Store directly to global address
                loadOperand(inst.src1);
                emit("sta " + inst.src2.name);
                if (inst.resultType == ir::Type::I32) {
                    emit("stx " + inst.src2.name + "+1");
                    emit("sty " + inst.src2.name + "+2");
                    emit("stz " + inst.src2.name + "+3");
                } else if (inst.resultType != ir::Type::I8) {
                    emit("stx " + inst.src2.name + "+1");
                }
            } else if (inst.src2.isVreg()) {
                // Is the target a local variable slot (direct write)
                // or a computed address (indirect write through pointer)?
                bool isLocalSlot = localSlotVregs_.count(inst.src2.vregId) > 0;
                if (isLocalSlot) {
                    // Direct vReg-to-vReg copy (local variable assignment)
                    loadOperand(inst.src1);
                    storeVreg(inst.src2.vregId);
                } else {
                    // Indirect store: write value through pointer address
                    auto addrAlloc = alloc_.getAlloc(inst.src2.vregId);
                    std::string zpPair;
                    if (addrAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << addrAlloc.offset;
                        zpPair = ss.str();
                    } else {
                        loadVreg(inst.src2.vregId);
                        emit("sta __zp_scratch2");
                        emit("stx __zp_scratch2+1");
                        zpPair = "__zp_scratch2";
                    }

                    // Now load the VALUE to store
                    loadOperand(inst.src1);

                    emit("ldy #0");
                    if (inst.resultType == ir::Type::I8) {
                        emit("sta (" + zpPair + "),y");
                    } else if (inst.resultType == ir::Type::I32) {
                        // Store 4 bytes. LDY clobbers Y, so save Y/Z to scratch3.
                        emit("sty __zp_scratch3");
                        emit("stz __zp_scratch3+1");
                        emit("sta (" + zpPair + "),y"); // lo
                        emit("txa"); emit("iny"); emit("sta (" + zpPair + "),y"); // byte 1
                        emit("lda __zp_scratch3"); emit("iny"); emit("sta (" + zpPair + "),y"); // byte 2
                        emit("lda __zp_scratch3+1"); emit("iny"); emit("sta (" + zpPair + "),y"); // byte 3
                    } else {
                        emit("sta (" + zpPair + "),y"); // store lo byte
                        emit("txa");
                        emit("iny");
                        emit("sta (" + zpPair + "),y"); // store hi byte
                    }
                }
            } else {
                loadOperand(inst.src1);
                emitComment("TODO: store to non-vReg address");
            }
            break;
        }
        case ir::Op::ADDR_GLOBAL: {
            emit("ldax #" + inst.src1.name);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_LOCAL: {
            if (inst.src1.isVreg()) {
                // Address of a local vReg — resolve by allocation
                auto alloc = alloc_.getAlloc(inst.src1.vregId);
                if (alloc.loc == VRegAllocator::IN_FRAME) {
                    emit("leax.fp __vr" + std::to_string(inst.src1.vregId));
                } else if (alloc.loc == VRegAllocator::IN_ZP) {
                    // ZP address: load the ZP address as a 16-bit value
                    emit("lda #" + std::to_string(alloc.offset));
                    emit("ldx #0");
                }
            } else {
                int offset = (int)inst.src1.immVal;
                emit("leax.fp " + std::to_string(offset));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_ELEM: {
            // Compute: base + index * elemSize
            int elemSize = (inst.args.size() > 0) ? (int)inst.args[0].immVal : 2;
            
            std::string baseMem;
            if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                emit("lda #<" + inst.src1.name);
                emit("ldx #>" + inst.src1.name);
                emit("sta __zp_scratch2");
                emit("stx __zp_scratch2+1");
                baseMem = "__zp_scratch2";
            } else {
                baseMem = src2MemOperand(inst.src1);
            }

            // Step 2: load index into AX
            loadOperand(inst.src2);
            
            // Step 3: multiply index by elemSize if needed
            if (elemSize > 1) {
                emit("mul.16 .AX, #" + std::to_string(elemSize));
            }
            
            // Step 4: add base address
            emit("add.16 .AX, " + baseMem);
            
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::COPY: {
            loadOperand(inst.src1);
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
            // Jump to common return point (endproc handles frame cleanup + rts)
            emit("bra @__return");
            break;
        }

        case ir::Op::RET_VOID: {
            emit("bra @__return");
            break;
        }

        case ir::Op::CALL:
        case ir::Op::CALL_VOID:
        case ir::Op::CALL_INDIRECT: {
            if (inst.callConv == ir::CallConv::ZP) {
                // ZP convention: store args to ZP param block (past scratch)
                int zpOff = (int)zeroPageStart_ + 8; // 8 bytes of scratch
                for (const auto& arg : inst.args) {
                    loadOperand(arg);
                    int ps = ir::typeSize(arg.type);
                    std::stringstream ss;
                    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << zpOff;
                    std::string zpAddr = ss.str();

                    if (arg.type == ir::Type::I32) {
                        emit("sta " + zpAddr);
                        ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (zpOff + 1);
                        emit("stx " + ss.str());
                        ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (zpOff + 2);
                        emit("sty " + ss.str());
                        ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (zpOff + 3);
                        emit("stz " + ss.str());
                    } else if (arg.type != ir::Type::I8) {
                        emit("sta " + zpAddr);
                        ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (zpOff + 1);
                        emit("stx " + ss.str());
                    } else {
                        emit("sta " + zpAddr);
                    }
                    zpOff += ir::typeSize(arg.type);
                }
            } else {
                // STACK convention: Push arguments right-to-left (C convention).
                // Phase 1: Load all args into ZP temp slots BEFORE any pushes,
                // so frame-relative loads aren't affected by SP/fp changes.
                // Phase 2: Push from ZP temps right-to-left.
                int argBytes = 0;
                int nArgs = (int)inst.args.size();
                auto zpSlotAddr = [&](int idx, int byteOff) {
                    std::stringstream ss;
                    ss << "$" << std::hex << std::uppercase << std::setfill('0')
                       << std::setw(2) << ((int)zeroPageStart_ + 0x20 + idx * 4 + byteOff);
                    return ss.str();
                };
                // Phase 1: load all args (right-to-left) into ZP scratch slots
                for (int ai = nArgs - 1; ai >= 0; ai--) {
                    const auto& arg = inst.args[ai];
                    loadOperand(arg);
                    int slot = nArgs - 1 - ai;
                    emit("sta " + zpSlotAddr(slot, 0));
                    emit("stx " + zpSlotAddr(slot, 1));
                    if (arg.type == ir::Type::I32) {
                        emit("sty " + zpSlotAddr(slot, 2));
                        emit("stz " + zpSlotAddr(slot, 3));
                    }
                }
                // Phase 2: push from ZP slots (same order — already right-to-left)
                for (int i = 0; i < nArgs; i++) {
                    const auto& arg = inst.args[nArgs - 1 - i]; // original arg (to get type)
                    int ps = ir::typeSize(arg.type);
                    if (ps < 2) ps = 2;
                    emit("lda " + zpSlotAddr(i, 0));
                    emit("ldx " + zpSlotAddr(i, 1));
                    if (arg.type == ir::Type::I32) {
                        emit("ldy " + zpSlotAddr(i, 2));
                        emit("ldz " + zpSlotAddr(i, 3));
                        emit("push .axyz");
                    } else {
                        emit("push .ax");
                    }
                    argBytes += ps;
                    emit(".var _fp = _fp + " + std::to_string(ps));
                }
                
                // JSR
                if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) {
                    if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                        emit("jsr " + inst.src1.name);
                    }
                } else {
                    // CALL_INDIRECT
                    loadOperand(inst.src1);
                    emit("sta __zp_scratch");
                    emit("stx __zp_scratch+1");
                    emit("jsr (__zp_scratch)");
                }

                // Restore _fp after callee cleaned params (rts #N popped argBytes)
                if (argBytes > 0) {
                    emit(".var _fp = _fp - " + std::to_string(argBytes));
                }
            }

            // If it's a direct call and we didn't jsr yet (ZP case)
            if (inst.callConv == ir::CallConv::ZP && (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID)) {
                if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                    emit("jsr " + inst.src1.name);
                }
            }

            // Return value is in A,X,Y,Z (for 32-bit) or A:X (for 16-bit)
            if (inst.op != ir::Op::CALL_VOID && inst.dest.isVreg()) {
                storeVreg(inst.dest.vregId);
            }
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

        case ir::Op::BFEXT: {
            loadOperand(inst.src1);
            int offset = (int)inst.args[0].immVal;
            int width = (int)inst.args[1].immVal;
            if (inst.src1.type == ir::Type::I16) {
                emit("bfext16 #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else {
                emit("bfext #" + std::to_string(offset) + ", #" + std::to_string(width));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BFINS: {
            // src1: new field value, src2: address of storage unit
            std::string addrStr;
            if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                addrStr = inst.src2.name;
            } else {
                loadVreg(inst.src2.vregId);
                emit("sta __zp_scratch2");
                emit("stx __zp_scratch2+1");
                addrStr = "__zp_scratch2";
            }
            loadOperand(inst.src1); // AX = new value
            int offset = (int)inst.args[0].immVal;
            int width = (int)inst.args[1].immVal;
            if (inst.resultType == ir::Type::I16) {
                emit("bfins16 " + addrStr + ", #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else {
                emit("bfins " + addrStr + ", #" + std::to_string(offset) + ", #" + std::to_string(width));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ASM_INLINE: {
            emit(inst.asmText);
            break;
        }

        case ir::Op::VA_START: {
            // Compute stack address past last named parameter
            // asmText = last param name; leax.fp gives its stack address
            std::string pName = inst.asmText;
            emit("leax.fp @_p_" + pName);
            // Add sizeof(param) to get past it to the first variadic arg
            emit("add.16 .AX, #2");
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
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
