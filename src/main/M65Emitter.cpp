#include "M65Emitter.hpp"
#include "AssemblerOpcodeDatabase.hpp"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

M65Emitter::M65Emitter(std::ostream& out, uint32_t zpStart) : out(&out), mode(Mode::TEXT), zeroPageStart(zpStart) {
    scratchZP_ = (uint8_t)zpStart;
    scratchZP2_ = (uint8_t)zpStart + 2;
    scratchZP3_ = (uint8_t)zpStart + 4;
}
M65Emitter::M65Emitter(std::vector<uint8_t>& binary, uint32_t zpStart) : binary(&binary), mode(Mode::BINARY), zeroPageStart(zpStart) {
    currentAddress = 0; // Will be set by setAddress()
    scratchZP_ = (uint8_t)zpStart;
    scratchZP2_ = (uint8_t)zpStart + 2;
    scratchZP3_ = (uint8_t)zpStart + 4;
}

static std::string hex8(uint8_t val) {
    std::stringstream ss;
    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << (int)val;
    return ss.str();
}

static std::string hex16(uint16_t val) {
    std::stringstream ss;
    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << (int)val;
    return ss.str();
}

static std::string hex32(uint32_t val) {
    std::stringstream ss;
    ss << "$" << std::hex << std::uppercase << std::setfill('0') << std::setw(8) << val;
    return ss.str();
}

void M65Emitter::emitDirective(const std::string& name, const std::string& arg) {
    if (mode == Mode::TEXT) {
        *out << "    ." << name;
        if (!arg.empty()) *out << " " << arg;
        *out << std::endl;
    }
}

void M65Emitter::emitRaw(const std::string& text) {
    if (mode == Mode::TEXT) {
        *out << text << std::endl;
    }
}

void M65Emitter::emitByte(uint8_t b) {
    if (mode == Mode::BINARY) {
        binary->push_back(b);
        currentAddress++;
    }
    else emitText(".byte", hex8(b));
}
void M65Emitter::emitWord(uint16_t w) {
    if (mode == Mode::BINARY) {
        binary->push_back(w & 0xFF);
        binary->push_back(w >> 8);
        currentAddress += 2;
    }
    else emitText(".word", hex16(w));
}

void M65Emitter::emitWordReloc(const std::string& symbolName, uint16_t value) {
    if (mode == Mode::BINARY) {
        symbolRelocs_.push_back({currentAddress, symbolName, 0x80, 0}); // R_WORD at current pos
        binary->push_back(value & 0xFF);
        binary->push_back(value >> 8);
        currentAddress += 2;
    }
    else emitText(".word", symbolName);
}

void M65Emitter::setAddress(uint32_t addr) {
    if (!addressSet) {
        currentAddress = addr;
        addressSet = true;
        if (mode == Mode::TEXT) emitText(".org", hex16((uint16_t)addr));
        return;
    }

    if (addr < currentAddress) {
        throw std::runtime_error("Cannot set address backward: current=" + hex16((uint16_t)currentAddress) + " new=" + hex16((uint16_t)addr));
    }

    if (addr > currentAddress) {
        uint32_t gap = addr - currentAddress;
        if (mode == Mode::BINARY) {
            for (uint32_t i = 0; i < gap; ++i) binary->push_back(0);
        } else {
            emitText(".org", hex16((uint16_t)addr));
        }
        currentAddress = addr;
    }
}
void M65Emitter::emitText(const std::string& mnemonic, const std::string& operand) {
    *out << "    " << std::left << std::setw(8) << mnemonic << operand << std::endl;
}

void M65Emitter::emitLabel(const std::string& label) {
    if (mode == Mode::TEXT) *out << label << ":" << std::endl;
}

void M65Emitter::emitComment(const std::string& comment) {
    if (mode == Mode::TEXT) *out << "    ; " << comment << std::endl;
}

void M65Emitter::emitInstruction(const std::string& mnemonic, AddressingMode amode, uint32_t value, bool hasValue) {
    if (mode == Mode::TEXT) {
        std::string operand = "";
        if (hasValue) {
            switch (amode) {
                case AddressingMode::IMMEDIATE: operand = "#" + hex8((uint8_t)value); break;
                case AddressingMode::IMMEDIATE16: operand = "#" + hex16((uint16_t)value); break;
                case AddressingMode::BASE_PAGE: operand = hex8((uint8_t)value); break;
                case AddressingMode::BASE_PAGE_X: operand = hex8((uint8_t)value) + ",x"; break;
                case AddressingMode::BASE_PAGE_Y: operand = hex8((uint8_t)value) + ",y"; break;
                case AddressingMode::ABSOLUTE:
                    // Emit ZP-range addresses as 2-digit hex so the assembler
                    // can select base-page addressing (saves 1 byte per instruction)
                    operand = (value < 256) ? hex8((uint8_t)value) : hex16((uint16_t)value);
                    break;
                case AddressingMode::ABSOLUTE_X:
                    operand = ((value < 256) ? hex8((uint8_t)value) : hex16((uint16_t)value)) + ",x";
                    break;
                case AddressingMode::ABSOLUTE_Y:
                    operand = ((value < 256) ? hex8((uint8_t)value) : hex16((uint16_t)value)) + ",y";
                    break;
                case AddressingMode::INDIRECT: operand = "(" + hex8((uint8_t)value) + ")"; break;
                case AddressingMode::BASE_PAGE_X_INDIRECT: operand = "(" + hex8((uint8_t)value) + ",x)"; break;
                case AddressingMode::BASE_PAGE_INDIRECT_Y: operand = "(" + hex8((uint8_t)value) + "),y"; break;
                case AddressingMode::BASE_PAGE_INDIRECT_Z: operand = "(" + hex8((uint8_t)value) + "),z"; break;
                case AddressingMode::BASE_PAGE_INDIRECT_SP_Y: operand = "(" + hex8((uint8_t)value) + ",sp),y"; break;
                case AddressingMode::ABSOLUTE_INDIRECT: operand = "(" + hex16((uint16_t)value) + ")"; break;
                case AddressingMode::ABSOLUTE_X_INDIRECT: operand = "(" + hex16((uint16_t)value) + ",x)"; break;
                case AddressingMode::STACK_RELATIVE: operand = std::to_string((int)value) + ",sp"; break;
                case AddressingMode::FLAT_INDIRECT_Z: operand = "[" + hex8((uint8_t)value) + "],z"; break;
                case AddressingMode::RELATIVE: {
                    int v = (int)(int8_t)value + 2; // +2 for BRA instruction size: offset → target
                    operand = "*" + (v >= 0 ? std::string("+") : "") + std::to_string(v);
                    break;
                }
                case AddressingMode::RELATIVE16: {
                    int v = (int)(int16_t)value + 3; // +3 for BRA word instruction size
                    operand = "*" + (v >= 0 ? std::string("+") : "") + std::to_string(v);
                    break;
                }
                default: operand = hex16((uint16_t)value); break;
            }
        } else if (amode == AddressingMode::ACCUMULATOR) {
            operand = "a";
        }
        emitText(mnemonic, operand);
    } else {
        std::string lowerMnemonic = mnemonic;
        std::transform(lowerMnemonic.begin(), lowerMnemonic.end(), lowerMnemonic.begin(), [](unsigned char c){ return std::tolower(c); });
        uint8_t op = AssemblerOpcodeDatabase::getOpcode(lowerMnemonic, amode);
        if (op == 0 && lowerMnemonic != "brk") {
            std::string msg = "'" + lowerMnemonic + "' does not support addressing mode '"
                + AssemblerOpcodeDatabase::AddressingModeToString(amode) + "'";
            auto validModes = AssemblerOpcodeDatabase::getValidAddressingModes(lowerMnemonic);
            if (!validModes.empty()) {
                msg += "; supported modes: ";
                for (size_t i = 0; i < validModes.size(); ++i) {
                    if (i > 0) msg += ", ";
                    msg += AssemblerOpcodeDatabase::AddressingModeToString(validModes[i]);
                }
            }
            if (amode == AddressingMode::STACK_RELATIVE) {
                msg += " (note: stack-relative is only available via simulated ops for lda/sta/ldx/ldy/ldz/stx/sty/stz/inc/dec/inw/dew/phw)";
            }
            throw std::runtime_error(msg);
        }
        emitByte(op);
        if (hasValue) {
            switch (amode) {
                case AddressingMode::IMMEDIATE:
                case AddressingMode::BASE_PAGE:
                case AddressingMode::BASE_PAGE_X:
                case AddressingMode::BASE_PAGE_Y:
                case AddressingMode::INDIRECT:
                case AddressingMode::BASE_PAGE_X_INDIRECT:
                case AddressingMode::BASE_PAGE_INDIRECT_Y:
                case AddressingMode::BASE_PAGE_INDIRECT_Z:
                case AddressingMode::BASE_PAGE_INDIRECT_SP_Y:
                case AddressingMode::STACK_RELATIVE:
                case AddressingMode::FLAT_INDIRECT_Z:
                case AddressingMode::RELATIVE:
                    emitByte((uint8_t)value);
                    break;
                case AddressingMode::IMMEDIATE16:
                case AddressingMode::ABSOLUTE:
                case AddressingMode::ABSOLUTE_X:
                case AddressingMode::ABSOLUTE_Y:
                case AddressingMode::ABSOLUTE_INDIRECT:
                case AddressingMode::ABSOLUTE_X_INDIRECT:
                case AddressingMode::RELATIVE16:
                    emitWord((uint16_t)value);
                    break;
                default:
                    break;
            }
        }
    }
}

void M65Emitter::lda_imm(uint8_t val) { ms_.setConst(REG_A, val); emitInstruction("lda", AddressingMode::IMMEDIATE, val, true); }
void M65Emitter::ldx_imm(uint8_t val) { ms_.setConst(REG_X, val); emitInstruction("ldx", AddressingMode::IMMEDIATE, val, true); }
void M65Emitter::ldy_imm(uint8_t val) { ms_.setConst(REG_Y, val); emitInstruction("ldy", AddressingMode::IMMEDIATE, val, true); }
void M65Emitter::ldz_imm(uint8_t val) { ms_.setConst(REG_Z, val); emitInstruction("ldz", AddressingMode::IMMEDIATE, val, true); }
void M65Emitter::phw_imm(uint16_t val) { ms_.spModified(); emitInstruction("phw", AddressingMode::IMMEDIATE16, val, true); }  // PHW changes stack layout; resets push tracking
void M65Emitter::adc_imm(uint8_t val) {
    // ADC depends on carry, so only propagate if both A and C are known
    if (ms_.reg[REG_A].isConst() && ms_.flags.c != FlagState::F_UNKNOWN) {
        int carry = (ms_.flags.c == FlagState::F_SET) ? 1 : 0;
        int result = (int)(ms_.reg[REG_A].constVal & 0xFF) + val + carry;
        ms_.setConst(REG_A, result & 0xFF);
        ms_.flags.setCarry(result > 0xFF);
        // V flag is complex; leave as unknown. setConst already set N/Z.
        ms_.flags.v = FlagState::F_UNKNOWN;
    } else {
        ms_.invalidateReg(REG_A); ms_.flags.invalidate();
    }
    emitInstruction("adc", AddressingMode::IMMEDIATE, val, true);
}
void M65Emitter::sbc_imm(uint8_t val) {
    if (ms_.reg[REG_A].isConst() && ms_.flags.c != FlagState::F_UNKNOWN) {
        int borrow = (ms_.flags.c == FlagState::F_SET) ? 0 : 1;
        int result = (int)(ms_.reg[REG_A].constVal & 0xFF) - val - borrow;
        ms_.setConst(REG_A, result & 0xFF);
        ms_.flags.setCarry(result >= 0);
        ms_.flags.v = FlagState::F_UNKNOWN;
    } else {
        ms_.invalidateReg(REG_A); ms_.flags.invalidate();
    }
    emitInstruction("sbc", AddressingMode::IMMEDIATE, val, true);
}
void M65Emitter::and_imm(uint8_t val) {
    if (ms_.reg[REG_A].isConst()) {
        ms_.setConst(REG_A, (ms_.reg[REG_A].constVal & 0xFF) & val);
    } else if (val == 0) {
        ms_.setConst(REG_A, 0);
    } else {
        ms_.invalidateReg(REG_A);
        // AND preserves nonzero only if both operands have overlapping set bits
        ms_.flags.setNZ(REG_A);
    }
    emitInstruction("and", AddressingMode::IMMEDIATE, val, true);
}
void M65Emitter::ora_imm(uint8_t val) {
    if (ms_.reg[REG_A].isConst()) {
        ms_.setConst(REG_A, (ms_.reg[REG_A].constVal & 0xFF) | val);
    } else if (val != 0) {
        // ORA with non-zero always produces non-zero
        ms_.invalidateMirrorsOfPub(REG_A);
        ms_.reg[REG_A] = ValueInfo::nonZero();
        ms_.flags.setNZFromValue(ms_.reg[REG_A], (int8_t)REG_A);
    } else {
        // ORA #0 is a no-op on A value, but sets N/Z flags
        ms_.flags.setNZFromValue(ms_.reg[REG_A], (int8_t)REG_A);
    }
    emitInstruction("ora", AddressingMode::IMMEDIATE, val, true);
}
void M65Emitter::eor_imm(uint8_t val) {
    if (ms_.reg[REG_A].isConst()) {
        ms_.setConst(REG_A, ((ms_.reg[REG_A].constVal & 0xFF) ^ val));
    } else {
        ms_.invalidateReg(REG_A); ms_.flags.setNZ(REG_A);
    }
    emitInstruction("eor", AddressingMode::IMMEDIATE, val, true);
}
void M65Emitter::cmp_imm(uint8_t val) {
    // CMP sets N/Z/C but doesn't modify A
    if (ms_.reg[REG_A].isConst()) {
        uint8_t a = (uint8_t)(ms_.reg[REG_A].constVal & 0xFF);
        ms_.flags.z = (a == val) ? FlagState::F_SET : FlagState::F_CLEAR;
        ms_.flags.c = (a >= val) ? FlagState::F_SET : FlagState::F_CLEAR;
        uint8_t diff = a - val;
        ms_.flags.n = (diff & 0x80) ? FlagState::F_SET : FlagState::F_CLEAR;
        ms_.flags.nzSourceReg = -1; // CMP result, not a register load
    } else {
        ms_.flags.invalidate();
    }
    emitInstruction("cmp", AddressingMode::IMMEDIATE, val, true);
}
void M65Emitter::cpx_imm(uint8_t val) {
    if (ms_.reg[REG_X].isConst()) {
        uint8_t x = (uint8_t)(ms_.reg[REG_X].constVal & 0xFF);
        ms_.flags.z = (x == val) ? FlagState::F_SET : FlagState::F_CLEAR;
        ms_.flags.c = (x >= val) ? FlagState::F_SET : FlagState::F_CLEAR;
        uint8_t diff = x - val;
        ms_.flags.n = (diff & 0x80) ? FlagState::F_SET : FlagState::F_CLEAR;
        ms_.flags.nzSourceReg = -1;
    } else {
        ms_.flags.invalidate();
    }
    emitInstruction("cpx", AddressingMode::IMMEDIATE, val, true);
}
void M65Emitter::cpx_zp(uint8_t addr) { ms_.flags.invalidate(); emitInstruction("cpx", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::cpx_abs(uint16_t addr) { ms_.flags.invalidate(); emitInstruction("cpx", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::cpy_imm(uint8_t val) {
    if (ms_.reg[REG_Y].isConst()) {
        uint8_t y = (uint8_t)(ms_.reg[REG_Y].constVal & 0xFF);
        ms_.flags.z = (y == val) ? FlagState::F_SET : FlagState::F_CLEAR;
        ms_.flags.c = (y >= val) ? FlagState::F_SET : FlagState::F_CLEAR;
        uint8_t diff = y - val;
        ms_.flags.n = (diff & 0x80) ? FlagState::F_SET : FlagState::F_CLEAR;
        ms_.flags.nzSourceReg = -1;
    } else {
        ms_.flags.invalidate();
    }
    emitInstruction("cpy", AddressingMode::IMMEDIATE, val, true);
}

// --- Absolute Mode ---
void M65Emitter::lda_abs(uint16_t addr) { ms_.invalidateReg(REG_A); emitInstruction("lda", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::ldx_abs(uint16_t addr) { ms_.invalidateReg(REG_X); emitInstruction("ldx", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::ldy_abs(uint16_t addr) { ms_.invalidateReg(REG_Y); emitInstruction("ldy", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::ldz_abs(uint16_t addr) { ms_.invalidateReg(REG_Z); emitInstruction("ldz", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::sta_abs(uint16_t addr) { ms_.storeAbs(addr, REG_A); emitInstruction("sta", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::stx_abs(uint16_t addr) { ms_.storeAbs(addr, REG_X); emitInstruction("stx", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::sty_abs(uint16_t addr) { ms_.storeAbs(addr, REG_Y); emitInstruction("sty", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::stz_abs(uint16_t addr) { ms_.storeAbs(addr, REG_Z); emitInstruction("stz", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::lda_abs_x(uint16_t addr) { ms_.invalidateReg(REG_A); emitInstruction("lda", AddressingMode::ABSOLUTE_X, addr, true); }
void M65Emitter::sta_abs_x(uint16_t addr) {
    // Indexed store — if X is a known constant, track the exact target address
    if (ms_.reg[REG_X].isConst()) {
        uint16_t target = addr + (uint16_t)(ms_.reg[REG_X].constVal & 0xFF);
        ms_.storeAbs(target, REG_A);
    }
    emitInstruction("sta", AddressingMode::ABSOLUTE_X, addr, true);
}
void M65Emitter::stz_abs_x(uint16_t addr) {
    if (ms_.reg[REG_X].isConst()) {
        uint16_t target = addr + (uint16_t)(ms_.reg[REG_X].constVal & 0xFF);
        ms_.storeAbs(target, REG_Z);
    }
    emitInstruction("stz", AddressingMode::ABSOLUTE_X, addr, true);
}
void M65Emitter::adc_abs(uint16_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("adc", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::sbc_abs(uint16_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("sbc", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::and_abs(uint16_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("and", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::ora_abs(uint16_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("ora", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::eor_abs(uint16_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("eor", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::cmp_abs(uint16_t addr) { ms_.flags.invalidate(); emitInstruction("cmp", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::asl_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("asl", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::rol_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("rol", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::lsr_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("lsr", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::ror_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("ror", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::inc_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("inc", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::dec_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("dec", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::inc_abs_x(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("inc", AddressingMode::ABSOLUTE_X, addr, true); }
void M65Emitter::dec_abs_x(uint16_t addr) { ms_.invalidateAbs(addr); ms_.flags.invalidate(); emitInstruction("dec", AddressingMode::ABSOLUTE_X, addr, true); }
void M65Emitter::bit_abs(uint16_t addr) { ms_.flags.invalidate(); emitInstruction("bit", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::asw_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.invalidateAbs(addr+1); ms_.flags.invalidate(); emitInstruction("asw", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::row_abs(uint16_t addr) { ms_.invalidateAbs(addr); ms_.invalidateAbs(addr+1); ms_.flags.invalidate(); emitInstruction("row", AddressingMode::ABSOLUTE, addr, true); }

// --- Zero Page Mode ---
void M65Emitter::lda_zp(uint8_t addr) { ms_.setRegFromZP(REG_A, addr); emitInstruction("lda", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::ldx_zp(uint8_t addr) { ms_.setRegFromZP(REG_X, addr); emitInstruction("ldx", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::ldy_zp(uint8_t addr) { ms_.setRegFromZP(REG_Y, addr); emitInstruction("ldy", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::ldz_zp(uint8_t addr) { ms_.setRegFromZP(REG_Z, addr); emitInstruction("ldz", AddressingMode::ABSOLUTE, addr, true); }
void M65Emitter::sta_zp(uint8_t addr) { ms_.storeZP(addr, REG_A); emitInstruction("sta", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::stx_zp(uint8_t addr) { ms_.storeZP(addr, REG_X); emitInstruction("stx", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::sty_zp(uint8_t addr) { ms_.storeZP(addr, REG_Y); emitInstruction("sty", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::stz_zp(uint8_t addr) { ms_.storeZP(addr, REG_Z); emitInstruction("stz", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::adc_zp(uint8_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("adc", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::sbc_zp(uint8_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("sbc", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::and_zp(uint8_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("and", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::ora_zp(uint8_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("ora", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::eor_zp(uint8_t addr) { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("eor", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::cmp_zp(uint8_t addr) { ms_.flags.invalidate(); emitInstruction("cmp", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::bit_zp(uint8_t addr) { ms_.flags.invalidate(); emitInstruction("bit", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::asl_zp(uint8_t addr) { ms_.invalidateZP(addr); ms_.flags.invalidate(); emitInstruction("asl", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::lsr_zp(uint8_t addr) { ms_.invalidateZP(addr); ms_.flags.invalidate(); emitInstruction("lsr", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::rol_zp(uint8_t addr) { ms_.invalidateZP(addr); ms_.flags.invalidate(); emitInstruction("rol", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::ror_zp(uint8_t addr) { ms_.invalidateZP(addr); ms_.flags.invalidate(); emitInstruction("ror", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::inc_zp(uint8_t addr) { ms_.invalidateZP(addr); ms_.flags.invalidate(); emitInstruction("inc", AddressingMode::BASE_PAGE, addr, true); }
void M65Emitter::dec_zp(uint8_t addr) { ms_.invalidateZP(addr); ms_.flags.invalidate(); emitInstruction("dec", AddressingMode::BASE_PAGE, addr, true); }

// --- Other Addressing Modes ---
void M65Emitter::recordSpBaseReloc(uint16_t addend) {
    // In BINARY mode, record the address of the upcoming 16-bit operand
    // (currentAddress + 1 for the opcode byte) as an __sp_base relocation site.
    if (mode == Mode::BINARY) {
        spBaseRelocs_.push_back({currentAddress + 1, addend});
    }
}

void M65Emitter::recordSymbolReloc(const std::string& name) {
    // In BINARY mode, record the address of the upcoming 16-bit operand
    // as a symbol relocation site (for simulated ops referencing global symbols).
    if (mode == Mode::BINARY) {
        symbolRelocs_.push_back({currentAddress + 1, name, 0x80, 0}); // R_WORD
    }
}

void M65Emitter::recordSymbolRelocLo(const std::string& name) {
    if (mode == Mode::BINARY) {
        symbolRelocs_.push_back({currentAddress + 1, name, 0x20, 0}); // R_LOW
    }
}

void M65Emitter::recordSymbolRelocHi(const std::string& name, uint8_t lowByte) {
    if (mode == Mode::BINARY) {
        symbolRelocs_.push_back({currentAddress + 1, name, 0x40, lowByte}); // R_HIGH
    }
}

void M65Emitter::recordSymbolReloc32Bit(const std::string& name) {
    if (mode == Mode::BINARY) {
        symbolRelocs_.push_back({currentAddress + 1, name, 0xA0, 0}); // R_LINEAR32
    }
}

void M65Emitter::setupFramePointer() {
    // Compute FP from current SP: FP = SP + 1
    uint8_t fp = framePointerZP_;
    tsx();
    txa();
    clc();
    adc_imm(1);
    sta_zp(fp);
    lda_imm((spBase_ >> 8) & 0xFF);
    sta_zp(fp + 1);
}

void M65Emitter::restoreFramePointer() {
    // Pop saved FP from stack (reverse of setupFramePointer save)
    uint8_t fp = framePointerZP_;
    pla();
    sta_zp(fp + 1);
    pla();
    sta_zp(fp);
}

void M65Emitter::lda_stack(uint8_t offset) {
    if (hasFramePointer()) {
        // Use frame pointer: LDY #offset; LDA ($FP),Y
        ldy_imm(offset);
        emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, framePointerZP_, true);
    } else {
        // Fallback: assume frame pointer at $FD/$FE (cc45 default)
        // When frame-relative variables are used, cc45 always sets FP at $FD/$FE
        // Use frame pointer indirect addressing: LDY #offset; LDA ($FD),Y
        ldy_imm(offset);
        emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, 0xFD, true);
    }
}

void M65Emitter::lda_stack_noTSX(uint8_t offset) {
    // LDA __sp_base+offset,X — caller must ensure X holds SP
    ms_.invalidateReg(REG_A);
    recordSpBaseReloc(offset);
    emitInstruction("lda", AddressingMode::ABSOLUTE_X, spBase_ + offset, true);
}
void M65Emitter::ldx_stack(uint8_t offset) {
    if (hasFramePointer()) {
        ldy_imm(offset);
        emitInstruction("ldx", AddressingMode::BASE_PAGE_INDIRECT_Y, framePointerZP_, true);
    } else {
        // Fallback: assume frame pointer at $FD/$FE (cc45 default)
        ldy_imm(offset);
        emitInstruction("ldx", AddressingMode::BASE_PAGE_INDIRECT_Y, 0xFD, true);
    }
    ms_.invalidateReg(REG_X);
}
void M65Emitter::ldy_stack(uint8_t offset) {
    if (hasFramePointer()) {
        // Special case: we need Y to hold the offset for indirect addressing.
        // We must load through scratch.
        lda_stack(offset);
        tay();
    } else {
        tsx();
        recordSpBaseReloc(offset);
        emitInstruction("ldy", AddressingMode::ABSOLUTE_X, spBase_ + offset, true);
    }
}
void M65Emitter::ldz_stack(uint8_t offset) {
    if (hasFramePointer()) {
        ldy_imm(offset);
        emitInstruction("ldz", AddressingMode::BASE_PAGE_INDIRECT_Y, framePointerZP_, true);
    } else {
        // Fallback: assume frame pointer at $FD/$FE (cc45 default)
        ldy_imm(offset);
        emitInstruction("ldz", AddressingMode::BASE_PAGE_INDIRECT_Y, 0xFD, true);
    }
}
void M65Emitter::sta_stack(uint8_t offset) {
    ms_.storeStack(offset, REG_A);
    if (hasFramePointer()) {
        ldy_imm(offset);
        emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, framePointerZP_, true);
    } else {
        // Fallback: assume frame pointer at $FD/$FE (cc45 default)
        ldy_imm(offset);
        emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, 0xFD, true);
    }
}
void M65Emitter::stx_stack(uint8_t offset) {
    ms_.storeStack(offset, REG_X);
    if (hasFramePointer()) {
        ldy_imm(offset);
        emitInstruction("stx", AddressingMode::BASE_PAGE_INDIRECT_Y, framePointerZP_, true);
    } else {
        // Fallback: assume frame pointer at $FD/$FE (cc45 default)
        ldy_imm(offset);
        emitInstruction("stx", AddressingMode::BASE_PAGE_INDIRECT_Y, 0xFD, true);
    }
}
void M65Emitter::sty_stack(uint8_t offset) {
    ms_.storeStack(offset, REG_Y);
    if (hasFramePointer()) {
        // Special case: we need Y for indirect offset.
        // We must save Y to scratch first, or use a temporary ZP pair.
        // For now, let's assume we can use Z as scratch if available?
        // Better: PHA; TYA; STA_STACK; PLA
        pha();
        tya();
        sta_stack(offset);
        pla();
    } else {
        tsx();
        recordSpBaseReloc(offset);
        emitInstruction("sty", AddressingMode::ABSOLUTE_X, spBase_ + offset, true);
    }
}
void M65Emitter::stz_stack(uint8_t offset) {
    ms_.storeStack(offset, REG_Z);
    if (hasFramePointer()) {
        // STZ doesn't support indirect addressing, so use A as intermediate
        sta_zp(scratchZP_);  // save A
        tza();               // A = Z
        ldy_imm(offset);
        emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, framePointerZP_, true);
        lda_zp(scratchZP_);  // restore A
    } else {
        // Fallback: assume frame pointer at $FD/$FE (cc45 default)
        // Since STZ doesn't support indirect addressing, use indirect store via A
        sta_zp(scratchZP_);  // save A
        tza();               // A = Z
        ldy_imm(offset);
        emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, 0xFD, true);
        lda_zp(scratchZP_);  // restore A
    }
}
void M65Emitter::inc_stack(uint8_t offset) {
    ms_.invalidateStack(offset);
    ms_.flags.invalidate();
    tsxCached();
    recordSpBaseReloc(offset);
    emitInstruction("inc", AddressingMode::ABSOLUTE_X, spBase_ + offset, true);
}
void M65Emitter::dec_stack(uint8_t offset) {
    ms_.invalidateStack(offset);
    ms_.flags.invalidate();
    tsxCached();
    recordSpBaseReloc(offset);
    emitInstruction("dec", AddressingMode::ABSOLUTE_X, spBase_ + offset, true);
}
void M65Emitter::lda_frame(uint8_t fpOff, uint8_t yOff) {
    uint8_t totalOff = fpOff + yOff;
    lda_stack(totalOff);
}
void M65Emitter::sta_frame(uint8_t fpOff, uint8_t yOff) {
    // Direct stack access: TSX; STA __sp_base + fpOff + yOff, X
    uint8_t totalOff = fpOff + yOff;
    sta_stack(totalOff);
}

void M65Emitter::lda_ind_z(uint8_t addr, bool flat) {
    ms_.invalidateReg(REG_A);
    if (mode == Mode::TEXT) {
        if (flat) emitText("lda", "[" + hex8(addr) + "],z");
        else { emitText("ldy", "#$00"); emitText("lda", "(" + hex8(addr) + "),y"); }
    } else {
        if (flat) { eom(); emitByte(0xB2); emitByte(addr); }
        else { ldy_imm(0); emitByte(0xB1); emitByte(addr); }
    }
}
void M65Emitter::sta_ind_z(uint8_t addr, bool flat) {
    if (mode == Mode::TEXT) {
        if (flat) emitText("sta", "[" + hex8(addr) + "],z");
        else { emitText("ldy", "#$00"); emitText("sta", "(" + hex8(addr) + "),y"); }
    } else {
        if (flat) { eom(); emitByte(0x92); emitByte(addr); }
        else { ldy_imm(0); emitByte(0x91); emitByte(addr); }
    }
}

// --- Register Transfers ---
void M65Emitter::tax() { ms_.setTransfer(REG_X, REG_A); emitInstruction("tax", AddressingMode::IMPLIED); }
void M65Emitter::txa() { ms_.setTransfer(REG_A, REG_X); emitInstruction("txa", AddressingMode::IMPLIED); }
void M65Emitter::tay() { ms_.setTransfer(REG_Y, REG_A); emitInstruction("tay", AddressingMode::IMPLIED); }
void M65Emitter::tya() { ms_.setTransfer(REG_A, REG_Y); emitInstruction("tya", AddressingMode::IMPLIED); }
void M65Emitter::taz() { ms_.setTransfer(REG_Z, REG_A); emitInstruction("taz", AddressingMode::IMPLIED); }
void M65Emitter::tza() { ms_.setTransfer(REG_A, REG_Z); emitInstruction("tza", AddressingMode::IMPLIED); }
void M65Emitter::tsx() {
    ms_.setTransfer(REG_X, REG_SP);
    emitInstruction("tsx", AddressingMode::IMPLIED);
}

void M65Emitter::invalidateXSP() { ms_.invalidateReg(REG_X); }

void M65Emitter::tsxCached() {
    // No longer caches — always emit TSX. The assembler optimizer
    // handles redundant TSX elimination with correct MachineState tracking.
    tsx();
}
void M65Emitter::txs() { ms_.setTransfer(REG_SP, REG_X); emitInstruction("txs", AddressingMode::IMPLIED); }
void M65Emitter::tsy() { ms_.setTransfer(REG_Y, REG_SP); emitInstruction("tsy", AddressingMode::IMPLIED); }
void M65Emitter::tys() { ms_.setTransfer(REG_SP, REG_Y); emitInstruction("tys", AddressingMode::IMPLIED); }
void M65Emitter::inx() {
    if (ms_.reg[REG_X].isConst()) ms_.setConst(REG_X, ((ms_.reg[REG_X].constVal & 0xFF) + 1) & 0xFF);
    else { ms_.invalidateReg(REG_X); ms_.flags.setNZ(REG_X); }
    emitInstruction("inx", AddressingMode::IMPLIED);
}
void M65Emitter::dex() {
    if (ms_.reg[REG_X].isConst()) ms_.setConst(REG_X, ((ms_.reg[REG_X].constVal & 0xFF) - 1) & 0xFF);
    else { ms_.invalidateReg(REG_X); ms_.flags.setNZ(REG_X); }
    emitInstruction("dex", AddressingMode::IMPLIED);
}
void M65Emitter::iny() {
    if (ms_.reg[REG_Y].isConst()) ms_.setConst(REG_Y, ((ms_.reg[REG_Y].constVal & 0xFF) + 1) & 0xFF);
    else { ms_.invalidateReg(REG_Y); ms_.flags.setNZ(REG_Y); }
    emitInstruction("iny", AddressingMode::IMPLIED);
}
void M65Emitter::dey() {
    if (ms_.reg[REG_Y].isConst()) ms_.setConst(REG_Y, ((ms_.reg[REG_Y].constVal & 0xFF) - 1) & 0xFF);
    else { ms_.invalidateReg(REG_Y); ms_.flags.setNZ(REG_Y); }
    emitInstruction("dey", AddressingMode::IMPLIED);
}
void M65Emitter::inz() {
    if (ms_.reg[REG_Z].isConst()) ms_.setConst(REG_Z, ((ms_.reg[REG_Z].constVal & 0xFF) + 1) & 0xFF);
    else { ms_.invalidateReg(REG_Z); ms_.flags.setNZ(REG_Z); }
    emitInstruction("inz", AddressingMode::IMPLIED);
}
void M65Emitter::dez() {
    if (ms_.reg[REG_Z].isConst()) ms_.setConst(REG_Z, ((ms_.reg[REG_Z].constVal & 0xFF) - 1) & 0xFF);
    else { ms_.invalidateReg(REG_Z); ms_.flags.setNZ(REG_Z); }
    emitInstruction("dez", AddressingMode::IMPLIED);
}

// --- Stack Operations ---
void M65Emitter::push(const std::string& reg) {
    if (mode == Mode::TEXT) {
        emitText("push", reg);
    } else {
        // Fallback or binary emission if needed, but cc45 mainly uses TEXT for ca45 input.
        // For binary, we'd need to manually call pha/phx/etc.
        if (reg == "a" || reg == ".a") pha();
        else if (reg == "x" || reg == ".x") phx();
        else if (reg == "y" || reg == ".y") phy();
        else if (reg == "z" || reg == ".z") phz();
        else if (reg == ".ax") { pha(); phx(); }
        else if (reg == ".ay") { pha(); phy(); }
        else if (reg == ".az") { pha(); phz(); }
        else if (reg == ".q" || reg == ".axyz") { pha(); phx(); phy(); phz(); }
    }
}

void M65Emitter::pop(const std::string& reg) {
    if (mode == Mode::TEXT) {
        emitText("pop", reg);
    } else {
        if (reg == "a" || reg == ".a") pla();
        else if (reg == "x" || reg == ".x") plx();
        else if (reg == "y" || reg == ".y") ply();
        else if (reg == "z" || reg == ".z") plz();
        else if (reg == ".ax") { plx(); pla(); }
        else if (reg == ".ay") { ply(); pla(); }
        else if (reg == ".az") { plz(); pla(); }
        else if (reg == ".q" || reg == ".axyz") { plz(); ply(); plx(); pla(); }
    }
}

// Push/pull: track values through hardware stack so pha/plx etc. preserve knowledge.
void M65Emitter::pha() { ms_.pushReg(REG_A); emitInstruction("pha", AddressingMode::IMPLIED); }
void M65Emitter::pla() { ms_.pullReg(REG_A); emitInstruction("pla", AddressingMode::IMPLIED); }
void M65Emitter::phx() { ms_.pushReg(REG_X); emitInstruction("phx", AddressingMode::IMPLIED); }
void M65Emitter::plx() { ms_.pullReg(REG_X); emitInstruction("plx", AddressingMode::IMPLIED); }
void M65Emitter::phy() { ms_.pushReg(REG_Y); emitInstruction("phy", AddressingMode::IMPLIED); }
void M65Emitter::ply() { ms_.pullReg(REG_Y); emitInstruction("ply", AddressingMode::IMPLIED); }
void M65Emitter::phz() { ms_.pushReg(REG_Z); emitInstruction("phz", AddressingMode::IMPLIED); }
void M65Emitter::plz() { ms_.pullReg(REG_Z); emitInstruction("plz", AddressingMode::IMPLIED); }

// --- ALU & Branching ---
void M65Emitter::clc() { ms_.flags.setCarry(false); emitInstruction("clc", AddressingMode::IMPLIED); }
void M65Emitter::sec() { ms_.flags.setCarry(true); emitInstruction("sec", AddressingMode::IMPLIED); }
void M65Emitter::cla() { lda_imm(0); }
void M65Emitter::clx() { ldx_imm(0); }
void M65Emitter::cly() { ldy_imm(0); }
void M65Emitter::clz() { ldz_imm(0); }
void M65Emitter::neg_a() {
    if (ms_.reg[REG_A].isConst()) {
        ms_.setConst(REG_A, (-(ms_.reg[REG_A].constVal & 0xFF)) & 0xFF);
    } else { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); }
    emitInstruction("neg", AddressingMode::ACCUMULATOR);
}
void M65Emitter::asl_a() {
    if (ms_.reg[REG_A].isConst()) {
        uint8_t a = (uint8_t)(ms_.reg[REG_A].constVal & 0xFF);
        ms_.flags.setCarry(a & 0x80);
        ms_.setConst(REG_A, (a << 1) & 0xFF);
    } else { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); }
    emitInstruction("asl", AddressingMode::ACCUMULATOR);
}
void M65Emitter::rol_a() { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("rol", AddressingMode::ACCUMULATOR); }
void M65Emitter::lsr_a() {
    if (ms_.reg[REG_A].isConst()) {
        uint8_t a = (uint8_t)(ms_.reg[REG_A].constVal & 0xFF);
        ms_.flags.setCarry(a & 0x01);
        ms_.setConst(REG_A, a >> 1);
    } else { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); }
    emitInstruction("lsr", AddressingMode::ACCUMULATOR);
}
void M65Emitter::ror_a() { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); emitInstruction("ror", AddressingMode::ACCUMULATOR); }
void M65Emitter::inc_a() {
    if (ms_.reg[REG_A].isConst()) {
        ms_.setConst(REG_A, ((ms_.reg[REG_A].constVal & 0xFF) + 1) & 0xFF);
    } else { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); }
    emitInstruction("inc", AddressingMode::ACCUMULATOR);
}
void M65Emitter::dec_a() {
    if (ms_.reg[REG_A].isConst()) {
        ms_.setConst(REG_A, ((ms_.reg[REG_A].constVal & 0xFF) - 1) & 0xFF);
    } else { ms_.invalidateReg(REG_A); ms_.flags.invalidate(); }
    emitInstruction("dec", AddressingMode::ACCUMULATOR);
}
void M65Emitter::eom() { emitInstruction("eom", AddressingMode::IMPLIED); }

void M65Emitter::bra(int8_t offset) { emitInstruction("bra", AddressingMode::RELATIVE, (uint32_t)offset, true); }
void M65Emitter::bne(int8_t offset) { emitInstruction("bne", AddressingMode::RELATIVE, (uint32_t)offset, true); }
void M65Emitter::beq(int8_t offset) { emitInstruction("beq", AddressingMode::RELATIVE, (uint32_t)offset, true); }
void M65Emitter::bcc(int8_t offset) { emitInstruction("bcc", AddressingMode::RELATIVE, (uint32_t)offset, true); }
void M65Emitter::bcs(int8_t offset) { emitInstruction("bcs", AddressingMode::RELATIVE, (uint32_t)offset, true); }
void M65Emitter::bmi(int8_t offset) { emitInstruction("bmi", AddressingMode::RELATIVE, (uint32_t)offset, true); }
void M65Emitter::bpl(int8_t offset) { emitInstruction("bpl", AddressingMode::RELATIVE, (uint32_t)offset, true); }

void M65Emitter::bra_label(const std::string& label) { emitText("bra", label); }
void M65Emitter::bne_label(const std::string& label) { emitText("bne", label); }
void M65Emitter::beq_label(const std::string& label) { emitText("beq", label); }
void M65Emitter::bcc_label(const std::string& label) { emitText("bcc", label); }
void M65Emitter::bcs_label(const std::string& label) { emitText("bcs", label); }
void M65Emitter::bmi_label(const std::string& label) { emitText("bmi", label); }
void M65Emitter::bpl_label(const std::string& label) { emitText("bpl", label); }

void M65Emitter::add_16_imm(uint16_t val) {
    if (mode == Mode::TEXT) {
        std::stringstream ss;
        ss << "#" << hex16(val);
        emitText("add.16", ".ax, " + ss.str());
    } else {
        clc();
        adc_imm(val & 0xFF);
        pha(); txa();
        adc_imm(val >> 8);
        tax(); pla();
    }
}

void M65Emitter::sub_16_imm(uint16_t val) {
    if (mode == Mode::TEXT) {
        std::stringstream ss;
        ss << "#" << hex16(val);
        emitText("sub.16", ".ax, " + ss.str());
    } else {
        sec();
        sbc_imm(val & 0xFF);
        pha(); txa();
        sbc_imm(val >> 8);
        tax(); pla();
    }
}

void M65Emitter::neg_16() {
    if (mode == Mode::TEXT) emitText("neg.16");
    else { eor_imm(0xFF); sec(); adc_imm(0); pha(); txa(); eor_imm(0xFF); adc_imm(0); tax(); pla(); }
}
void M65Emitter::not_16() {
    if (mode == Mode::TEXT) emitText("not.16");
    else { eor_imm(0xFF); pha(); txa(); eor_imm(0xFF); tax(); pla(); }
}

void M65Emitter::transfer_ax_to_zp(uint8_t addr) { sta_zp(addr); stx_zp(addr + 1); }

void M65Emitter::add_32_imm(uint32_t val) {
    if (mode == Mode::TEXT) {
        emitText("add.32", ".AXYZ, #" + hex32(val));
    } else {
        clc();
        adc_imm(val & 0xFF);
        pha(); txa(); adc_imm((val >> 8) & 0xFF); tax();
        tya(); adc_imm((val >> 16) & 0xFF); tay();
        tza(); adc_imm((val >> 24) & 0xFF); taz(); pla();
    }
}

void M65Emitter::sub_32_imm(uint32_t val) {
    if (mode == Mode::TEXT) {
        emitText("sub.32", ".AXYZ, #" + hex32(val));
    } else {
        sec();
        sbc_imm(val & 0xFF);
        pha(); txa(); sbc_imm((val >> 8) & 0xFF); tax();
        tya(); sbc_imm((val >> 16) & 0xFF); tay();
        tza(); sbc_imm((val >> 24) & 0xFF); taz(); pla();
    }
}

void M65Emitter::neg_32() {
    if (mode == Mode::TEXT) emitText("neg.32");
    else {
        eor_imm(0xFF); clc(); adc_imm(1);
        pha(); txa(); eor_imm(0xFF); adc_imm(0); tax();
        tya(); eor_imm(0xFF); adc_imm(0); tay();
        tza(); eor_imm(0xFF); adc_imm(0); taz(); pla();
    }
}

void M65Emitter::not_32() {
    if (mode == Mode::TEXT) emitText("not.32");
    else {
        eor_imm(0xFF);
        pha(); txa(); eor_imm(0xFF); tax();
        tya(); eor_imm(0xFF); tay();
        tza(); eor_imm(0xFF); taz(); pla();
    }
}

void M65Emitter::sxt_16() {
    if (mode == Mode::TEXT) emitText("sxt.16");
    else {
        pha(); txa(); cmp_imm(0x80);
        lda_imm(0); bcc(0x02); lda_imm(0xFF);
        tay(); taz(); pla();
    }
}

size_t M65Emitter::emitBranchPlaceholder(uint8_t opcode) {
    if (!binary) return 0;
    size_t branchPos = binary->size();
    binary->push_back(opcode);
    binary->push_back(0x00);  // placeholder offset
    currentAddress += 2;
    return branchPos;
}

void M65Emitter::patchBranchTarget(size_t branchPos) {
    if (!binary || branchPos + 1 >= binary->size()) return;
    // Relative branch: offset from byte AFTER the branch instruction (branchPos + 2)
    int offset = (int)binary->size() - (int)(branchPos + 2);
    (*binary)[branchPos + 1] = (uint8_t)offset;
}
