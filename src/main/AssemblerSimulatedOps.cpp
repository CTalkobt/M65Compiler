#include "AssemblerSimulatedOps.hpp"
#include "AssemblerParser.hpp"
#include "M65Emitter.hpp"
#include "AssemblerExpression.hpp"
#include "StringUtil.hpp"
#include "Mega65Registers.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>

void AssemblerSimulatedOps::emitExpressionCode(AssemblerParser* parser, M65Emitter& e, const std::string& target, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    auto ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!ast) return;

    int width = 8;
    if (target == ".AX" || target == ".AY" || target == ".AZ") width = 16;
    else if (target == ".AXY" || target == ".AXZ") width = 24;
    else if (target == ".Q" || target == ".AXYZ") width = 32;
    else if (target[0] != '.') width = 16;

    if (ast->isConstant(parser)) {
        uint32_t val = ast->getValue(parser);
        e.lda_imm(val & 0xFF);
        if (width >= 16) e.ldx_imm((val >> 8) & 0xFF);
    } else {
        ast->emit(e, parser, width, target);
    }

    if (target[0] != '.') {
        Symbol* sym = parser->resolveSymbol(target, scopePrefix);
        uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(target); } catch(...) { addr = 0; } }
        e.sta_addr(addr);
        if (width >= 16) {
            e.txa();
            e.sta_addr(addr + 1);
        }
    }
}

void AssemblerSimulatedOps::emitMulCode(AssemblerParser* parser, M65Emitter& e, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    int bytes = width / 8;
    if (bytes < 1) bytes = 1;
    if (bytes > 4) bytes = 4;
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH);
    if (isImmediate) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;
    auto storeMath = [&](uint16_t regBase, int i, const std::string& src) {
        if (src == ".A") e.sta_addr(regBase + i);
        else if (src == ".X") { e.txa(); e.sta_addr(regBase + i); }
        else if (src == ".Y") { e.tya(); e.sta_addr(regBase + i); }
        else if (src == ".Z") { e.tza(); e.sta_addr(regBase + i); }
        else {
            Symbol* sym = parser->resolveSymbol(src, scopePrefix);
            uint32_t addr = i; if (sym) addr += sym->value; else { try { addr += parseNumericLiteral(src); } catch(...) { addr += 0; } }
            e.lda_addr(addr); e.sta_addr(regBase + i);
        }
    };
    if (dest == ".A" || dest == ".AX" || dest == ".AXY" || dest == ".AXYZ" || dest == ".Q") {
        if (bytes >= 1) storeMath(m65::MULT_ARG1, 0, ".A");
        if (bytes >= 2) storeMath(m65::MULT_ARG1, 1, ".X");
        if (bytes >= 3) storeMath(m65::MULT_ARG1, 2, ".Y");
        if (bytes >= 4) storeMath(m65::MULT_ARG1, 3, ".Z");
    } else for (int i = 0; i < bytes; ++i) storeMath(m65::MULT_ARG1, i, dest);

    if (isImmediate && srcAst->isConstant(parser)) {
        uint32_t val = srcAst->getValue(parser);
        for (int i = 0; i < bytes; ++i) {
            e.lda_imm((val >> (i * 8)) & 0xFF);
            e.sta_addr(m65::MULT_ARG2 + i);
        }
    } else {
        int srcTokenIdx = isImmediate ? tokenIndex + 1 : tokenIndex;
        std::string srcName = parser->tokens[srcTokenIdx].value;
        if (parser->tokens[srcTokenIdx].type == AssemblerTokenType::REGISTER) srcName = "." + srcName;
        if (srcName == ".A" || srcName == ".AX" || srcName == ".AXY" || srcName == ".AXYZ" || srcName == ".Q") {
            if (bytes >= 1) storeMath(m65::MULT_ARG2, 0, ".A");
            if (bytes >= 2) storeMath(m65::MULT_ARG2, 1, ".X");
            if (bytes >= 3) storeMath(m65::MULT_ARG2, 2, ".Y");
            if (bytes >= 4) storeMath(m65::MULT_ARG2, 3, ".Z");
        } else for (int i = 0; i < bytes; ++i) storeMath(m65::MULT_ARG2, i, srcName);
    }
    if (dest == ".A" || dest == ".AX" || dest == ".AXY" || dest == ".AXYZ" || dest == ".Q") {
        // Read result into registers: load high bytes first into their
        // target registers, then load the low byte into A last
        if (bytes >= 4) { e.lda_addr(m65::MULT_RES + 3); e.taz(); }
        if (bytes >= 3) { e.lda_addr(m65::MULT_RES + 2); e.tay(); }
        if (bytes >= 2) { e.lda_addr(m65::MULT_RES + 1); e.tax(); }
        e.lda_addr(m65::MULT_RES);
    } else {
        for (int i = 0; i < bytes; ++i) {
            e.lda_addr(m65::MULT_RES + i);
            Symbol* sym = parser->resolveSymbol(dest, scopePrefix);
            uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } }
            e.sta_addr(addr + i);
        }
    }
}

void AssemblerSimulatedOps::emitDivCode(AssemblerParser* parser, M65Emitter& e, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    int bytes = width / 8;
    if (bytes < 1) bytes = 1;
    if (bytes > 4) bytes = 4;
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH);
    if (isImmediate) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;
    auto storeMath = [&](uint16_t regBase, int i, const std::string& src) {
        if (src == ".A") e.sta_addr(regBase + i);
        else if (src == ".X") { e.txa(); e.sta_addr(regBase + i); }
        else if (src == ".Y") { e.tya(); e.sta_addr(regBase + i); }
        else if (src == ".Z") { e.tza(); e.sta_addr(regBase + i); }
        else {
            Symbol* sym = parser->resolveSymbol(src, scopePrefix);
            uint32_t addr = i; if (sym) addr += sym->value; else { try { addr += parseNumericLiteral(src); } catch(...) { addr += 0; } }
            e.lda_addr(addr); e.sta_addr(regBase + i);
        }
    };
    if (dest == ".A" || dest == ".AX" || dest == ".AXY" || dest == ".AXYZ" || dest == ".Q") {
        if (bytes >= 1) storeMath(m65::DIV_ARG1, 0, ".A");
        if (bytes >= 2) storeMath(m65::DIV_ARG1, 1, ".X");
        if (bytes >= 3) storeMath(m65::DIV_ARG1, 2, ".Y");
        if (bytes >= 4) storeMath(m65::DIV_ARG1, 3, ".Z");
    } else for (int i = 0; i < bytes; ++i) storeMath(m65::DIV_ARG1, i, dest);

    if (isImmediate && srcAst->isConstant(parser)) {
        uint32_t val = srcAst->getValue(parser);
        if (val == 0) throw std::runtime_error("Division by zero at line " + std::to_string(parser->tokens[tokenIndex].line));
        for (int i = 0; i < bytes; ++i) {
            e.lda_imm((val >> (i * 8)) & 0xFF);
            e.sta_addr(m65::DIV_ARG2 + i);
        }
    } else {
        int srcTokenIdx = isImmediate ? tokenIndex + 1 : tokenIndex;
        std::string srcName = parser->tokens[srcTokenIdx].value;
        if (parser->tokens[srcTokenIdx].type == AssemblerTokenType::REGISTER) srcName = "." + srcName;
        if (srcName == ".A" || srcName == ".AX" || srcName == ".AXY" || srcName == ".AXYZ" || srcName == ".Q") {
            if (bytes >= 1) storeMath(m65::DIV_ARG2, 0, ".A");
            if (bytes >= 2) storeMath(m65::DIV_ARG2, 1, ".X");
            if (bytes >= 3) storeMath(m65::DIV_ARG2, 2, ".Y");
            if (bytes >= 4) storeMath(m65::DIV_ARG2, 3, ".Z");
        } else for (int i = 0; i < bytes; ++i) storeMath(m65::DIV_ARG2, i, srcName);
    }
    e.bit_addr(m65::MATH_BUSY_STATUS); e.bne(-5);
    if (dest == ".A" || dest == ".AX" || dest == ".AXY" || dest == ".AXYZ" || dest == ".Q") {
        if (bytes >= 4) { e.lda_addr(m65::DIV_RES + 3); e.taz(); }
        if (bytes >= 3) { e.lda_addr(m65::DIV_RES + 2); e.tay(); }
        if (bytes >= 2) { e.lda_addr(m65::DIV_RES + 1); e.tax(); }
        e.lda_addr(m65::DIV_RES);
    } else {
        for (int i = 0; i < bytes; ++i) {
            e.lda_addr(m65::DIV_RES + i);
            Symbol* sym = parser->resolveSymbol(dest, scopePrefix);
            uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } }
            e.sta_addr(addr + i);
        }
    }
}

void AssemblerSimulatedOps::emitStackIncDecCode(AssemblerParser* parser, M65Emitter& e, bool isInc, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        // Frame-relative 16-bit inc/dec: load, modify, store back
        if (isInc) {
            e.lda_frame(fa.fpOff, fa.yOff); e.clc(); e.adc_imm(1); e.sta_frame(fa.fpOff, fa.yOff);
            e.lda_frame(fa.fpOff, fa.yOff + 1); e.adc_imm(0); e.sta_frame(fa.fpOff, fa.yOff + 1);
        } else {
            e.lda_frame(fa.fpOff, fa.yOff); e.sec(); e.sbc_imm(1); e.sta_frame(fa.fpOff, fa.yOff);
            e.lda_frame(fa.fpOff, fa.yOff + 1); e.sbc_imm(0); e.sta_frame(fa.fpOff, fa.yOff + 1);
        }
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        if (e.hasFramePointer()) {
            uint8_t fp = e.framePointerZP();
            if (isInc) {
                e.ldy_imm(offset);
                e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                e.clc(); e.adc_imm(1);
                e.emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                auto br183 = e.emitBranchPlaceholder(0xD0); // BNE: skip hi byte increment
                e.ldy_imm(offset + 1);
                e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                e.adc_imm(0);
                e.emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                e.patchBranchTarget(br183);
            } else {
                e.ldy_imm(offset);
                e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                auto br191 = e.emitBranchPlaceholder(0xD0); // BNE: if lo != 0, skip hi decrement
                e.ldy_imm(offset + 1);
                e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                e.sec(); e.sbc_imm(1);
                e.emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                e.patchBranchTarget(br191);
                e.ldy_imm(offset);
                e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
                e.sec(); e.sbc_imm(1);
                e.emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, fp, true);
            }
        } else {
            uint16_t sb = e.spBase();
            e.tsx();
            if (isInc) {
                e.recordSpBaseReloc(offset); e.inc_abs_x(sb + offset);
                auto br206 = e.emitBranchPlaceholder(0xD0); // BNE: skip hi byte increment
                e.recordSpBaseReloc(offset + 1); e.inc_abs_x(sb + offset + 1);
                e.patchBranchTarget(br206);
            } else {
                e.recordSpBaseReloc(offset); e.lda_abs_x(sb + offset);
                auto br210 = e.emitBranchPlaceholder(0xD0); // BNE: if lo != 0, skip hi decrement
                e.recordSpBaseReloc(offset + 1); e.dec_abs_x(sb + offset + 1);
                e.patchBranchTarget(br210);
                e.recordSpBaseReloc(offset); e.dec_abs_x(sb + offset);
            }
        }
    }
}

void AssemblerSimulatedOps::emitStackIncDec8Code(AssemblerParser* parser, M65Emitter& e, bool isInc, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.lda_frame(fa.fpOff, fa.yOff);
        if (isInc) e.inc_a(); else e.dec_a();
        e.sta_frame(fa.fpOff, fa.yOff);
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        uint16_t sb = e.spBase();
        e.tsx();
        e.recordSpBaseReloc(offset);
        if (isInc) e.inc_abs_x(sb + offset); else e.dec_abs_x(sb + offset);
    }
}

void AssemblerSimulatedOps::emitAddSub16Code(AssemblerParser* parser, M65Emitter& e, bool isAdd, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;
    std::string DEST = dest;
    if (!DEST.empty() && DEST[0] != '.' && DEST[0] != '$' && !isdigit(DEST[0]) && DEST[0] != '#') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);

    auto emitWithTarget = [&](const std::string& target) {
        if (target == ".AX" || target == ".AY" || target == ".AZ") {
            char reg2 = target[2];
            if (isAdd) e.clc(); else e.sec();
            if (isImmediate && srcAst->isConstant(parser)) {
                uint32_t val = srcAst->getValue(parser);
                if (isAdd) e.adc_imm(val & 0xFF); else e.sbc_imm(val & 0xFF);
                e.pha(); if (reg2 == 'X') e.txa(); else if (reg2 == 'Y') e.tya(); else e.tza();
                if (isAdd) e.adc_imm((val >> 8) & 0xFF); else e.sbc_imm((val >> 8) & 0xFF);
                if (reg2 == 'X') e.tax(); else if (reg2 == 'Y') e.tay(); else if (reg2 == 'Z') e.taz();
                e.pla();
            } else {
                uint32_t addr = 0;
                try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
                catch(...) {
                     std::string src = parser->tokens[tokenIndex].value;
                     if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
                     Symbol* sym = parser->resolveSymbol(src, scopePrefix);
                     if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src); } catch(...) { addr = 0; } }
                }
                if (isAdd) e.adc_addr(addr); else e.sbc_addr(addr);
                e.pha(); if (reg2 == 'X') e.txa(); else if (reg2 == 'Y') e.tya(); else if (reg2 == 'Z') e.tza();
                if (isAdd) e.adc_addr(addr + 1); else e.sbc_addr(addr + 1);
                if (reg2 == 'X') e.tax(); else if (reg2 == 'Y') e.tay(); else if (reg2 == 'Z') e.taz();
                e.pla();
            }
        } else {
            // Memory target
            Symbol* sym = parser->resolveSymbol(dest, scopePrefix);
            uint32_t dAddr = 0; if (sym) dAddr = sym->value; else { try { dAddr = parseNumericLiteral(dest); } catch(...) { dAddr = 0; } }
            if (isAdd) e.clc(); else e.sec();
            if (isImmediate && srcAst->isConstant(parser)) {
                uint32_t val = srcAst->getValue(parser);
                e.lda_addr(dAddr); if (isAdd) e.adc_imm(val & 0xFF); else e.sbc_imm(val & 0xFF); e.sta_addr(dAddr);
                e.lda_addr(dAddr+1); if (isAdd) e.adc_imm((val >> 8) & 0xFF); else e.sbc_imm((val >> 8) & 0xFF); e.sta_addr(dAddr+1);
            } else {
                uint32_t sAddr = 0;
                try { sAddr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
                catch(...) {
                     std::string src = parser->tokens[tokenIndex].value;
                     if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
                     Symbol* symS = parser->resolveSymbol(src, scopePrefix);
                     if (symS) sAddr = symS->value; else { try { sAddr = parseNumericLiteral(src); } catch(...) { sAddr = 0; } }
                }
                e.lda_addr(dAddr); if (isAdd) e.adc_addr(sAddr); else e.sbc_addr(sAddr); e.sta_addr(dAddr);
                e.lda_addr(dAddr+1); if (isAdd) e.adc_addr(sAddr+1); else e.sbc_addr(sAddr+1); e.sta_addr(dAddr+1);
            }
        }
    };
    emitWithTarget(DEST);
}

void AssemblerSimulatedOps::emitBitwise16Code(AssemblerParser* parser, M65Emitter& e, const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;
    std::string DEST = dest;
    if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    std::string M = mnemonic; std::transform(M.begin(), M.end(), M.begin(), ::toupper);
    if (DEST == ".AX") {
        if (isImmediate && srcAst->isConstant(parser)) {
            uint32_t val = srcAst->getValue(parser);
            if (M == "AND.16") e.and_imm(val & 0xFF); else if (M == "ORA.16") e.ora_imm(val & 0xFF); else if (M == "EOR.16") e.eor_imm(val & 0xFF);
            e.pha(); e.txa();
            if (M == "AND.16") e.and_imm((val >> 8) & 0xFF); else if (M == "ORA.16") e.ora_imm((val >> 8) & 0xFF); else if (M == "EOR.16") e.eor_imm((val >> 8) & 0xFF);
            e.tax(); e.pla();
        } else {
            std::string src = parser->tokens[tokenIndex].value;
            if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
            else if (!src.empty() && src[0] != '.' && (src=="A"||src=="X"||src=="Y"||src=="Z"||src=="a"||src=="x"||src=="y"||src=="z")) src = "." + src;
            Symbol* sym = parser->resolveSymbol(src, scopePrefix);
            uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src); } catch(...) { addr = 0; } }
            if (M == "AND.16") e.and_addr(addr); else if (M == "ORA.16") e.ora_addr(addr); else if (M == "EOR.16") e.eor_addr(addr);
            e.pha(); e.txa();
            if (M == "AND.16") e.and_addr(addr + 1); else if (M == "ORA.16") e.ora_addr(addr + 1); else if (M == "EOR.16") e.eor_addr(addr + 1);
            e.tax(); e.pla();
        }
    } else throw std::runtime_error("Simulated bitwise 16-bit only supports .AX destination");
}

void AssemblerSimulatedOps::emitCMP16Code(AssemblerParser* parser, M65Emitter& e, const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto src2Ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!src2Ast) return;
    std::string SRC1 = src1;
    if (!SRC1.empty() && SRC1[0] != '.') SRC1 = "." + SRC1;
    std::transform(SRC1.begin(), SRC1.end(), SRC1.begin(), ::toupper);
    if (SRC1 == ".AX") {
        if (isImmediate && src2Ast->isConstant(parser)) {
            uint32_t val = src2Ast->getValue(parser);
            // Compare high bytes first (X vs high byte of val)
            e.cpx_imm((val >> 8) & 0xFF); auto br = e.emitBranchPlaceholder(0xD0); e.cmp_imm(val & 0xFF); e.patchBranchTarget(br);
        } else {
            std::string src2 = parser->tokens[tokenIndex].value;
            if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src2 = "." + src2;
            else if (!src2.empty() && src2[0] != '.' && (src2=="A"||src2=="X"||src2=="Y"||src2=="Z"||src2=="a"||src2=="x"||src2=="y"||src2=="z")) src2 = "." + src2;
            Symbol* sym = parser->resolveSymbol(src2, scopePrefix);
            uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src2); } catch(...) { addr = 0; } }
            // Compare high bytes first (X vs memory high byte)
            e.cpx_addr(addr + 1); auto br = e.emitBranchPlaceholder(0xD0); e.cmp_addr(addr); e.patchBranchTarget(br);
        }
    } else throw std::runtime_error("Simulated CMP.16 only supports .AX as first operand");
}

void AssemblerSimulatedOps::emitCMP_S16Code(AssemblerParser* parser, M65Emitter& e, const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto src2Ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!src2Ast) return;
    std::string SRC1 = src1;
    if (!SRC1.empty() && SRC1[0] != '.') SRC1 = "." + SRC1;
    std::transform(SRC1.begin(), SRC1.end(), SRC1.begin(), ::toupper);
    if (SRC1 == ".AX") {
        if (isImmediate && src2Ast->isConstant(parser)) {
            uint32_t val = src2Ast->getValue(parser);
            e.sta_scratch();
            e.txa();
            e.eor_imm(0x80);
            e.cmp_imm(((val >> 8) & 0xFF) ^ 0x80);
            auto br = e.emitBranchPlaceholder(0xD0); // bne done
            e.lda_scratch();
            e.cmp_imm(val & 0xFF);
            e.patchBranchTarget(br);
        } else {
            uint32_t addr = 0;
            try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
            catch(...) {
                std::string src2 = parser->tokens[tokenIndex].value;
                if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src2 = "." + src2;
                Symbol* sym = parser->resolveSymbol(src2, scopePrefix);
                if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src2); } catch(...) { addr = 0; } }
            }
            e.sta_scratch();
            e.txa();
            e.eor_imm(0x80);
            e.sta_scratch2(); // save AX high byte signed in scratch2
            e.lda_addr(addr + 1);
            e.eor_imm(0x80);
            e.cmp_zp(e.scratchZP2()); // compare high bytes signed
            auto br = e.emitBranchPlaceholder(0xD0); // bne done
            e.lda_scratch();
            e.cmp_addr(addr);
            e.patchBranchTarget(br);
        }
    } else throw std::runtime_error("Simulated CMP.S16 only supports .AX as first operand");
}

void AssemblerSimulatedOps::emitLDWCode(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix, bool forceStack) {
    int idx = tokenIndex;
    uint32_t offset = 0;
    bool isStack = forceStack ? (offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix), true) : parser->isStackRelativeOperand(tokenIndex, offset, scopePrefix);
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX" || DEST == ".AY" || DEST == ".AZ") {
        char reg2 = DEST[2];
        if (isStack) {
            auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
            if (fa.isFrame) {
                // Frame-relative: use direct stack access
                uint8_t totalOff = fa.fpOff + fa.yOff;
                if (reg2 == 'X') {
                    e.lda_stack(totalOff + 1); e.sta_scratch();
                    e.lda_stack(totalOff); e.ldx_scratch();
                } else if (reg2 == 'Y') {
                    e.lda_stack(totalOff + 1); e.sta_scratch();
                    e.lda_stack(totalOff); e.ldy_addr(0);
                } else { // Z
                    e.lda_stack(totalOff + 1); e.taz();
                    e.lda_stack(totalOff);
                }
            } else if (reg2 == 'Z') {
                // Load hi first into Z, then lo into A
                e.lda_stack(offset + 1); e.taz(); e.lda_stack(offset);
            } else {
                // Load hi into temp (ZP $00), then load lo into A, then move to reg2
                e.lda_stack(offset + 1); e.sta_scratch(); // save hi to ZP $00
                e.lda_stack(offset); // A = lo byte
                if (reg2 == 'X') e.ldx_scratch();
                else if (reg2 == 'Y') e.ldy_addr(0);
            }
        }
        else {
            bool isImm = false; if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) { isImm = true; idx++; }
            auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
            if (!srcAst) return;
            if (isImm) {
                uint32_t val = srcAst->getValue(parser); e.lda_imm(val & 0xFF); uint8_t val2 = (val >> 8) & 0xFF;
                if (reg2 == 'X') e.ldx_imm(val2); else if (reg2 == 'Y') e.ldy_imm(val2); else if (reg2 == 'Z') e.ldz_imm(val2);
            } else {
                uint32_t addr = 0;
                try {
                    addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
                } catch (...) {
                    std::string srcName = parser->tokens[tokenIndex].value;
                    if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) srcName = "." + srcName;
                    else if (!srcName.empty() && srcName[0] != '.' && (srcName=="A"||srcName=="X"||srcName=="Y"||srcName=="Z"||srcName=="a"||srcName=="x"||srcName=="y"||srcName=="z")) srcName = "." + srcName;
                    Symbol* sym = parser->resolveSymbol(srcName, scopePrefix);
                    if (sym) addr = sym->value;
                    else {
                        try { addr = parseNumericLiteral(srcName); }
                        catch(...) { addr = 0; } // Assume 0 for forward references during pass1
                    }
                }
                // Record symbol relocation for each absolute address reference
                std::string symName = parser->tokens[tokenIndex].value;
                Symbol* relSym = parser->resolveSymbol(symName, scopePrefix);
                if (relSym && relSym->isAddress) { e.recordSymbolReloc(symName); }
                e.lda_addr(addr);
                uint32_t addr2 = addr + 1;
                if (relSym && relSym->isAddress) { e.recordSymbolReloc(symName); }
                if (reg2 == 'X') e.ldx_addr(addr2); else if (reg2 == 'Y') e.ldy_addr(addr2); else if (reg2 == 'Z') e.ldz_addr(addr2);
            }
        }
    } else if (DEST == ".XY") {
        // Load 16-bit into X(lo)/Y(hi) — used for KERNAL pointer args
        if (isStack) {
            auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
            if (fa.isFrame) {
                uint8_t totalOff = fa.fpOff + fa.yOff;
                e.lda_stack(totalOff + 1); e.tay();
                e.lda_stack(totalOff); e.tax();
            } else {
                e.lda_stack(offset + 1); e.tay();
                e.lda_stack(offset); e.tax();
            }
        } else {
            bool isImm = false; if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) { isImm = true; idx++; }
            auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
            if (!srcAst) return;
            if (isImm) {
                uint32_t val = srcAst->getValue(parser);
                e.ldx_imm(val & 0xFF); e.ldy_imm((val >> 8) & 0xFF);
            } else {
                uint32_t addr = 0;
                try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
                catch (...) {
                    Symbol* sym = parser->resolveSymbol(parser->tokens[tokenIndex].value, scopePrefix);
                    if (sym) addr = sym->value;
                }
                e.ldx_addr(addr); e.ldy_addr(addr + 1);
            }
        }
    } else throw std::runtime_error("Simulated LDW only supports .AX, .AY, .AZ, .XY");
}

void AssemblerSimulatedOps::emitSTWCode(AssemblerParser* parser, M65Emitter& e, const std::string& src, int tokenIndex, const std::string& scopePrefix, bool forceStack) {
    uint32_t offset = 0;
    bool isStack = forceStack ? (offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix), true) : parser->isStackRelativeOperand(tokenIndex, offset, scopePrefix);
    std::string SRC = src; if (!SRC.empty() && SRC[0] != '.' && SRC[0] != '#') SRC = "." + SRC;
    std::transform(SRC.begin(), SRC.end(), SRC.begin(), ::toupper);
    if (SRC[0] == '#') {
        int valIdx = tokenIndex - 1; while (valIdx >= 0 && parser->tokens[valIdx].type != AssemblerTokenType::HASH) valIdx--;
        if (valIdx >= 0) valIdx++; else valIdx = tokenIndex - 2;
        uint32_t val = parser->evaluateExpressionAt(valIdx, scopePrefix);
        uint8_t low = val & 0xFF, high = (val >> 8) & 0xFF;
        if (isStack) {
            auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
            if (fa.isFrame) {
                e.lda_imm(low); e.sta_frame(fa.fpOff, fa.yOff);
                e.lda_imm(high); e.sta_frame(fa.fpOff, fa.yOff + 1);
            } else {
                e.lda_imm(low); e.sta_stack(offset); if (high == 0) e.stz_stack(offset + 1); else { e.lda_imm(high); e.sta_stack(offset + 1); }
            }
        }
        else {
            uint32_t addr = 0;
            try {
                addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
            } catch (...) {
                std::string dest = parser->tokens[tokenIndex].value;
                if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) dest = "." + dest;
                Symbol* sym = parser->resolveSymbol(dest, scopePrefix);
                if (sym) addr = sym->value;
                else {
                    if (!parser->isPass1()) { // Only try to parse numeric literal if not pass1
                        try { addr = parseNumericLiteral(dest); }
                        catch(...) { addr = 0; }
                    } else {
                        addr = 0; // Assume 0 for forward references during pass1
                    }
                }
            }
            e.lda_imm(low); e.sta_addr(addr); if (high == 0) e.stz_addr(addr + 1); else { e.lda_imm(high); e.sta_addr(addr + 1); }
        }
        return;
    }
    if (SRC == ".AX" || SRC == ".AY" || SRC == ".AZ") {
        char reg2 = SRC[2];
        if (isStack) {
            auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
            if (fa.isFrame) {
                // Frame-relative store: same pattern as non-frame (save hi to scratch first)
                uint8_t totalOff = fa.fpOff + fa.yOff;
                if (reg2 == 'X') { e.stx_scratch(); e.sta_stack(totalOff); e.lda_scratch(); e.sta_stack(totalOff + 1); }
                else if (reg2 == 'Y') { e.sty_scratch(); e.sta_stack(totalOff); e.lda_scratch(); e.sta_stack(totalOff + 1); }
                else if (reg2 == 'Z') { e.sta_stack(totalOff); e.tza(); e.sta_stack(totalOff + 1); }
            } else {
                // Save hi byte register to Z without using stack (PHA would shift SP)
                if (reg2 == 'X') { e.stx_scratch(); e.sta_stack(offset); e.lda_scratch(); e.sta_stack(offset + 1); }
                else if (reg2 == 'Y') { e.sty_scratch(); e.sta_stack(offset); e.lda_scratch(); e.sta_stack(offset + 1); }
                else if (reg2 == 'Z') { e.sta_stack(offset); e.tza(); e.sta_stack(offset + 1); }
            }
        }
        else {
            std::string dest = parser->tokens[tokenIndex].value; if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) dest = "." + dest;
            else if (!dest.empty() && dest[0] != '.' && (dest=="A"||dest=="X"||dest=="Y"||dest=="Z"||dest=="a"||dest=="x"||dest=="y"||dest=="z")) dest = "." + dest;
            Symbol* sym = parser->resolveSymbol(dest, scopePrefix); uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } }
            if (sym && sym->isAddress) { e.recordSymbolReloc(dest); }
            e.sta_addr(addr); uint32_t addr2 = addr + 1;
            if (sym && sym->isAddress) { e.recordSymbolReloc(dest); }
            if (reg2 == 'X') e.stx_addr(addr2); else if (reg2 == 'Y') e.sty_addr(addr2); else if (reg2 == 'Z') e.stz_addr(addr2);
        }
    } else if (SRC == ".XY") {
        // Store X(lo)/Y(hi) to memory
        if (isStack) {
            auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
            if (fa.isFrame) {
                uint8_t totalOff = fa.fpOff + fa.yOff;
                e.txa(); e.sta_stack(totalOff); e.tya(); e.sta_stack(totalOff + 1);
            } else {
                e.txa(); e.sta_stack(offset); e.tya(); e.sta_stack(offset + 1);
            }
        } else {
            uint32_t addr = 0;
            try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
            catch (...) {
                Symbol* sym = parser->resolveSymbol(parser->tokens[tokenIndex].value, scopePrefix);
                if (sym) addr = sym->value;
            }
            e.stx_addr(addr); e.sty_addr(addr + 1);
        }
    } else throw std::runtime_error("Simulated STW only supports .AX, .AY, .AZ, .XY");
}

void AssemblerSimulatedOps::emitLDA_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.lda_frame(fa.fpOff, fa.yOff);
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.lda_stack(offset);
    }
}

void AssemblerSimulatedOps::emitSTA_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.sta_frame(fa.fpOff, fa.yOff);
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.sta_stack(offset);
    }
}

void AssemblerSimulatedOps::emitLDX_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.lda_frame(fa.fpOff, fa.yOff); e.tax();
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.lda_stack(offset); e.tax();
    }
}

void AssemblerSimulatedOps::emitLDY_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.lda_frame(fa.fpOff, fa.yOff); e.tay();
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.lda_stack(offset); e.tay();
    }
}

void AssemblerSimulatedOps::emitLDZ_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.lda_frame(fa.fpOff, fa.yOff); e.taz();
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.lda_stack(offset); e.taz();
    }
}

void AssemblerSimulatedOps::emitSTX_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.txa(); e.sta_frame(fa.fpOff, fa.yOff);
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.txa(); e.sta_stack(offset);
    }
}

void AssemblerSimulatedOps::emitSTY_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.tya(); e.sta_frame(fa.fpOff, fa.yOff);
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.tya(); e.sta_stack(offset);
    }
}

void AssemblerSimulatedOps::emitSTZ_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        e.lda_imm(0); e.sta_frame(fa.fpOff, fa.yOff);
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        e.stz_stack(offset);
    }
}

void AssemblerSimulatedOps::emitSwapCode(AssemblerParser* parser, M65Emitter& e, const std::string& r1, int tokenIndex, const std::string&) {
    std::string r2 = parser->tokens[tokenIndex].value; if (!r2.empty() && r2[0] != '.') r2 = "." + r2;
    std::transform(r2.begin(), r2.end(), r2.begin(), ::toupper);
    std::string R1 = r1; if (!R1.empty() && R1[0] != '.') R1 = "." + R1;
    std::transform(R1.begin(), R1.end(), R1.begin(), ::toupper);
    if (R1 == ".A" || r2 == ".A") {
        std::string other = (R1 == ".A") ? r2 : R1;
        if (other == ".X") { e.pha(); e.txa(); e.plx(); } else if (other == ".Y") { e.pha(); e.tya(); e.ply(); } else if (other == ".Z") { e.pha(); e.tza(); e.plz(); }
        else throw std::runtime_error("SWAP only supports A, X, Y, Z");
    } else {
        if ((R1 == ".X" && r2 == ".Y") || (R1 == ".Y" && r2 == ".X")) { e.phx(); e.tya(); e.tax(); e.ply(); }
        else if ((R1 == ".X" && r2 == ".Z") || (R1 == ".Z" && r2 == ".X")) { e.phx(); e.tza(); e.tax(); e.plz(); }
        else if ((R1 == ".Y" && r2 == ".Z") || (R1 == ".Z" && r2 == ".Y")) { e.phy(); e.tza(); e.tay(); e.plz(); }
        else throw std::runtime_error("SWAP only supports A, X, Y, Z");
    }
}

void AssemblerSimulatedOps::emitZeroCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string&) {
    int idx = tokenIndex;
    while (idx < (int)parser->tokens.size()) {
        std::string reg = parser->tokens[idx].value; std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
        if (reg == "A") e.cla(); else if (reg == "X") e.clx(); else if (reg == "Y") e.cly(); else if (reg == "Z") e.clz();
        else throw std::runtime_error("ZERO only supports A, X, Y, Z");
        if (idx + 1 < (int)parser->tokens.size() && parser->tokens[idx+1].type == AssemblerTokenType::COMMA) idx += 2; else break;
    }
}

void AssemblerSimulatedOps::emitNegNot16Code(AssemblerParser* parser, M65Emitter& e, bool isNeg, const std::string& operand, int tokenIndex, const std::string& scopePrefix) {
    std::string OP = operand; if (!OP.empty() && OP[0] != '.') OP = "." + OP;
    std::transform(OP.begin(), OP.end(), OP.begin(), ::toupper);
    if (OP == ".AX" || OP == "") { if (isNeg) e.neg_16(); else e.not_16(); return; }
    uint32_t offset = 0;
    if (parser->isStackRelativeOperand(tokenIndex, offset, scopePrefix)) {
        auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
        if (fa.isFrame) {
            if (isNeg) {
                e.lda_frame(fa.fpOff, fa.yOff); e.eor_imm(0xFF); e.sec(); e.adc_imm(1); e.sta_frame(fa.fpOff, fa.yOff);
                e.lda_frame(fa.fpOff, fa.yOff + 1); e.eor_imm(0xFF); e.adc_imm(0); e.sta_frame(fa.fpOff, fa.yOff + 1);
            } else {
                e.lda_frame(fa.fpOff, fa.yOff); e.eor_imm(0xFF); e.sta_frame(fa.fpOff, fa.yOff);
                e.lda_frame(fa.fpOff, fa.yOff + 1); e.eor_imm(0xFF); e.sta_frame(fa.fpOff, fa.yOff + 1);
            }
        } else {
            uint16_t sb = e.spBase();
            e.tsx();
            if (isNeg) {
                e.recordSpBaseReloc(offset); e.lda_abs_x(sb + offset); e.eor_imm(0xFF); e.sec(); e.adc_imm(1);
                e.recordSpBaseReloc(offset); e.sta_abs_x(sb + offset);
                e.recordSpBaseReloc(offset + 1); e.lda_abs_x(sb + offset + 1); e.eor_imm(0xFF); e.adc_imm(0);
                e.recordSpBaseReloc(offset + 1); e.sta_abs_x(sb + offset + 1);
            } else {
                e.recordSpBaseReloc(offset); e.lda_abs_x(sb + offset); e.eor_imm(0xFF);
                e.recordSpBaseReloc(offset); e.sta_abs_x(sb + offset);
                e.recordSpBaseReloc(offset + 1); e.lda_abs_x(sb + offset + 1); e.eor_imm(0xFF);
                e.recordSpBaseReloc(offset + 1); e.sta_abs_x(sb + offset + 1);
            }
        }
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(operand, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(operand); } catch(...) { addr = 0; } } }
        if (isNeg) { e.lda_addr(addr); e.eor_imm(0xFF); e.sec(); e.adc_imm(1); e.sta_addr(addr); e.lda_addr(addr + 1); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr + 1); }
        else { e.lda_addr(addr); e.eor_imm(0xFF); e.sta_addr(addr); e.lda_addr(addr + 1); e.eor_imm(0xFF); e.sta_addr(addr + 1); }
    }
}

void AssemblerSimulatedOps::emitABS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX" || DEST == "") { e.txa(); auto br720 = e.emitBranchPlaceholder(0x10); e.neg_16(); e.patchBranchTarget(br720); return; }
    uint32_t offset = 0;
    if (parser->isStackRelativeOperand(tokenIndex, offset, scopePrefix)) {
        auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
        if (fa.isFrame) {
            e.lda_frame(fa.fpOff, fa.yOff + 1); auto br725 = e.emitBranchPlaceholder(0x10); // BPL: skip negation
            e.lda_frame(fa.fpOff, fa.yOff); e.eor_imm(0xFF); e.sec(); e.adc_imm(1); e.sta_frame(fa.fpOff, fa.yOff);
            e.lda_frame(fa.fpOff, fa.yOff + 1); e.eor_imm(0xFF); e.adc_imm(0); e.sta_frame(fa.fpOff, fa.yOff + 1);
            e.patchBranchTarget(br725);
        } else {
            uint16_t sb = e.spBase();
            e.tsx();
            e.recordSpBaseReloc(offset + 1); e.lda_abs_x(sb + offset + 1); auto br731 = e.emitBranchPlaceholder(0x10); // BPL: skip negation
            e.recordSpBaseReloc(offset); e.lda_abs_x(sb + offset); e.eor_imm(0xFF); e.sec(); e.adc_imm(1);
            e.recordSpBaseReloc(offset); e.sta_abs_x(sb + offset);
            e.recordSpBaseReloc(offset + 1); e.lda_abs_x(sb + offset + 1); e.eor_imm(0xFF); e.adc_imm(0);
            e.recordSpBaseReloc(offset + 1); e.sta_abs_x(sb + offset + 1);
            e.patchBranchTarget(br731);
        }
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.lda_addr(addr + 1); auto br741 = e.emitBranchPlaceholder(0x10); // BPL: skip negation
        e.lda_addr(addr); e.eor_imm(0xFF); e.sec(); e.adc_imm(1); e.sta_addr(addr);
        e.lda_addr(addr + 1); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr + 1);
        e.patchBranchTarget(br741);
    }
}

void AssemblerSimulatedOps::emitChkZeroCode(AssemblerParser* /*parser*/, M65Emitter& e, bool is16, bool isInverse, int, const std::string&) {
    if (is16) { e.cmp_imm(0); auto br755 = e.emitBranchPlaceholder(0xD0); e.txa(); e.patchBranchTarget(br755); } else e.cmp_imm(0);
    if (isInverse) { auto brCond = e.emitBranchPlaceholder(0xD0); e.lda_imm(0); auto brSkip = e.emitBranchPlaceholder(0x80); e.patchBranchTarget(brCond); e.lda_imm(1); e.patchBranchTarget(brSkip); }
    else { auto brCond = e.emitBranchPlaceholder(0xF0); e.lda_imm(0); auto brSkip = e.emitBranchPlaceholder(0x80); e.patchBranchTarget(brCond); e.lda_imm(1); e.patchBranchTarget(brSkip); }
}

void AssemblerSimulatedOps::emitBranch16Code(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string&) {
    int idx = tokenIndex; if (idx >= (int)parser->tokens.size()) return;
    std::string condition = parser->tokens[idx++].value; std::transform(condition.begin(), condition.end(), condition.begin(), ::toupper);
    if (condition == "BEQ") e.beq(0x00); else if (condition == "BNE") e.bne(0x00);
}

void AssemblerSimulatedOps::emitSelectCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex; if (idx >= (int)parser->tokens.size()) return;
    idx++; // Skip reg
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto val1Ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto val2Ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!val1Ast || !val2Ast) return;
    auto brCond769 = e.emitBranchPlaceholder(0xD0); e.lda_imm(val2Ast->getValue(parser)); auto brSkip769 = e.emitBranchPlaceholder(0x80); e.patchBranchTarget(brCond769); e.lda_imm(val1Ast->getValue(parser)); e.patchBranchTarget(brSkip769);
}

// Helper: compute effective address of a stack variable into AX.
// When frame pointer is active: FP + addend (simple ZP addition, no relocs).
// Otherwise: TSX; TXA; CLC; ADC #lo(__sp_base+addend); ... with relocs.
static void emitSpBaseAddrCalc(AssemblerParser* parser, M65Emitter& e, uint8_t addend) {
    if (e.hasFramePointer()) {
        // FP-relative: AX = FP + addend
        uint8_t fp = e.framePointerZP();
        e.clc();
        e.lda_zp(fp);
        e.adc_imm(addend);
        e.pha();
        e.lda_zp(fp + 1);
        e.adc_imm(0);
        e.tax();
        e.pla();
        return;
    }
    bool spExtern = parser->isExternSymbol("__sp_base");
    e.tsx(); e.txa(); e.clc();
    if (spExtern) {
        e.recordSymbolRelocLo("__sp_base");
        e.adc_imm(addend);
    } else {
        uint16_t total = e.spBase() + addend;
        e.adc_imm(total & 0xFF);
    }
    e.pha();
    if (spExtern) {
        e.recordSymbolRelocHi("__sp_base", addend);
        e.lda_imm(0);
    } else {
        uint16_t total = e.spBase() + addend;
        e.lda_imm((total >> 8) & 0xFF);
    }
    e.adc_imm(0); e.tax(); e.pla();
}

void AssemblerSimulatedOps::emitPtrStackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        // Frame-relative: compute SP + spBase + fpOff + yOff (direct stack address)
        emitSpBaseAddrCalc(parser, e, fa.fpOff + fa.yOff);
    } else {
        // Check if the operand is a stack-relative variable vs global/absolute address.
        // Stack variables (.var) are always defined before use and will be found by resolveSymbol.
        // If the symbol is not found or is not stack-relative, treat it as a global address.
        std::string baseName = parser->tokens[tokenIndex].value;
        Symbol* sym = parser->resolveSymbol(baseName, scopePrefix);
        // If the symbol is unresolved in pass1, assume it's a stack-relative variable to be conservative.
        // This ensures the largest possible code path is chosen in pass1 to prevent size growth in pass2.
        bool isStack = (sym && (sym->isStackRelative || (!sym->isAddress && sym->isVariable))) ||
                       (!sym && parser->isPass1());
        if (isStack) {
            // Stack-relative variable: compute SP + spBase + offset
            uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
            emitSpBaseAddrCalc(parser, e, (uint8_t)offset);
        } else {
            // Global/absolute address: load directly into AX
            uint32_t addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
            e.lda_imm(addr & 0xFF);
            e.ldx_imm((addr >> 8) & 0xFF);
        }
    }
}

void AssemblerSimulatedOps::emitPtrDerefCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    std::string src = parser->tokens[tokenIndex].value; if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
    Symbol* sym = parser->resolveSymbol(src, scopePrefix); uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src); } catch(...) { addr = 0; } }
    // Load 16-bit value from address pointed to by ZP pair at addr/addr+1
    // LDY #0; LDA ($addr),Y → lo byte; PHA; LDY #1; LDA ($addr),Y → hi byte; TAX; PLA
    e.ldy_imm(0);
    e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
    e.pha();
    e.ldy_imm(1);
    e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
    e.tax(); e.pla();
}

void AssemblerSimulatedOps::emitFillCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix, bool forceStack) {
    int idx = tokenIndex;
    if (idx < 0 || idx >= (int)parser->tokens.size()) return;

    enum OperandKind { NONE, REGISTER, STACK_REL, VALUE };
    enum OperandWidth { WIDTH_16 = 2, WIDTH_24 = 3, WIDTH_32 = 4 };

    struct Operand {
        OperandKind kind = NONE;
        OperandWidth width = WIDTH_16;
        std::string regName;
        uint32_t value = 0;
        std::string symbolName;
    };

    // Helper to detect operand width from register name
    auto getRegisterWidth = [](const std::string& regName) -> OperandWidth {
        std::string upper = regName;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        // 32-bit: AXYZ, Q
        if (upper == "AXYZ" || upper == "Q") return WIDTH_32;
        // 24-bit: AXY, AXZ, AYZ (zero-extended to 32-bit in buffer)
        if (upper == "AXY" || upper == "AXZ" || upper == "AYZ") return WIDTH_32;
        // 16-bit: everything else
        return WIDTH_16;
    };

    // Helper to detect operand width from value
    auto getValueWidth = [](uint32_t value) -> OperandWidth {
        if (value <= 0xFFFF) return WIDTH_16;
        return WIDTH_32;
    };

    auto parseOperand = [&](int& curIdx) -> Operand {
        Operand op;
        if (curIdx >= (int)parser->tokens.size() || parser->tokens[curIdx].type == AssemblerTokenType::COMMA) {
            return op;
        }

        const auto& tok = parser->tokens[curIdx];

        if (tok.type == AssemblerTokenType::REGISTER) {
            op.kind = REGISTER;
            op.regName = tok.value;
            op.width = getRegisterWidth(tok.value);
            curIdx++;
            return op;
        }

        if (tok.type == AssemblerTokenType::DIRECTIVE) {
            std::string upper = tok.value;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper == "AX" || upper == "AY" || upper == "AZ" ||
                upper == "XY" || upper == "XZ" || upper == "YZ" ||
                upper == "XYZ" || upper == "AXY" || upper == "AXZ" || upper == "AYZ" || upper == "AXYZ" || upper == "Q") {
                op.kind = REGISTER;
                op.regName = upper;
                op.width = getRegisterWidth(upper);
                curIdx++;
                return op;
            }
        }

        uint32_t stackOffset = 0;
        if (parser->isStackRelativeOperand(curIdx, stackOffset, scopePrefix)) {
            op.kind = STACK_REL;
            op.value = stackOffset;
            op.width = WIDTH_16;  // Stack-relative is always 16-bit
            parseExprAST(parser->tokens, curIdx, parser->symbolTable, scopePrefix);
            if (curIdx < (int)parser->tokens.size() && parser->tokens[curIdx].type == AssemblerTokenType::COMMA) curIdx++;
            if (curIdx < (int)parser->tokens.size() && (parser->tokens[curIdx].value == "s" || parser->tokens[curIdx].value == "S")) curIdx++;
            return op;
        }

        int symIdx = curIdx;
        auto ast = parseExprAST(parser->tokens, curIdx, parser->symbolTable, scopePrefix);
        if (ast) {
            op.kind = VALUE;
            op.value = ast->getValue(parser);
            // Check if this is a symbol that needs relocation
            if (symIdx < (int)parser->tokens.size()) {
                std::string name = parser->tokens[symIdx].value;
                Symbol* sym = parser->resolveSymbol(name, scopePrefix);
                if (sym && sym->isAddress) {
                    op.symbolName = name;
                    op.width = getValueWidth(sym->value);  // Use symbol's value for width
                } else {
                    op.width = getValueWidth(op.value);
                }
            } else {
                op.width = getValueWidth(op.value);
            }
        }
        return op;
    };

    // Parse destination and length operands
    Operand destOp = parseOperand(idx);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    Operand lenOp = parseOperand(idx);

    if (forceStack && destOp.kind != STACK_REL) {
        destOp.kind = STACK_REL;
    }

    uint16_t sb = e.spBase();
    std::string bufSymbol = "__fill_dma_buf";
    uint8_t bufAddrZP = 0x02;

    // Load buffer address from symbol
    e.recordSymbolRelocHi(bufSymbol, 0);
    e.lda_imm(0);
    e.sta_zp(bufAddrZP + 1);

    e.recordSymbolRelocLo(bufSymbol);
    e.lda_imm(0);
    e.sta_zp(bufAddrZP);

    // Helper for indirect addressing
    auto sta_ind_z_with_y = [&](uint8_t addr, int offset) {
        e.ldy_imm(offset);
        e.emitByte(0x91);
        e.emitByte(addr);
    };

    // Write command byte (0x03 for FILL)
    e.lda_imm(0x03);
    sta_ind_z_with_y(bufAddrZP, 0);

    // Calculate buffer offsets based on operand widths (length, destination)
    // Layout: buf+0=cmd, buf+1+=length, then dest, dest_bank, then extra fields
    int lenOffset = 1;
    int lenSize = (int)lenOp.width;
    int destOffset = lenOffset + lenSize;
    int destSize = (int)destOp.width;
    int destBankOffset = destOffset + destSize;
    int extraOffset = destBankOffset + 1;

    // Helper lambda to store register value with width awareness
    auto storeRegValue = [&](const std::string& reg, int bufOffset, OperandWidth width) {
        std::string upper = reg;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (width == WIDTH_32) {
            // 4-byte register values
            if (upper == "AXYZ" || upper == "Q") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
            else if (upper == "AXY") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
            else if (upper == "AXZ") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
            else if (upper == "AYZ") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
        } else {
            // 16-bit (2-byte) register values
            if (upper == "AX") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "AY") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "AZ") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "XY") {
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "XZ") {
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "YZ") {
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "X") {
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "Y") {
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "Z") {
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else if (upper == "A") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
        }
    };

    // Write destination address
    if (destOp.kind == REGISTER) {
        storeRegValue(destOp.regName, destOffset, destOp.width);
    } else if (destOp.kind == STACK_REL) {
        uint16_t addr = sb + destOp.value;
        e.tsx(); e.txa(); e.clc(); e.adc_imm(addr & 0xFF);
        sta_ind_z_with_y(bufAddrZP, destOffset);
        e.lda_imm((addr >> 8) & 0xFF);
        if ((addr & 0xFF) == 0) e.adc_imm(0);
        sta_ind_z_with_y(bufAddrZP, destOffset + 1);
        // Zero-fill remaining bytes if 32-bit
        if (destOp.width == WIDTH_32) {
            e.lda_imm(0);
            sta_ind_z_with_y(bufAddrZP, destOffset + 2);
            sta_ind_z_with_y(bufAddrZP, destOffset + 3);
        }
    } else if (destOp.kind == VALUE) {
        if (destOp.width == WIDTH_16) {
            uint8_t lo = destOp.value & 0xFF;
            uint8_t hi = (destOp.value >> 8) & 0xFF;
            if (lo == 0) {
                e.lda_imm(0);
            } else {
                if (!destOp.symbolName.empty()) e.recordSymbolRelocLo(destOp.symbolName);
                e.lda_imm(lo);
            }
            sta_ind_z_with_y(bufAddrZP, destOffset);
            if (hi == 0) {
                e.lda_imm(0);
            } else {
                if (!destOp.symbolName.empty()) e.recordSymbolRelocHi(destOp.symbolName, lo);
                e.lda_imm(hi);
            }
            sta_ind_z_with_y(bufAddrZP, destOffset + 1);
        } else {
            // 32-bit destination
            uint8_t b0 = destOp.value & 0xFF;
            uint8_t b1 = (destOp.value >> 8) & 0xFF;
            uint8_t b2 = (destOp.value >> 16) & 0xFF;
            uint8_t b3 = (destOp.value >> 24) & 0xFF;

            if (b0 == 0) {
                e.lda_imm(0);
            } else {
                if (!destOp.symbolName.empty()) e.recordSymbolRelocLo(destOp.symbolName);
                e.lda_imm(b0);
            }
            sta_ind_z_with_y(bufAddrZP, destOffset);

            if (b1 == 0) {
                e.lda_imm(0);
            } else {
                if (!destOp.symbolName.empty()) e.recordSymbolRelocHi(destOp.symbolName, b0);
                e.lda_imm(b1);
            }
            sta_ind_z_with_y(bufAddrZP, destOffset + 1);

            // Byte 2 (low byte of upper 16-bit)
            if (b2 == 0) {
                e.lda_imm(0);
            } else {
                if (!destOp.symbolName.empty()) e.recordSymbolReloc32Bit(destOp.symbolName);
                e.lda_imm(b2);
            }
            sta_ind_z_with_y(bufAddrZP, destOffset + 2);

            // Byte 3 (high byte of upper 16-bit)
            e.lda_imm(b3);
            sta_ind_z_with_y(bufAddrZP, destOffset + 3);
        }
    }

    // Write length (count)
    if (lenOp.kind == REGISTER) {
        storeRegValue(lenOp.regName, lenOffset, lenOp.width);
    } else if (lenOp.kind == VALUE) {
        if (lenOp.width == WIDTH_16) {
            uint8_t lo = lenOp.value & 0xFF;
            uint8_t hi = (lenOp.value >> 8) & 0xFF;
            e.lda_imm(lo);
            sta_ind_z_with_y(bufAddrZP, lenOffset);
            e.lda_imm(hi);
            sta_ind_z_with_y(bufAddrZP, lenOffset + 1);
        } else {
            // 32-bit length
            uint8_t b0 = lenOp.value & 0xFF;
            uint8_t b1 = (lenOp.value >> 8) & 0xFF;
            uint8_t b2 = (lenOp.value >> 16) & 0xFF;
            uint8_t b3 = (lenOp.value >> 24) & 0xFF;

            e.lda_imm(b0);
            sta_ind_z_with_y(bufAddrZP, lenOffset);
            e.lda_imm(b1);
            sta_ind_z_with_y(bufAddrZP, lenOffset + 1);
            e.lda_imm(b2);
            sta_ind_z_with_y(bufAddrZP, lenOffset + 2);
            e.lda_imm(b3);
            sta_ind_z_with_y(bufAddrZP, lenOffset + 3);
        }
    }

    // Zero-fill remaining fields
    e.lda_imm(0);
    sta_ind_z_with_y(bufAddrZP, destBankOffset);     // Destination bank/flags
    sta_ind_z_with_y(bufAddrZP, extraOffset);        // Command MSB
    if (extraOffset + 1 < 256) sta_ind_z_with_y(bufAddrZP, extraOffset + 1);  // Modulo LSB
    if (extraOffset + 2 < 256) sta_ind_z_with_y(bufAddrZP, extraOffset + 2);  // Modulo MSB

    // Trigger DMA
    e.lda_zp(bufAddrZP + 1);
    e.sta_addr(m65::DMA_ADDR_MI);
    e.stz_addr(m65::DMA_ADDR_HI);
    e.lda_zp(bufAddrZP);
    e.sta_addr(m65::DMA_ADDR_LO);
    e.stz_addr(m65::DMA_CONTROL);
}

void AssemblerSimulatedOps::emitMoveCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix, bool forceStack) {

    int idx = tokenIndex;
    if (idx < 0 || idx >= (int)parser->tokens.size()) return;

    enum OperandKind { NONE, REGISTER, STACK_REL, VALUE };
    enum OperandWidth { WIDTH_16 = 2, WIDTH_24 = 3, WIDTH_32 = 4 };

    struct Operand {
        OperandKind kind = NONE;
        OperandWidth width = WIDTH_16;
        std::string regName;      // register name for REGISTER operands
        uint32_t value = 0;       // value for VALUE or STACK_REL operands
        std::string symbolName;   // symbol relocation if isAddress
    };

    // Helper to detect operand width from register name
    auto getRegisterWidth = [](const std::string& regName) -> OperandWidth {
        std::string upper = regName;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        // 32-bit: AXYZ, Q
        if (upper == "AXYZ" || upper == "Q") return WIDTH_32;
        // 24-bit: AXY, AXZ, AYZ (zero-extended to 32-bit in buffer)
        if (upper == "AXY" || upper == "AXZ" || upper == "AYZ") return WIDTH_32;
        // 16-bit: everything else (single regs, pairs)
        return WIDTH_16;
    };

    // Helper to detect operand width from value
    auto getValueWidth = [](uint32_t value) -> OperandWidth {
        if (value <= 0xFFFF) return WIDTH_16;
        return WIDTH_32;
    };

    auto parseOperand = [&](int& curIdx) -> Operand {
        Operand op;
        if (curIdx >= (int)parser->tokens.size() || parser->tokens[curIdx].type == AssemblerTokenType::COMMA) {
            return op;  // NONE
        }

        const auto& tok = parser->tokens[curIdx];

        // Check for REGISTER token (standard registers like "A", "X", "Y", "Z")
        if (tok.type == AssemblerTokenType::REGISTER) {
            op.kind = REGISTER;
            op.regName = tok.value;
            op.width = getRegisterWidth(tok.value);
            curIdx++;
            return op;
        }

        // Check for DIRECTIVE token with register pair names (AX, AY, AZ, XY, XZ, YZ, etc.)
        if (tok.type == AssemblerTokenType::DIRECTIVE) {
            std::string upper = tok.value;
            std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
            if (upper == "AX" || upper == "AY" || upper == "AZ" ||
                upper == "XY" || upper == "XZ" || upper == "YZ" ||
                upper == "XYZ" || upper == "AXY" || upper == "AXZ" || upper == "AYZ" || upper == "AXYZ" || upper == "Q") {
                op.kind = REGISTER;
                op.regName = upper;
                op.width = getRegisterWidth(upper);
                curIdx++;
                return op;
            }
        }

        // Check for stack-relative operand (always 16-bit for local addresses)
        uint32_t stackOffset = 0;
        if (parser->isStackRelativeOperand(curIdx, stackOffset, scopePrefix)) {
            op.kind = STACK_REL;
            op.value = stackOffset;
            op.width = WIDTH_16;  // Stack-relative is always 16-bit
            parseExprAST(parser->tokens, curIdx, parser->symbolTable, scopePrefix);
            if (curIdx < (int)parser->tokens.size() && parser->tokens[curIdx].type == AssemblerTokenType::COMMA) curIdx++;
            if (curIdx < (int)parser->tokens.size() && (parser->tokens[curIdx].value == "s" || parser->tokens[curIdx].value == "S")) curIdx++;
            return op;
        }

        // Parse as expression (constant or symbol)
        int symIdx = curIdx;
        auto ast = parseExprAST(parser->tokens, curIdx, parser->symbolTable, scopePrefix);
        if (ast) {
            op.kind = VALUE;
            op.value = ast->getValue(parser);
            // Check if this is a symbol that needs relocation
            if (symIdx < (int)parser->tokens.size()) {
                std::string name = parser->tokens[symIdx].value;
                Symbol* sym = parser->resolveSymbol(name, scopePrefix);
                if (sym && sym->isAddress) {
                    op.symbolName = name;
                    op.width = getValueWidth(sym->value);  // Use symbol's value for width
                } else {
                    op.width = getValueWidth(op.value);
                }
            } else {
                op.width = getValueWidth(op.value);
            }
        }
        return op;
    };

    // Parse up to 3 operands: src, dest, count
    Operand ops[3];
    ops[0] = parseOperand(idx);  // src
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    ops[1] = parseOperand(idx);  // dest
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    ops[2] = parseOperand(idx);  // count

    // Handle forceStack: if neither src nor dest is stack-relative, make one of them so
    if (forceStack && ops[0].kind != STACK_REL && ops[1].kind != STACK_REL) {
        if (ops[0].kind == VALUE && ops[1].kind != VALUE) {
            ops[0].kind = STACK_REL;
        } else {
            ops[1].kind = STACK_REL;
        }
    }

    // Get stack base for stack-relative calculations
    uint16_t sb = e.spBase();

    // Buffer is allocated in BSS segment, referenced via symbol relocations
    std::string bufSymbol = "__move_dma_buf";

    // Load buffer address from symbol into zero page ($02-$03)
    // This allows runtime relocation while maintaining clean code generation
    uint8_t bufAddrZP = 0x02;
    e.recordSymbolRelocHi(bufSymbol, 0);
    e.lda_imm(0);  // placeholder, linker fills in hi byte
    e.sta_zp(bufAddrZP + 1);

    e.recordSymbolRelocLo(bufSymbol);
    e.lda_imm(0);  // placeholder, linker fills in lo byte
    e.sta_zp(bufAddrZP);

    // Phase 1: Store register operands using ($bufAddrZP),Y addressing
    // Helper to emit STA ($addr),Y with custom Y value (avoiding sta_ind_z which hardcodes LDY #0)
    auto sta_ind_z_with_y = [&](uint8_t addr, int offset) {
        e.ldy_imm(offset);
        e.emitByte(0x91);  // STA ($addr),Y opcode
        e.emitByte(addr);
    };

    auto storeRegValue = [&](const std::string& reg, int bufOffset, OperandWidth width) {
        std::string upper = reg;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (width == WIDTH_32) {
            // 4-byte register values (32-bit and 24-bit zero-extended)
            if (upper == "AXYZ" || upper == "Q") {
                // Full 32-bit: A, X, Y, Z
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
            else if (upper == "AXY") {
                // 24-bit: A, X, Y, then zero for MSB
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
            else if (upper == "AXZ") {
                // 24-bit: A, X, Z, then zero for MSB
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
            else if (upper == "AYZ") {
                // 24-bit: A, Y, Z, then zero for MSB
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
        } else {
            // 16-bit (2-byte) register values
            // 2-byte register pairs
            if (upper == "AX") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "AY") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "AZ") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "XY") {
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "XZ") {
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "YZ") {
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            // Single registers: extend to 16-bit with high byte = 0
            else if (upper == "X") {
                e.txa(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "Y") {
                e.tya(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "Z") {
                e.tza(); sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
            else if (upper == "A") {
                sta_ind_z_with_y(bufAddrZP, bufOffset);
                e.lda_imm(0); sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            }
        }
    };

    // Calculate buffer offsets based on operand widths (count, src, dest)
    // Layout: buf+0=cmd, buf+1+=count, then src, src_bank, then dest, dest_bank, then extra fields
    int countOffset = 1;
    int countSize = (int)ops[2].width;
    int srcOffset = countOffset + countSize;
    int srcSize = (int)ops[0].width;
    int srcBankOffset = srcOffset + srcSize;
    int destOffset = srcBankOffset + 1;
    int destSize = (int)ops[1].width;
    int destBankOffset = destOffset + destSize;
    int extraOffset = destBankOffset + 1;

    // Phase 1: Store register operands
    for (int i = 0; i < 3; i++) {
        if (ops[i].kind == REGISTER) {
            int bufOffset = (i == 2) ? countOffset : (i == 0) ? srcOffset : destOffset;
            storeRegValue(ops[i].regName, bufOffset, ops[i].width);
        }
    }

    // Phase 2: Stack-relative operands (always 16-bit)
    for (int i = 0; i < 3; i++) {
        if (ops[i].kind == STACK_REL) {
            int bufOffset = (i == 2) ? countOffset : (i == 0) ? srcOffset : destOffset;
            uint16_t addr = sb + ops[i].value;
            e.tsx(); e.txa(); e.clc(); e.adc_imm(addr & 0xFF);
            sta_ind_z_with_y(bufAddrZP, bufOffset);
            e.lda_imm((addr >> 8) & 0xFF);
            if ((addr & 0xFF) == 0) e.adc_imm(0);
            sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            // Zero-fill remaining bytes if 32-bit field
            if (ops[i].width == WIDTH_32) {
                e.lda_imm(0);
                sta_ind_z_with_y(bufAddrZP, bufOffset + 2);
                sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
        }
    }

    // Phase 3: Value/expression operands
    for (int i = 0; i < 3; i++) {
        if (ops[i].kind == VALUE) {
            int bufOffset = (i == 2) ? countOffset : (i == 0) ? srcOffset : destOffset;
            OperandWidth width = ops[i].width;

            if (width == WIDTH_16) {
                // 16-bit: store 2 bytes
                uint8_t lo = ops[i].value & 0xFF;
                uint8_t hi = (ops[i].value >> 8) & 0xFF;

                if (lo == 0) {
                    e.lda_imm(0);
                } else {
                    if (!ops[i].symbolName.empty()) e.recordSymbolRelocLo(ops[i].symbolName);
                    e.lda_imm(lo);
                }
                sta_ind_z_with_y(bufAddrZP, bufOffset);

                if (hi == 0) {
                    e.lda_imm(0);
                } else {
                    if (!ops[i].symbolName.empty()) e.recordSymbolRelocHi(ops[i].symbolName, lo);
                    e.lda_imm(hi);
                }
                sta_ind_z_with_y(bufAddrZP, bufOffset + 1);
            } else {
                // 32-bit: store 4 bytes with relocation support
                uint8_t b0 = ops[i].value & 0xFF;
                uint8_t b1 = (ops[i].value >> 8) & 0xFF;
                uint8_t b2 = (ops[i].value >> 16) & 0xFF;
                uint8_t b3 = (ops[i].value >> 24) & 0xFF;

                // Byte 0 (low byte of 32-bit value)
                if (b0 == 0) {
                    e.lda_imm(0);
                } else {
                    if (!ops[i].symbolName.empty()) e.recordSymbolRelocLo(ops[i].symbolName);
                    e.lda_imm(b0);
                }
                sta_ind_z_with_y(bufAddrZP, bufOffset);

                // Byte 1 (high byte of lower 16-bit)
                if (b1 == 0) {
                    e.lda_imm(0);
                } else {
                    if (!ops[i].symbolName.empty()) e.recordSymbolRelocHi(ops[i].symbolName, b0);
                    e.lda_imm(b1);
                }
                sta_ind_z_with_y(bufAddrZP, bufOffset + 1);

                // Byte 2 (low byte of upper 16-bit)
                if (b2 == 0) {
                    e.lda_imm(0);
                } else {
                    if (!ops[i].symbolName.empty()) e.recordSymbolReloc32Bit(ops[i].symbolName);
                    e.lda_imm(b2);
                }
                sta_ind_z_with_y(bufAddrZP, bufOffset + 2);

                // Byte 3 (high byte of upper 16-bit)
                e.lda_imm(b3);
                sta_ind_z_with_y(bufAddrZP, bufOffset + 3);
            }
        }
    }

    // Phase 4: Zero-fill unspecified operands and always-zero fields
    e.lda_imm(0);
    sta_ind_z_with_y(bufAddrZP, 0);   // Command = COPY at buf+0

    // Zero-fill missing operands
    if (ops[2].kind == NONE) {
        // Count is zero
        for (int j = 0; j < countSize; j++) {
            sta_ind_z_with_y(bufAddrZP, countOffset + j);
        }
    }
    if (ops[0].kind == NONE) {
        // Source is zero
        for (int j = 0; j < srcSize; j++) {
            sta_ind_z_with_y(bufAddrZP, srcOffset + j);
        }
    }
    if (ops[1].kind == NONE) {
        // Destination is zero
        for (int j = 0; j < destSize; j++) {
            sta_ind_z_with_y(bufAddrZP, destOffset + j);
        }
    }

    // Zero-fill bank/flags fields
    sta_ind_z_with_y(bufAddrZP, srcBankOffset);      // Source bank/flags
    sta_ind_z_with_y(bufAddrZP, destBankOffset);     // Destination bank/flags
    sta_ind_z_with_y(bufAddrZP, extraOffset);        // Command MSB
    if (extraOffset + 1 < 256) sta_ind_z_with_y(bufAddrZP, extraOffset + 1);  // Modulo LSB
    if (extraOffset + 2 < 256) sta_ind_z_with_y(bufAddrZP, extraOffset + 2);  // Modulo MSB

    // Trigger DMA - load buffer address and pass to DMA controller
    e.lda_zp(bufAddrZP + 1);  // hi byte
    e.sta_addr(m65::DMA_ADDR_MI);
    e.stz_addr(m65::DMA_ADDR_HI);
    e.lda_zp(bufAddrZP);      // lo byte
    e.sta_addr(m65::DMA_ADDR_LO);
    e.stz_addr(m65::DMA_CONTROL);        // Triggers DMA
}

void AssemblerSimulatedOps::emitFlatMemoryCode(AssemblerParser* parser, M65Emitter& e, const std::string& mnemonic, int tokenIndex, const std::string& scopePrefix) {
    std::string op = parser->tokens[tokenIndex].value;
    Symbol* sym = parser->resolveSymbol(op, scopePrefix); uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(op); } catch(...) { addr = 0; } }
    e.eom();
    if (mnemonic == "LDW.F") { e.lda_addr(addr); e.eom(); e.ldx_addr(addr + 1); }
    else if (mnemonic == "STW.F") { e.sta_addr(addr); e.eom(); e.stx_addr(addr + 1); }
    else if (mnemonic == "INC.F") e.inc_addr(addr); else if (mnemonic == "DEC.F") e.dec_addr(addr);
}

void AssemblerSimulatedOps::emitPHWStackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    auto fa = parser->resolveFrameAccess(tokenIndex, scopePrefix);
    if (fa.isFrame) {
        // Push 16-bit frame-relative value: hi byte first, then lo
        uint8_t totalOff = fa.fpOff + fa.yOff;
        e.lda_stack(totalOff + 1); e.pha();
        e.lda_stack(totalOff); e.pha();
    } else {
        uint32_t offset = parser->evaluateExpressionAt(tokenIndex, scopePrefix);
        uint16_t sb = e.spBase();
        e.tsx();
        e.recordSpBaseReloc(offset + 1); e.lda_abs_x(sb + offset + 1); e.pha();
        e.recordSpBaseReloc(offset); e.lda_abs_x(sb + offset); e.pha();
    }
}

void AssemblerSimulatedOps::emitASWCode(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX") {
        e.asl_a(); e.pha(); e.txa(); e.rol_a(); e.tax(); e.pla();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.asw_abs(addr);
    }
}

void AssemblerSimulatedOps::emitROWCode(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX") {
        e.pha(); e.txa(); e.ror_a(); e.tax(); e.pla(); e.ror_a();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.row_abs(addr);
    }
}

void AssemblerSimulatedOps::emitLSL16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX") {
        e.asl_a(); e.pha(); e.txa(); e.rol_a(); e.tax(); e.pla();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.asl_addr(addr); e.rol_addr(addr + 1);
    }
}

void AssemblerSimulatedOps::emitLSR16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX") {
        e.pha(); e.txa(); e.lsr_a(); e.tax(); e.pla(); e.ror_a();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.lsr_addr(addr + 1); e.ror_addr(addr);
    }
}

void AssemblerSimulatedOps::emitROL16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX") {
        e.rol_a(); e.pha(); e.txa(); e.rol_a(); e.tax(); e.pla();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.rol_addr(addr); e.rol_addr(addr + 1);
    }
}

void AssemblerSimulatedOps::emitROR16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX") {
        e.pha(); e.txa(); e.ror_a(); e.tax(); e.pla(); e.ror_a();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.ror_addr(addr + 1); e.ror_addr(addr);
    }
}

void AssemblerSimulatedOps::emitASR16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AX") {
        e.pha(); e.txa(); e.cmp_imm(0x80); e.ror_a(); e.tax(); e.pla(); e.ror_a();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.lda_addr(addr + 1); e.cmp_imm(0x80); e.ror_addr(addr + 1); e.ror_addr(addr);
    }
}

void AssemblerSimulatedOps::emitSXT8Code(AssemblerParser*, M65Emitter& e, int, const std::string&) {
    e.pha(); e.cmp_imm(0x80); e.lda_imm(0); auto br1659 = e.emitBranchPlaceholder(0x90); e.lda_imm(0xFF); e.patchBranchTarget(br1659); e.tax(); e.pla();
}

static std::vector<std::string> getRegistersFromMnemonic(std::string reg) {
    std::transform(reg.begin(), reg.end(), reg.begin(), ::toupper);
    if (!reg.empty() && reg[0] == '.') reg = reg.substr(1);
    
    if (reg == "A") return {"A"};
    if (reg == "X") return {"X"};
    if (reg == "Y") return {"Y"};
    if (reg == "Z") return {"Z"};
    if (reg == "AX") return {"A", "X"};
    if (reg == "AY") return {"A", "Y"};
    if (reg == "AZ") return {"A", "Z"};
    if (reg == "XY") return {"X", "Y"};
    if (reg == "XZ") return {"X", "Z"};
    if (reg == "YZ") return {"Y", "Z"};
    if (reg == "AXY") return {"A", "X", "Y"};
    if (reg == "AXZ") return {"A", "X", "Z"};
    if (reg == "AYZ") return {"A", "Y", "Z"};
    if (reg == "XYZ") return {"X", "Y", "Z"};
    if (reg == "AXYZ" || reg == "Q") return {"A", "X", "Y", "Z"};
    return {};
}

void AssemblerSimulatedOps::emitPushPopCode(AssemblerParser*, M65Emitter& e, bool isPush, const std::string& reg, int, const std::string&) {
    std::vector<std::string> regs = getRegistersFromMnemonic(reg);
    if (isPush) {
        // Push in reverse order so low byte (A) ends up at lower address (little-endian)
        for (int i = (int)regs.size() - 1; i >= 0; --i) {
            const auto& r = regs[i];
            if (r == "A") e.pha();
            else if (r == "X") e.phx();
            else if (r == "Y") e.phy();
            else if (r == "Z") e.phz();
        }
    } else {
        // Pop in forward order (inverse of reversed push)
        for (const auto& r : regs) {
            if (r == "A") e.pla();
            else if (r == "X") e.plx();
            else if (r == "Y") e.ply();
            else if (r == "Z") e.plz();
        }
    }
}

int AssemblerSimulatedOps::getPushPopSize(AssemblerParser*, bool, const std::string& reg, int, const std::string&) {
    return (int)getRegistersFromMnemonic(reg).size();
}

uint32_t AssemblerSimulatedOps::resolveAbsAddr(AssemblerParser* parser, int tokenIndex, const std::string& scopePrefix) {
    std::string srcName = parser->tokens[tokenIndex].value;
    Symbol* sym = parser->resolveSymbol(srcName, scopePrefix);
    if (sym) return sym->value;
    try { return parseNumericLiteral(srcName); } catch(...) { return 0; }
}

// Emit signed 16-bit multiply/divide/mod.
// op: 0=mul, 1=div, 2=mod
// Left in .AX, right from tokenIndex (constant or memory address).
// Hardware: mul=$D770/$D774→$D778, div=$D760/$D764→$D768, remainder=$D770.
// $D76E used as sign scratch byte.
void AssemblerSimulatedOps::emitSignedMathOp(AssemblerParser* parser, M65Emitter& e, int op,
                             const std::string& /*dest*/, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH);
    if (isImmediate) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;

    const uint16_t SIGN = m65::MATH_SIGN;
    uint16_t leftBase  = (op == 0) ? m65::MULT_ARG1 : m65::DIV_ARG1;
    uint16_t rightBase = (op == 0) ? m65::MULT_ARG2 : m65::DIV_ARG2;

    bool srcIsConst = isImmediate && srcAst->isConstant(parser);

    // --- Step 1: Save sign of left operand and compute abs(left) ---
    // Must preserve AX intact for neg_16, so save A before using it.
    e.pha();                              // save original A (byte 0)
    e.txa();                              // A = X = byte 1 (sign byte)
    e.sta_addr(SIGN);                     // SIGN = left sign byte
    auto brPos = e.emitBranchPlaceholder(0x10); // BPL: left is positive
    e.pla();                              // restore A for neg_16
    e.neg_16();                           // AX = abs(left)
    auto brDone = e.emitBranchPlaceholder(0x80); // BRA: skip positive-path pla
    e.patchBranchTarget(brPos);
    e.pla();                              // restore A (positive case)
    e.patchBranchTarget(brDone);

    // --- Step 2: Store abs(left) in hardware regs ---
    e.sta_addr(leftBase);
    e.txa();
    e.sta_addr(leftBase + 1);

    // --- Step 3: Compute result sign (XOR of operand signs for mul/div) ---
    if (op != 2 && !srcIsConst) {
        uint32_t srcAddr = resolveAbsAddr(parser, tokenIndex, scopePrefix);
        e.lda_addr(srcAddr + 1);      // right high byte
        e.eor_addr(SIGN);             // XOR signs
        e.sta_addr(SIGN);             // SIGN = left_sign ^ right_sign
    } else if (op != 2 && srcIsConst) {
        int32_t val = (int32_t)(int16_t)srcAst->getValue(parser);
        if (val < 0) {
            e.lda_addr(SIGN);
            e.eor_imm(0x80);
            e.sta_addr(SIGN);
        }
    }

    // --- Step 4: Store abs(right) in hardware regs ---
    if (srcIsConst) {
        int32_t val = (int32_t)(int16_t)srcAst->getValue(parser);
        if (val < 0) val = -val;
        e.lda_imm(val & 0xFF);        e.sta_addr(rightBase);
        e.lda_imm((val >> 8) & 0xFF); e.sta_addr(rightBase + 1);
    } else {
        uint32_t srcAddr = resolveAbsAddr(parser, tokenIndex, scopePrefix);
        // Load source low/high into hw regs
        e.lda_addr(srcAddr);       e.sta_addr(rightBase);       // 3+3=6
        e.lda_addr(srcAddr + 1);   e.sta_addr(rightBase + 1);   // 3+3=6
        // If source negative, negate the stored 16-bit value in hw regs
        // Check sign: lda_abs already loaded high byte and set N flag
        // Negation block: lda(3)+eor(2)+sec(1)+adc(2)+sta(3)+lda(3)+eor(2)+adc(2)+sta(3) = 21 bytes
        auto br1784 = e.emitBranchPlaceholder(0x10);              // BPL: skip negation
        e.lda_addr(rightBase);     e.eor_imm(0xFF); e.sec(); e.adc_imm(1); e.sta_addr(rightBase);      // 11
        e.lda_addr(rightBase + 1); e.eor_imm(0xFF);            e.adc_imm(0); e.sta_addr(rightBase + 1); // 10
        e.patchBranchTarget(br1784);
    }

    // --- Step 5: Wait for hardware (div/mod only) ---
    if (op != 0) {
        e.bit_addr(m65::MATH_BUSY_STATUS); e.bne(-5);
    }

    // --- Step 6: Read result ---
    if (op == 0) {
        e.lda_addr(m65::MULT_RES); e.ldx_addr(m65::MULT_RES + 1);
    } else if (op == 1) {
        e.lda_addr(m65::DIV_RES); e.ldx_addr(m65::DIV_RES + 1);
    } else {
        e.lda_addr(m65::DIV_REM); e.ldx_addr(m65::DIV_REM + 1);
    }

    // --- Step 7: Sign correction ---
    // If SIGN bit 7 set, negate .AX.
    // Sequence: pha(1) lda_abs(3) bpl(2) pla(1) neg_16(13) bra(2) pla(1) = 23 total
    // bpl skips: pla(1)+neg_16(13)+bra(2) = 16 bytes
    e.pha();
    e.lda_addr(SIGN);
    auto br1810 = e.emitBranchPlaceholder(0x10); // BPL: skip sign correction
    e.pla();
    e.neg_16();
    auto br1813 = e.emitBranchPlaceholder(0x80); // BRA: skip positive-path pla
    e.patchBranchTarget(br1810);
    e.pla();
    e.patchBranchTarget(br1813);
}

// emitSignedMathOp32 — 32-bit signed mul/div/mod via abs→unsigned hw→sign fixup
// op: 0=mul, 1=div, 2=mod
// Assumes .AXYZ holds left operand; src2 from operand token.
void AssemblerSimulatedOps::emitSignedMathOp32(AssemblerParser* parser, M65Emitter& e, int op,
                             const std::string& /*dest*/, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH);
    if (isImmediate) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;

    const uint16_t SIGN = m65::MATH_SIGN;
    uint16_t leftBase  = (op == 0) ? m65::MULT_ARG1 : m65::DIV_ARG1;
    uint16_t rightBase = (op == 0) ? m65::MULT_ARG2 : m65::DIV_ARG2;
    bool srcIsConst = isImmediate && srcAst->isConstant(parser);

    // --- Step 1: Save sign of left operand and compute abs(left) ---
    // Must preserve AXYZ intact for neg_32, so save A before using it.
    e.pha();                              // save original A (byte 0)
    e.tza();                              // A = Z = byte 3 (sign byte)
    e.sta_addr(SIGN);                     // SIGN = left sign byte
    auto brPos = e.emitBranchPlaceholder(0x10); // BPL: left is positive, skip neg
    e.pla();                              // restore A for neg_32
    e.neg_32();                           // AXYZ = abs(left)
    auto brDone = e.emitBranchPlaceholder(0x80); // BRA: skip positive-path pla
    e.patchBranchTarget(brPos);
    e.pla();                              // restore A (positive case)
    e.patchBranchTarget(brDone);

    // --- Step 2: Store abs(left) in hardware regs ---
    e.sta_addr(leftBase);
    e.txa(); e.sta_addr(leftBase + 1);
    e.tya(); e.sta_addr(leftBase + 2);
    e.tza(); e.sta_addr(leftBase + 3);

    // --- Step 3: Compute result sign (XOR of operand signs for mul/div) ---
    if (op != 2 && !srcIsConst) {
        uint32_t srcAddr = resolveAbsAddr(parser, tokenIndex, scopePrefix);
        e.lda_addr(srcAddr + 3);          // right high byte (byte 3)
        e.eor_addr(SIGN);                 // XOR signs
        e.sta_addr(SIGN);                 // SIGN = left_sign ^ right_sign
    } else if (op != 2 && srcIsConst) {
        int64_t val = (int64_t)(int32_t)srcAst->getValue(parser);
        if (val < 0) {
            e.lda_addr(SIGN);
            e.eor_imm(0x80);
            e.sta_addr(SIGN);
        }
    }

    // --- Step 4: Store abs(right) in hardware regs ---
    if (srcIsConst) {
        int64_t val = (int64_t)(int32_t)srcAst->getValue(parser);
        if (val < 0) val = -val;
        for (int i = 0; i < 4; ++i) {
            e.lda_imm((val >> (i * 8)) & 0xFF);
            e.sta_addr(rightBase + i);
        }
    } else {
        uint32_t srcAddr = resolveAbsAddr(parser, tokenIndex, scopePrefix);
        for (int i = 0; i < 4; ++i) {
            e.lda_addr(srcAddr + i);
            e.sta_addr(rightBase + i);
        }
        // If source negative, negate the stored 32-bit value in hw regs
        // Last lda loaded byte 3 (sign byte) — N flag is set if negative
        auto br2 = e.emitBranchPlaceholder(0x10); // BPL: skip negation
        e.lda_addr(rightBase);     e.eor_imm(0xFF); e.sec(); e.adc_imm(0); e.sta_addr(rightBase);
        e.lda_addr(rightBase + 1); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(rightBase + 1);
        e.lda_addr(rightBase + 2); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(rightBase + 2);
        e.lda_addr(rightBase + 3); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(rightBase + 3);
        e.patchBranchTarget(br2);
    }

    // --- Step 5: Wait for hardware (div/mod only) ---
    if (op != 0) {
        e.bit_addr(m65::MATH_BUSY_STATUS); e.bne(-5);
    }

    // --- Step 6: Read result into .AXYZ ---
    if (op == 0) {
        e.lda_addr(m65::MULT_RES + 3); e.taz();
        e.lda_addr(m65::MULT_RES + 2); e.tay();
        e.lda_addr(m65::MULT_RES + 1); e.tax();
        e.lda_addr(m65::MULT_RES);
    } else if (op == 1) {
        e.lda_addr(m65::DIV_RES + 3); e.taz();
        e.lda_addr(m65::DIV_RES + 2); e.tay();
        e.lda_addr(m65::DIV_RES + 1); e.tax();
        e.lda_addr(m65::DIV_RES);
    } else {
        e.lda_addr(m65::DIV_REM + 3); e.taz();
        e.lda_addr(m65::DIV_REM + 2); e.tay();
        e.lda_addr(m65::DIV_REM + 1); e.tax();
        e.lda_addr(m65::DIV_REM);
    }

    // --- Step 7: Sign correction ---
    // If SIGN bit 7 set, negate .AXYZ
    e.pha();
    e.lda_addr(SIGN);
    auto br3 = e.emitBranchPlaceholder(0x10); // BPL: skip sign correction
    e.pla();
    e.neg_32();
    auto br4 = e.emitBranchPlaceholder(0x80); // BRA: skip positive-path pla
    e.patchBranchTarget(br3);
    e.pla();
    e.patchBranchTarget(br4);
}

// mul.s16 .ax, src — Signed 16-bit multiply
void AssemblerSimulatedOps::emitMulS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    emitSignedMathOp(parser, e, 0, dest, tokenIndex, scopePrefix);
}

// div.s16 .ax, src — Signed 16-bit divide
void AssemblerSimulatedOps::emitDivS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    emitSignedMathOp(parser, e, 1, dest, tokenIndex, scopePrefix);
}

// mod.16 .ax, src — Unsigned 16-bit modulo (div then read remainder)
// mod.s16 .ax, src — Signed 16-bit modulo
void AssemblerSimulatedOps::emitMod16Code(AssemblerParser* parser, M65Emitter& e, bool isSigned, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    if (isSigned) {
        emitSignedMathOp(parser, e, 2, dest, tokenIndex, scopePrefix);
    } else {
        // Unsigned mod: perform div.16, then read remainder from $D770/$D771
        // (The div hardware leaves the remainder there after any division.)
        emitDivCode(parser, e, 16, dest, tokenIndex, scopePrefix);
        // divCode loaded quotient into .AX from $D768. Override with remainder:
        e.lda_addr(m65::DIV_REM);
        e.ldx_addr(m65::DIV_REM + 1);
    }
}

// --- Frame-pointer pseudo-ops ---
// All use ($FP,SP),Y addressing where _fp is the SP-relative offset to the frame base

// lda.fp varOffset — Load 8-bit value from frame-relative offset into A
void AssemblerSimulatedOps::emitLDA_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    e.lda_frame(fpOff, yOff);
}

// sta.fp varOffset — Store 8-bit value from A to frame-relative offset
void AssemblerSimulatedOps::emitSTA_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    e.sta_frame(fpOff, yOff);
}

// ldax.fp varOffset — Load 16-bit value from frame into AX (lo in A, hi in X)
void AssemblerSimulatedOps::emitLDAX_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    if (e.hasFramePointer()) {
        e.lda_stack(totalOff + 1);
        e.sta_scratch();
        e.lda_stack(totalOff);
        e.ldx_scratch();
    } else {
        // Single TSX (cached): hi → push → lo → plx. X holds old SP throughout;
        // pha changes hardware SP but X still valid for stack-relative loads.
        e.tsxCached();
        e.lda_stack_noTSX(totalOff + 1);  // A = hi byte
        e.pha();                           // push hi (SP changes but X unchanged)
        e.lda_stack_noTSX(totalOff);       // A = lo byte (X still = old SP)
        e.plx();                           // X = hi byte
    }
}

// lday.fp varOffset — Load 16-bit value from frame into AY (lo in A, hi in Y)
// Use when Z must be preserved (e.g., loop counter).
void AssemblerSimulatedOps::emitLDAY_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    e.ldy_stack(totalOff + 1);       // Y = hi byte
    e.lda_stack(totalOff);           // A = lo byte
}

// stay.fp varOffset — Store AY (lo=A, hi=Y) to frame-relative offset
// Use when Z must be preserved (e.g., loop counter).
void AssemblerSimulatedOps::emitSTAY_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    e.sta_stack(totalOff);           // TSX + STA abs,X (lo byte)
    e.sty_stack(totalOff + 1);       // TSX + STY abs,X (hi byte)
}

// ldaz.fp varOffset — Load 16-bit value from frame into AZ (lo in A, hi in Z)
// Preferred over ldax.fp: leaves X free for TSX, no scratch needed.
void AssemblerSimulatedOps::emitLDAZ_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    e.ldz_stack(totalOff + 1);       // Z = hi byte
    e.lda_stack(totalOff);           // A = lo byte
}

// stax.fp varOffset — Store AX (16-bit) to frame-relative offset
// Transfers X→Z internally for IRQ-safe frame access (no ZP scratch).
// For constant values, prefer staz.fp which avoids the transfer entirely.
void AssemblerSimulatedOps::emitSTAX_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    // Move X (hi byte) to Z via A, preserving A on stack
    e.pha(); e.txa(); e.taz(); e.pla();
    e.sta_stack(totalOff);           // TSX + STA abs,X (lo byte)
    e.stz_stack(totalOff + 1);       // STZ abs,X (hi byte)
}

// staz.fp varOffset — Store AZ (lo=A, hi=Z) to frame-relative offset
// Preferred over stax.fp: no ZP scratch needed, IRQ-safe.
void AssemblerSimulatedOps::emitSTAZ_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    e.sta_stack(totalOff);           // TSX + STA abs,X (lo byte)
    e.stz_stack(totalOff + 1);       // STZ abs,X (hi byte)
}

// ldaxyz.fp varOffset — Load 32-bit value from frame into A,X,Y,Z
void AssemblerSimulatedOps::emitLDAXYZ_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    
    // Load all bytes into scratch first (lda_stack clobbers X in non-FP mode)
    e.lda_stack(totalOff + 3); e.sta_scratch3_hi();
    e.lda_stack(totalOff + 2); e.sta_scratch3();
    e.lda_stack(totalOff + 1); e.sta_scratch2();
    e.lda_stack(totalOff); // byte 0 stays in A
    
    e.ldx_scratch2();
    e.ldy_scratch3();
    e.ldz_scratch3_hi();
}

// staxyz.fp varOffset — Store 32-bit value from A,X,Y,Z to frame
void AssemblerSimulatedOps::emitSTAXYZ_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    uint8_t totalOff = fpOff + yOff;
    
    // Store all bytes to scratch first (sta_stack clobbers X in non-FP mode)
    e.sta_scratch();      // A
    e.stx_scratch2();     // X
    e.sty_scratch3();     // Y
    e.stz_scratch3_hi();  // Z

    e.lda_scratch();    e.sta_stack(totalOff);
    e.lda_scratch2();   e.sta_stack(totalOff + 1);
    e.lda_scratch3();   e.sta_stack(totalOff + 2);
    e.lda_scratch3_hi(); e.sta_stack(totalOff + 3);
}

// leax.fp varOffset — Load effective address of frame variable into AX
// Computes: AX = __sp_base + _fp + varOffset + SPL
// Uses emitSpBaseAddrCalc for correct relocation in .o45 mode.
void AssemblerSimulatedOps::emitLEAX_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)parser->evaluateExpressionAt(tokenIndex, scopePrefix);
    emitSpBaseAddrCalc(parser, e, fpOff + yOff);
}

// move.fp dest, src, len — Block copy between frame-relative addresses
void AssemblerSimulatedOps::emitMOVE_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    if (idx < 0 || idx >= (int)parser->tokens.size()) return;

    Symbol* fpSym = parser->resolveSymbol("_fp", scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint16_t sb = e.spBase();

    // Parse dest offset
    auto destAST = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    uint32_t destOff = destAST ? destAST->getValue(parser) : 0;
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;

    // Parse src offset
    auto srcAST = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    uint32_t srcOff = srcAST ? srcAST->getValue(parser) : 0;
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;

    // Parse length
    uint32_t lenVal = 0;
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) {
        idx++;
    }
    auto lenAST = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    lenVal = lenAST ? lenAST->getValue(parser) : 0;

    // Compute absolute addresses: SP + spBase + fpOff + varOffset
    // We need to compute at runtime since SP is dynamic.
    // Build DMA job on stack (same as emitMoveCode but with frame-relative sources)
    uint16_t srcTotal = sb + fpOff + (uint16_t)srcOff;
    uint16_t destTotal = sb + fpOff + (uint16_t)destOff;

    // Push DMA job onto stack and trigger
    e.pha(); e.phw_imm(0); e.lda_imm(0); e.pha();
    // dest address: SPL + destTotal
    e.tsx(); e.txa(); e.clc(); e.adc_imm(destTotal & 0xFF); e.pha();
    e.lda_imm((destTotal >> 8) & 0xFF); e.adc_imm(0); e.pha();
    // dest bank + command
    e.lda_imm(0); e.pha();
    // src address: SPL + srcTotal
    e.tsx(); e.txa(); e.clc(); e.adc_imm((srcTotal + 6) & 0xFF); e.pha();  // +6 for stack growth
    e.lda_imm((srcTotal >> 8) & 0xFF); e.adc_imm(0); e.pha();
    // length
    e.lda_imm(lenVal >> 8); e.pha(); e.lda_imm(lenVal & 0xFF); e.pha();
    // DMA command byte
    e.lda_imm(0x00); e.pha();
    // Trigger DMA
    e.tsx(); e.txa(); e.clc(); e.adc_imm(sb & 0xFF);
    e.sta_addr(m65::DMA_ADDR_LO);
    e.lda_imm(sb >> 8); e.sta_addr(m65::DMA_ADDR_MI);
    e.stz_addr(m65::DMA_ADDR_HI); e.stz_addr(m65::DMA_CONTROL);
    // Clean up stack
    e.tsx(); e.txa(); e.clc(); e.adc_imm(12); e.tax(); e.txs();
    e.pla();
}

// ============================================================================
// Bitfield pseudo-ops
// ============================================================================

// bfext #bitoff, #width — extract bitfield from A (8-bit) or AX (16-bit)
// A (or AX) already contains the storage unit. Result in A with upper bits zeroed.
void AssemblerSimulatedOps::emitBFExtCode(AssemblerParser* parser, M65Emitter& e, int unitWidth, int tokenIndex, const std::string& scopePrefix) {
    // unitWidth: 8, 16, or 32 (byte count of storage unit)
    // Parse: #bitoff, #width
    auto parseImm = [&](int& idx) -> uint32_t {
        if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
        auto ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
        if (!ast) throw std::runtime_error("Expected immediate value in bitfield op");
        return ast->getValue(parser);
    };
    auto skipComma = [&](int& idx) {
        if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    };
    int idx = tokenIndex;
    uint32_t bitOff = parseImm(idx);
    skipComma(idx);
    uint32_t bitWidth = parseImm(idx);

    if (unitWidth == 32) {
        // AXYZ contains 32-bit storage unit (A=byte0, X=byte1, Y=byte2, Z=byte3)
        // Save all 4 bytes to scratch ZP area, shift right by bitOff, mask
        uint8_t s0 = e.scratchZP();      // byte 0
        uint8_t s1 = e.scratchZP() + 1;  // byte 1
        uint8_t s2 = e.scratchZP() + 2;  // byte 2 (scratch2)
        uint8_t s3 = e.scratchZP() + 3;  // byte 3 (scratch2+1)
        e.sta_zp(s0);
        e.stx_zp(s1);
        e.sty_zp(s2);
        e.stz_zp(s3);
        for (uint32_t i = 0; i < bitOff; i++) {
            e.emitInstruction("lsr", AddressingMode::BASE_PAGE, s3, true);
            e.emitInstruction("ror", AddressingMode::BASE_PAGE, s2, true);
            e.emitInstruction("ror", AddressingMode::BASE_PAGE, s1, true);
            e.emitInstruction("ror", AddressingMode::BASE_PAGE, s0, true);
        }
        uint32_t mask = (bitWidth >= 32) ? 0xFFFFFFFF : ((1u << bitWidth) - 1);
        e.lda_zp(s0);
        e.and_imm(mask & 0xFF);
        if (bitWidth > 8) {
            e.pha();
            e.lda_zp(s1);
            e.and_imm((mask >> 8) & 0xFF);
            e.tax();
            e.pla();
        } else {
            e.ldx_imm(0);
        }
        e.ldy_imm(0);
        e.ldz_imm(0);
    } else if (unitWidth == 16) {
        // AX contains 16-bit storage unit (A=lo, X=hi)
        e.stx_scratch();
        for (uint32_t i = 0; i < bitOff; i++) {
            e.emitInstruction("lsr", AddressingMode::BASE_PAGE, e.scratchZP(), true);
            e.ror_a();
        }
        uint16_t mask = (uint16_t)((1u << bitWidth) - 1);
        e.and_imm(mask & 0xFF);
        if (bitWidth > 8) {
            e.pha();
            e.lda_scratch();
            e.and_imm((mask >> 8) & 0xFF);
            e.tax();
            e.pla();
        } else {
            e.ldx_imm(0);
        }
    } else {
        // A contains 8-bit storage unit
        for (uint32_t i = 0; i < bitOff; i++) {
            e.lsr_a();
        }
        uint8_t mask = (uint8_t)((1u << bitWidth) - 1);
        e.and_imm(mask);
        e.ldx_imm(0);
    }
}

// bfins[.sp/.ind] addr, #bitoff, #width
// A contains new value to insert. Performs RMW on the storage unit.
// mode: 0=absolute, 1=stack-relative, 2=indirect (ZP pointer)
// Optimizations:
//   - Single bit on ZP/abs: use SMBn/RMBn when value is constant 0 or 1
//   - Multiple aligned bits on ZP: use multiple SMBn/RMBn when value is constant
//   - Multi-bit on abs/ZP: use TRB+TSB
void AssemblerSimulatedOps::emitBFInsCode(AssemblerParser* parser, M65Emitter& e, int unitWidth, int mode, int tokenIndex, const std::string& scopePrefix) {
    bool is16 = (unitWidth >= 16); // 16 and 32-bit both use the 16-bit RMW path for now
    auto parseImm = [&](int& idx) -> uint32_t {
        if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
        auto ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
        if (!ast) throw std::runtime_error("Expected immediate value in bitfield op");
        return ast->getValue(parser);
    };
    auto skipComma = [&](int& idx) {
        if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    };
    int idx = tokenIndex;

    // Parse address operand
    auto addrAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!addrAst) throw std::runtime_error("Expected address in bfins");
    uint32_t addr = addrAst->getValue(parser);
    bool addrIsZP = (addr < 0x100 && mode != 1); // ZP addressable (not stack-relative)

    skipComma(idx);
    uint32_t bitOff = parseImm(idx);
    skipComma(idx);
    uint32_t bitWidth = parseImm(idx);

    uint8_t mask8 = (uint8_t)((1u << bitWidth) - 1);
    uint16_t mask16 = (uint16_t)((1u << bitWidth) - 1);
    uint8_t shiftedMask8 = (uint8_t)(mask8 << bitOff);
    uint16_t shiftedMask16 = (uint16_t)(mask16 << bitOff);

    if (is16) {
        // 16-bit storage unit. A=lo of new value, X=hi of new value.
        // Mask to bitWidth, shift left by bitOff, then do RMW on both bytes.
        // Use scratch for lo byte, stack for hi byte.
        e.and_imm(mask16 & 0xFF); e.sta_scratch();
        e.txa(); e.and_imm((mask16 >> 8) & 0xFF); e.pha(); // hi on stack

        // Shift scratch(lo) and stack-top(hi) left by bitOff
        for (uint32_t i = 0; i < bitOff; i++) {
            e.emitInstruction("asl", AddressingMode::BASE_PAGE, e.scratchZP(), true);
            // Rotate carry into hi: pull, rol, push
            e.pla(); e.rol_a(); e.pha();
        }

        // Mask to exact field position
        e.lda_scratch(); e.and_imm(shiftedMask16 & 0xFF); e.sta_scratch();
        e.pla(); e.and_imm((shiftedMask16 >> 8) & 0xFF); e.pha(); // hi still on stack

        // RMW lo byte
        if (mode == 1) {
            e.lda_stack(addr + 1); // +1 because we pushed 1 byte
            e.and_imm(~shiftedMask16 & 0xFF); e.ora_zp(e.scratchZP()); e.sta_stack(addr + 1);
        } else if (mode == 2) {
            e.ldy_imm(0);
            e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
            e.and_imm(~shiftedMask16 & 0xFF); e.ora_zp(e.scratchZP());
            e.emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
        } else {
            if (addrIsZP) {
                e.lda_imm(shiftedMask16 & 0xFF);
                e.emitInstruction("trb", AddressingMode::BASE_PAGE, addr, true);
                e.lda_scratch();
                e.emitInstruction("tsb", AddressingMode::BASE_PAGE, addr, true);
            } else {
                e.lda_addr(addr); e.and_imm(~shiftedMask16 & 0xFF); e.ora_zp(e.scratchZP()); e.sta_addr(addr);
            }
        }

        // RMW hi byte: pull prepared hi from stack into scratch
        e.pla(); e.sta_scratch();
        if (mode == 1) {
            e.lda_stack(addr + 1); // stack restored now
            e.and_imm((~shiftedMask16 >> 8) & 0xFF); e.ora_zp(e.scratchZP()); e.sta_stack(addr + 1);
        } else if (mode == 2) {
            e.ldy_imm(1);
            e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
            e.and_imm((~shiftedMask16 >> 8) & 0xFF); e.ora_zp(e.scratchZP());
            e.emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
        } else {
            if (addrIsZP) {
                e.lda_imm((shiftedMask16 >> 8) & 0xFF);
                e.emitInstruction("trb", AddressingMode::BASE_PAGE, addr + 1, true);
                e.lda_scratch();
                e.emitInstruction("tsb", AddressingMode::BASE_PAGE, addr + 1, true);
            } else {
                e.lda_addr(addr + 1); e.and_imm((~shiftedMask16 >> 8) & 0xFF); e.ora_zp(e.scratchZP()); e.sta_addr(addr + 1);
            }
        }
        return;
    }

    // 8-bit storage unit. A contains new value (low byte only).
    // Check for constant-foldable optimizations via SMB/RMB
    // The new value to insert is in A. We check if it was loaded from a known constant
    // by examining the preceding instruction context — but since we can't peek backwards,
    // we always attempt the general path and let the assembler itself check.
    //
    // However, for single-bit fields we can use SMB/RMB unconditionally based on the value:
    // The compiler will emit `lda #0` or `lda #1` before bfins for single-bit constant stores.
    // We check the token *before* the address to see if the value was immediate.
    //
    // Optimization strategy for ZP/absolute targets:
    // 1. Single bit, constant value: SMBn/RMBn (ZP only) — 2 bytes, 1 instruction
    // 2. Multi-bit, constant value: multiple SMBn/RMBn (ZP only, ≤ bitWidth instructions)
    // 3. Multi-bit, abs/ZP: TRB + TSB — atomic RMW
    // 4. Stack-relative/indirect: general shift/mask/ORA

    if (mode == 0 && addrIsZP) {
        // ZP target: can potentially use SMBn/RMBn
        // A has new value. Save it, then do the insert.
        // For general case with TRB/TSB on ZP:
        e.and_imm(mask8);            // mask new value to width
        for (uint32_t i = 0; i < bitOff; i++) e.asl_a(); // shift into position
        e.sta_scratch();             // save prepared value

        // Clear field bits with TRB
        e.lda_imm(shiftedMask8);
        e.emitInstruction("trb", AddressingMode::BASE_PAGE, addr, true);

        // Set new bits with TSB
        e.lda_scratch();
        e.emitInstruction("tsb", AddressingMode::BASE_PAGE, addr, true);
    } else if (mode == 0) {
        // Absolute (non-ZP) target: use TRB/TSB with absolute mode
        e.and_imm(mask8);
        for (uint32_t i = 0; i < bitOff; i++) e.asl_a();
        e.sta_scratch();

        e.lda_imm(shiftedMask8);
        e.emitInstruction("trb", AddressingMode::ABSOLUTE, addr, true);

        e.lda_scratch();
        e.emitInstruction("tsb", AddressingMode::ABSOLUTE, addr, true);
    } else if (mode == 1) {
        // Stack-relative: general shift/mask/ORA
        e.and_imm(mask8);
        for (uint32_t i = 0; i < bitOff; i++) e.asl_a();
        e.sta_scratch();             // prepared new value

        e.lda_stack(addr);           // load old storage byte
        e.and_imm(~shiftedMask8 & 0xFF); // clear field bits
        e.ora_zp(e.scratchZP());             // OR in new bits
        e.sta_stack(addr);           // store back
    } else {
        // Indirect via ZP pointer
        e.and_imm(mask8);
        for (uint32_t i = 0; i < bitOff; i++) e.asl_a();
        e.sta_scratch();

        e.ldy_imm(0);
        e.emitInstruction("lda", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
        e.and_imm(~shiftedMask8 & 0xFF);
        e.ora_zp(e.scratchZP());
        e.emitInstruction("sta", AddressingMode::BASE_PAGE_INDIRECT_Y, addr, true);
    }
}

// ========================================================================
// 32-bit simulated operations
// Convention: .AXYZ register quad — A=byte0(lo), X=byte1, Y=byte2, Z=byte3(hi)
// Memory: addr+0..addr+3 (little-endian)
// ========================================================================

// SXT.16 — Sign-extend 16-bit AX to 32-bit AXYZ
void AssemblerSimulatedOps::emitSXT16Code(AssemblerParser*, M65Emitter& e, int, const std::string&) {
    // A=lo, X=hi already set. Sign-extend X into Y,Z.
    e.pha();             // save A
    e.txa();             // A = high byte of 16-bit value
    e.cmp_imm(0x80);     // test sign bit → carry set if negative
    e.lda_imm(0);
    auto br2231 = e.emitBranchPlaceholder(0x90); // BCC: skip sign fill
    e.lda_imm(0xFF);     // A = sign extension byte
    e.patchBranchTarget(br2231);
    e.tay();             // Y = sign
    e.taz();             // Z = sign
    e.pla();             // restore A (lo byte)
}

// ADD.32 / SUB.32 — 32-bit addition/subtraction
void AssemblerSimulatedOps::emitAddSub32Code(AssemblerParser* parser, M65Emitter& e, bool isAdd,
    const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;
    std::string DEST = dest;
    if (!DEST.empty() && DEST[0] != '.' && DEST[0] != '$' && !isdigit(DEST[0]) && DEST[0] != '#') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);

    if (DEST == ".AXYZ" || DEST == ".Q" || DEST == "") {
        if (isAdd) e.clc(); else e.sec();
        if (isImmediate && srcAst->isConstant(parser)) {
            uint32_t val = srcAst->getValue(parser);
            // byte 0 (A)
            if (isAdd) e.adc_imm(val & 0xFF); else e.sbc_imm(val & 0xFF);
            e.pha();
            // byte 1 (X)
            e.txa();
            if (isAdd) e.adc_imm((val >> 8) & 0xFF); else e.sbc_imm((val >> 8) & 0xFF);
            e.tax();
            // byte 2 (Y)
            e.tya();
            if (isAdd) e.adc_imm((val >> 16) & 0xFF); else e.sbc_imm((val >> 16) & 0xFF);
            e.tay();
            // byte 3 (Z)
            e.tza();
            if (isAdd) e.adc_imm((val >> 24) & 0xFF); else e.sbc_imm((val >> 24) & 0xFF);
            e.taz();
            e.pla();
        } else {
            uint32_t addr = 0;
            try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
            catch(...) {
                std::string src = parser->tokens[tokenIndex].value;
                if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
                Symbol* sym = parser->resolveSymbol(src, scopePrefix);
                if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src); } catch(...) { addr = 0; } }
            }
            if (isAdd) e.adc_addr(addr); else e.sbc_addr(addr);
            e.pha(); e.txa();
            if (isAdd) e.adc_addr(addr + 1); else e.sbc_addr(addr + 1);
            e.tax(); e.tya();
            if (isAdd) e.adc_addr(addr + 2); else e.sbc_addr(addr + 2);
            e.tay(); e.tza();
            if (isAdd) e.adc_addr(addr + 3); else e.sbc_addr(addr + 3);
            e.taz(); e.pla();
        }
    } else {
        // Memory destination
        Symbol* sym = parser->resolveSymbol(dest, scopePrefix);
        uint32_t dAddr = 0; if (sym) dAddr = sym->value; else { try { dAddr = parseNumericLiteral(dest); } catch(...) { dAddr = 0; } }
        if (isAdd) e.clc(); else e.sec();
        if (isImmediate && srcAst->isConstant(parser)) {
            uint32_t val = srcAst->getValue(parser);
            e.lda_addr(dAddr);   if (isAdd) e.adc_imm(val & 0xFF);         else e.sbc_imm(val & 0xFF);         e.sta_addr(dAddr);
            e.lda_addr(dAddr+1); if (isAdd) e.adc_imm((val >> 8) & 0xFF);  else e.sbc_imm((val >> 8) & 0xFF);  e.sta_addr(dAddr+1);
            e.lda_addr(dAddr+2); if (isAdd) e.adc_imm((val >> 16) & 0xFF); else e.sbc_imm((val >> 16) & 0xFF); e.sta_addr(dAddr+2);
            e.lda_addr(dAddr+3); if (isAdd) e.adc_imm((val >> 24) & 0xFF); else e.sbc_imm((val >> 24) & 0xFF); e.sta_addr(dAddr+3);
        } else {
            uint32_t sAddr = 0;
            try { sAddr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
            catch(...) {
                std::string src = parser->tokens[tokenIndex].value;
                if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
                Symbol* symS = parser->resolveSymbol(src, scopePrefix);
                if (symS) sAddr = symS->value; else { try { sAddr = parseNumericLiteral(src); } catch(...) { sAddr = 0; } }
            }
            e.lda_addr(dAddr);   if (isAdd) e.adc_addr(sAddr);   else e.sbc_addr(sAddr);   e.sta_addr(dAddr);
            e.lda_addr(dAddr+1); if (isAdd) e.adc_addr(sAddr+1); else e.sbc_addr(sAddr+1); e.sta_addr(dAddr+1);
            e.lda_addr(dAddr+2); if (isAdd) e.adc_addr(sAddr+2); else e.sbc_addr(sAddr+2); e.sta_addr(dAddr+2);
            e.lda_addr(dAddr+3); if (isAdd) e.adc_addr(sAddr+3); else e.sbc_addr(sAddr+3); e.sta_addr(dAddr+3);
        }
    }
}

// AND.32 / ORA.32 / EOR.32 — 32-bit bitwise operations (AXYZ only)
void AssemblerSimulatedOps::emitBitwise32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto srcAst = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!srcAst) return;
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    std::string M = mnemonic; std::transform(M.begin(), M.end(), M.begin(), ::toupper);

    auto doOp = [&](auto immFn, auto absFn) {
        if (DEST == ".AXYZ" || DEST == ".Q") {
            if (isImmediate && srcAst->isConstant(parser)) {
                uint32_t val = srcAst->getValue(parser);
                immFn(val & 0xFF);
                e.pha(); e.txa(); immFn((val >> 8) & 0xFF); e.tax();
                e.tya(); immFn((val >> 16) & 0xFF); e.tay();
                e.tza(); immFn((val >> 24) & 0xFF); e.taz(); e.pla();
            } else {
                std::string src = parser->tokens[tokenIndex].value;
                if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
                Symbol* sym = parser->resolveSymbol(src, scopePrefix);
                uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src); } catch(...) { addr = 0; } }
                absFn(addr);
                e.pha(); e.txa(); absFn(addr + 1); e.tax();
                e.tya(); absFn(addr + 2); e.tay();
                e.tza(); absFn(addr + 3); e.taz(); e.pla();
            }
        } else throw std::runtime_error("Simulated bitwise 32-bit only supports .AXYZ destination");
    };

    if (M == "AND" || M == "AND.32") doOp([&](uint8_t v){ e.and_imm(v); }, [&](uint16_t a){ e.and_addr(a); });
    else if (M == "ORA" || M == "ORA.32") doOp([&](uint8_t v){ e.ora_imm(v); }, [&](uint16_t a){ e.ora_addr(a); });
    else if (M == "EOR" || M == "EOR.32") doOp([&](uint8_t v){ e.eor_imm(v); }, [&](uint16_t a){ e.eor_addr(a); });
}

// CMP.32 — 32-bit unsigned comparison (.AXYZ vs immediate or memory)
// Compare byte0(A), if NE→done; else byte1(X), ...; else byte3(Z).
// Uses deferred branch patching via emitBranchPlaceholder/patchBranchTarget.
void AssemblerSimulatedOps::emitCMP32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto src2Ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!src2Ast) return;
    std::string SRC1 = src1; if (!SRC1.empty() && SRC1[0] != '.') SRC1 = "." + SRC1;
    std::transform(SRC1.begin(), SRC1.end(), SRC1.begin(), ::toupper);
    if (SRC1 == ".AXYZ" || SRC1 == ".Q") {
        // Compare high-to-low: Z (byte 3), Y (byte 2), X (byte 1), A (byte 0)
        // Early exit on inequality — the flags from the first unequal byte determine the result
        // Save A (byte 0) to scratch since TZA/TYA/TXA will clobber it
        size_t br0, br1, br2;
        uint8_t zpScratch = e.getZP(0); // __zp_scratch ($08)
        e.sta_zp(zpScratch);
        if (isImmediate && src2Ast->isConstant(parser)) {
            uint32_t val = src2Ast->getValue(parser);
            e.tza(); e.cmp_imm((val >> 24) & 0xFF);
            br0 = e.emitBranchPlaceholder(0xD0); // BNE
            e.tya(); e.cmp_imm((val >> 16) & 0xFF);
            br1 = e.emitBranchPlaceholder(0xD0); // BNE
            e.txa(); e.cmp_imm((val >> 8) & 0xFF);
            br2 = e.emitBranchPlaceholder(0xD0); // BNE
            e.lda_zp(zpScratch); e.cmp_imm(val & 0xFF);
        } else {
            uint32_t addr = 0;
            try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
            catch(...) {
                std::string src = parser->tokens[tokenIndex].value;
                if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
                Symbol* sym = parser->resolveSymbol(src, scopePrefix);
                if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src); } catch(...) { addr = 0; } }
            }
            e.tza(); e.cmp_addr(addr + 3);
            br0 = e.emitBranchPlaceholder(0xD0); // BNE
            e.tya(); e.cmp_addr(addr + 2);
            br1 = e.emitBranchPlaceholder(0xD0); // BNE
            e.txa(); e.cmp_addr(addr + 1);
            br2 = e.emitBranchPlaceholder(0xD0); // BNE
            e.lda_zp(zpScratch); e.cmp_addr(addr);
        }
        e.patchBranchTarget(br0);
        e.patchBranchTarget(br1);
        e.patchBranchTarget(br2);
    } else throw std::runtime_error("Simulated CMP.32 only supports .AXYZ as first operand");
}

// CMP.S32 — 32-bit signed comparison
// Compare bytes 0-2 unsigned, then byte 3 with sign-flip (EOR #$80).
void AssemblerSimulatedOps::emitCMP_S32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    int idx = tokenIndex;
    bool isImmediate = (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) ||
                       (idx > 0 && parser->tokens[idx - 1].type == AssemblerTokenType::HASH);
    if (idx < (int)parser->tokens.size() && parser->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    auto src2Ast = parseExprAST(parser->tokens, idx, parser->symbolTable, scopePrefix);
    if (!src2Ast) return;
    std::string SRC1 = src1; if (!SRC1.empty() && SRC1[0] != '.') SRC1 = "." + SRC1;
    std::transform(SRC1.begin(), SRC1.end(), SRC1.begin(), ::toupper);
    if (SRC1 == ".AXYZ" || SRC1 == ".Q") {
        // Signed compare high-to-low: Z^$80 (byte 3, sign-flipped), Y (byte 2), X (byte 1), A (byte 0)
        // Save A (byte 0) to scratch since TZA/TYA/TXA will clobber it
        size_t br0, br1, br2;
        uint8_t zpScratch = e.getZP(0); // __zp_scratch ($08)
        e.sta_zp(zpScratch);
        if (isImmediate && src2Ast->isConstant(parser)) {
            uint32_t val = src2Ast->getValue(parser);
            e.tza(); e.eor_imm(0x80); e.cmp_imm(((val >> 24) & 0xFF) ^ 0x80);
            br0 = e.emitBranchPlaceholder(0xD0); // BNE
            e.tya(); e.cmp_imm((val >> 16) & 0xFF);
            br1 = e.emitBranchPlaceholder(0xD0); // BNE
            e.txa(); e.cmp_imm((val >> 8) & 0xFF);
            br2 = e.emitBranchPlaceholder(0xD0); // BNE
            e.lda_zp(zpScratch); e.cmp_imm(val & 0xFF);
        } else {
            uint32_t addr = 0;
            try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
            catch(...) {
                std::string src = parser->tokens[tokenIndex].value;
                if (parser->tokens[tokenIndex].type == AssemblerTokenType::REGISTER) src = "." + src;
                Symbol* sym = parser->resolveSymbol(src, scopePrefix);
                if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(src); } catch(...) { addr = 0; } }
            }
            e.sta_zp(zpScratch + 1); // save A to scratch+1 (scratch used by sign-flip below)
            e.tza(); e.eor_imm(0x80); e.sta_scratch();
            e.lda_addr(addr + 3); e.eor_imm(0x80); e.cmp_scratch();
            br0 = e.emitBranchPlaceholder(0xD0); // BNE
            e.tya(); e.cmp_addr(addr + 2);
            br1 = e.emitBranchPlaceholder(0xD0); // BNE
            e.txa(); e.cmp_addr(addr + 1);
            br2 = e.emitBranchPlaceholder(0xD0); // BNE
            e.lda_zp(zpScratch + 1); e.cmp_addr(addr);
        }
        e.patchBranchTarget(br0);
        e.patchBranchTarget(br1);
        e.patchBranchTarget(br2);
    } else throw std::runtime_error("Simulated CMP.S32 only supports .AXYZ as first operand");
}

// NEG.32 / NOT.32 — 32-bit negation/complement
void AssemblerSimulatedOps::emitNegNot32Code(AssemblerParser* parser, M65Emitter& e, bool isNeg,
    const std::string& operand, int tokenIndex, const std::string& scopePrefix) {
    std::string OP = operand; if (!OP.empty() && OP[0] != '.') OP = "." + OP;
    std::transform(OP.begin(), OP.end(), OP.begin(), ::toupper);
    if (OP == ".AXYZ" || OP == ".Q" || OP == "") {
        // NOT: invert all 4 bytes
        e.eor_imm(0xFF);
        e.pha(); e.txa(); e.eor_imm(0xFF); e.tax();
        e.tya(); e.eor_imm(0xFF); e.tay();
        e.tza(); e.eor_imm(0xFF); e.taz(); e.pla();
        if (isNeg) {
            // NEG = NOT + 1: add 1 to the result
            e.clc(); e.adc_imm(1);
            e.pha(); e.txa(); e.adc_imm(0); e.tax();
            e.tya(); e.adc_imm(0); e.tay();
            e.tza(); e.adc_imm(0); e.taz(); e.pla();
        }
    } else {
        // Memory operand
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(operand, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(operand); } catch(...) { addr = 0; } } }
        if (isNeg) {
            e.lda_addr(addr);   e.eor_imm(0xFF); e.clc(); e.adc_imm(1); e.sta_addr(addr);
            e.lda_addr(addr+1); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr+1);
            e.lda_addr(addr+2); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr+2);
            e.lda_addr(addr+3); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr+3);
        } else {
            e.lda_addr(addr);   e.eor_imm(0xFF); e.sta_addr(addr);
            e.lda_addr(addr+1); e.eor_imm(0xFF); e.sta_addr(addr+1);
            e.lda_addr(addr+2); e.eor_imm(0xFF); e.sta_addr(addr+2);
            e.lda_addr(addr+3); e.eor_imm(0xFF); e.sta_addr(addr+3);
        }
    }
}

// ABS.32 — 32-bit absolute value
// Uses deferred branch patching for the BPL skip.
void AssemblerSimulatedOps::emitABS32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AXYZ" || DEST == ".Q" || DEST == "") {
        // Test sign bit of Z (byte 3)
        e.pha(); e.tza();
        size_t brSkip = e.emitBranchPlaceholder(0x10); // BPL → skip negation
        e.pla();
        // Negate AXYZ: NOT all + add 1
        e.eor_imm(0xFF); e.clc(); e.adc_imm(1);
        e.pha(); e.txa(); e.eor_imm(0xFF); e.adc_imm(0); e.tax();
        e.tya(); e.eor_imm(0xFF); e.adc_imm(0); e.tay();
        e.tza(); e.eor_imm(0xFF); e.adc_imm(0); e.taz(); e.pla();
        size_t brDone = e.emitBranchPlaceholder(0x80); // BRA → skip the positive-path PLA
        e.patchBranchTarget(brSkip);
        e.pla(); // positive path: restore A unchanged
        e.patchBranchTarget(brDone);
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.lda_addr(addr + 3);
        size_t brSkip = e.emitBranchPlaceholder(0x10); // BPL → skip
        e.lda_addr(addr);   e.eor_imm(0xFF); e.clc(); e.adc_imm(1); e.sta_addr(addr);
        e.lda_addr(addr+1); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr+1);
        e.lda_addr(addr+2); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr+2);
        e.lda_addr(addr+3); e.eor_imm(0xFF); e.adc_imm(0); e.sta_addr(addr+3);
        e.patchBranchTarget(brSkip);
    }
}

// LSL.32 — 32-bit logical shift left (one bit)
void AssemblerSimulatedOps::emitLSL32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AXYZ" || DEST == ".Q") {
        // ASL A, then ROL through X, Y, Z
        e.asl_a();
        e.pha(); e.txa(); e.rol_a(); e.tax();
        e.tya(); e.rol_a(); e.tay();
        e.tza(); e.rol_a(); e.taz(); e.pla();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.asl_addr(addr); e.rol_addr(addr + 1); e.rol_addr(addr + 2); e.rol_addr(addr + 3);
    }
}

// LSR.32 — 32-bit logical shift right (one bit)
void AssemblerSimulatedOps::emitLSR32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AXYZ" || DEST == ".Q") {
        // LSR Z, then ROR through Y, X, A
        e.pha(); e.tza(); e.lsr_a(); e.taz();
        e.tya(); e.ror_a(); e.tay();
        e.txa(); e.ror_a(); e.tax(); e.pla(); e.ror_a();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.lsr_addr(addr + 3); e.ror_addr(addr + 2); e.ror_addr(addr + 1); e.ror_addr(addr);
    }
}

// ROL.32 — 32-bit rotate left (through carry)
void AssemblerSimulatedOps::emitROL32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AXYZ" || DEST == ".Q") {
        e.rol_a();
        e.pha(); e.txa(); e.rol_a(); e.tax();
        e.tya(); e.rol_a(); e.tay();
        e.tza(); e.rol_a(); e.taz(); e.pla();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.rol_addr(addr); e.rol_addr(addr + 1); e.rol_addr(addr + 2); e.rol_addr(addr + 3);
    }
}

// ROR.32 — 32-bit rotate right (through carry)
void AssemblerSimulatedOps::emitROR32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AXYZ" || DEST == ".Q") {
        e.pha(); e.tza(); e.ror_a(); e.taz();
        e.tya(); e.ror_a(); e.tay();
        e.txa(); e.ror_a(); e.tax(); e.pla(); e.ror_a();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.ror_addr(addr + 3); e.ror_addr(addr + 2); e.ror_addr(addr + 1); e.ror_addr(addr);
    }
}

// ASR.32 — 32-bit arithmetic shift right (preserves sign)
void AssemblerSimulatedOps::emitASR32Code(AssemblerParser* parser, M65Emitter& e,
    const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST == ".AXYZ" || DEST == ".Q") {
        // CMP #$80 on high byte sets carry = sign bit, then ROR chain
        e.pha(); e.tza(); e.cmp_imm(0x80); e.ror_a(); e.taz();
        e.tya(); e.ror_a(); e.tay();
        e.txa(); e.ror_a(); e.tax(); e.pla(); e.ror_a();
    } else {
        uint32_t addr = 0;
        try { addr = parser->evaluateExpressionAt(tokenIndex, scopePrefix); }
        catch(...) { Symbol* sym = parser->resolveSymbol(dest, scopePrefix); if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } } }
        e.lda_addr(addr + 3); e.cmp_imm(0x80);
        e.ror_addr(addr + 3); e.ror_addr(addr + 2); e.ror_addr(addr + 1); e.ror_addr(addr);
    }
}

// ============================================================================
// Unified dispatch wrappers
// Each adapts the (parser, emitter, stmt) signature to existing emitXxxCode.
// ============================================================================

using Stmt = AssemblerParser::Statement;

void AssemblerSimulatedOps::dispatch_Expr(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitExpressionCode(p, e, s->exprTarget, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Mul(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitMulCode(p, e, s->mulWidth, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Div(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitDivCode(p, e, s->mulWidth, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_StackIncDec(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitStackIncDecCode(p, e, s->type == Stmt::STACK_INC, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_StackIncDec8(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitStackIncDec8Code(p, e, s->type == Stmt::STACK_INC8, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_AddSub16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    bool isAdd = (s->type == Stmt::ADD16 || s->type == Stmt::ADDS16);
    emitAddSub16Code(p, e, isAdd, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Bitwise16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitBitwise16Code(p, e, s->instr.mnemonic, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_CMP16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitCMP16Code(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_CMP_S16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitCMP_S16Code(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDW(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDWCode(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix,
                s->instr.mnemonic == "ldw.sp");
}
void AssemblerSimulatedOps::dispatch_STW(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTWCode(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix,
                s->instr.mnemonic == "stw.sp");
}
void AssemblerSimulatedOps::dispatch_Fill(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitFillCode(p, e, s->instr.operandTokenIndex, s->scopePrefix,
                 s->instr.mnemonic == "fill.sp");
}
void AssemblerSimulatedOps::dispatch_Copy(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitMoveCode(p, e, s->instr.operandTokenIndex, s->scopePrefix,
                 s->instr.mnemonic == "move.sp");
}
void AssemblerSimulatedOps::dispatch_Swap(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSwapCode(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_NegNot16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    bool isNeg = (s->type == Stmt::NEG16 || s->type == Stmt::NEG_S16);
    emitNegNot16Code(p, e, isNeg, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_ABS16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitABS16Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_ChkZero(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    bool is16 = (s->type == Stmt::CHKZERO16 || s->type == Stmt::CHKNONZERO16);
    bool isInverse = (s->type == Stmt::CHKNONZERO8 || s->type == Stmt::CHKNONZERO16);
    emitChkZeroCode(p, e, is16, isInverse, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Branch16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitBranch16Code(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Select(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSelectCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_PtrStack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitPtrStackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_PtrDeref(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitPtrDerefCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_FlatMemory(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitFlatMemoryCode(p, e, s->instr.mnemonic, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_PHWStack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitPHWStackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_ASW(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitASWCode(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_ROW(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitROWCode(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Shift16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    switch (s->type) {
        case Stmt::LSL16: case Stmt::LSL_S16: emitLSL16Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::LSR16: case Stmt::LSR_S16: emitLSR16Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::ROL16: case Stmt::ROL_S16: emitROL16Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::ROR16: case Stmt::ROR_S16: emitROR16Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::ASR16: case Stmt::ASR_S16: emitASR16Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        default: break;
    }
}
void AssemblerSimulatedOps::dispatch_SXT8(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSXT8Code(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_SXT16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSXT16Code(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_AddSub32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    bool isAdd = (s->type == Stmt::ADD32 || s->type == Stmt::ADDS32);
    emitAddSub32Code(p, e, isAdd, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Bitwise32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitBitwise32Code(p, e, s->instr.mnemonic, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_CMP32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitCMP32Code(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_CMP_S32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitCMP_S32Code(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_NegNot32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    bool isNeg = (s->type == Stmt::NEG32 || s->type == Stmt::NEG_S32);
    emitNegNot32Code(p, e, isNeg, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_ABS32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitABS32Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Shift32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    switch (s->type) {
        case Stmt::LSL32: case Stmt::LSL_S32: emitLSL32Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::LSR32: case Stmt::LSR_S32: emitLSR32Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::ROL32: case Stmt::ROL_S32: emitROL32Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::ROR32: case Stmt::ROR_S32: emitROR32Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        case Stmt::ASR32: case Stmt::ASR_S32: emitASR32Code(p, e, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix); break;
        default: break;
    }
}
void AssemblerSimulatedOps::dispatch_PushPop(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitPushPopCode(p, e, s->type == Stmt::PUSH, s->instr.operand, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDA_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDA_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STA_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTA_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDX_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDX_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDY_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDY_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDZ_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDZ_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STX_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTX_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STY_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTY_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STZ_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTZ_StackCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Zero(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitZeroCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDA_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDA_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STA_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTA_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDAX_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDAX_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STAX_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTAX_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDAY_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDAY_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STAY_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTAY_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDAZ_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDAZ_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STAZ_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTAZ_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LDAXYZ_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLDAXYZ_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_STAXYZ_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSTAXYZ_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_LEAX_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitLEAX_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_MOVE_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitMOVE_FPCode(p, e, s->instr.operandTokenIndex, s->scopePrefix);
}
// inc.fp / dec.fp — 8-bit increment/decrement of frame-relative variable
void AssemblerSimulatedOps::dispatch_INC_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    Symbol* fpSym = p->resolveSymbol("_fp", s->scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)p->evaluateExpressionAt(s->instr.operandTokenIndex, s->scopePrefix);
    e.inc_stack(fpOff + yOff);
}
void AssemblerSimulatedOps::dispatch_DEC_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    Symbol* fpSym = p->resolveSymbol("_fp", s->scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)p->evaluateExpressionAt(s->instr.operandTokenIndex, s->scopePrefix);
    e.dec_stack(fpOff + yOff);
}
// inc.16fp / dec.16fp — 16-bit increment/decrement of frame-relative variable
void AssemblerSimulatedOps::dispatch_INC16_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    Symbol* fpSym = p->resolveSymbol("_fp", s->scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)p->evaluateExpressionAt(s->instr.operandTokenIndex, s->scopePrefix);
    uint8_t off = fpOff + yOff;
    e.inc_stack(off);
    auto br = e.emitBranchPlaceholder(0xD0); // bne skip
    e.inc_stack(off + 1);
    e.patchBranchTarget(br);
}
void AssemblerSimulatedOps::dispatch_DEC16_FP(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    Symbol* fpSym = p->resolveSymbol("_fp", s->scopePrefix);
    uint8_t fpOff = fpSym ? (uint8_t)fpSym->value : 0;
    uint8_t yOff = (uint8_t)p->evaluateExpressionAt(s->instr.operandTokenIndex, s->scopePrefix);
    uint8_t off = fpOff + yOff;
    e.lda_stack(off);                       // check lo byte for borrow
    auto br = e.emitBranchPlaceholder(0xD0); // bne skip
    e.dec_stack(off + 1);                   // borrow: dec hi
    e.patchBranchTarget(br);
    e.dec_stack(off);                       // dec lo
}
void AssemblerSimulatedOps::dispatch_BFExt(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    int unitWidth = (s->type == Stmt::BFEXT32) ? 32 : (s->type == Stmt::BFEXT16) ? 16 : 8;
    emitBFExtCode(p, e, unitWidth, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_BFIns(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    bool is16 = (s->type == Stmt::BFINS16 || s->type == Stmt::BFINS16_SP || s->type == Stmt::BFINS16_IND);
    bool is32 = (s->type == Stmt::BFINS32);
    int mode = 0;
    if (s->type == Stmt::BFINS_SP || s->type == Stmt::BFINS16_SP) mode = 1;
    else if (s->type == Stmt::BFINS_IND || s->type == Stmt::BFINS16_IND) mode = 2;
    emitBFInsCode(p, e, is32 ? 32 : is16 ? 16 : 8, mode, s->instr.operandTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_MulS16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitMulS16Code(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_DivS16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitDivS16Code(p, e, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Mod16(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitMod16Code(p, e, s->type == Stmt::MOD_S16, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}

void AssemblerSimulatedOps::dispatch_MulS32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSignedMathOp32(p, e, 0, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_DivS32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitSignedMathOp32(p, e, 1, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}
void AssemblerSimulatedOps::dispatch_Mod32(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    emitMod32Code(p, e, s->type == Stmt::MOD_S32, s->instr.operand, s->exprTokenIndex, s->scopePrefix);
}

void AssemblerSimulatedOps::emitMod32Code(AssemblerParser* parser, M65Emitter& e, bool isSigned, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    if (isSigned) {
        // Signed 32-bit modulo: abs→unsigned div→read remainder→sign fixup
        emitSignedMathOp32(parser, e, 2, dest, tokenIndex, scopePrefix);
        return;
    } else {
        // Unsigned mod: perform div.32, then read remainder from $D76C-$D76F
        emitDivCode(parser, e, 32, dest, tokenIndex, scopePrefix);
        // divCode loaded quotient into .AXYZ from $D768. Override with remainder:
        std::string DEST = dest; if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
        std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
        if (DEST == ".Q" || DEST == ".AXYZ") {
            e.lda_addr(m65::DIV_REM + 3); e.taz();
            e.lda_addr(m65::DIV_REM + 2); e.tay();
            e.lda_addr(m65::DIV_REM + 1); e.tax();
            e.lda_addr(m65::DIV_REM);
        } else {
            for (int i = 0; i < 4; ++i) {
                e.lda_addr(m65::DIV_REM + i);
                Symbol* sym = parser->resolveSymbol(dest, scopePrefix);
                uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parseNumericLiteral(dest); } catch(...) { addr = 0; } }
                e.sta_addr(addr + i);
            }
        }
    }
}

void AssemblerSimulatedOps::emitSpBaseAddrCalc16(AssemblerParser* parser, M65Emitter& e, uint16_t addend) {
    if (e.hasFramePointer()) {
        uint8_t fp = e.framePointerZP();
        e.clc();
        e.lda_zp(fp);
        e.adc_imm(addend & 0xFF);
        e.pha();
        e.lda_zp(fp + 1);
        e.adc_imm((addend >> 8) & 0xFF);
        e.tax();
        e.pla();
        return;
    }
    bool spExtern = parser->isExternSymbol("__sp_base");
    e.tsx(); e.txa(); e.clc();
    if (spExtern) {
        e.recordSymbolRelocLo("__sp_base");
        e.adc_imm(addend & 0xFF);
    } else {
        uint16_t total = e.spBase() + addend;
        e.adc_imm(total & 0xFF);
    }
    e.pha();
    if (spExtern) {
        e.recordSymbolRelocHi("__sp_base", addend & 0xFF);
        e.lda_imm((addend >> 8) & 0xFF);
    } else {
        uint16_t total = e.spBase() + addend;
        e.lda_imm((total >> 8) & 0xFF);
    }
    e.adc_imm(0); e.tax(); e.pla();
}

void AssemblerSimulatedOps::emitLoadIndexToAX(AssemblerParser* parser, M65Emitter& e, bool indexIsReg, const std::string& indexRegName, int idxIndexStart, const std::string& scopePrefix) {
    if (indexIsReg) {
        if (indexRegName == ".ax" || indexRegName == "ax") {
            // Already in AX
        } else if (indexRegName == ".a" || indexRegName == "a") {
            e.ldx_imm(0);
        } else if (indexRegName == ".x" || indexRegName == "x") {
            e.txa();
            e.ldx_imm(0);
        } else if (indexRegName == ".y" || indexRegName == "y") {
            e.tya();
            e.ldx_imm(0);
        } else if (indexRegName == ".z" || indexRegName == "z") {
            e.tza();
            e.ldx_imm(0);
        }
    } else {
        uint32_t offset = 0;
        bool isStack = parser->isStackRelativeOperand(idxIndexStart, offset, scopePrefix);
        if (isStack) {
            auto fa = parser->resolveFrameAccess(idxIndexStart, scopePrefix);
            if (fa.isFrame) {
                uint8_t totalOff = fa.fpOff + fa.yOff;
                e.lda_stack(totalOff + 1); e.sta_scratch();
                e.lda_stack(totalOff); e.ldx_scratch();
            } else {
                e.lda_stack(offset + 1); e.sta_scratch();
                e.lda_stack(offset); e.ldx_scratch();
            }
        } else {
            std::string symName = parser->tokens[idxIndexStart].value;
            bool isZP = (!symName.empty() && symName[0] == '$');
            Symbol* sym = parser->resolveSymbol(symName, scopePrefix);
            uint32_t addr = 0;
            if (sym) addr = sym->value;
            else {
                try { addr = parser->evaluateExpressionAt(idxIndexStart, scopePrefix); }
                catch (...) { addr = 0; }
            }
            if (isZP) {
                e.lda_zp(addr & 0xFF);
                e.ldx_zp((addr + 1) & 0xFF);
            } else {
                if (sym && sym->isAddress) { e.recordSymbolReloc(symName); }
                e.lda_abs(addr);
                if (sym && sym->isAddress) { e.recordSymbolReloc(symName); }
                e.ldx_abs(addr + 1);
            }
        }
    }
}

void AssemblerSimulatedOps::emitWriteOperand16(AssemblerParser* parser, M65Emitter& e, bool isReg, const std::string& regName, int idxStart, uint16_t destAddr, const std::string& scopePrefix) {
    if (isReg) {
        if (regName == ".ax" || regName == "ax") {
            e.sta_addr(destAddr);
            e.txa(); e.sta_addr(destAddr + 1);
        } else if (regName == ".a" || regName == "a") {
            e.sta_addr(destAddr);
            e.stz_addr(destAddr + 1);
        } else if (regName == ".x" || regName == "x") {
            e.txa(); e.sta_addr(destAddr);
            e.stz_addr(destAddr + 1);
        } else if (regName == ".y" || regName == "y") {
            e.tya(); e.sta_addr(destAddr);
            e.stz_addr(destAddr + 1);
        } else if (regName == ".z" || regName == "z") {
            e.tza(); e.sta_addr(destAddr);
            e.stz_addr(destAddr + 1);
        }
    } else {
        uint32_t offset = 0;
        bool isStack = parser->isStackRelativeOperand(idxStart, offset, scopePrefix);
        if (isStack) {
            auto fa = parser->resolveFrameAccess(idxStart, scopePrefix);
            if (fa.isFrame) {
                uint8_t totalOff = fa.fpOff + fa.yOff;
                e.lda_stack(totalOff); e.sta_addr(destAddr);
                e.lda_stack(totalOff + 1); e.sta_addr(destAddr + 1);
            } else {
                e.lda_stack(offset); e.sta_addr(destAddr);
                e.lda_stack(offset + 1); e.sta_addr(destAddr + 1);
            }
        } else {
            std::string symName = parser->tokens[idxStart].value;
            bool isZP = (!symName.empty() && symName[0] == '$');
            Symbol* sym = parser->resolveSymbol(symName, scopePrefix);
            uint32_t addr = 0; if (sym) addr = sym->value; else { try { addr = parser->evaluateExpressionAt(idxStart, scopePrefix); } catch(...) { addr = 0; } }
            if (isZP) {
                e.lda_zp(addr & 0xFF); e.sta_addr(destAddr);
                e.ldx_zp((addr + 1) & 0xFF); e.stx_addr(destAddr + 1);
            } else {
                if (sym && sym->isAddress) { e.recordSymbolReloc(symName); }
                e.lda_abs(addr); e.sta_addr(destAddr);
                if (sym && sym->isAddress) { e.recordSymbolReloc(symName); }
                e.ldx_abs(addr + 1); e.stx_addr(destAddr + 1);
            }
        }
    }
}

void AssemblerSimulatedOps::emitLoadAddrConst(AssemblerParser* parser, M65Emitter& e, const std::string& name, uint16_t offset, char reg2) {
    Symbol* sym = parser->resolveSymbol(name, "");
    uint32_t baseVal = 0;
    bool isReloc = parser->isRelocatableSymbol(name);
    if (sym) {
        baseVal = sym->value;
    } else {
        try { baseVal = parseNumericLiteral(name); } catch (...) { baseVal = 0; }
    }
    uint32_t total = baseVal + offset;

    if (isReloc) {
        e.recordSymbolRelocLo(name);
    }
    e.lda_imm(total & 0xFF);

    if (isReloc) {
        e.recordSymbolRelocHi(name, total & 0xFF);
    }
    if (reg2 == 'X') e.ldx_imm((total >> 8) & 0xFF);
    else if (reg2 == 'Y') e.ldy_imm((total >> 8) & 0xFF);
    else if (reg2 == 'Z') e.ldz_imm((total >> 8) & 0xFF);
}

void AssemblerSimulatedOps::dispatch_StructElem(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    int idx = s->exprTokenIndex;
    if (idx < 0 || idx >= (int)p->tokens.size()) return;
    
    bool baseIsReg = false;
    std::string baseRegName;
    int idxBaseStart = idx;
    bool baseIsImmediate = false;
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::HASH) {
        baseIsImmediate = true;
        idx++;
    }
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::REGISTER) {
        baseIsReg = true;
        baseRegName = p->tokens[idx].value;
        idx++;
    } else {
        auto baseAst = parseExprAST(p->tokens, idx, p->symbolTable, s->scopePrefix);
    }
    
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    
    auto offsetAst = parseExprAST(p->tokens, idx, p->symbolTable, s->scopePrefix);
    uint16_t offsetVal = offsetAst ? offsetAst->getValue(p) : 0;
    
    auto baseFa = p->resolveFrameAccess(idxBaseStart, s->scopePrefix);
    if (baseFa.isFrame) {
        emitSpBaseAddrCalc16(p, e, baseFa.fpOff + baseFa.yOff + offsetVal);
    } else if (baseIsReg) {
        if (offsetVal > 0) {
            e.clc();
            e.adc_imm(offsetVal & 0xFF);
            e.pha(); e.txa(); e.adc_imm((offsetVal >> 8) & 0xFF); e.tax(); e.pla();
        }
    } else {
        std::string baseSymName = baseIsImmediate ? p->tokens[idxBaseStart + 1].value : p->tokens[idxBaseStart].value;
        Symbol* baseSym = p->resolveSymbol(baseSymName, s->scopePrefix);
        uint32_t baseAddr = 0;
        if (baseSym) baseAddr = baseSym->value;
        else {
            try { baseAddr = p->evaluateExpressionAt(idxBaseStart + (baseIsImmediate ? 1 : 0), s->scopePrefix); }
            catch (...) { baseAddr = 0; }
        }
        if (baseIsImmediate) {
            emitLoadAddrConst(p, e, baseSymName, offsetVal, 'X');
        } else {
            bool isZP = (!baseSymName.empty() && baseSymName[0] == '$');
            if (isZP) {
                e.lda_zp(baseAddr & 0xFF);
                e.ldx_zp((baseAddr + 1) & 0xFF);
            } else {
                Symbol* relSym = p->resolveSymbol(baseSymName, s->scopePrefix);
                if (relSym && relSym->isAddress) { e.recordSymbolReloc(baseSymName); }
                e.lda_abs(baseAddr);
                if (relSym && relSym->isAddress) { e.recordSymbolReloc(baseSymName); }
                e.ldx_abs(baseAddr + 1);
            }
            if (offsetVal > 0) {
                e.clc();
                e.adc_imm(offsetVal & 0xFF);
                e.pha(); e.txa(); e.adc_imm((offsetVal >> 8) & 0xFF); e.tax(); e.pla();
            }
        }
    }
    
    std::string DEST = s->instr.operand;
    if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST != ".AX") {
        uint32_t destOffset = 0;
        bool destIsStack = p->isStackRelativeOperand(s->instr.operandTokenIndex, destOffset, s->scopePrefix);
        if (destIsStack) {
            auto destFa = p->resolveFrameAccess(s->instr.operandTokenIndex, s->scopePrefix);
            if (destFa.isFrame) {
                uint8_t totalOff = destFa.fpOff + destFa.yOff;
                e.stx_scratch(); e.sta_stack(totalOff); e.lda_scratch(); e.sta_stack(totalOff + 1);
            } else {
                e.stx_scratch(); e.sta_stack(destOffset); e.lda_scratch(); e.sta_stack(destOffset + 1);
            }
        } else {
            std::string destSymName = s->instr.operand;
            bool isZP = (!destSymName.empty() && destSymName[0] == '$');
            Symbol* destSym = p->resolveSymbol(destSymName, s->scopePrefix);
            uint32_t destAddr = 0;
            if (destSym) destAddr = destSym->value;
            else {
                try { destAddr = p->evaluateExpressionAt(s->instr.operandTokenIndex, s->scopePrefix); }
                catch (...) { destAddr = 0; }
            }
            if (isZP) {
                e.sta_zp(destAddr & 0xFF);
                e.txa(); e.sta_zp((destAddr + 1) & 0xFF);
            } else {
                if (destSym && destSym->isAddress) { e.recordSymbolReloc(destSymName); }
                e.sta_abs(destAddr);
                if (destSym && destSym->isAddress) { e.recordSymbolReloc(destSymName); }
                e.txa(); e.sta_abs(destAddr + 1);
            }
        }
    }
}

void AssemblerSimulatedOps::dispatch_AddrElem(AssemblerParser* p, M65Emitter& e, Stmt* s) {
    int idx = s->exprTokenIndex;
    if (idx < 0 || idx >= (int)p->tokens.size()) return;

    bool baseIsReg = false;
    std::string baseRegName;
    int idxBaseStart = idx;
    bool baseIsImmediate = false;
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::HASH) {
        baseIsImmediate = true;
        idx++;
    }
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::REGISTER) {
        baseIsReg = true;
        baseRegName = p->tokens[idx].value;
        idx++;
    } else {
        auto baseAst = parseExprAST(p->tokens, idx, p->symbolTable, s->scopePrefix);
    }
    
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    
    bool indexIsReg = false;
    std::string indexRegName;
    int idxIndexStart = idx;
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::REGISTER) {
        indexIsReg = true;
        indexRegName = p->tokens[idx].value;
        idx++;
    } else {
        auto indexAst = parseExprAST(p->tokens, idx, p->symbolTable, s->scopePrefix);
    }
    
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::COMMA) idx++;
    if (idx < (int)p->tokens.size() && p->tokens[idx].type == AssemblerTokenType::HASH) idx++;
    
    auto strideAst = parseExprAST(p->tokens, idx, p->symbolTable, s->scopePrefix);
    uint16_t strideVal = strideAst ? strideAst->getValue(p) : 1;
    if (strideVal > 1) {
        emitWriteOperand16(p, e, indexIsReg, indexRegName, idxIndexStart, m65::MULT_ARG1, s->scopePrefix);
        
        e.lda_imm(strideVal & 0xFF);
        e.sta_addr(m65::MULT_ARG2);
        e.lda_imm((strideVal >> 8) & 0xFF);
        e.sta_addr(m65::MULT_ARG2 + 1);
        
        e.stz_addr(m65::MULT_ARG1 + 2);
        e.stz_addr(m65::MULT_ARG1 + 3);
        e.stz_addr(m65::MULT_ARG2 + 2);
        e.stz_addr(m65::MULT_ARG2 + 3);
        
        auto baseFa = p->resolveFrameAccess(idxBaseStart, s->scopePrefix);
        if (baseFa.isFrame && !baseIsImmediate) {
            // Frame-relative base (stack local). Immediate (#symbol) bases are
            // never frame-relative — they're global/static data addresses.
            emitSpBaseAddrCalc16(p, e, baseFa.fpOff + baseFa.yOff);
        } else if (baseIsReg) {
            // Already in AX
        } else {
            std::string baseSymName = baseIsImmediate ? p->tokens[idxBaseStart + 1].value : p->tokens[idxBaseStart].value;
            Symbol* baseSym = p->resolveSymbol(baseSymName, s->scopePrefix);
            uint32_t baseAddr = 0;
            if (baseSym) baseAddr = baseSym->value;
            else {
                try { baseAddr = p->evaluateExpressionAt(idxBaseStart + (baseIsImmediate ? 1 : 0), s->scopePrefix); }
                catch (...) { baseAddr = 0; }
            }
            if (baseIsImmediate) {
                emitLoadAddrConst(p, e, baseSymName, 0, 'X');
            } else {
                bool isZP = (!baseSymName.empty() && baseSymName[0] == '$');
                if (isZP) {
                    e.lda_zp(baseAddr & 0xFF);
                    e.ldx_zp((baseAddr + 1) & 0xFF);
                } else {
                    Symbol* relSym = p->resolveSymbol(baseSymName, s->scopePrefix);
                    if (relSym && relSym->isAddress) { e.recordSymbolReloc(baseSymName); }
                    e.lda_abs(baseAddr);
                    if (relSym && relSym->isAddress) { e.recordSymbolReloc(baseSymName); }
                    e.ldx_abs(baseAddr + 1);
                }
            }
        }
        
        e.clc();
        e.adc_addr(m65::MULT_RES);
        e.pha(); e.txa(); e.adc_addr(m65::MULT_RES + 1); e.tax(); e.pla();
        
    } else {
        if (baseIsReg) {
            e.sta_zp(e.scratchZP());
            e.stx_zp(e.scratchZP() + 1);
            emitLoadIndexToAX(p, e, indexIsReg, indexRegName, idxIndexStart, s->scopePrefix);
            e.clc();
            e.adc_zp(e.scratchZP());
            e.pha(); e.txa(); e.adc_zp(e.scratchZP() + 1); e.tax(); e.pla();
        } else {
            emitLoadIndexToAX(p, e, indexIsReg, indexRegName, idxIndexStart, s->scopePrefix);
            e.sta_zp(e.scratchZP());
            e.stx_zp(e.scratchZP() + 1);
            
            auto baseFa = p->resolveFrameAccess(idxBaseStart, s->scopePrefix);
            if (baseFa.isFrame) {
                emitSpBaseAddrCalc16(p, e, baseFa.fpOff + baseFa.yOff);
            } else {
                std::string baseSymName = baseIsImmediate ? p->tokens[idxBaseStart + 1].value : p->tokens[idxBaseStart].value;
                Symbol* baseSym = p->resolveSymbol(baseSymName, s->scopePrefix);
                uint32_t baseAddr = 0;
                if (baseSym) baseAddr = baseSym->value;
                else {
                    try { baseAddr = p->evaluateExpressionAt(idxBaseStart + (baseIsImmediate ? 1 : 0), s->scopePrefix); }
                    catch (...) { baseAddr = 0; }
                }
                if (baseIsImmediate) {
                    emitLoadAddrConst(p, e, baseSymName, 0, 'X');
                } else {
                    bool isZP = (!baseSymName.empty() && baseSymName[0] == '$');
                    if (isZP) {
                        e.lda_zp(baseAddr & 0xFF);
                        e.ldx_zp((baseAddr + 1) & 0xFF);
                    } else {
                        Symbol* relSym = p->resolveSymbol(baseSymName, s->scopePrefix);
                        if (relSym && relSym->isAddress) { e.recordSymbolReloc(baseSymName); }
                        e.lda_abs(baseAddr);
                        if (relSym && relSym->isAddress) { e.recordSymbolReloc(baseSymName); }
                        e.ldx_abs(baseAddr + 1);
                    }
                }
            }
            e.clc();
            e.adc_zp(e.scratchZP());
            e.pha(); e.txa(); e.adc_zp(e.scratchZP() + 1); e.tax(); e.pla();
        }
    }
    
    std::string DEST = s->instr.operand;
    if (!DEST.empty() && DEST[0] != '.') DEST = "." + DEST;
    std::transform(DEST.begin(), DEST.end(), DEST.begin(), ::toupper);
    if (DEST != ".AX") {
        uint32_t destOffset = 0;
        bool destIsStack = p->isStackRelativeOperand(s->instr.operandTokenIndex, destOffset, s->scopePrefix);
        if (destIsStack) {
            auto destFa = p->resolveFrameAccess(s->instr.operandTokenIndex, s->scopePrefix);
            if (destFa.isFrame) {
                uint8_t totalOff = destFa.fpOff + destFa.yOff;
                e.stx_scratch(); e.sta_stack(totalOff); e.lda_scratch(); e.sta_stack(totalOff + 1);
            } else {
                e.stx_scratch(); e.sta_stack(destOffset); e.lda_scratch(); e.sta_stack(destOffset + 1);
            }
        } else {
            std::string destSymName = s->instr.operand;
            bool isZP = (!destSymName.empty() && destSymName[0] == '$');
            Symbol* destSym = p->resolveSymbol(destSymName, s->scopePrefix);
            uint32_t destAddr = 0;
            if (destSym) destAddr = destSym->value;
            else {
                try { destAddr = p->evaluateExpressionAt(s->instr.operandTokenIndex, s->scopePrefix); }
                catch (...) { destAddr = 0; }
            }
            if (isZP) {
                e.sta_zp(destAddr & 0xFF);
                e.txa(); e.sta_zp((destAddr + 1) & 0xFF);
            } else {
                if (destSym && destSym->isAddress) { e.recordSymbolReloc(destSymName); }
                e.sta_abs(destAddr);
                if (destSym && destSym->isAddress) { e.recordSymbolReloc(destSymName); }
                e.txa(); e.sta_abs(destAddr + 1);
            }
        }
    }
}
