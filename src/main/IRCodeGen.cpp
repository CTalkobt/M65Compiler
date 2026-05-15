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
                // Value is in A:X — we need to spill to ZP scratch so the simulated op
                // can read it as a memory operand. Use __zp_scratch and __zp_scratch+1.
                emit("sta __zp_scratch");
                emit("stx __zp_scratch+1");
                return "__zp_scratch";
        }
    }
    return "#0";
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
    } else {
        // Standalone PRG mode: emit startup stub
        bool hasMain = false;
        for (const auto& fn : mod.functions) {
            if (fn.name == "_main") { hasMain = true; break; }
        }
        out_ << "* = $2000\n";
        emit("__sp_base = $0101");
        emit("__zp_scratch = $02");
        emit("__zp_scratch2 = $04");
        if (hasMain) {
            emit("jsr _main");
        }
        // Halt: infinite loop (BRK vectors to $FFFE which is uninitialized RAM)
        emitLabel("__halt");
        emit("jmp __halt");
        emitBlank();
    }

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
            if (g.type == ir::Type::I8) {
                emit(".byte " + std::to_string((int)(g.initValue & 0xFF)));
            } else if (g.type == ir::Type::I32) {
                emit(".dword " + std::to_string((int)g.initValue));
            } else {
                emit(".word " + std::to_string((int)(g.initValue & 0xFFFF)));
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

void IRCodeGen::emitFunction(const ir::Function& fn, bool relocMode) {
    emitComment("function " + fn.name);

    // Run register allocator
    alloc_.analyze(fn);

    // Copy local slot info from IR function
    localSlotVregs_ = fn.localSlotVregs;

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
    localFrameSize_ = localFrameSize;  // save for RET cleanup
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

    // No .var for params — the proc directive assigns param offsets
    // in reverse order (last param at _fp+2, first at highest offset).
    // ldax.fp _p_name uses the proc-assigned offsets directly.

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

    // Common return point — clean up frame, then endproc emits rts
    emitLabel("@__return");
    if (localFrameSize > 0) {
        // Preserve return value in Y while popping frame
        emit("tay");
        for (int i = 0; i < localFrameSize; i++) emit("pla");
        emit("tya");
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
            // For commutative ADD: if src1 is in frame (would clobber scratch),
            // swap operands — load src1 as memory, src2 into AX
            bool src1InFrame = inst.src1.isVreg() &&
                alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME;
            bool src2InFrame = inst.src2.isVreg() &&
                alloc_.getAlloc(inst.src2.vregId).loc == VRegAllocator::IN_FRAME;
            std::string op16 = (inst.op == ir::Op::ADD) ? "add.16" : "sub.16";
            if (inst.op == ir::Op::ADD && src2InFrame && !src1InFrame) {
                // Swap: load src2 into AX, use src1 as memory operand
                std::string src1mem = src2MemOperand(inst.src1);
                loadOperand(inst.src2);
                emit(op16 + " .AX, " + src1mem);
            } else {
                std::string src2mem = src2MemOperand(inst.src2);
                loadOperand(inst.src1);
                emit(op16 + " .AX, " + src2mem);
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
            if (inst.op == ir::Op::AND) mn = "and.16";
            else if (inst.op == ir::Op::OR) mn = "ora.16";
            else mn = "eor.16";
            if (src2InFrame && !src1InFrame) {
                std::string src1mem = src2MemOperand(inst.src1);
                loadOperand(inst.src2);
                emit(mn + " .AX, " + src1mem);
            } else {
                std::string src2mem = src2MemOperand(inst.src2);
                loadOperand(inst.src1);
                emit(mn + " .AX, " + src2mem);
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
            emit("mul.16 .AX, " + src2);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::DIV: {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            emit("div.16 .AX, " + src2);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MOD: {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            emit("mod.16 .AX, " + src2);
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
            // Signed ops (CMP_LT/LE/GT/GE) use cmp.s16.
            // Unsigned ops (CMP_LTU/LEU/GTU/GEU) and eq/ne use cmp.16.
            // The IRBuilder selects signed vs unsigned based on operand type:
            // int (default, unsigned) → CMP_LTU etc; signed int → CMP_LT etc.
            bool isSigned = (inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_LE ||
                             inst.op == ir::Op::CMP_GT || inst.op == ir::Op::CMP_GE);
            emit(isSigned ? ("cmp.s16 .AX, " + src2) : ("cmp.16 .AX, " + src2));
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
                emit("ldx " + inst.src1.name + "+1");
            } else if (inst.src1.isVreg()) {
                // Load value from address held in a vReg via (ZP),Y
                auto addrAlloc = alloc_.getAlloc(inst.src1.vregId);
                std::string zpPair;
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
                emit("ldy #1");
                emit("lda (" + zpPair + "),y");  // hi byte
                emit("tax");
                emit("ldy #0");
                emit("lda (" + zpPair + "),y");  // A = lo
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::STORE: {
            if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                // Store directly to global address
                loadOperand(inst.src1);
                emit("sta " + inst.src2.name);
                emit("stx " + inst.src2.name + "+1");
            } else if (inst.src2.isVreg() && inst.src1.isVreg()) {
                // Is the target a local variable slot (direct write)
                // or a computed address (indirect write through pointer)?
                bool isLocalSlot = localSlotVregs_.count(inst.src2.vregId) > 0;
                if (isLocalSlot) {
                    // Direct vReg-to-vReg copy (local variable assignment)
                    loadVreg(inst.src1.vregId);
                    storeVreg(inst.src2.vregId);
                } else {
                    // Indirect store: write value through pointer address
                    // If address vReg is already in ZP, use it directly for (ZP),Y
                    auto addrAlloc = alloc_.getAlloc(inst.src2.vregId);
                    std::string zpPair;
                    if (addrAlloc.loc == VRegAllocator::IN_ZP) {
                        // Address already in ZP — use directly
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << addrAlloc.offset;
                        zpPair = ss.str();
                    } else {
                        // Load address into __zp_scratch2 (NOT __zp_scratch,
                        // because loadVreg for the value uses ldax.fp which
                        // clobbers __zp_scratch internally)
                        loadVreg(inst.src2.vregId);
                        emit("sta __zp_scratch2");
                        emit("stx __zp_scratch2+1");
                        zpPair = "__zp_scratch2";
                    }
                    loadVreg(inst.src1.vregId);  // load value
                    emit("ldy #0");
                    emit("sta (" + zpPair + "),y"); // store lo byte
                    if (inst.resultType != ir::Type::I8) {
                        emit("txa");
                        emit("iny");
                        emit("sta (" + zpPair + "),y");
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
            int offset = (int)inst.src1.immVal;
            emit("leax.fp " + std::to_string(offset));
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_ELEM: {
            // base + index * elemSize
            // src1 = base address, src2 = index, args[0] = elemSize
            int elemSize = (inst.args.size() > 0) ? (int)inst.args[0].immVal : 2;
            // Get base as memory operand FIRST (may use ldax.fp which clobbers AX)
            std::string baseSrc = src2MemOperand(inst.src1);
            // Then load index into AX
            loadOperand(inst.src2);
            if (elemSize > 1) {
                emit("mul.16 .AX, #" + std::to_string(elemSize));
            }
            // Add base address (commutative: index + base = base + index)
            emit("add.16 .AX, " + baseSrc);
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
        case ir::Op::CALL_VOID: {
            // Push arguments left-to-right using push .ax (matches legacy CodeGenerator).
            // proc assigns params in reverse: last param at _fp+2, first at highest offset.
            {
            int argBytes = (int)inst.args.size() * 2;
            for (const auto& arg : inst.args) {
                loadOperand(arg);
                emit("push .ax");
                emit(".var _fp = _fp + 2");
            }
            // JSR — callee's endproc handles param cleanup via rts #N
            if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                emit("jsr " + inst.src1.name);
            }
            // Restore _fp after callee cleaned params (rts #N popped argBytes)
            if (argBytes > 0) {
                emit(".var _fp = _fp - " + std::to_string(argBytes));
            }
            // Return value is in A:X
            if (inst.op == ir::Op::CALL && inst.dest.isVreg()) {
                storeVreg(inst.dest.vregId);
            }
            }
            break;
        }

        case ir::Op::CALL_INDIRECT: {
            // Push arguments left-to-right
            for (const auto& arg : inst.args) {
                loadOperand(arg);
                emit("push .ax");
            }
            // Load function pointer and call
            loadOperand(inst.src1);
            emitComment("TODO: indirect call via JSR ($ZP)");
            // Callee handles cleanup — no caller-side stack adjustment
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
