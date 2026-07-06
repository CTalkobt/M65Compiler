#include "IRCodeGen.hpp"
#include "OpEffect.hpp"
#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>

// Convert IEEE 754 double to CBM 40-bit float (5 bytes)
static void doubleToCBM40(double val, uint8_t out[5]) {
    if (val == 0.0) { out[0]=out[1]=out[2]=out[3]=out[4]=0; return; }
    bool neg = val < 0; if (neg) val = -val;
    int exp; double mant = std::frexp(val, &exp);
    int cbmExp = exp + 128;
    if (cbmExp < 0) cbmExp = 0; if (cbmExp > 255) cbmExp = 255;
    out[0] = (uint8_t)cbmExp;
    uint32_t m = (uint32_t)(mant * 4294967296.0);
    out[1] = (uint8_t)(m >> 24); out[2] = (uint8_t)(m >> 16);
    out[3] = (uint8_t)(m >> 8);  out[4] = (uint8_t)(m);
    if (neg) out[1] |= 0x80; else out[1] &= 0x7F;
}


static std::string hex8(uint8_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)val;
    return ss.str();
}

static std::string hex16(uint16_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int)val;
    return ss.str();
}

static std::string hex32(uint32_t val) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << (int)val;
    return ss.str();
}

IRCodeGen::IRCodeGen(std::ostream& out) : out_(out) {}

const ir::Inst* IRCodeGen::peekNextInst() const {
    if (!currentFn_) return nullptr;
    const auto& block = currentFn_->blocks[currentBlockIdx_];
    if (currentInstInBlock_ + 1 < block.insts.size())
        return &block.insts[currentInstInBlock_ + 1];
    return nullptr;
}

void IRCodeGen::emit(const std::string& line, const std::string& reason) {
    out_ << "    " << line;
    if (emitReasons_ && !reason.empty()) {
        out_ << "  ; [" << reason << "]";
    }
    out_ << "\n";

    // Auto-update MachineState for simulated ops (contain '.' in mnemonic).
    // Native instructions are handled with precise ms_ updates by the caller.
    // Simulated ops (like stax.fp, add.16, mul.s16) need table-driven invalidation
    // because the caller can't know what the expansion will clobber.
    if (!line.empty() && line[0] != '.' && line[0] != ';') {
        // Check if this is a simulated op (contains '.' in the mnemonic portion)
        auto space = line.find(' ');
        std::string mnem = (space != std::string::npos) ? line.substr(0, space) : line;
        if (mnem.find('.') != std::string::npos) {
            const OpEffect& effect = getOpEffect(mnem);
            effect.apply(ms_);
        }
    }
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
    vregSizes_ = fn.vregSizes; // Must set before allocSlot calls (struct/array size overrides)
    ir::Function& mutableFn = const_cast<ir::Function&>(fn);
    mutableFn.vregOffsets.clear();

    // 1. Allocate static link first if it exists
    if (fn.isNested && fn.staticLinkVreg != -1) {
        int off = allocSlot(fn.staticLinkVreg, ir::Type::PTR);
        mutableFn.vregOffsets[fn.staticLinkVreg] = off;
    }

    // 2. Allocate all named variables (locals and parameters)
    for (const auto& [name, vid] : fn.localNames) {
        if (mutableFn.vregOffsets.count(vid)) continue;

        ir::Type t = ir::Type::I16;
        if (fn.vregTypes.count(vid)) {
            t = fn.vregTypes.at(vid);
        }

        int off = allocSlot(vid, t);
        mutableFn.vregOffsets[vid] = off;
    }

    // 3. Scan all instructions for any other vReg definitions (temporaries)
    for (const auto& block : fn.blocks) {
        for (const auto& inst : block.insts) {
            if (inst.dest.isVreg()) {
                uint32_t vid = inst.dest.vregId;
                if (mutableFn.vregOffsets.count(vid)) continue;
                
                int off = allocSlot(vid, inst.resultType != ir::Type::VOID ? inst.resultType : ir::Type::I16);
                mutableFn.vregOffsets[vid] = off;
            }
        }
    }
}

// ============================================================================
// Load/store vRegs via frame pointer
// ============================================================================

std::string IRCodeGen::vregDesc(uint32_t vregId) {
    if (!emitReasons_) return "";
    auto alloc = alloc_.getAlloc(vregId);
    std::string loc;
    switch (alloc.loc) {
        case VRegAllocator::IN_AX: loc = "AX"; break;
        case VRegAllocator::IN_ZP: loc = "ZP:$" + hex8((uint8_t)alloc.offset); break;
        case VRegAllocator::IN_FRAME: loc = "frame"; break;
    }
    std::string ty;
    switch (alloc.type) {
        case ir::Type::I8: ty = "I8"; break;
        case ir::Type::I16: ty = "I16/PTR"; break;  // PTR == I16 on 45GS02
        case ir::Type::I32: ty = "I32"; break;
        default: ty = "?"; break;
    }
    return "v" + std::to_string(vregId) + "(" + ty + "," + loc + ")";
}

void IRCodeGen::loadVreg(uint32_t vregId) {
    auto alloc = alloc_.getAlloc(vregId);
    std::string r = "loadVreg " + vregDesc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            // Already in A:X — check if it's still there
            if (alloc_.isInAX(vregId, currentInstIdx_)) return; // no-op!
            // Fell through from A:X to somewhere — treat as frame
            if (vregOffset_.count(vregId)) {
                emit("ldax.fp " + std::to_string(vregOffset_[vregId]), r);
            }
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            std::string zpAddr = ss.str();
            if (alloc.type == ir::Type::I8) {
                emit("lda " + zpAddr, r);
                emit("ldx #0", "zext I8→I16");
            } else if (alloc.type == ir::Type::I32) {
                emit("lda " + zpAddr, r);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit("ldx " + ss.str(), r);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 2);
                emit("ldy " + ss.str(), r);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 3);
                emit("ldz " + ss.str(), r);
            } else {
                emit("lda " + zpAddr, r);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit("ldx " + ss.str(), r);
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            std::string sym = "__vr" + std::to_string(vregId);
            if (alloc.type == ir::Type::I32) {
                emit("ldaxyz.fp " + sym, r);
            } else if (alloc.type == ir::Type::I8) {
                emit("lda.fp " + sym, r);
            } else {
                emit("ldax.fp " + sym, r);
            }
            break;
        }
    }
}

void IRCodeGen::loadVregA(uint32_t vregId) {
    auto alloc = alloc_.getAlloc(vregId);
    if (alloc.type != ir::Type::I8) { loadVreg(vregId); return; }
    std::string r = "loadVregA " + vregDesc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            if (alloc_.isInAX(vregId, currentInstIdx_)) return;
            if (vregOffset_.count(vregId))
                emit("lda.fp " + std::to_string(vregOffset_[vregId]), r);
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            emit("lda " + ss.str(), r);
            break;
        }
        case VRegAllocator::IN_FRAME:
            emit("lda.fp __vr" + std::to_string(vregId), r);
            break;
    }
}

void IRCodeGen::storeVreg(uint32_t vregId) {
    auto alloc = alloc_.getAlloc(vregId);
    std::string r = "storeVreg " + vregDesc(vregId);
    switch (alloc.loc) {
        case VRegAllocator::IN_AX:
            break;
        case VRegAllocator::IN_ZP: {
            std::stringstream ss;
            ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
            std::string zpAddr = ss.str();
            if (alloc.type == ir::Type::I8) {
                emit("sta " + zpAddr, r);
            } else if (alloc.type == ir::Type::I32) {
                emit("sta " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit(ms_.regsEqual(REG_A, REG_X) ? "sta " + ss.str() : "stx " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 2);
                emit(ms_.regsEqual(REG_A, REG_Y) ? "sta " + ss.str() : "sty " + ss.str());
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 3);
                emit(ms_.regsEqual(REG_A, REG_Z) ? "sta " + ss.str() : "stz " + ss.str());
            } else {
                emit("sta " + zpAddr);
                ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)(alloc.offset + 1);
                emit(ms_.regsEqual(REG_A, REG_X) ? "sta " + ss.str() : "stx " + ss.str());
            }
            break;
        }
        case VRegAllocator::IN_FRAME: {
            std::string sym = "__vr" + std::to_string(vregId);
            if (alloc.type == ir::Type::I32) {
                emit("staxyz.fp " + sym);
            } else if (alloc.type == ir::Type::I8) {
                emit("sta.fp " + sym);
            } else if (valueByte_[1] == REG_Z) {
                emit("staz.fp " + sym);
            } else {
                emit("stax.fp " + sym);
            }
            break;
        }
    }
}

// Load only A from an operand (no X/Y/Z). For I8 use only.
void IRCodeGen::loadOperandA(const ir::Operand& op) {
    switch (op.kind) {
        case ir::OperandKind::VREG:
            loadVregA(op.vregId);
            break;
        case ir::OperandKind::IMM:
            emit("lda #" + std::to_string((int)(op.immVal & 0xFF)));
            break;
        default:
            loadOperand(op); // fallback
            break;
    }
}

void IRCodeGen::loadOperand(const ir::Operand& op) {
    switch (op.kind) {
        case ir::OperandKind::VREG:
            loadVreg(op.vregId);
            break;
        case ir::OperandKind::IMM:
            emit("lda #" + std::to_string((int)(op.immVal & 0xFF)));
            if (op.type == ir::Type::I32) {
                emit("ldx #" + std::to_string((int)((op.immVal >> 8) & 0xFF)));
                emit("ldy #" + std::to_string((int)((op.immVal >> 16) & 0xFF)));
                emit("ldz #" + std::to_string((int)((op.immVal >> 24) & 0xFF)));
            } else if (op.type != ir::Type::I8) {
                emit("ldx #" + std::to_string((int)((op.immVal >> 8) & 0xFF)));
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
        // Check for suppressed CONST vreg — use immediate instead of ZP
        auto cit = vregConstVal_.find(op.vregId);
        if (cit != vregConstVal_.end() && suppressedVregs_.count(op.vregId)) {
            return "#" + std::to_string((int)(cit->second & 0xFFFF));
        }
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
    if (op.kind == ir::OperandKind::GLOBAL) {
        // Load global address into scratch for use as memory operand
        emit("lda #<" + op.name);
        emit("sta __zp_scratch2");
        emit("lda #>" + op.name);
        emit("sta __zp_scratch2+1");
        return "__zp_scratch2";
    }
    return "#0";
}

// ============================================================================
// Module-level emission
// ============================================================================

void IRCodeGen::generate(const ir::Module& mod, uint32_t zpStart, bool relocMode, bool zpCallMode, bool emitReasons) {
    emitReasons_ = emitReasons;
    relocMode_ = relocMode;
    zpCallMode_ = zpCallMode;
    zeroPageStart_ = zpStart;
    sourceFile_ = mod.sourceFile;

    // Pre-scan all functions to compute frame offsets (needed for capture)
    std::map<std::string, const ir::Function*> funcMap;
    for (const auto& fn : mod.functions) {
        prescanFunction(fn);
        funcMap[fn.name] = &fn;
    }
    setFunctionMap(&funcMap);

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

        // --- Set Base Page register ---
        if (mod.setBP >= 0) {
            emitComment("Set B (base page) register to " + std::to_string(mod.setBP));
            emit("lda #" + std::to_string(mod.setBP));
            emit("tab");
        }

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
            // Save return value to $02/$03 (below save/restore range $08-$FF)
            emit("sta $02");
            emit("stx $03");
            emitComment("Restore ZP $08-$FF from BSS buffer");
            emit("ldx #0");
            emitLabel("@__zp_restore_loop");
            emit("lda __zp_save_buf,x");
            emit("sta $08,x");
            emit("inx");
            emit("cpx #248");
            emit("bne @__zp_restore_loop");
            // Reload return value
            emit("lda $02");
            emit("ldx $03");
        }
        emitLabel("__halt");
        emit("jmp __halt");
    }

    auto hex8 = [](uint32_t val) {
        std::stringstream ss;
        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (val & 0xFF);
        return ss.str();
    };

    std::string symDir = relocMode ? ".weak" : ".global";
    emit(symDir + " __static_chain");
    emit(symDir + " __zp_scratch");
    emit(symDir + " __zp_scratch2");
    emit(symDir + " __zp_scratch3");
    emit(symDir + " __zp_scratch4");
    emit("__static_chain = " + hex8(zeroPageStart_ - 2));
    emit("__zp_scratch = " + hex8(zeroPageStart_));
    emit("__zp_scratch2 = " + hex8(zeroPageStart_ + 2));
    emit("__zp_scratch3 = " + hex8(zeroPageStart_ + 4));
    emit("__zp_scratch4 = " + hex8(zeroPageStart_ + 6));
    emitBlank();

    // Emit extern declarations
    for (const auto& ext : mod.externs) {
        emit(".extern " + ext);
    }

    // Auto-emit float runtime externs if any float ops are used
    if (relocMode) {
        bool hasFloat = false;
        bool hasFloatArg = false;
        for (const auto& fn : mod.functions) {
            for (const auto& blk : fn.blocks) {
                for (const auto& inst : blk.insts) {
                    if (inst.op >= ir::Op::FADD && inst.op <= ir::Op::FCONST) {
                        hasFloat = true;
                    }
                    if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_INDIRECT || inst.op == ir::Op::CALL_VOID) {
                        for (const auto& arg : inst.args) {
                            if (arg.type == ir::Type::F32) { hasFloatArg = true; break; }
                        }
                    }
                    if (hasFloat && hasFloatArg) break;
                }
                if (hasFloat && hasFloatArg) break;
            }
            if (hasFloat && hasFloatArg) break;
        }
        if (hasFloat) {
            emit(".extern __float_a");
            emit(".extern __float_b");
            emit(".extern __float_add");
            emit(".extern __float_sub");
            emit(".extern __float_mul");
            emit(".extern __float_div");
            emit(".extern __float_neg");
            emit(".extern __float_cmp");
            emit(".extern __float_itof");
            emit(".extern __float_ftoi");
        }
        if (hasFloatArg) {
            emit(".extern _printf_float");
        }
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

    // ZP save buffer: label at end of program, no .res needed.
    // The 248 bytes after the program are unused RAM — written before read.
    if (mod.saveZP) {
        emitBlank();
        emitLabel("__zp_save_buf");
    }
}

void IRCodeGen::emitGlobals(const ir::Module& mod, bool relocMode) {
    // Deduplicate globals: extern decl + definition → keep definition only
    std::set<std::string> emittedGlobals;

    // Emit .global directives for non-static globals and functions
    if (relocMode) {
        for (const auto& g : mod.globals) {
            if (!g.isStatic && !emittedGlobals.count(g.name)) {
                emit(".global " + g.name);
                emittedGlobals.insert(g.name);
            }
        }
        for (const auto& fn : mod.functions) {
            if (!fn.isStatic) {
                if (fn.isWeak) emit(".weak " + fn.name);
                else emit(".global " + fn.name);
            }
        }
        emitBlank();
    }

    // Deduplicate globals for data/bss emission: if a name appears multiple times
    // (extern decl + definition), keep the last occurrence (the definition).
    std::map<std::string, size_t> globalLastIdx;
    for (size_t i = 0; i < mod.globals.size(); i++) {
        globalLastIdx[mod.globals[i].name] = i;
    }

    // Emit data section for initialized globals
    bool hasData = false;
    bool hasBss = false;
    for (size_t gi = 0; gi < mod.globals.size(); gi++) {
        const auto& g = mod.globals[gi];
        if (globalLastIdx[g.name] != gi) continue; // skip earlier duplicate
        if (g.hasInitValue) {
            // Initialized global: goes to data segment.
            // Must switch back from BSS if we were emitting uninitialized globals.
            if (!hasData || hasBss) {
                if (relocMode) {
                    emit(".segment \"data\"");
                    if (!hasData) {
                        // Pad byte: .o65 relocation encoding uses delta $00 as
                        // end-of-table, so relocations at segment offset 0 can't
                        // be encoded. A 1-byte pad ensures offset >= 1.
                        emit(".byte 0");
                    }
                }
                hasData = true;
                hasBss = false; // reset so next uninit global re-emits .segment "bss"
            }
            emitLabel(g.name);
            // Phase 3: Vtable — emit function address entries
            if (!g.vtableMethodNames.empty()) {
                for (size_t slot = 0; slot < g.vtableMethodNames.size(); slot++) {
                    const auto& methodName = g.vtableMethodNames[slot];
                    if (!methodName.empty()) {
                        emit(".word " + methodName);
                        // Phase 4: Emit .vtable_entry for linker devirtualization
                        if (relocMode) {
                            emit(".vtable_entry " + g.name + ", " + std::to_string(slot) + ", " + methodName);
                        }
                    } else {
                        emit(".word 0"); // empty vtable slot
                    }
                }
            } else if (!g.initList.empty()) {
                // Array initializer
                for (size_t i = 0; i < g.initList.size(); i++) {
                    // Check for symbolic label reference (e.g. string literal pointer)
                    if (i < g.initLabels.size() && !g.initLabels[i].empty()) {
                        emit(".word " + g.initLabels[i]);
                    } else if (g.type == ir::Type::I8) emit(".byte " + std::to_string((int)(g.initList[i] & 0xFF)));
                    else if (g.type == ir::Type::I32) emit(".dword " + std::to_string((int)g.initList[i]));
                    else emit(".word " + std::to_string((int)(g.initList[i] & 0xFFFF)));
                }
                // Handle partial initialization: padding with zeros
                int bytesEmitted = (int)g.initList.size() * ir::typeSize(g.type);
                if (bytesEmitted < g.size) {
                    emit(".res " + std::to_string(g.size - bytesEmitted));
                }
            } else if (!g.initLabels.empty() && !g.initLabels[0].empty()) {
                // Scalar symbolic reference (e.g. char *p = "hello")
                emit(".word " + g.initLabels[0]);
            } else {
                // Scalar initializer
                if (g.type == ir::Type::F32) {
                    // Convert stored double bits to CBM 40-bit float
                    double val; std::memcpy(&val, &g.initValue, sizeof(double));
                    uint8_t cbm[5]; doubleToCBM40(val, cbm);
                    emit(".byte $" + hex8(cbm[0]) + ", $" + hex8(cbm[1]) + ", $" +
                         hex8(cbm[2]) + ", $" + hex8(cbm[3]) + ", $" + hex8(cbm[4]));
                } else if (g.type == ir::Type::I8) {
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
            // Uninitialized global: goes to bss segment
            if (!hasBss) {
                if (hasData) emitBlank();
                if (relocMode) emit(".segment \"bss\"");
                hasBss = true;
            }
            emitLabel(g.name);
            emit(".res " + std::to_string(g.size > 0 ? g.size : ir::typeSize(g.type)));
        }
    }
    if (hasData || hasBss) emitBlank();

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
        if (sl.encoding == 1) {
            emit(".ascii \"" + sl.value + "\"");
        } else if (sl.encoding == 2) {
            emit(".screencode \"" + sl.value + "\"");
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
                case ir::Op::MUL_U:
                case ir::Op::DIV:
                case ir::Op::DIV_U:
                case ir::Op::MOD:
                case ir::Op::MOD_U:
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
                case ir::Op::DEREF:
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
                    fc.flags |= C | N | ZF;
                    break;

                case ir::Op::CPU_REG_READ:
                    fc.regs |= A | X; // result goes to AX
                    if (inst.resultType == ir::Type::I32) fc.regs |= Y | Z;
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::CPU_REG_WRITE:
                    fc.regs |= A | X | Y | Z; // conservative write
                    fc.flags |= N | ZF;
                    break;

                case ir::Op::CPU_FLAG_READ:
                    fc.regs |= A; // lda #0; adc #0
                    fc.flags |= C | N | ZF | V;
                    break;

                case ir::Op::CPU_FLAG_WRITE:
                    fc.regs |= A; // lda val
                    fc.flags |= C | N | ZF | V;
                    break;
            }
        }
    }

    return fc;
}

void IRCodeGen::emitFunction(const ir::Function& fn, bool relocMode, bool isMainWithZPSave) {
    emitComment("function " + fn.name);

    // Re-run prescan to restore vregOffset_ for this function
    prescanFunction(fn);

    // Run register allocator
    alloc_.analyze(fn);

    // Copy local slot info from IR function
    localSlotVregs_ = fn.localSlotVregs;
    vregConstVal_.clear();
    vregSizes_ = fn.vregSizes;

    // Pre-scan: identify CONST vregs and detect which are only used as
    // STORE addresses. These can be suppressed (no emit, no frame slot)
    // because the codegen uses the constant value directly for sta $ADDR.
    suppressedVregs_.clear();
    {
        // Pass 1: collect all CONST vreg values
        std::map<uint32_t, int64_t> constDefs;
        for (const auto& blk : fn.blocks) {
            for (const auto& inst : blk.insts) {
                if (inst.op == ir::Op::CONST && inst.dest.isVreg())
                    constDefs[inst.dest.vregId] = inst.src1.immVal;
            }
        }
        // Pass 2: identify CALL instructions where all args are simple
        // (const vregs or immediates). Mark those args' local slot vregs as dead.
        auto isSimpleArg = [&](const ir::Operand& arg) -> bool {
            if (arg.isImm()) return true;
            if (arg.kind == ir::OperandKind::GLOBAL) return true;
            if (arg.isVreg() && constDefs.count(arg.vregId)) return true;
            return false;
        };
        std::set<uint32_t> deadCallArgSlots; // local slots only used by phw-eligible calls
        for (const auto& blk : fn.blocks) {
            for (const auto& inst : blk.insts) {
                if (inst.op != ir::Op::CALL && inst.op != ir::Op::CALL_VOID) continue;
                if (inst.callConv == ir::CallConv::ZP) continue;
                bool allSimple = true;
                for (const auto& a : inst.args)
                    if (!isSimpleArg(a)) { allSimple = false; break; }
                if (allSimple) {
                    for (const auto& a : inst.args)
                        if (a.isVreg()) deadCallArgSlots.insert(a.vregId);
                }
            }
        }

        // Pass 3: for each CONST vreg, check if it can be suppressed.
        // Suppressible if ONLY used as:
        //   (a) src2 of STORE (address operand, not local slot), OR
        //   (b) src1 of STORE where src2 is a known-constant address, OR
        //   (c) arg of an all-simple CALL (phw path), OR
        //   (d) src1 of STORE to a local slot that is a dead call arg slot
        for (auto& [vid, val] : constDefs) {
            bool canSuppress = true;
            bool hasUse = false;
            for (const auto& blk : fn.blocks) {
                for (const auto& inst : blk.insts) {
                    if (inst.op == ir::Op::NOP) continue;
                    // Check args: OK if CALL with all-simple args
                    for (const auto& a : inst.args) {
                        if (a.isVreg() && a.vregId == vid) {
                            if ((inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) &&
                                deadCallArgSlots.count(vid)) {
                                hasUse = true;
                            } else {
                                canSuppress = false;
                            }
                            break;
                        }
                    }
                    if (!canSuppress) break;
                    // Check src1
                    if (inst.src1.isVreg() && inst.src1.vregId == vid) {
                        if (inst.op == ir::Op::STORE && inst.src2.isVreg()) {
                            if (constDefs.count(inst.src2.vregId) &&
                                !fn.localSlotVregs.count(inst.src2.vregId)) {
                                hasUse = true; // value of const-addr store
                            } else if (fn.localSlotVregs.count(inst.src2.vregId) &&
                                       deadCallArgSlots.count(inst.src2.vregId)) {
                                hasUse = true; // store to dead call arg slot
                            } else {
                                canSuppress = false; break;
                            }
                        } else {
                            canSuppress = false; break;
                        }
                    }
                    // Check src2: OK if STORE address (not local slot), CMP, or ADD/SUB operand
                    if (inst.src2.isVreg() && inst.src2.vregId == vid) {
                        if (inst.op == ir::Op::STORE && !fn.localSlotVregs.count(vid)) {
                            hasUse = true;
                        } else if (inst.op == ir::Op::CMP_EQ || inst.op == ir::Op::CMP_NE ||
                                   inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_LE ||
                                   inst.op == ir::Op::CMP_GT || inst.op == ir::Op::CMP_GE ||
                                   inst.op == ir::Op::CMP_LTU || inst.op == ir::Op::CMP_LEU ||
                                   inst.op == ir::Op::CMP_GTU || inst.op == ir::Op::CMP_GEU) {
                            hasUse = true; // CMP already uses vregConstVal_ for immediate
                        } else if (inst.op == ir::Op::ADD || inst.op == ir::Op::SUB) {
                            hasUse = true; // store-fused ADD/SUB uses vregConstVal_ for immediate
                        } else {
                            canSuppress = false; break;
                        }
                    }
                }
                if (!canSuppress) break;
            }
            if (canSuppress && hasUse) {
                suppressedVregs_.insert(vid);
                vregConstVal_[vid] = val;
            }
        }
        // Also suppress the dead call arg LOCAL SLOT vregs themselves
        for (uint32_t slotId : deadCallArgSlots) {
            if (fn.localSlotVregs.count(slotId)) {
                suppressedVregs_.insert(slotId);
            }
        }
    }

    // Prescan to compute vregSizes_ and initial frame layout
    prescanFunction(fn);

    // Override: only allocate frame slots for vRegs assigned to frame by allocator.
    // Use prescanFunction's localNames order (alphabetical) for deterministic offsets.
    resetFrame();
    // First: allocate named locals that are in frame, in localNames order (deterministic)
    for (const auto& [name, vid] : fn.localNames) {
        if (suppressedVregs_.count(vid)) continue;
        auto alloc = alloc_.getAlloc(vid);
        if (alloc.loc == VRegAllocator::IN_FRAME) {
            allocSlot(vid, alloc.type);
        }
    }
    // Then: allocate remaining frame vRegs (temporaries) in live-range order
    for (const auto& lr : alloc_.liveRanges()) {
        if (suppressedVregs_.count(lr.vregId)) continue;
        if (vregOffset_.count(lr.vregId)) continue; // already allocated above
        auto alloc = alloc_.getAlloc(lr.vregId);
        if (alloc.loc == VRegAllocator::IN_FRAME) {
            allocSlot(lr.vregId, lr.type);
        }
    }

    currentInstIdx_ = 0;

    // __naked: emit only proc label + body + endproc, no prologue/epilogue
    if (fn.isNaked) {
        emit("proc " + fn.name);
        for (const auto& block : fn.blocks) {
            emitLabel("@" + block.label);
            for (const auto& inst : block.insts) {
                emitInst(inst);
                currentInstIdx_++;
            }
        }
        emit("endproc");
        emitBlank();
        return;
    }

    // proc directive with parameter specs
    // For regparm, first param is in A/AX — not on stack, not in proc line
    std::string procLine = "proc " + fn.name;
    for (size_t i = 0; i < fn.paramTypes.size(); i++) {
        if (fn.isRegparm && i == 0) continue; // first param in register
        procLine += ", ";
        int pSize = ir::typeSize(fn.paramTypes[i]);
        if (pSize == 1) procLine += "B";
        else if (pSize == 4) procLine += "D";
        else procLine += "W";
        if (i < fn.paramNames.size() && !fn.paramNames[i].empty()) {
            procLine += "#@_p_" + fn.paramNames[i];
        } else {
            procLine += "#@_p_" + std::to_string(i);
        }
    }
    emit(procLine);

    // Reset source location tracking for this function
    lastLocLine_ = -1;
    lastLocFile_.clear();

    // __interrupt: save all registers at entry
    if (fn.isInterrupt) {
        emit("pha"); emit("phx"); emit("phy"); emit("phz");
    }

    // Frame pointer variable
    emit(".var _fp = 0");

    // Emit .loc for function declaration line (before prologue and param loading,
    // so objdump attributes setup code to the function signature, not the first statement)
    if (fn.declLine > 0 && !sourceFile_.empty()) {
        // Use line mapping if available to find correct source file for this line
        std::string locFile = sourceFile_;
        int displayLine = fn.declLine;

        if (!lineToFileMap_.empty()) {
            // Find the context that applies to this line
            for (auto it = lineToFileMap_.rbegin(); it != lineToFileMap_.rend(); ++it) {
                if (it->first <= fn.declLine) {
                    const auto& context = it->second;
                    locFile = context.first;
                    // Convert absolute line to file-relative line
                    displayLine = context.second + (fn.declLine - it->first);
                    break;
                }
            }
        }

        emit(".loc \"" + locFile + "\", " + std::to_string(displayLine));
        lastLocFile_ = locFile;
        lastLocLine_ = displayLine;
    }

    // Allocate frame only for frame-allocated vRegs
    int localFrameSize = frameSize_;
    localFrameSize_ = localFrameSize;  // save for RET cleanup
    if (localFrameSize > 128) {
        // Frame + params + return addr + dynamic pushes must fit in uint8_t offsets
        std::cerr << fn.name << ": warning: large frame (" << localFrameSize
                  << " bytes) may cause incorrect stack-relative addressing\n";
    }
    if (localFrameSize > 0) {
        emitComment("frame: " + std::to_string(localFrameSize) + " bytes (frame-allocated vRegs only)");
        for (int i = 0; i < localFrameSize; i += 2) {
            emit("phw #0");
        }
    }

    // Set up ZP frame pointer for stack-relative access (only for stack calling convention)
    // FP = __sp_base + SPL at function entry. Store in $FD/$FE.
    // This maintains the frame pointer for use by inline assembly or future optimizations.
    useStackParams_ = !zpCallMode_ || fn.isVariadic;
    if (useStackParams_) {
        emit("tsx");
        emit("txa");
        emit("clc");
        emit("adc #1");
        emit("sta $FD");
        emit("lda #$01");
        emit("adc #0");
        emit("sta $FE");
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
    bool useStackParams = !zpCallMode_ || fn.isVariadic;
    if (useStackParams) {
        // Stack convention (or variadic in zpCall mode): args pushed right-to-left,
        // first param is closest to SP (smallest offset past frame + return address).
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
    if (useStackParams) {
        // Stack convention: params are on the stack, copy to ZP temps
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            uint32_t vid = (uint32_t)i;
            if (fn.isNested && i == fn.paramTypes.size() - 1 && fn.staticLinkVreg != -1) {
                // vid = (uint32_t)fn.staticLinkVreg; // Wait! No more hidden param!
            }

            if (fn.isRegparm && i == 0) {
                // First param already in A (I8) or AX (I16) — just store
                storeVreg(vid);
                continue;
            }
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            if (fn.paramTypes[i] == ir::Type::F32) {
                // 5-byte float: copy from stack frame to ZP vreg
                auto alloc = alloc_.getAlloc(vid);
                std::string za = "$" + hex8((uint8_t)alloc.offset);
                for (int bi = 0; bi < 5; bi++) {
                    emit("lda.fp @_p_" + pName + "+" + std::to_string(bi));
                    emit("sta " + za + "+" + std::to_string(bi));
                }
            } else if (fn.paramTypes[i] == ir::Type::I32) {
                emit("ldaxyz.fp @_p_" + pName);
                storeVreg(vid);
            } else {
                emit("ldax.fp @_p_" + pName);
                storeVreg(vid);
            }
        }
    } else {
        // ZP call convention: params already in ZP block, copy to vReg slots
        for (size_t i = 0; i < fn.paramTypes.size(); i++) {
            uint32_t vid = (uint32_t)i;
            if (fn.isRegparm && i == 0) {
                // First param already in A (I8) or AX (I16) — just store
                storeVreg(vid);
                continue;
            }
            std::string pName = (i < fn.paramNames.size() && !fn.paramNames[i].empty())
                ? fn.paramNames[i] : std::to_string(i);
            // ZP params are at $10 + offset; load from the .var @_p_ location
            emit("lda @_p_" + pName);
            emit("ldx @_p_" + pName + "+1");
            storeVreg(vid);
        }
    }

    // Emit blocks (with instruction index tracking for allocator queries)
    currentInstIdx_ = 0;
    currentFn_ = &fn;
    for (size_t bi = 0; bi < fn.blocks.size(); bi++) {
        const auto& block = fn.blocks[bi];
        currentBlockIdx_ = bi;
        // Track the next block's label for no-op branch elimination
        nextBlockLabel_ = (bi + 1 < fn.blocks.size()) ? fn.blocks[bi + 1].label : "__return";
        emitLabel("@" + block.label);
        for (size_t ii = 0; ii < block.insts.size(); ii++) {
            currentInstInBlock_ = ii;
            emitInst(block.insts[ii]);
            currentInstIdx_++;
        }
    }
    nextBlockLabel_.clear();

    // Common return point — clean up frame, then endproc emits rts
    emitLabel("@__return");
    if (localFrameSize > 0) {
        int retSize = ir::typeSize(fn.returnType);
        if (retSize >= 4) {
            // I32: return value in A/X/Y/Z — Z carries byte 3, save it
            std::string restoreLabel = "@__restore_epilogue_z_" + std::to_string(labelCounter_++);
            emit("stz " + restoreLabel + "+1");
            for (int i = 0; i < localFrameSize; i++) emit("plz");
            emitLabel(restoreLabel);
            emit("ldz #0");
        } else {
            // VOID/I8/I16: PLZ doesn't clobber A, X, or Y
            for (int i = 0; i < localFrameSize; i++) emit("plz");
        }
    }

    // __interrupt: restore registers and return with RTI
    if (fn.isInterrupt) {
        emit("plz"); emit("ply"); emit("plx"); emit("pla");
        emit("rti");
    }

    // Function attribute directives with per-function clobber analysis
    auto fc = computeFuncClobbers(fn);
    {
        std::string funcFlags = (zpCallMode_ && !fn.isVariadic) ? "zp_call" : "stack_call";
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
    // Reset value-role tracking for each new instruction
    clearValueRoles();

    // Emit .loc directive when source location changes
    if (inst.loc.valid() && (inst.loc.line != lastLocLine_ || inst.loc.file != lastLocFile_)) {
        lastLocLine_ = inst.loc.line;
        lastLocFile_ = inst.loc.file;
        emit(".loc \"" + inst.loc.file + "\", " + std::to_string(inst.loc.line));
    }

    // Build IR instruction description for -Rcodegen annotations
    auto irDesc = [&](const std::string& extra = "") -> std::string {
        if (!emitReasons_) return "";
        std::string name;
        switch (inst.op) {
            case ir::Op::NOP: name = "NOP"; break;
            case ir::Op::CONST: name = "CONST"; break;
            case ir::Op::ADD: name = "ADD"; break;
            case ir::Op::SUB: name = "SUB"; break;
            case ir::Op::MUL: name = "MUL"; break;
            case ir::Op::MUL_U: name = "MUL_U"; break;
            case ir::Op::DIV: name = "DIV"; break;
            case ir::Op::DIV_U: name = "DIV_U"; break;
            case ir::Op::MOD: name = "MOD"; break;
            case ir::Op::MOD_U: name = "MOD_U"; break;
            case ir::Op::AND: name = "AND"; break;
            case ir::Op::OR: name = "OR"; break;
            case ir::Op::XOR: name = "XOR"; break;
            case ir::Op::NOT: name = "NOT"; break;
            case ir::Op::NEG: name = "NEG"; break;
            case ir::Op::SHL: name = "SHL"; break;
            case ir::Op::SHR: name = "SHR"; break;
            case ir::Op::ASR: name = "ASR"; break;
            case ir::Op::CMP_EQ: name = "CMP_EQ"; break;
            case ir::Op::CMP_NE: name = "CMP_NE"; break;
            case ir::Op::CMP_LT: name = "CMP_LT"; break;
            case ir::Op::CMP_LE: name = "CMP_LE"; break;
            case ir::Op::CMP_GT: name = "CMP_GT"; break;
            case ir::Op::CMP_GE: name = "CMP_GE"; break;
            case ir::Op::CMP_LTU: name = "CMP_LTU"; break;
            case ir::Op::CMP_LEU: name = "CMP_LEU"; break;
            case ir::Op::CMP_GTU: name = "CMP_GTU"; break;
            case ir::Op::CMP_GEU: name = "CMP_GEU"; break;
            case ir::Op::LOAD: name = "LOAD"; break;
            case ir::Op::STORE: name = "STORE"; break;
            case ir::Op::ADDR_GLOBAL: name = "ADDR_GLOBAL"; break;
            case ir::Op::ADDR_LOCAL: name = "ADDR_LOCAL"; break;
            case ir::Op::ADDR_ELEM: name = "ADDR_ELEM"; break;
            case ir::Op::TRUNC: name = "TRUNC"; break;
            case ir::Op::ZEXT: name = "ZEXT"; break;
            case ir::Op::SEXT: name = "SEXT"; break;
            case ir::Op::BR: name = "BR"; break;
            case ir::Op::BR_COND: name = "BR_COND"; break;
            case ir::Op::CALL: name = "CALL"; break;
            case ir::Op::CALL_VOID: name = "CALL_VOID"; break;
            case ir::Op::RET: name = "RET"; break;
            case ir::Op::RET_VOID: name = "RET_VOID"; break;
            case ir::Op::COPY: name = "COPY"; break;
            case ir::Op::DEREF: name = "DEREF"; break;
            default: name = "OP" + std::to_string((int)inst.op); break;
        }
        std::string s = name;
        if (inst.dest.isVreg()) s += " → " + vregDesc(inst.dest.vregId);
        if (!extra.empty()) s += " " + extra;
        return s;
    };

    // Reset store-forwarding state unless this is a STORE that will consume it
    if (inst.op != ir::Op::STORE && resultInAX_ != -1) {
        resultInAX_ = -1;
    }

    switch (inst.op) {
        case ir::Op::NOP:
            break;

        case ir::Op::CONST: {
            int val = (int)inst.src1.immVal;
            if (inst.dest.isVreg()) vregConstVal_[inst.dest.vregId] = val;
            // Skip emission for suppressed CONST vregs (address-only, used by direct store)
            if (inst.dest.isVreg() && suppressedVregs_.count(inst.dest.vregId)) break;

            // --- CONST direct store optimization ---
            // If the next instruction is STORE of this CONST's result to a ZP local,
            // store directly to the local's ZP address without round-tripping through temp.
            if (inst.dest.isVreg() && (inst.resultType == ir::Type::I16 || inst.resultType == ir::Type::I8)) {
                auto* nextInst = peekNextInst();
                if (nextInst && nextInst->op == ir::Op::STORE &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && localSlotVregs_.count(nextInst->src2.vregId)) {
                    auto destAlloc = alloc_.getAlloc(nextInst->src2.vregId);
                    if (destAlloc.loc == VRegAllocator::IN_ZP) {
                        std::string r = irDesc("val=" + std::to_string(val) + " → direct store");
                        uint8_t b0 = val & 0xFF;
                        uint8_t b1 = (val >> 8) & 0xFF;
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << destAlloc.offset;
                        emit("lda #" + std::to_string((int)b0), r);
                        emit("sta " + ss.str());
                        if (inst.resultType != ir::Type::I8) {
                            ss.str(""); ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (destAlloc.offset + 1);
                            if (b1 == b0) {
                                emit("sta " + ss.str());
                            } else {
                                emit("ldx #" + std::to_string((int)b1));
                                emit("stx " + ss.str());
                            }
                        }
                        // Mark that the next STORE should be skipped
                        resultInAX_ = -2; // special: means "next STORE already handled"
                        ms_.invalidateAll();
                        break;
                    }
                    if (destAlloc.loc == VRegAllocator::IN_FRAME && inst.resultType == ir::Type::I16) {
                        // Direct store to frame via staz.fp — load hi into Z, skip X entirely
                        std::string r = irDesc("val=" + std::to_string(val) + " → direct frame store");
                        std::string sym = "__vr" + std::to_string(nextInst->src2.vregId);
                        uint8_t b0 = val & 0xFF;
                        uint8_t b1 = (val >> 8) & 0xFF;
                        emit("lda #" + std::to_string((int)b0), r);
                        if (b1 == b0) {
                            emit("taz", r);
                        } else {
                            emit("ldz #" + std::to_string((int)b1), r);
                        }
                        emit("staz.fp " + sym, r);
                        resultInAX_ = -2;
                        ms_.invalidateAll();
                        break;
                    }
                }
            }

            std::string r = irDesc("val=" + std::to_string(val));
            uint8_t b0 = val & 0xFF;
            uint8_t b1 = (val >> 8) & 0xFF;
            uint8_t b2 = (val >> 16) & 0xFF;
            uint8_t b3 = (val >> 24) & 0xFF;
            ms_.setConst(REG_A, b0);
            emit("lda #" + std::to_string((int)b0), r);
            valueByte_[0] = REG_A;
            // Can skip tax/tay/taz when byte matches A only if storeVreg
            // uses per-byte stores (IN_ZP). Frame stores use stax.fp/staxyz.fp
            // which require actual register values in X (or X/Y/Z for I32).
            bool canSkipTransfer = inst.dest.isVreg() &&
                alloc_.getAlloc(inst.dest.vregId).loc == VRegAllocator::IN_ZP;
            if (inst.resultType == ir::Type::I32) {
                if (b1 == b0 && canSkipTransfer) { ms_.setConst(REG_X, b1); } else if (b1 == b0) { emit("tax", r); ms_.setTransfer(REG_X, REG_A); valueByte_[1] = REG_X; } else { emit("ldx #" + std::to_string((int)b1), r); ms_.setConst(REG_X, b1); valueByte_[1] = REG_X; }
                if (b2 == b0 && canSkipTransfer) { ms_.setConst(REG_Y, b2); } else if (b2 == b0) { emit("tay", r); ms_.setTransfer(REG_Y, REG_A); valueByte_[2] = REG_Y; } else { emit("ldy #" + std::to_string((int)b2), r); ms_.setConst(REG_Y, b2); valueByte_[2] = REG_Y; }
                if (b3 == b0 && canSkipTransfer) { ms_.setConst(REG_Z, b3); } else if (b3 == b0) { emit("taz", r); ms_.setTransfer(REG_Z, REG_A); valueByte_[3] = REG_Z; } else { emit("ldz #" + std::to_string((int)b3), r); ms_.setConst(REG_Z, b3); valueByte_[3] = REG_Z; }
            } else if (inst.resultType != ir::Type::I8) {
                if (b1 == b0 && canSkipTransfer) {
                    ms_.setConst(REG_X, b1);
                    // X not actually loaded — valueByte_[1] stays VB_NONE
                    // storeVreg for IN_ZP uses sta for both bytes when A==X
                } else if (b1 == b0) {
                    emit("tax", r); ms_.setTransfer(REG_X, REG_A);
                    valueByte_[1] = REG_X;
                } else {
                    emit("ldx #" + std::to_string((int)b1), r); ms_.setConst(REG_X, b1);
                    valueByte_[1] = REG_X;
                }
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            // stax.fp clobbers X via TSX — don't claim result is in AX after frame store
            if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                resultInAX_ = (int32_t)inst.dest.vregId;
            else
                resultInAX_ = -1;
            ms_.invalidateAll(); // conservative: reset after CONST+store
            break;
        }

        case ir::Op::ADD:
        case ir::Op::SUB: {
            // --- I16 INC/DEC peephole for local += 1 / local -= 1 ---
            // Pattern: %n = add/sub i16 %local, 1; store %n, %local
            // Emit: inc $ZP; bne +2; inc $ZP+1 (or dec variant)
            // Check if src2 is immediate 1, or a CONST vreg holding 1
            bool src2IsOne = (inst.src2.isImm() && inst.src2.immVal == 1);
            if (!src2IsOne && inst.src2.isVreg()) {
                auto cit = vregConstVal_.find(inst.src2.vregId);
                if (cit != vregConstVal_.end() && cit->second == 1) src2IsOne = true;
            }
            if (inst.resultType == ir::Type::I16 && src2IsOne &&
                inst.src1.isVreg() && inst.dest.isVreg()) {
                auto* nextInst = peekNextInst();
                if (nextInst && nextInst->op == ir::Op::STORE &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && nextInst->src2.vregId == inst.src1.vregId &&
                    localSlotVregs_.count(nextInst->src2.vregId)) {
                    auto srcAlloc = alloc_.getAlloc(inst.src1.vregId);
                    if (srcAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream lo, hi;
                        lo << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << srcAlloc.offset;
                        hi << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (srcAlloc.offset + 1);
                        std::string r = irDesc(inst.op == ir::Op::ADD ? "inc16 ZP" : "dec16 ZP");
                        if (inst.op == ir::Op::ADD) {
                            emit("inc " + lo.str(), r);
                            emit("bne *+4");
                            emit("inc " + hi.str());
                        } else {
                            emit("lda " + lo.str(), r);
                            emit("bne *+4");
                            emit("dec " + hi.str());
                            emit("dec " + lo.str());
                        }
                        resultInAX_ = -2;
                        ms_.invalidateAll();
                        break;
                    }
                    if (srcAlloc.loc == VRegAllocator::IN_FRAME) {
                        std::string sym = "__vr" + std::to_string(inst.src1.vregId);
                        std::string r = irDesc(inst.op == ir::Op::ADD ? "inc16 frame" : "dec16 frame");
                        emit(inst.op == ir::Op::ADD ? "inc.16f " + sym : "dec.16f " + sym, r);
                        resultInAX_ = -2;
                        ms_.invalidateAll();
                        break;
                    }
                }
            }

            // I8 fast path: use native 8-bit add/sub
            if (inst.resultType == ir::Type::I8) {
                // Check for inc a / dec a optimization
                bool isInc = (inst.op == ir::Op::ADD && inst.src2.isImm() && inst.src2.immVal == 1);
                bool isDec = (inst.op == ir::Op::SUB && inst.src2.isImm() && inst.src2.immVal == 1);
                loadOperandA(inst.src1);
                if (isInc) {
                    emit("inc a");
                } else if (isDec) {
                    emit("dec a");
                } else {
                    std::string src2 = src2MemOperand(inst.src2);
                    if (inst.op == ir::Op::ADD) {
                        emit("clc");
                        emit("adc " + src2);
                    } else {
                        emit("sec");
                        emit("sbc " + src2);
                    }
                }
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                // stax.fp clobbers X via TSX — don't claim result is in AX after frame store
                if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                    resultInAX_ = (int32_t)inst.dest.vregId;
                else
                    resultInAX_ = -1;
                break;
            }

            // For commutative ADD: if src1 is in frame (would clobber scratch),
            // swap operands — load src1 as memory, src2 into AX
            bool src1InFrame = inst.src1.isVreg() &&
                alloc_.getAlloc(inst.src1.vregId).loc == VRegAllocator::IN_FRAME;
            bool src2InFrame = inst.src2.isVreg() &&
                alloc_.getAlloc(inst.src2.vregId).loc == VRegAllocator::IN_FRAME;

            // --- I16 add/sub store-fused peephole ---
            // When result is immediately stored to a ZP local, avoid pha/txa/adc/tax/pla.
            // Load only lo byte of src1, do lo add+store, then load hi byte of src1
            // directly (skip ldx + txa):
            //   lda src1_lo; clc; adc src2_lo; sta dest_lo; lda src1_hi; adc src2_hi; sta dest_hi
            // Store-fused check: can we emit inline add and store directly to ZP?
            bool addSubFused = false;
            if (inst.resultType == ir::Type::I16 && inst.dest.isVreg() && !src1InFrame && !src2InFrame) {
                // Check 1: next instruction is STORE to a ZP local
                auto* nextInst = peekNextInst();
                bool storeToLocal = nextInst && nextInst->op == ir::Op::STORE &&
                    nextInst->src1.isVreg() && nextInst->src1.vregId == inst.dest.vregId &&
                    nextInst->src2.isVreg() && localSlotVregs_.count(nextInst->src2.vregId) &&
                    alloc_.getAlloc(nextInst->src2.vregId).loc == VRegAllocator::IN_ZP;
                // Check 2: dest vreg itself is ZP-allocated (result stored to own slot)
                bool destIsZP = alloc_.getAlloc(inst.dest.vregId).loc == VRegAllocator::IN_ZP;

                VRegAllocator::Allocation destAlloc;
                if (storeToLocal) {
                    destAlloc = alloc_.getAlloc(nextInst->src2.vregId);
                } else if (destIsZP) {
                    destAlloc = alloc_.getAlloc(inst.dest.vregId);
                }

                if (storeToLocal || destIsZP) {
                    if (destAlloc.loc == VRegAllocator::IN_ZP) {
                        // Determine src1 hi byte source
                        std::string src1hi;
                        if (inst.src1.isImm()) {
                            src1hi = "#" + std::to_string((int)((inst.src1.immVal >> 8) & 0xFF));
                        } else if (inst.src1.isVreg()) {
                            auto s1alloc = alloc_.getAlloc(inst.src1.vregId);
                            if (s1alloc.loc == VRegAllocator::IN_ZP) {
                                std::stringstream ss;
                                ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (s1alloc.offset + 1);
                                src1hi = ss.str();
                            }
                        }
                        if (!src1hi.empty()) {
                            std::string r = irDesc("store-fused add/sub.16");
                            // Use immediate for CONST vreg src2
                            std::string src2mem;
                            if (inst.src2.isVreg()) {
                                auto cit = vregConstVal_.find(inst.src2.vregId);
                                if (cit != vregConstVal_.end()) {
                                    src2mem = "#" + std::to_string((int)(cit->second & 0xFFFF));
                                } else {
                                    src2mem = src2MemOperand(inst.src2);
                                }
                            } else {
                                src2mem = src2MemOperand(inst.src2);
                            }
                            // Load only lo byte of src1 (no ldx for hi — we use lda src1hi later)
                            if (inst.src1.isImm()) {
                                emit("lda #" + std::to_string((int)(inst.src1.immVal & 0xFF)), r);
                            } else if (inst.src1.isVreg()) {
                                auto s1a = alloc_.getAlloc(inst.src1.vregId);
                                if (s1a.loc == VRegAllocator::IN_ZP) {
                                    std::stringstream ss;
                                    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << s1a.offset;
                                    emit("lda " + ss.str(), r);
                                } else {
                                    loadOperand(inst.src1); // fallback: loads A:X
                                }
                            } else {
                                loadOperand(inst.src1);
                            }
                            std::stringstream dlo, dhi;
                            dlo << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << destAlloc.offset;
                            dhi << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (destAlloc.offset + 1);
                            emit(inst.op == ir::Op::ADD ? "clc" : "sec");
                            emit(inst.op == ir::Op::ADD ? "adc " + src2mem : "sbc " + src2mem);
                            emit("sta " + dlo.str());
                            emit("lda " + src1hi);
                            // Hi byte of src2
                            if (src2mem[0] == '#') {
                                int64_t immVal = 0;
                                try { immVal = std::stol(src2mem.substr(1)); } catch (...) {}
                                emit(inst.op == ir::Op::ADD ?
                                    "adc #" + std::to_string((int)((immVal >> 8) & 0xFF)) :
                                    "sbc #" + std::to_string((int)((immVal >> 8) & 0xFF)));
                            } else {
                                emit(inst.op == ir::Op::ADD ? "adc " + src2mem + "+1" : "sbc " + src2mem + "+1");
                            }
                            emit("sta " + dhi.str());
                            if (storeToLocal) resultInAX_ = -2; // skip next STORE
                            ms_.invalidateAll();
                            addSubFused = true;
                        }
                    }
                }
            }
            if (addSubFused) break;

            std::string mn;
            if (inst.resultType == ir::Type::I32) {
                mn = (inst.op == ir::Op::ADD) ? "add.32" : "sub.32";
            } else {
                mn = (inst.op == ir::Op::ADD) ? "add.16" : "sub.16";
            }
            std::string reg = (inst.resultType == ir::Type::I32) ? ".AXYZ" : ".AX";

            if (inst.resultType == ir::Type::I32 && src1InFrame && src2InFrame) {
                // Both I32 operands in frame: ldaxyz.fp clobbers __zp_scratch2/__zp_scratch3.
                // Load src2 first, save to safe ZP area ($10-$13), then load src1 into AXYZ.
                auto hex8 = [](int v) { std::stringstream s; s << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << v; return s.str(); };
                int safeZP = (int)zeroPageStart_ + 8; // past scratch area
                loadOperand(inst.src2);
                emit("sta " + hex8(safeZP));
                emit("stx " + hex8(safeZP + 1));
                emit("sty " + hex8(safeZP + 2));
                emit("stz " + hex8(safeZP + 3));
                loadOperand(inst.src1);
                emit(mn + " " + reg + ", " + hex8(safeZP));
            } else if (inst.op == ir::Op::ADD && src2InFrame && !src1InFrame) {
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
            // stax.fp clobbers X via TSX — don't claim result is in AX after frame store
            if (inst.dest.isVreg() && alloc_.getAlloc(inst.dest.vregId).loc != VRegAllocator::IN_FRAME)
                resultInAX_ = (int32_t)inst.dest.vregId;
            else
                resultInAX_ = -1;
            break;
        }

        case ir::Op::AND:
        case ir::Op::OR:
        case ir::Op::XOR: {
            // I8 fast path: use native 8-bit bitwise ops
            if (inst.resultType == ir::Type::I8) {
                // Check for single-bit AND test fused with BR_COND → BBS/BBR
                bool canFuse = false;
                if (inst.dest.isVreg() && currentFn_) {
                    auto& blk = currentFn_->blocks[currentBlockIdx_];
                    size_t instIdx = &inst - &blk.insts[0];
                    if (instIdx + 1 < blk.insts.size()) {
                        const auto& next = blk.insts[instIdx + 1];
                        if (next.op == ir::Op::BR_COND && next.src1.isVreg() &&
                            next.src1.vregId == inst.dest.vregId) {
                            canFuse = true;
                        }
                    }
                }

                // Single-bit AND with fused branch: use BBS/BBR if source is in ZP
                // Check both immediate operands and vreg operands holding known constants
                int64_t andMask = -1;
                if (inst.src2.isImm()) {
                    andMask = inst.src2.immVal;
                } else if (inst.src2.isVreg()) {
                    // Check if the vreg was defined by a CONST in this block
                    auto& blk = currentFn_->blocks[currentBlockIdx_];
                    size_t instIdx = &inst - &blk.insts[0];
                    for (size_t k = 0; k < instIdx; k++) {
                        auto& prev = blk.insts[k];
                        if (prev.op == ir::Op::CONST && prev.dest.isVreg() &&
                            prev.dest.vregId == inst.src2.vregId) {
                            andMask = prev.src1.immVal;
                        }
                    }
                }
                if (canFuse && inst.op == ir::Op::AND && andMask >= 0) {
                    int64_t mask = andMask;
                    int bitN = -1;
                    if (mask > 0 && (mask & (mask - 1)) == 0) {
                        // Power of 2 — find bit number
                        for (int b = 0; b < 8; b++) {
                            if (mask == (1 << b)) { bitN = b; break; }
                        }
                    }
                    if (bitN >= 0 && inst.src1.isVreg()) {
                        auto srcAlloc = alloc_.getAlloc(inst.src1.vregId);
                        if (srcAlloc.loc == VRegAllocator::IN_ZP) {
                            // BBS/BBR: test bit directly in ZP, branch without touching registers
                            std::string zpAddr = "$" + hex8((uint8_t)srcAlloc.offset);
                            // Record for BR_COND to emit bbs/bbr
                            lastCmp_ = {ir::Op::CMP_NE, inst.dest.vregId, true};
                            lastCmp_.bbsZpAddr = zpAddr;
                            lastCmp_.bbsBitN = bitN;
                            break;
                        }
                    }
                }

                // General I8 bitwise op
                std::string src2 = src2MemOperand(inst.src2);
                loadOperandA(inst.src1);
                if (inst.op == ir::Op::AND) emit("and " + src2, irDesc());
                else if (inst.op == ir::Op::OR) emit("ora " + src2, irDesc());
                else emit("eor " + src2, irDesc());

                if (canFuse) {
                    // Record for BR_COND: treat as CMP_NE (nonzero = true)
                    lastCmp_ = {ir::Op::CMP_NE, inst.dest.vregId, true};
                } else {
                    if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                    lastCmp_.valid = false;
                }
                break;
            }

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
            bool is32 = (inst.resultType == ir::Type::I32);
            std::string shOp = is32 ? "lsl.32 .AXYZ" : "lsl.16 .AX";
            int shiftCount = -1;
            if (inst.src2.isImm()) {
                shiftCount = (int)(inst.src2.immVal & 0xFF);
            }
            loadOperand(inst.src1);
            if (shiftCount >= 0) {
                if (shiftCount == 8 && !is32) {
                    emit("tax"); emit("lda #0");
                } else if (shiftCount == 8 && is32) {
                    // Shift bytes up: Z=Y, Y=X, X=A, A=0
                    emit("pha"); emit("tya"); emit("taz"); emit("txa"); emit("tay"); emit("pla"); emit("tax"); emit("lda #0");
                } else if (shiftCount == 16 && is32) {
                    // A→Y, X→Z, clear A,X
                    emit("tay"); emit("txa"); emit("taz"); emit("lda #0"); emit("ldx #0");
                } else if (shiftCount == 24 && is32) {
                    // A→Z, clear A,X,Y
                    emit("taz"); emit("lda #0"); emit("ldx #0"); emit("ldy #0");
                } else if (shiftCount >= (is32 ? 32 : 16)) {
                    // Shift entirely out — result is 0
                    emit("lda #0"); emit("ldx #0");
                    if (is32) { emit("ldy #0"); emit("ldz #0"); }
                } else {
                    int maxBits = is32 ? 32 : 16;
                    for (int i = 0; i < shiftCount && i < maxBits; i++) emit(shOp);
                }
            } else {
                std::string cntOp = src2MemOperand(inst.src2);
                std::string lbl = "@__shl_loop_" + std::to_string(labelCounter_++);
                std::string done = "@__shl_done_" + std::to_string(labelCounter_++);
                if (is32) {
                    emit("lda " + cntOp);
                    emit("sta __zp_scratch4");
                    loadOperand(inst.src1);
                    emit("lda __zp_scratch4");
                    emit("beq " + done);
                    emit("sta __zp_scratch4");
                    emitLabel(lbl);
                    emit(shOp);
                    emit("dec __zp_scratch4");
                    emit("bne " + lbl);
                } else {
                    emit("ldy " + cntOp);
                    emit("beq " + done);
                    emitLabel(lbl);
                    emit(shOp);
                    emit("dey");
                    emit("bne " + lbl);
                }
                emitLabel(done);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::SHR:
        case ir::Op::ASR: {
            bool is32 = (inst.resultType == ir::Type::I32);
            std::string mn;
            if (is32) mn = (inst.op == ir::Op::ASR) ? "asr.32 .AXYZ" : "lsr.32 .AXYZ";
            else mn = (inst.op == ir::Op::ASR) ? "asr.16 .AX" : "lsr.16 .AX";
            int shiftCount = -1;
            if (inst.src2.isImm()) {
                shiftCount = (int)(inst.src2.immVal & 0xFF);
            }
            loadOperand(inst.src1);
            if (shiftCount >= 0) {
                bool isASR = (inst.op == ir::Op::ASR);
                if (shiftCount >= (is32 ? 32 : 16)) {
                    // Shift entirely out — result is 0 (SHR) or 0/-1 (ASR)
                    if (isASR) {
                        if (is32) emit("tza"); else emit("txa"); // sign byte → A
                        emit("cmp #$80"); emit("lda #0"); emit("sbc #0"); // $FF if neg, $00 if pos
                        emit("tax");
                        if (is32) { emit("tay"); emit("taz"); }
                    } else {
                        emit("lda #0"); emit("ldx #0");
                        if (is32) { emit("ldy #0"); emit("ldz #0"); }
                    }
                } else if (shiftCount == 8 && !is32) {
                    // I16 >> 8: high byte → low byte
                    emit("txa");
                    emit(isASR ? "sxt.8" : "ldx #0");
                } else if (shiftCount == 8 && is32 && !isASR) {
                    // I32 >> 8 unsigned: A=X, X=Y, Y=Z, Z=0
                    emit("txa"); emit("pha"); emit("tya"); emit("tax");
                    emit("tza"); emit("tay"); emit("ldz #0"); emit("pla");
                } else if (shiftCount == 8 && is32 && isASR) {
                    // I32 >> 8 signed: A=X, X=Y, Y=Z, Z=sign_ext(Z)
                    emit("txa"); emit("pha"); emit("tya"); emit("tax");
                    emit("tza"); emit("tay");             // Y = old Z (byte 3)
                    emit("cmp #$80"); emit("lda #0"); emit("sbc #0"); // sign extend
                    emit("taz"); emit("pla");             // Z = sign, A = old X
                } else if (shiftCount == 16 && is32 && !isASR) {
                    // I32 >> 16 unsigned: A=Y, X=Z, Y=Z=0
                    emit("tza"); emit("tax");
                    emit("tya");
                    emit("ldy #0"); emit("ldz #0");
                } else if (shiftCount == 16 && is32 && isASR) {
                    // I32 >> 16 signed: A=Y, X=Z, Y=Z=sign_ext(Z)
                    emit("tya"); emit("pha");             // save Y (new byte 0)
                    emit("tza"); emit("tax");             // X = old Z (new byte 1)
                    emit("cmp #$80"); emit("lda #0"); emit("sbc #0"); // sign extend
                    emit("tay"); emit("taz");             // Y=Z=sign
                    emit("pla");                          // A = old Y (byte 0)
                } else if (shiftCount == 24 && is32 && !isASR) {
                    // I32 >> 24 unsigned: A=Z, X=Y=Z=0
                    emit("tza"); emit("ldx #0"); emit("ldy #0"); emit("ldz #0");
                } else if (shiftCount == 24 && is32 && isASR) {
                    // I32 >> 24 signed: A=Z, X=Y=Z=sign_ext(Z)
                    emit("tza"); emit("cmp #$80"); emit("pha"); // save Z, set sign
                    emit("lda #0"); emit("sbc #0");       // A = sign
                    emit("tax"); emit("tay"); emit("taz"); // X=Y=Z=sign
                    emit("pla");                           // A = old Z (byte 0)
                } else {
                    // General case: loop of single-bit shifts
                    int maxBits = is32 ? 32 : 16;
                    for (int i = 0; i < shiftCount && i < maxBits; i++) emit(mn);
                }
            } else {
                std::string cntOp = src2MemOperand(inst.src2);
                std::string lbl = "@__shr_loop_" + std::to_string(labelCounter_++);
                std::string done = "@__shr_done_" + std::to_string(labelCounter_++);
                if (is32) {
                    // 32-bit shift clobbers Y — use ZP counter instead of DEY
                    emit("lda " + cntOp);
                    emit("sta __zp_scratch4");
                    loadOperand(inst.src1);
                    emit("lda __zp_scratch4");
                    emit("beq " + done);
                    emit("sta __zp_scratch4");
                    emitLabel(lbl);
                    emit(mn);
                    emit("dec __zp_scratch4");
                    emit("bne " + lbl);
                } else {
                    emit("ldy " + cntOp);
                    emit("beq " + done);
                    emitLabel(lbl);
                    emit(mn);
                    emit("dey");
                    emit("bne " + lbl);
                }
                emitLabel(done);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MUL: {
            std::string src2 = src2MemOperand(inst.src2);
            loadOperand(inst.src1);
            if (inst.resultType == ir::Type::I32) {
                emit("mul.s32 .AXYZ, " + src2);
            } else {
                emit("mul.s16 .AX, " + src2);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MUL_U: {
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
                emit("div.s32 .AXYZ, " + src2);
            } else {
                emit("div.s16 .AX, " + src2);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::DIV_U: {
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
                emit("mod.s32 .AXYZ, " + src2);
            } else {
                emit("mod.s16 .AX, " + src2);
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::MOD_U: {
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
            // Check if src2 is a vreg holding a known constant — use immediate directly
            std::string src2;
            bool src2IsConst = false;
            int64_t src2ConstVal = 0;
            if (inst.src2.isVreg() && currentFn_) {
                for (const auto& blk : currentFn_->blocks) {
                    for (const auto& ii : blk.insts) {
                        if (ii.op == ir::Op::CONST && ii.dest.isVreg() &&
                            ii.dest.vregId == inst.src2.vregId) {
                            src2IsConst = true;
                            src2ConstVal = ii.src1.immVal;
                        }
                    }
                }
            }
            if (src2IsConst) {
                src2 = "#" + std::to_string((int)src2ConstVal);
            } else {
                src2 = src2MemOperand(inst.src2);
            }
            bool isSigned = (inst.op == ir::Op::CMP_LT || inst.op == ir::Op::CMP_LE ||
                             inst.op == ir::Op::CMP_GT || inst.op == ir::Op::CMP_GE);

            // Optimization: CMP_EQ/CMP_NE with 0 → use ORA to test zero
            // For ZP-allocated I16: lda zp_lo; ora zp_hi (3 instructions, 5 bytes)
            // For other I16: loadOperand then stx scratch; ora scratch
            if (src2IsConst && src2ConstVal == 0 &&
                (inst.op == ir::Op::CMP_EQ || inst.op == ir::Op::CMP_NE) &&
                inst.src1.type != ir::Type::I32) {
                if (inst.src1.type == ir::Type::I8) {
                    loadOperand(inst.src1);
                    emit("cmp #0");
                } else if (inst.src1.isVreg()) {
                    auto alloc = alloc_.getAlloc(inst.src1.vregId);
                    if (alloc.loc == VRegAllocator::IN_ZP) {
                        std::string lo = "$" + hex8(alloc.offset);
                        std::string hi = "$" + hex8(alloc.offset + 1);
                        emit("lda " + lo);
                        emit("ora " + hi);
                    } else {
                        loadOperand(inst.src1);
                        emit("stx __zp_scratch");
                        emit("ora __zp_scratch");
                    }
                } else {
                    loadOperand(inst.src1);
                    emit("stx __zp_scratch");
                    emit("ora __zp_scratch");
                }
            } else {
                if (inst.src1.type == ir::Type::I8) {
                    loadOperandA(inst.src1); // A-only: preserves N/Z flags for peephole
                    emit("cmp " + src2);
                } else if (inst.src1.type == ir::Type::I32) {
                    loadOperand(inst.src1);
                    emit(std::string(isSigned ? "cmp.s32" : "cmp.32") + " .AXYZ, " + src2);
                } else {
                    loadOperand(inst.src1);
                    emit(std::string(isSigned ? "cmp.s16" : "cmp.16") + " .AX, " + src2);
                }
            }
            } // end src2/signed scope

            // Check if this CMP result is consumed solely by the next BR_COND.
            // If so, skip boolean materialization and let BR_COND use flags directly.
            bool canFuse = false;
            if (inst.dest.isVreg() && currentFn_) {
                // Peek at next instruction in current block
                auto& blk = currentFn_->blocks[currentBlockIdx_];
                size_t instIdx = &inst - &blk.insts[0];
                if (instIdx + 1 < blk.insts.size()) {
                    const auto& next = blk.insts[instIdx + 1];
                    if (next.op == ir::Op::BR_COND && next.src1.isVreg() &&
                        next.src1.vregId == inst.dest.vregId) {
                        canFuse = true;
                    }
                }
            }

            if (canFuse) {
                // Record for the following BR_COND to use
                lastCmp_ = {inst.op, inst.dest.vregId, true};
                // Don't materialize boolean — BR_COND will branch on flags
            } else {
                lastCmp_.valid = false;
                // Branch IMMEDIATELY (flags live), set result to 0 or 1
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
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            }
            break;
        }

        case ir::Op::LOAD: {
            // Fused ADDR_ELEM+LOAD: args carry base/index/stride from the
            // eliminated ADDR_ELEM. Compute address inline to __zp_scratch.
            if (inst.args.size() >= 3 && inst.resultType != ir::Type::F32) {
                // args[0]=base, args[1]=index, args[2]=stride
                auto& base = inst.args[0];
                auto& index = inst.args[1];
                int stride = (int)inst.args[2].immVal;

                std::string baseStr = (base.kind == ir::OperandKind::GLOBAL) ?
                    "#" + base.name : src2MemOperand(base);
                if (index.isImm()) {
                    uint32_t offset = index.immVal * stride;
                    emit("struct_elem.16 __zp_scratch, " + baseStr + ", #" + std::to_string(offset));
                } else {
                    std::string indexStr = src2MemOperand(index);
                    if (index.type == ir::Type::I8) { loadOperand(index); indexStr = ".AX"; }
                    emit("addr_elem.16 __zp_scratch, " + baseStr + ", " + indexStr + ", #" + std::to_string(stride));
                }
                // Now load from (__zp_scratch),Y
                emit("ldy #0");
                emit("lda (__zp_scratch),y");
                if (inst.resultType == ir::Type::I32) {
                    emit("pha"); emit("iny"); emit("lda (__zp_scratch),y");
                    emit("tax"); emit("iny"); emit("lda (__zp_scratch),y");
                    emit("tay"); emit("iny"); emit("lda (__zp_scratch),y");
                    emit("taz"); emit("tya"); emit("tay"); emit("pla");
                } else if (inst.resultType != ir::Type::I8) {
                    emit("pha"); emit("iny"); emit("lda (__zp_scratch),y");
                    emit("tax"); emit("pla");
                } else {
                    emit("ldx #0");
                }
                if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
                ms_.invalidateAll();
                break;
            }

            if (inst.resultType == ir::Type::F32 && inst.dest.isVreg()) {
                // 5-byte float load: copy to dest vreg ZP
                auto dAlloc = alloc_.getAlloc(inst.dest.vregId);
                std::string da = "$" + hex8((uint8_t)dAlloc.offset);
                if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                    for (int i = 0; i < 5; i++) {
                        emit("lda " + inst.src1.name + "+" + std::to_string(i));
                        emit("sta " + da + "+" + std::to_string(i));
                    }
                } else {
                    // Indirect load via (ZP),Y
                    std::string zpPair;
                    if (inst.src1.isVreg()) {
                        auto addrAlloc = alloc_.getAlloc(inst.src1.vregId);
                        if (addrAlloc.loc == VRegAllocator::IN_ZP)
                            zpPair = "$" + hex8((uint8_t)addrAlloc.offset);
                        else { loadVreg(inst.src1.vregId); emit("sta __zp_scratch"); emit("stx __zp_scratch+1"); zpPair = "__zp_scratch"; }
                    } else { loadOperand(inst.src1); emit("sta __zp_scratch"); emit("stx __zp_scratch+1"); zpPair = "__zp_scratch"; }
                    for (int i = 0; i < 5; i++) {
                        emit("ldy #" + std::to_string(i));
                        emit("lda (" + zpPair + "),y");
                        emit("sta " + da + "+" + std::to_string(i));
                    }
                }
                break;
            }
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
                } else if (inst.resultType == ir::Type::I32) {
                    emit("lda (" + zpPair + "),y"); // byte 0
                    emit("pha");
                    emit("iny");
                    emit("lda (" + zpPair + "),y"); // byte 1
                    emit("pha");
                    emit("iny");
                    emit("lda (" + zpPair + "),y"); // byte 2
                    emit("pha");
                    emit("iny");
                    emit("lda (" + zpPair + "),y"); // byte 3
                    emit("taz");
                    emit("ply");
                    emit("plx");
                    emit("pla");
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
            std::string storeR = irDesc(inst.src2.kind == ir::OperandKind::GLOBAL ? "→ " + inst.src2.name : "");

            // Fused ADDR_ELEM+STORE: compute address inline to __zp_scratch
            if (inst.args.size() >= 3 && inst.resultType != ir::Type::F32) {
                auto& base = inst.args[0];
                auto& index = inst.args[1];
                int stride = (int)inst.args[2].immVal;

                // Load value first (before addr_elem clobbers A:X)
                loadOperand(inst.src1);
                emit("pha"); // save value on stack
                if (inst.resultType != ir::Type::I8) emit("phx");

                std::string baseStr = (base.kind == ir::OperandKind::GLOBAL) ?
                    "#" + base.name : src2MemOperand(base);
                if (index.isImm()) {
                    uint32_t offset = index.immVal * stride;
                    emit("struct_elem.16 __zp_scratch, " + baseStr + ", #" + std::to_string(offset));
                } else {
                    std::string indexStr = src2MemOperand(index);
                    if (index.type == ir::Type::I8) { loadOperand(index); indexStr = ".AX"; }
                    emit("addr_elem.16 __zp_scratch, " + baseStr + ", " + indexStr + ", #" + std::to_string(stride));
                }

                // Restore value and store via (__zp_scratch),Y
                if (inst.resultType != ir::Type::I8) emit("plx");
                emit("pla");
                emit("ldy #0");
                emit("sta (__zp_scratch),y");
                if (inst.resultType == ir::Type::I32) {
                    emit("txa"); emit("iny"); emit("sta (__zp_scratch),y");
                    emit("tya"); emit("iny"); emit("sta (__zp_scratch),y"); // wait, Y is used for index
                    // I32 fused store is complex — skip for now, fall through to normal path
                } else if (inst.resultType != ir::Type::I8) {
                    emit("txa"); emit("iny"); emit("sta (__zp_scratch),y");
                }
                ms_.invalidateAll();
                break;
            }

            if (inst.resultType == ir::Type::F32 && inst.src1.isVreg()) {
                // 5-byte float store from src1 vreg ZP to dest address
                auto sAlloc = alloc_.getAlloc(inst.src1.vregId);
                std::string sa = "$" + hex8((uint8_t)sAlloc.offset);
                if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                    for (int i = 0; i < 5; i++) {
                        emit("lda " + sa + "+" + std::to_string(i));
                        emit("sta " + inst.src2.name + "+" + std::to_string(i));
                    }
                } else if (inst.src2.isVreg()) {
                    bool isLocalSlot = localSlotVregs_.count(inst.src2.vregId) > 0;
                    if (isLocalSlot) {
                        // Direct 5-byte copy to local variable's ZP storage
                        auto destAlloc = alloc_.getAlloc(inst.src2.vregId);
                        std::string da = "$" + hex8((uint8_t)destAlloc.offset);
                        for (int i = 0; i < 5; i++) {
                            emit("lda " + sa + "+" + std::to_string(i));
                            emit("sta " + da + "+" + std::to_string(i));
                        }
                    } else {
                        // Indirect store via (ZP),Y — dest vreg holds a pointer address
                        std::string zpPair;
                        auto addrAlloc = alloc_.getAlloc(inst.src2.vregId);
                        if (addrAlloc.loc == VRegAllocator::IN_ZP)
                            zpPair = "$" + hex8((uint8_t)addrAlloc.offset);
                        else { loadVreg(inst.src2.vregId); emit("sta __zp_scratch"); emit("stx __zp_scratch+1"); zpPair = "__zp_scratch"; }
                        for (int i = 0; i < 5; i++) {
                            emit("ldy #" + std::to_string(i));
                            emit("lda " + sa + "+" + std::to_string(i));
                            emit("sta (" + zpPair + "),y");
                        }
                    }
                }
                break;
            }
            if (inst.src2.kind == ir::OperandKind::GLOBAL) {
                // Store directly to global address
                if (inst.resultType == ir::Type::I8 && inst.src1.isVreg()) {
                    // I8: only need A, skip ldx #0
                    auto valAlloc = alloc_.getAlloc(inst.src1.vregId);
                    if (valAlloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream vs;
                        vs << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << valAlloc.offset;
                        emit("lda " + vs.str());
                    } else if (valAlloc.loc == VRegAllocator::IN_FRAME) {
                        emit("lda.fp __vr" + std::to_string(inst.src1.vregId));
                    } else {
                        loadOperand(inst.src1);
                    }
                } else if (inst.resultType == ir::Type::I8 && inst.src1.isImm()) {
                    emit("lda #" + std::to_string((int)(inst.src1.immVal & 0xFF)));
                } else {
                    loadOperand(inst.src1);
                }
                emit("sta " + inst.src2.name, storeR);
                if (inst.resultType == ir::Type::I32) {
                    emit("stx " + inst.src2.name + "+1", storeR);
                    emit("sty " + inst.src2.name + "+2", storeR);
                    emit("stz " + inst.src2.name + "+3", storeR);
                } else if (inst.resultType != ir::Type::I8) {
                    emit("stx " + inst.src2.name + "+1", storeR);
                }
            } else if (inst.src2.isVreg()) {
                // Optimization: if the address vreg is a known constant,
                // emit direct absolute store instead of indirect.
                auto constIt = vregConstVal_.find(inst.src2.vregId);
                if (constIt != vregConstVal_.end() &&
                    !localSlotVregs_.count(inst.src2.vregId)) {
                    std::string addr = "$" + hex16((uint16_t)constIt->second);
                    // Load value — use constant directly if available
                    auto valConst = (inst.src1.isVreg()) ? vregConstVal_.find(inst.src1.vregId) : vregConstVal_.end();
                    if (inst.src1.isImm()) {
                        int v = (int)inst.src1.immVal;
                        emit("lda #" + std::to_string(v & 0xFF));
                        if (inst.resultType == ir::Type::I32) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                            emit("ldy #" + std::to_string((v >> 16) & 0xFF));
                            emit("ldz #" + std::to_string((v >> 24) & 0xFF));
                        } else if (inst.resultType != ir::Type::I8) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                        }
                    } else if (valConst != vregConstVal_.end()) {
                        int v = (int)valConst->second;
                        emit("lda #" + std::to_string(v & 0xFF));
                        if (inst.resultType == ir::Type::I32) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                            emit("ldy #" + std::to_string((v >> 16) & 0xFF));
                            emit("ldz #" + std::to_string((v >> 24) & 0xFF));
                        } else if (inst.resultType != ir::Type::I8) {
                            emit("ldx #" + std::to_string((v >> 8) & 0xFF));
                        }
                    } else if (inst.resultType == ir::Type::I8 && inst.src1.isVreg()) {
                        loadVregA(inst.src1.vregId);
                    } else {
                        loadOperand(inst.src1);
                    }
                    emit("sta " + addr);
                    if (inst.resultType == ir::Type::I32) {
                        emit("stx " + addr + "+1");
                        emit("sty " + addr + "+2");
                        emit("stz " + addr + "+3");
                    } else if (inst.resultType != ir::Type::I8) {
                        emit("stx " + addr + "+1");
                    }
                    break;
                }

                // Skip STORE to suppressed vregs (dead call arg slots)
                if (suppressedVregs_.count(inst.src2.vregId)) break;

                // Is the target a local variable slot (direct write)
                // or a computed address (indirect write through pointer)?
                bool isLocalSlot = localSlotVregs_.count(inst.src2.vregId) > 0;
                if (isLocalSlot) {
                    // Check if previous instruction already handled this STORE
                    if (resultInAX_ == -2) {
                        resultInAX_ = -1;
                        break;
                    }
                    // Store-forwarding: if src1 result is still in A:X (or A:Z), skip reload
                    if (inst.src1.isVreg() && (int32_t)inst.src1.vregId == resultInAX_) {
                        // Result still in registers from previous instruction — store directly.
                        // If the previous storeVreg used stax.fp (frame store), X was clobbered
                        // by TSX but the hi byte is in Z (from the internal TXA;TAZ sequence).
                        valueByte_[0] = REG_A;
                        auto prevAlloc = alloc_.getAlloc(inst.src1.vregId);
                        valueByte_[1] = (prevAlloc.loc == VRegAllocator::IN_FRAME) ? REG_Z : REG_X;
                        storeVreg(inst.src2.vregId);
                    } else {
                        loadOperand(inst.src1);
                        valueByte_[0] = REG_A;
                        valueByte_[1] = REG_X;
                        storeVreg(inst.src2.vregId);
                    }
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
                    if (inst.resultType == ir::Type::I8) {
                        // 8-bit store: only need A, skip X load
                        if (inst.src1.isImm()) {
                            emit("lda #" + std::to_string((int)(inst.src1.immVal & 0xFF)));
                        } else if (inst.src1.isVreg()) {
                            auto valAlloc = alloc_.getAlloc(inst.src1.vregId);
                            if (valAlloc.loc == VRegAllocator::IN_ZP) {
                                std::stringstream vs;
                                vs << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << valAlloc.offset;
                                emit("lda " + vs.str());
                            } else if (valAlloc.loc == VRegAllocator::IN_FRAME) {
                                emit("lda.fp __vr" + std::to_string(inst.src1.vregId));
                            } else {
                                loadOperand(inst.src1);
                            }
                        } else {
                            loadOperand(inst.src1);
                        }
                    } else {
                        loadOperand(inst.src1);
                    }

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

        case ir::Op::LOAD_ZP: {
            std::string addr = inst.src1.name;
            emit("lda " + addr);
            emit("ldx " + addr + "+1");
            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::STORE_ZP: {
            loadVreg(inst.src1.vregId);
            std::string addr = inst.src2.name; // target ZP name
            emit("sta " + addr);
            emit("stx " + addr + "+1");
            break;
        }

        case ir::Op::TRAMPOLINE: {
            // src1 = target func, src2 = link, args[0] = buffer vreg
            uint32_t bufVregId = inst.args[0].vregId;

            // Step 1: Get address of buffer on stack into scratch $08/$09
            emit("leax.fp " + std::to_string(vregOffset_.at(bufVregId)));
            emit("sta $08");
            emit("stx $09");

            // Step 2: Initialize trampoline code (11 bytes)
            // 0: $A9 <lo_link, 2: $85 <chain, 4: $A9 <hi_link, 6: $85 <chain+1, 8: $4C <lo_tgt >hi_tgt

            // Load link into AX
            loadOperand(inst.src2);

            // link lo (A)
            emit("pha");
            emit("ldy #0");
            emit("lda #$A9"); // lda #lo
            emit("sta ($08),y");
            emit("pla");
            emit("iny");
            emit("sta ($08),y");

            // sta __static_chain
            emit("iny");
            emit("lda #$85");
            emit("sta ($08),y");
            emit("iny");
            emit("lda #<__static_chain");
            emit("sta ($08),y");

            // link hi (X)
            emit("txa");
            emit("pha");
            emit("iny");
            emit("lda #$A9"); // lda #hi
            emit("sta ($08),y");
            emit("pla");
            emit("iny");
            emit("sta ($08),y");

            // sta __static_chain+1
            emit("iny");
            emit("lda #$85");
            emit("sta ($08),y");
            emit("iny");
            emit("lda #<(__static_chain+1)");
            emit("sta ($08),y");

            // jmp _target
            emit("iny");
            emit("lda #$4C"); // jmp abs
            emit("sta ($08),y");
            emit("iny");
            emit("lda #<" + inst.src1.name);
            emit("sta ($08),y");
            emit("iny");
            emit("lda #>" + inst.src1.name);
            emit("sta ($08),y");

            // Result: A:X = address of trampoline
            emit("lda $08");
            emit("ldx $09");
            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_GLOBAL: {
            emit("ldax #" + inst.src1.name);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_LABEL: {
            emit("ldax #@" + inst.src1.name);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_UPLEVEL: {
            int levels = (int)inst.args[1].immVal;
            const ir::Operand& targetVar = inst.args[0];
            int targetOffset = 0;
            if (targetVar.vregId != 0xFFFFFFFF) {
                if (functionMap_) {
                    if (functionMap_->count(targetVar.name)) {
                        const auto* targetFn = functionMap_->at(targetVar.name);
                        if (targetFn->vregOffsets.count(targetVar.vregId)) {
                            targetOffset = targetFn->vregOffsets.at(targetVar.vregId);
                        }
                    }
                }
            }

            // Start by loading the static link (pointer to direct parent frame)
            loadVreg(inst.src1.vregId);

            // Follow static chain if target is higher than direct parent
            for (int i = 0; i < levels - 1; i++) {
                emit("sta $08");
                emit("stx $09");
                // Static link is always at offset 0 in every nested frame
                emit("ldy #0");
                emit("lda ($08),y");
                emit("pha");
                emit("iny");
                emit("lda ($08),y");
                emit("tax");
                emit("pla");
                // Result in AX
            }

            if (targetOffset != 0) {
                emit("clc");
                emit("adc #" + std::to_string(targetOffset & 0xFF));
                emit("pha");
                emit("txa");
                emit("adc #" + std::to_string((targetOffset >> 8) & 0xFF));
                emit("tax");
                emit("pla");
            }

            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_LOCAL: {
            if (inst.src1.isVreg()) {
                // Use vregOffset_ (consistent with storeVreg/loadVreg frame offsets)
                auto vit = vregOffset_.find(inst.src1.vregId);
                if (vit != vregOffset_.end()) {
                    emit("leax.fp " + std::to_string(vit->second));
                } else {
                    // Fallback: allocate now (ensures consistency for first-use)
                    auto alloc = alloc_.getAlloc(inst.src1.vregId);
                    if (alloc.loc == VRegAllocator::IN_ZP) {
                        emit("lda #" + std::to_string(alloc.offset));
                        emit("ldx #0");
                    } else {
                        int offset = allocSlot(inst.src1.vregId, alloc.type);
                        emit("leax.fp " + std::to_string(offset));
                    }
                }
            } else {
                int offset = (int)inst.src1.immVal;
                emit("leax.fp " + std::to_string(offset));
            }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BR_INDIRECT: {
            loadVreg(inst.src1.vregId);
            emit("sta $0008");
            emit("stx $0009");
            emit("jmp ($0008)");
            break;
        }

        case ir::Op::GET_FP: {
            emit("lda $FD");
            emit("ldx $FE");
            storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::ADDR_ELEM: {
            int elemSize = (inst.args.size() > 0) ? (int)inst.args[0].immVal : 2;
            
            auto operandToString = [&](const ir::Operand& op) -> std::string {
                if (op.isImm()) {
                    return "#" + std::to_string((int)op.immVal);
                }
                if (op.isVreg()) {
                    auto alloc = alloc_.getAlloc(op.vregId);
                    if (alloc.loc == VRegAllocator::IN_ZP) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << alloc.offset;
                        return ss.str();
                    } else if (alloc.loc == VRegAllocator::IN_AX) {
                        return ".AX";
                    }
                }
                if (op.kind == ir::OperandKind::GLOBAL) {
                    return op.name;
                }
                return src2MemOperand(op);
            };

            std::string baseStr;
            if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                baseStr = "#" + inst.src1.name;
            } else {
                baseStr = operandToString(inst.src1);
            }
            
            std::string destStr = ".AX";
            bool storeNeeded = true;
            if (inst.dest.isVreg()) {
                auto destAlloc = alloc_.getAlloc(inst.dest.vregId);
                if (destAlloc.loc == VRegAllocator::IN_ZP) {
                    std::stringstream ss;
                    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << destAlloc.offset;
                    destStr = ss.str();
                    storeNeeded = false;
                }
            }
            
            if (inst.src2.isImm()) {
                uint32_t offset = inst.src2.immVal * elemSize;
                emit("struct_elem.16 " + destStr + ", " + baseStr + ", #" + std::to_string(offset));
            } else {
                std::string indexStr = operandToString(inst.src2);
                if (inst.src2.type == ir::Type::I8) {
                    loadOperand(inst.src2);
                    indexStr = ".AX";
                }
                emit("addr_elem.16 " + destStr + ", " + baseStr + ", " + indexStr + ", #" + std::to_string(elemSize));
            }
            
            if (storeNeeded && inst.dest.isVreg()) {
                storeVreg(inst.dest.vregId);
            }
            break;
        }

        case ir::Op::COPY:
        case ir::Op::DEREF: {
            loadOperand(inst.src1);
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::BR: {
            if (inst.src1.kind == ir::OperandKind::LABEL) {
                // Skip branch if target is the immediately following block
                if (inst.src1.name != nextBlockLabel_) {
                    emit("bra @" + inst.src1.name);
                }
            }
            break;
        }

        case ir::Op::BR_COND: {
            // src1 = condition, dest = true label, src2 = false label
            std::string trueTarget = (inst.dest.kind == ir::OperandKind::LABEL) ? "@" + inst.dest.name : "";
            std::string falseTarget = (inst.src2.kind == ir::OperandKind::LABEL) ? "@" + inst.src2.name : "";
            bool falseFallsThrough = (inst.src2.kind == ir::OperandKind::LABEL && inst.src2.name == nextBlockLabel_);

            // Fused compare-and-branch: if the condition is from a CMP that just set flags,
            // branch directly using the comparison flags instead of materializing a boolean.
            if (lastCmp_.valid && inst.src1.isVreg() && inst.src1.vregId == lastCmp_.destVreg) {
                // BBS/BBR single-bit test: branch directly on ZP bit, no register usage
                if (!lastCmp_.bbsZpAddr.empty() && lastCmp_.bbsBitN >= 0) {
                    std::string bitStr = std::to_string(lastCmp_.bbsBitN);
                    // CMP_NE means "bit set = true", so bbs = true target, bbr = false target
                    emit("bbs" + bitStr + " " + lastCmp_.bbsZpAddr + ", " + trueTarget,
                         "single-bit test: bit " + bitStr);
                    if (!falseFallsThrough && !falseTarget.empty()) {
                        emit("bra " + falseTarget);
                    }
                    lastCmp_.valid = false;
                    lastCmp_.bbsZpAddr.clear();
                    lastCmp_.bbsBitN = -1;
                    break;
                }

                switch (lastCmp_.op) {
                    case ir::Op::CMP_EQ:
                        emit("beq " + trueTarget);
                        break;
                    case ir::Op::CMP_NE:
                        emit("bne " + trueTarget);
                        break;
                    case ir::Op::CMP_LT:
                    case ir::Op::CMP_LTU:
                        emit("bcc " + trueTarget);
                        break;
                    case ir::Op::CMP_LE:
                    case ir::Op::CMP_LEU:
                        emit("bcc " + trueTarget);
                        emit("beq " + trueTarget);
                        break;
                    case ir::Op::CMP_GT:
                    case ir::Op::CMP_GTU:
                        emit("beq " + falseTarget);
                        emit("bcs " + trueTarget);
                        falseFallsThrough = false; // already handled
                        break;
                    case ir::Op::CMP_GE:
                    case ir::Op::CMP_GEU:
                        emit("bcs " + trueTarget);
                        break;
                    default:
                        emit("bne " + trueTarget);
                        break;
                }
                if (!falseFallsThrough && !falseTarget.empty()) {
                    emit("bra " + falseTarget);
                }
                lastCmp_.valid = false;
                break;
            }

            // Load and test the condition value
            if (inst.src1.isVreg()) {
                auto alloc = alloc_.getAlloc(inst.src1.vregId);
                if (alloc.type == ir::Type::I8) {
                    // I8: load A only (no ldx #0), Z set from lda — branch directly
                    loadOperandA(inst.src1);
                    if (!trueTarget.empty()) emit("bne " + trueTarget);
                } else {
                    // I16: loadOperand loads A then X. Z reflects X from ldx.
                    loadOperand(inst.src1);
                    if (!trueTarget.empty()) emit("bne " + trueTarget); // X != 0
                    emit("cmp #$00"); // test A (low byte)
                    if (!trueTarget.empty()) emit("bne " + trueTarget); // A != 0
                }
            } else {
                loadOperand(inst.src1);
                emit("cmp #$00");
                if (!trueTarget.empty()) emit("bne " + trueTarget);
            }
            if (!falseFallsThrough && !falseTarget.empty()) {
                emit("bra " + falseTarget);
            }
            break;
        }

        case ir::Op::RET: {
            loadOperand(inst.src1);
            // Jump to common return point (endproc handles frame cleanup + rts)
            if (nextBlockLabel_ != "__return") {
                emit("bra @__return");
            }
            break;
        }

        case ir::Op::RET_VOID: {
            if (nextBlockLabel_ != "__return") {
                emit("bra @__return");
            }
            break;
        }

        case ir::Op::CALL:
        case ir::Op::CALL_VOID:
        case ir::Op::CALL_INDIRECT: {
            if (inst.callConv == ir::CallConv::ZP) {
                // ZP convention: store args to ZP param block (past scratch)
                int zpOff = (int)zeroPageStart_ + 8; // 8 bytes of scratch
                for (size_t argIdx = 0; argIdx < inst.args.size(); argIdx++) {
                    const auto& arg = inst.args[argIdx];
                    if (inst.isRegparm && argIdx == 0) {
                        // First param: leave in A/AX, skip ZP store
                        loadOperand(arg);
                        zpOff += ir::typeSize(arg.type);
                        continue;
                    }
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
                //
                // "Simple" args (immediates, known constants, globals) can be
                // pushed directly without ZP temp round-trip.  "Complex" args
                // (vregs from computation that may reference the stack frame)
                // still use the two-phase ZP temp approach to avoid _fp corruption.
                int argBytes = 0;
                int nArgs = (int)inst.args.size();

                // Classify args: true = simple (can push inline)
                auto isSimpleArg = [&](const ir::Operand& arg) -> bool {
                    if (arg.isImm()) return true;
                    if (arg.kind == ir::OperandKind::GLOBAL) return true;
                    if (arg.isVreg() && vregConstVal_.count(arg.vregId)) return true;
                    return false;
                };

                // Check if ALL args are simple — if so, skip ZP temps entirely
                bool allSimple = true;
                for (int ai = 0; ai < nArgs; ai++) {
                    if (!isSimpleArg(inst.args[ai])) { allSimple = false; break; }
                }

                if (allSimple) {
                    // Fast path: push all args inline right-to-left
                    // Use phw #imm16 for constant 16-bit args (3 bytes vs 6)
                    for (int ai = nArgs - 1; ai >= 0; ai--) {
                        if (inst.isRegparm && ai == 0) continue;
                        const auto& arg = inst.args[ai];
                        int ps = ir::typeSize(arg.type);
                        if (ps < 2) ps = 2;

                        // Get constant value if available
                        bool hasConst = false;
                        int constVal = 0;
                        if (arg.isImm()) { hasConst = true; constVal = (int)arg.immVal; }
                        else if (arg.isVreg() && vregConstVal_.count(arg.vregId)) {
                            hasConst = true; constVal = (int)vregConstVal_[arg.vregId];
                        }

                        if (hasConst && arg.type == ir::Type::I32) {
                            // I32: push high word then low word
                            int hiWord = (constVal >> 16) & 0xFFFF;
                            int loWord = constVal & 0xFFFF;
                            emit("lda #" + std::to_string(hiWord & 0xFF));
                            emit("ldx #" + std::to_string((hiWord >> 8) & 0xFF));
                            emit("push .ax");
                            emit("lda #" + std::to_string(loWord & 0xFF));
                            emit("ldx #" + std::to_string((loWord >> 8) & 0xFF));
                            emit("push .ax");
                        } else if (hasConst) {
                            // Push 16-bit constant via push .ax (little-endian on stack)
                            emit("lda #" + std::to_string(constVal & 0xFF));
                            emit("ldx #" + std::to_string((constVal >> 8) & 0xFF));
                            emit("push .ax");
                        } else if (arg.kind == ir::OperandKind::GLOBAL && arg.type != ir::Type::I32) {
                            // Global symbol
                            emit("ldax #" + arg.name);
                            emit("push .ax");
                        } else if (arg.type == ir::Type::F32 && arg.isVreg()) {
                            // Float: push 5 bytes from ZP (push byte 4 first, byte 0 last)
                            auto alloc = alloc_.getAlloc(arg.vregId);
                            std::string za = "$" + hex8((uint8_t)alloc.offset);
                            for (int bi = 4; bi >= 0; bi--) {
                                emit("lda " + za + "+" + std::to_string(bi));
                                emit("pha");
                            }
                        } else {
                            // Complex operand — load and push
                            loadOperand(arg);
                            emit("push .ax");
                        }
                        argBytes += ps;
                        emit(".var _fp = _fp + " + std::to_string(ps));
                    }
                    // For regparm: load first arg into A/AX last
                    if (inst.isRegparm && nArgs > 0) {
                        const auto& arg0 = inst.args[0];
                        if (arg0.isImm()) {
                            int val = (int)arg0.immVal;
                            emit("lda #" + std::to_string(val & 0xFF));
                            if (arg0.type != ir::Type::I8)
                                emit("ldx #" + std::to_string((val >> 8) & 0xFF));
                        } else if (arg0.isVreg() && vregConstVal_.count(arg0.vregId)) {
                            int val = (int)vregConstVal_[arg0.vregId];
                            emit("lda #" + std::to_string(val & 0xFF));
                            if (arg0.type != ir::Type::I8)
                                emit("ldx #" + std::to_string((val >> 8) & 0xFF));
                        } else {
                            loadOperand(arg0);
                        }
                    }
                } else {
                    // Two-phase path: load all args into ZP temp slots BEFORE any pushes,
                    // so frame-relative loads aren't affected by SP/fp changes.
                    // Compute per-slot offsets based on actual arg sizes
                    std::vector<int> slotOffsets(nArgs, 0);
                    {
                        int off = 0;
                        for (int ai = nArgs - 1; ai >= 0; ai--) {
                            int slot = nArgs - 1 - ai;
                            slotOffsets[slot] = off;
                            int sz = ir::typeSize(inst.args[ai].type);
                            if (sz < 2) sz = 2;
                            off += sz;
                        }
                    }
                    auto zpSlotAddr = [&](int idx, int byteOff) {
                        std::stringstream ss;
                        ss << "$" << std::hex << std::uppercase << std::setfill('0')
                           << std::setw(2) << ((int)zeroPageStart_ + 0x20 + slotOffsets[idx] + byteOff);
                        return ss.str();
                    };
                    // Phase 1: load all args (right-to-left) into ZP scratch slots
                    for (int ai = nArgs - 1; ai >= 0; ai--) {
                        const auto& arg = inst.args[ai];
                        int slot = nArgs - 1 - ai;
                        if (arg.type == ir::Type::F32 && arg.isVreg()) {
                            // F32: copy 5 bytes from vreg ZP to scratch slot
                            auto alloc = alloc_.getAlloc(arg.vregId);
                            std::string za = "$" + hex8((uint8_t)alloc.offset);
                            for (int bi = 0; bi < 5; bi++) {
                                emit("lda " + za + "+" + std::to_string(bi));
                                emit("sta " + zpSlotAddr(slot, bi));
                            }
                        } else {
                            loadOperand(arg);
                            emit("sta " + zpSlotAddr(slot, 0));
                            emit("stx " + zpSlotAddr(slot, 1));
                            if (arg.type == ir::Type::I32) {
                                emit("sty " + zpSlotAddr(slot, 2));
                                emit("stz " + zpSlotAddr(slot, 3));
                            }
                        }
                    }
                    // Phase 2: push from ZP slots (same order — already right-to-left)
                    for (int i = 0; i < nArgs; i++) {
                        int origIdx = nArgs - 1 - i;
                        if (inst.isRegparm && origIdx == 0) continue;
                        const auto& arg = inst.args[origIdx];
                        int ps = ir::typeSize(arg.type);
                        if (ps < 2) ps = 2;
                        if (arg.type == ir::Type::F32) {
                            // F32: push 5 bytes (byte 4 first, byte 0 last)
                            for (int bi = 4; bi >= 0; bi--) {
                                emit("lda " + zpSlotAddr(i, bi));
                                emit("pha");
                            }
                        } else if (arg.type == ir::Type::I32) {
                            emit("lda " + zpSlotAddr(i, 0));
                            emit("ldx " + zpSlotAddr(i, 1));
                            emit("ldy " + zpSlotAddr(i, 2));
                            emit("ldz " + zpSlotAddr(i, 3));
                            emit("push .axyz");
                        } else {
                            emit("lda " + zpSlotAddr(i, 0));
                            emit("ldx " + zpSlotAddr(i, 1));
                            emit("push .ax");
                        }
                        argBytes += ps;
                        emit(".var _fp = _fp + " + std::to_string(ps));
                    }
                    // For regparm: load first arg into A/AX last
                    if (inst.isRegparm && nArgs > 0) {
                        int slot0 = nArgs - 1;
                        emit("lda " + zpSlotAddr(slot0, 0));
                        if (inst.args[0].type != ir::Type::I8)
                            emit("ldx " + zpSlotAddr(slot0, 1));
                    }
                }
                
                // JSR
                if (inst.op == ir::Op::CALL || inst.op == ir::Op::CALL_VOID) {
                    if (inst.src1.kind == ir::OperandKind::GLOBAL) {
                        emit("jsr " + inst.src1.name);
                    }
                } else {
                    // CALL_INDIRECT
                    loadOperand(inst.src1);
                    std::string callLabel = "@__call_site_" + std::to_string(labelCounter_++);
                    emit("sta " + callLabel + "+1");
                    emit("stx " + callLabel + "+2");
                    emitLabel(callLabel);
                    emit("jsr $0000");
                }

                // Recalculate frame pointer after JSR (SP may have changed)
                // FP must be re-initialized from current SP for .fp addressing to work correctly
                if (useStackParams_) {
                    emit("tsx");
                    emit("txa");
                    emit("clc");
                    emit("adc #1");
                    emit("sta $FD");
                    emit("lda #$01");
                    emit("adc #0");
                    emit("sta $FE");
                }

                // Caller-side stack cleanup: pop argBytes with PLZ instructions
                // (RTS #N opcode $62 is unreliable on some hardware)
                if (argBytes > 0) {
                    bool saveZ = (inst.op != ir::Op::CALL_VOID && inst.resultType == ir::Type::I32);
                    std::string restoreLabel;
                    if (saveZ) {
                        restoreLabel = "@__restore_caller_z_" + std::to_string(labelCounter_++);
                        emit("stz " + restoreLabel + "+1");
                    }
                    for (int i = 0; i < argBytes; i++) {
                        emit("plz");
                    }
                    if (saveZ) {
                        emitLabel(restoreLabel);
                        emit("ldz #0");
                    }
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
            // Skip storeVreg if the return value is never used (dead vreg).
            if (inst.op != ir::Op::CALL_VOID && inst.dest.isVreg()) {
                bool isUsed = false;
                if (currentFn_) {
                    for (const auto& blk : currentFn_->blocks) {
                        for (const auto& ii : blk.insts) {
                            if (&ii == &inst) continue;
                            if (ii.src1.isVreg() && ii.src1.vregId == inst.dest.vregId) { isUsed = true; break; }
                            if (ii.src2.isVreg() && ii.src2.vregId == inst.dest.vregId) { isUsed = true; break; }
                            for (const auto& a : ii.args)
                                if (a.isVreg() && a.vregId == inst.dest.vregId) { isUsed = true; break; }
                            if (isUsed) break;
                        }
                        if (isUsed) break;
                    }
                } else {
                    isUsed = true; // conservative: assume used if no function context
                }
                if (isUsed) storeVreg(inst.dest.vregId);
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
            if (inst.resultType == ir::Type::I32 && inst.src1.type != ir::Type::I32) {
                if (inst.src1.type == ir::Type::I8) emit("ldx #0");
                emit("ldy #0");
                emit("ldz #0");
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
            if (inst.src1.type == ir::Type::I32) {
                emit("bfext32 #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else if (inst.src1.type == ir::Type::I16) {
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
            if (inst.resultType == ir::Type::I32) {
                emit("bfins32 " + addrStr + ", #" + std::to_string(offset) + ", #" + std::to_string(width));
            } else if (inst.resultType == ir::Type::I16) {
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
            // Compute stack address of first variadic argument.
            // Variadic functions always receive ALL args on the stack (even in zpCall mode,
            // the caller pushes everything for variadic calls). So named params are on the
            // stack at their proc-declared offsets, and variadic args follow.
            std::string pName = inst.asmText;
            emit("leax.fp @_p_" + pName);
            emit("add.16 .AX, #2");
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CPU_REG_READ: {
            std::string reg = inst.asmText;
            std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
            if (reg == "A") ;
            else if (reg == "X") emit("txa");
            else if (reg == "Y") emit("tya");
            else if (reg == "Z") emit("tza");
            else if (reg == "AX") ;
            else if (reg == "AY") { emit("phy"); emit("plx"); }
            else if (reg == "AZ") { emit("phz"); emit("plx"); }
            else if (reg == "XY") { emit("txa"); emit("phy"); emit("plx"); emit("pla"); } // Swap AX/XY
            else if (reg == "SP") { emit("tsy"); emit("phy"); emit("plx"); }
            else if (reg == "Q" || reg == "AXYZ") ; // already in AXYZ
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CPU_REG_WRITE: {
            std::string reg = inst.asmText;
            std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
            
            bool optimized = false;
            if (inst.src1.isImm()) {
                uint32_t val = (uint32_t)inst.src1.immVal;
                if (reg == "A") { emit("lda #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "X") { emit("ldx #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "Y") { emit("ldy #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "Z") { emit("ldz #$" + hex8((uint8_t)val)); optimized = true; }
                else if (reg == "AX") { emit("ldax #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "AY") { emit("lday #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "AZ") { emit("ldaz #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "XY") { emit("ldxy #$" + hex16((uint16_t)val)); optimized = true; }
                else if (reg == "Q" || reg == "AXYZ") { emit("ldq #$" + hex32(val)); optimized = true; }
            } else if (inst.src1.isVreg()) {
                auto alloc = alloc_.getAlloc(inst.src1.vregId);
                if (alloc.loc == VRegAllocator::IN_ZP) {
                    std::string zp = "$" + hex8(alloc.offset);
                    if (reg == "A") { emit("lda " + zp); optimized = true; }
                    else if (reg == "X") { emit("ldx " + zp); optimized = true; }
                    else if (reg == "Y") { emit("ldy " + zp); optimized = true; }
                    else if (reg == "Z") { emit("ldz " + zp); optimized = true; }
                    else if (reg == "AX") { emit("ldax " + zp); optimized = true; }
                    else if (reg == "AY") { emit("lday " + zp); optimized = true; }
                    else if (reg == "AZ") { emit("ldaz " + zp); optimized = true; }
                    else if (reg == "XY") { emit("ldxy " + zp); optimized = true; }
                    else if (reg == "Q" || reg == "AXYZ") { emit("ldq " + zp); optimized = true; }
                }
            }

            if (!optimized) {
                if (inst.src1.isVreg()) loadVreg(inst.src1.vregId);
                else if (inst.src1.isImm()) {
                    if (inst.src1.type == ir::Type::I16) emit("ldax #$" + hex16((uint16_t)inst.src1.immVal));
                    else if (inst.src1.type == ir::Type::I32) emit("ldq #$" + hex32((uint32_t)inst.src1.immVal));
                    else emit("lda #$" + hex8((uint8_t)inst.src1.immVal));
                }
                if (reg == "A") ;
                else if (reg == "X") emit("tax");
                else if (reg == "Y") emit("tay");
                else if (reg == "Z") emit("taz");
                else if (reg == "AX") ;
                else if (reg == "AY") { emit("phx"); emit("ply"); }
                else if (reg == "AZ") { emit("phx"); emit("plz"); }
                else if (reg == "XY") { emit("tax"); emit("phx"); emit("ply"); emit("plx"); }
                else if (reg == "SP") { emit("phx"); emit("ply"); emit("tys"); }
            }
            break;
        }

        case ir::Op::CPU_FLAG_READ: {
            std::string flag = inst.asmText;
            std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);
            emit("lda #0");
            if (flag == "CARRY") emit("adc #0");
            else if (flag == "ZERO") { emit("bne *+3"); emit("lda #1"); }
            else if (flag == "NEGATIVE") { emit("bpl *+3"); emit("lda #1"); }
            else if (flag == "OVERFLOW") { emit("bvc *+3"); emit("lda #1"); }
            if (inst.dest.isVreg()) storeVreg(inst.dest.vregId);
            break;
        }

        case ir::Op::CPU_FLAG_WRITE: {
            std::string flag = inst.asmText;
            std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);

            bool optimized = false;
            if (inst.src1.isImm()) {
                int val = (int)inst.src1.immVal;
                if (flag == "CARRY") { emit(val ? "sec" : "clc"); optimized = true; }
                else if (flag == "DECIMAL") { emit(val ? "sed" : "cld"); optimized = true; }
                else if (flag == "INTERRUPT") { emit(val ? "sei" : "cli"); optimized = true; }
                else if (flag == "OVERFLOW" && val == 0) { emit("clv"); optimized = true; }
            }

            if (!optimized) {
                if (inst.src1.isVreg()) loadVreg(inst.src1.vregId);
                else if (inst.src1.isImm()) emit("lda #$" + hex8((uint8_t)inst.src1.immVal));
                if (flag == "CARRY") { emit("lsr a"); }
                else if (flag == "ZERO") { emit("cmp #0"); }
            }
            break;
        }

        case ir::Op::PHI: {
            // PHI nodes are not directly executable in linear code.
            // For now, skip — the IRBuilder's control flow already sets
            // values before branching.
            break;
        }

        // Float operations
        case ir::Op::FCONST: {
            int64_t bits = inst.src1.immVal; double val;
            std::memcpy(&val, &bits, sizeof(double));
            uint8_t cbm[5]; doubleToCBM40(val, cbm);
            auto da = alloc_.getAlloc(inst.dest.vregId);
            std::string d = "$" + hex8((uint8_t)da.offset);
            for (int i = 0; i < 5; i++) {
                emit("lda #$" + hex8(cbm[i]));
                emit("sta " + d + "+" + std::to_string(i));
            }
            break;
        }
        case ir::Op::FADD: case ir::Op::FSUB:
        case ir::Op::FMUL: case ir::Op::FDIV: {
            auto s1 = alloc_.getAlloc(inst.src1.vregId);
            auto s2 = alloc_.getAlloc(inst.src2.vregId);
            emit("MOVE $" + hex8((uint8_t)s1.offset) + ", __float_a, 5");
            emit("MOVE $" + hex8((uint8_t)s2.offset) + ", __float_b, 5");
            if (inst.op == ir::Op::FADD) emit("jsr __float_add");
            else if (inst.op == ir::Op::FSUB) emit("jsr __float_sub");
            else if (inst.op == ir::Op::FMUL) emit("jsr __float_mul");
            else emit("jsr __float_div");
            auto da = alloc_.getAlloc(inst.dest.vregId);
            emit("MOVE __float_a, $" + hex8((uint8_t)da.offset) + ", 5");
            break;
        }
        case ir::Op::FNEG: {
            auto s1 = alloc_.getAlloc(inst.src1.vregId);
            emit("MOVE $" + hex8((uint8_t)s1.offset) + ", __float_a, 5");
            emit("jsr __float_neg");
            auto da = alloc_.getAlloc(inst.dest.vregId);
            emit("MOVE __float_a, $" + hex8((uint8_t)da.offset) + ", 5");
            break;
        }
        case ir::Op::FCMP: {
            auto s1 = alloc_.getAlloc(inst.src1.vregId);
            auto s2 = alloc_.getAlloc(inst.src2.vregId);
            emit("MOVE $" + hex8((uint8_t)s1.offset) + ", __float_a, 5");
            emit("MOVE $" + hex8((uint8_t)s2.offset) + ", __float_b, 5");
            emit("jsr __float_cmp");
            auto da = alloc_.getAlloc(inst.dest.vregId);
            emit("sta $" + hex8((uint8_t)da.offset));
            break;
        }
        case ir::Op::ITOF: {
            loadOperand(inst.src1);
            emit("jsr __float_itof");
            auto da = alloc_.getAlloc(inst.dest.vregId);
            emit("MOVE __float_a, $" + hex8((uint8_t)da.offset) + ", 5");
            break;
        }
        case ir::Op::FTOI: {
            auto s1 = alloc_.getAlloc(inst.src1.vregId);
            emit("MOVE $" + hex8((uint8_t)s1.offset) + ", __float_a, 5");
            emit("jsr __float_ftoi");
            auto da = alloc_.getAlloc(inst.dest.vregId);
            emit("sta $" + hex8((uint8_t)da.offset));
            emit("stx $" + hex8((uint8_t)(da.offset + 1)));
            break;
        }

        default:
            emitComment("TODO: unimplemented IR op");
            break;
    }
}
