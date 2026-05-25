#include "AssemblerParser.hpp"
#include "AssemblerExpression.hpp"
#include "AssemblerOptimizer.hpp"
#include "AssemblerSimulatedOps.hpp"
#include "AssemblerGenerator.hpp"
#include "M65Emitter.hpp"
#include "O45Types.hpp"
#include "StringUtil.hpp"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <memory>
#include <cmath>

// --- AssemblerParser Implementation ---

AssemblerParser::AssemblerParser(const std::vector<AssemblerToken>& tokens) : tokens(tokens), pos(0), pc(0) {
    switchSegment("default");
}

AssemblerParser::AssemblerParser(const std::vector<AssemblerToken>& tokens, const std::map<std::string, uint32_t>& predefinedSymbols) : tokens(tokens), pos(0), pc(0), nextScopeId(0) {
    for (const auto& [name, val] : predefinedSymbols) symbolTable[name] = {val, false, 2, false, false, val, false, 0, false, 0, ""};
    switchSegment("default");
}

void AssemblerParser::switchSegment(const std::string& name) {
    if (currentSegment) {
        currentSegment->pc = pc;
    }
    if (segments.find(name) == segments.end()) {
        auto seg = std::make_shared<Segment>();
        seg->name = name;
        seg->pc = 0;
        seg->hasOrg = false;
        segments[name] = seg;
        segmentOrder.push_back(seg);
    }
    currentSegment = segments[name];
    pc = currentSegment->pc;
}

const AssemblerToken& AssemblerParser::peek() const {
    if (pos >= tokens.size()) return tokens.back();
    return tokens[pos];
}

const AssemblerToken& AssemblerParser::advance() {
    if (pos < tokens.size()) pos++;
    return tokens[pos - 1];
}

bool AssemblerParser::match(AssemblerTokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

uint32_t AssemblerParser::evaluateExpressionAt(int index, const std::string& scopePrefix) {
    if (index < 0 || index >= (int)tokens.size()) return 0;
    int idx = index;
    auto ast = parseExprAST(tokens, idx, symbolTable, scopePrefix);
    if (!ast) throw std::runtime_error("Expected expression at line " + std::to_string(tokens[index].line));
    return ast->getValue(this);
}

Symbol* AssemblerParser::resolveSymbol(const std::string& name, const std::string& scopePrefix) {
    // For @ local labels outside proc scope, try auto-scope under last global label
    if (!name.empty() && name[0] == '@' && !currentLocalScope_.empty() && scopePrefix.empty()) {
        std::string scopedName = currentLocalScope_ + ":" + name;
        if (symbolTable.count(scopedName)) return &symbolTable.at(scopedName);
    }
    std::string current = scopePrefix;
    while (true) {
        std::string fullName = current + name;
        if (symbolTable.count(fullName)) return &symbolTable.at(fullName);
        if (current.empty()) break;
        if (current.size() < 2) { current = ""; continue; }
        size_t p = current.find_last_of(':', current.size() - 2);
        if (p == std::string::npos) current = "";
        else current = current.substr(0, p + 1);
    }
    return nullptr;
}

bool AssemblerParser::isRelocatableSymbol(const std::string& name) const {
    if (externIndex.count(name)) return true;
    auto it = symbolTable.find(name);
    if (it == symbolTable.end()) return false;
    const auto& seg = it->second.segment;
    if (seg.empty() || seg == "__extern__") return seg == "__extern__";
    auto sit = segments.find(seg);
    return sit != segments.end() && !sit->second->hasOrg;
}

uint32_t AssemblerParser::getZPStart() const {
    if (symbolTable.count("cc45.zeroPageStart")) {
        return symbolTable.at("cc45.zeroPageStart").value;
    }
    return 0x02;
}

uint16_t AssemblerParser::getSpBase() const {
    if (symbolTable.count("__sp_base")) {
        return (uint16_t)symbolTable.at("__sp_base").value;
    }
    return 0x0101;
}

uint8_t AssemblerParser::getScratchZP() const {
    if (symbolTable.count("__zp_scratch")) {
        return (uint8_t)symbolTable.at("__zp_scratch").value;
    }
    return 0x02;
}

const AssemblerToken& AssemblerParser::expect(AssemblerTokenType type, const std::string& message) {
    if (peek().type == type) return advance();
    throw std::runtime_error(message + " at " + std::to_string(peek().line) + ":" + std::to_string(peek().column));
}

bool AssemblerParser::isStackRelativeOperand(int tokenIndex, uint32_t& offset, const std::string& scopePrefix) {
    if (tokenIndex < 0 || tokenIndex >= (int)tokens.size()) return false;
    int idx = tokenIndex;
    try {
        auto ast = parseExprAST(tokens, idx, symbolTable, scopePrefix);
        if (ast && idx + 1 < (int)tokens.size() && 
            tokens[idx].type == AssemblerTokenType::COMMA &&
            (tokens[idx+1].value == "s" || tokens[idx+1].value == "S")) {
            try { offset = ast->getValue(this); } catch (...) { offset = 0; }
            return true;
        }
    } catch (...) {}
    return false;
}

AssemblerParser::FrameAccessInfo AssemblerParser::resolveFrameAccess(int tokenIndex, const std::string& scopePrefix) {
    FrameAccessInfo info;
    if (tokenIndex < 0 || tokenIndex >= (int)tokens.size()) return info;
    std::string baseName = tokens[tokenIndex].value;
    Symbol* sym = resolveSymbol(baseName, scopePrefix);
    if (sym && sym->isFrameRelative) {
        info.isFrame = true;
        uint32_t exprVal = evaluateExpressionAt(tokenIndex, scopePrefix);
        info.yOff = (uint8_t)exprVal;
        Symbol* fpSym = resolveSymbol("_fp", scopePrefix);
        info.fpOff = fpSym ? (uint8_t)fpSym->value : 1;
    }
    return info;
}

bool AssemblerParser::optimize() { return AssemblerOptimizer::optimize(this); }

void AssemblerParser::emitExpressionCode(std::vector<uint8_t>& binary, const std::string& target, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitExpressionCode(this, e, target, tokenIndex, scopePrefix);
}

void AssemblerParser::emitMulCode(std::vector<uint8_t>& binary, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitMulCode(this, e, width, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitDivCode(std::vector<uint8_t>& binary, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitDivCode(this, e, width, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitStackIncDecCode(std::vector<uint8_t>& binary, bool isInc, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitStackIncDecCode(this, e, isInc, tokenIndex, scopePrefix);
}

void AssemblerParser::emitStackIncDec8Code(std::vector<uint8_t>& binary, bool isInc, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitStackIncDec8Code(this, e, isInc, tokenIndex, scopePrefix);
}

void AssemblerParser::emitAddSub16Code(std::vector<uint8_t>& binary, bool isAdd, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitAddSub16Code(this, e, isAdd, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitBitwise16Code(std::vector<uint8_t>& binary, const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitBitwise16Code(this, e, mnemonic, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitCMP16Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitCMP16Code(this, e, src1, tokenIndex, scopePrefix);
}

void AssemblerParser::emitCMP_S16Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitCMP_S16Code(this, e, src1, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLDWCode(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix, bool forceStack) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDWCode(this, e, dest, tokenIndex, scopePrefix, forceStack);
}

void AssemblerParser::emitSTWCode(std::vector<uint8_t>& binary, const std::string& src, int tokenIndex, const std::string& scopePrefix, bool forceStack) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTWCode(this, e, src, tokenIndex, scopePrefix, forceStack);
}

void AssemblerParser::emitLDA_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDA_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSTA_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTA_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLDX_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDX_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLDY_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDY_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLDZ_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDZ_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSTX_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTX_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSTY_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTY_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSTZ_StackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTZ_StackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSwapCode(std::vector<uint8_t>& binary, const std::string& r1, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSwapCode(this, e, r1, tokenIndex, scopePrefix);
}

void AssemblerParser::emitZeroCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitZeroCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitNegNot16Code(std::vector<uint8_t>& binary, bool isNeg, const std::string& operand, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitNegNot16Code(this, e, isNeg, operand, tokenIndex, scopePrefix);
}

void AssemblerParser::emitABS16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitABS16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitChkZeroCode(std::vector<uint8_t>& binary, bool is16, bool isInverse, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitChkZeroCode(this, e, is16, isInverse, tokenIndex, scopePrefix);
}

void AssemblerParser::emitBranch16Code(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitBranch16Code(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSelectCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSelectCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitPtrStackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitPtrStackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitPtrDerefCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitPtrDerefCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitFillCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix, bool forceStack) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitFillCode(this, e, tokenIndex, scopePrefix, forceStack);
}

void AssemblerParser::emitMoveCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix, bool forceStack) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitMoveCode(this, e, tokenIndex, scopePrefix, forceStack);
}

void AssemblerParser::emitFlatMemoryCode(std::vector<uint8_t>& binary, const std::string& mnemonic, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitFlatMemoryCode(this, e, mnemonic, tokenIndex, scopePrefix);
}

void AssemblerParser::emitPHWStackCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitPHWStackCode(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::pass1() {
    segments.clear();
    segmentOrder.clear();
    segmentStack.clear();
    switchSegment("default");
    pos = 0;
    statements.clear();
    scopeStack.clear();
    nextScopeId = 0;
    procedures.clear();
    currentProc = nullptr;
    firstOrgAddress = 0xFFFFFFFF;
    std::vector<std::shared_ptr<ProcContext>> pass1ProcStack;
    std::string autoLabelScope; // auto-scope for @ local labels outside proc
    currentLocalScope_ = ""; // reset for this pass

    auto currentScopePrefix = [&]() {
        std::string p = "";
        for (const auto& s : scopeStack) p += s + ":";
        return p;
    };

    while (pos < tokens.size()) {
        while (match(AssemblerTokenType::NEWLINE));
        if (peek().type == AssemblerTokenType::END_OF_FILE) break;

        auto stmt = std::make_unique<Statement>();
        stmt->address = pc;
        stmt->line = peek().line;
        stmt->scopePrefix = currentScopePrefix();
        stmt->segmentName = currentSegment->name;
        stmt->sourceFile = currentSourceFile_;
        stmt->sourceLine = currentSourceLine_;

        if ((peek().type == AssemblerTokenType::IDENTIFIER || peek().type == AssemblerTokenType::INSTRUCTION) && pos + 1 < tokens.size() && tokens[pos+1].type == AssemblerTokenType::COLON) {
            std::string labelName = advance().value;
            if (!labelName.empty() && labelName[0] != '@' && scopeStack.empty()) {
                // Non-@ labels outside proc become scope anchors for @ local labels
                autoLabelScope = labelName;
                currentLocalScope_ = labelName;
            }
            // @ labels get prefixed with the auto-scope; non-@ labels use normal scope
            if (!labelName.empty() && labelName[0] == '@' && scopeStack.empty() && !autoLabelScope.empty()) {
                stmt->label = autoLabelScope + ":" + labelName;
            } else {
                stmt->label = stmt->scopePrefix + labelName;
            }
            advance();
            if (symbolTable.count(stmt->label) && symbolTable[stmt->label].isAddress) {
                errors.push_back("Error: Duplicate label '" + stmt->label + "' at line " + std::to_string(stmt->line));
            }
            symbolTable[stmt->label] = {pc, true, 2, false, false, pc, false, 0, false, 0, currentSegment->name};
        }
        stmt->localLabelScope = autoLabelScope;

        if (peek().type == AssemblerTokenType::IDENTIFIER && pos + 1 < tokens.size() && tokens[pos+1].type == AssemblerTokenType::EQUALS) {
            std::string name = advance().value;
            advance(); // =
            uint32_t val = evaluateExpressionAt((int)pos, stmt->scopePrefix);
            std::string fullSymName = stmt->scopePrefix + name;
            if (symbolTable.count(fullSymName) && symbolTable[fullSymName].isConstant) {
                errors.push_back("Error: Redefinition of constant symbol '" + fullSymName + "'");
            }
            symbolTable[fullSymName] = {val, false, 2, false, false, val, false, 0, false, 0, ""};
            while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
            continue;
        }

        if (match(AssemblerTokenType::OPEN_CURLY)) {
            scopeStack.push_back("_S" + std::to_string(nextScopeId++));
            segmentStack.push_back("");
            stmt->size = 0;
            statements.push_back(std::move(stmt));
            continue;
        }
        else if (match(AssemblerTokenType::CLOSE_CURLY)) {
            if (!scopeStack.empty()) scopeStack.pop_back();
            if (!segmentStack.empty()) {
                std::string prevSeg = segmentStack.back();
                segmentStack.pop_back();
                if (!prevSeg.empty()) switchSegment(prevSeg);
            }
            stmt->size = 0;
            statements.push_back(std::move(stmt));
            continue;
        }
        else if (match(AssemblerTokenType::DIRECTIVE)) {
            stmt->type = Statement::DIRECTIVE;
            stmt->dir.name = tokens[pos-1].value;

            // .repeat N / .endrepeat — compile-time loop unrolling
            if (stmt->dir.name == "repeat") {
                int repeatCount = (int)evaluateExpressionAt((int)pos, currentScopePrefix());
                while (pos < tokens.size() && tokens[pos].type != AssemblerTokenType::NEWLINE &&
                       tokens[pos].type != AssemblerTokenType::END_OF_FILE) pos++;
                if (pos < tokens.size() && tokens[pos].type == AssemblerTokenType::NEWLINE) pos++;

                // Collect tokens until .endrepeat
                size_t bodyStart = pos;
                int depth = 1;
                while (pos < tokens.size() && depth > 0) {
                    if (tokens[pos].type == AssemblerTokenType::DIRECTIVE) {
                        if (tokens[pos].value == "repeat") depth++;
                        else if (tokens[pos].value == "endrepeat") {
                            depth--;
                            if (depth == 0) break;
                        }
                    }
                    pos++;
                }
                size_t bodyEnd = pos;
                if (pos < tokens.size()) pos++; // skip .endrepeat
                // Skip trailing newline
                if (pos < tokens.size() && tokens[pos].type == AssemblerTokenType::NEWLINE) pos++;

                // Splice N copies of the body tokens into the token stream
                if (repeatCount > 0 && bodyEnd > bodyStart) {
                    std::vector<AssemblerToken> bodyTokens(tokens.begin() + bodyStart, tokens.begin() + bodyEnd);
                    std::vector<AssemblerToken> expanded;
                    for (int r = 0; r < repeatCount; r++) {
                        for (auto& t : bodyTokens) expanded.push_back(t);
                    }
                    tokens.insert(tokens.begin() + pos, expanded.begin(), expanded.end());
                }
                continue;
            }

            if (stmt->dir.name == "endrepeat") {
                // Stray .endrepeat without matching .repeat — skip
                while (pos < tokens.size() && tokens[pos].type != AssemblerTokenType::NEWLINE) pos++;
                continue;
            }

            if (stmt->dir.name == "segment" || stmt->dir.name == "code" || stmt->dir.name == "data" || stmt->dir.name == "bss") {
                    std::string newSeg;
                    if (stmt->dir.name == "segment") {
                        if (peek().type == AssemblerTokenType::STRING_LITERAL) newSeg = advance().value;
                        else newSeg = expect(AssemblerTokenType::IDENTIFIER, "Expected segment name").value;
                    } else newSeg = stmt->dir.name;
                    std::string oldSeg = currentSegment->name;
                    switchSegment(newSeg);
                    if (match(AssemblerTokenType::OPEN_CURLY)) {
                        scopeStack.push_back("_S" + std::to_string(nextScopeId++));
                        segmentStack.push_back(oldSeg);
                    }
                    stmt->size = 0;
                    stmt->address = pc;
                    stmt->segmentName = currentSegment->name;
                    statements.push_back(std::move(stmt));
                    continue;
            }

            if (stmt->dir.name == "segmentOrder") {
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                    if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                    requestedSegmentOrder.push_back(advance().value);
                }
                stmt->size = 0;
                statements.push_back(std::move(stmt));
                continue;
            }

            if (stmt->dir.name == "var") {
                std::string vN = expect(AssemblerTokenType::IDENTIFIER, "Expected var name").value;
                std::string scVN = stmt->scopePrefix + vN;
                stmt->dir.varName = scVN;
                if (symbolTable.count(scVN) && symbolTable[scVN].isConstant) {
                    errors.push_back("Error: Cannot redefine constant '" + scVN + "' as variable");
                }
                if (match(AssemblerTokenType::EQUALS)) {
                    stmt->dir.varType = Directive::ASSIGN;
                    stmt->dir.tokenIndex = (int)pos;
                    uint32_t val = evaluateExpressionAt((int)pos, stmt->scopePrefix);
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                    symbolTable[scVN] = {val, false, 2, true, false, val, false, 0, false, 0, ""};
                }
                else if (match(AssemblerTokenType::INCREMENT)) {
                    stmt->dir.varType = Directive::INC;
                    if (symbolTable.count(scVN)) symbolTable[scVN].value++;
                }
                else if (match(AssemblerTokenType::DECREMENT)) {
                    stmt->dir.varType = Directive::DEC;
                    if (symbolTable.count(scVN)) symbolTable[scVN].value--;
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "local") {
                std::string vN = expect(AssemblerTokenType::IDENTIFIER, "Expected local name").value;
                std::string scVN = stmt->scopePrefix + vN;
                stmt->dir.varName = scVN;
                if (match(AssemblerTokenType::EQUALS)) {
                    stmt->dir.varType = Directive::ASSIGN;
                    stmt->dir.tokenIndex = (int)pos;
                    uint32_t val = evaluateExpressionAt((int)pos, stmt->scopePrefix);
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                    Symbol sym = {val, false, 2, true, false, val, false, 0, true, (int)val, ""};
                    symbolTable[scVN] = sym;
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "const") {
                std::string cN = expect(AssemblerTokenType::IDENTIFIER, "Expected constant name").value;
                std::string scCN = stmt->scopePrefix + cN;
                stmt->dir.varName = scCN;
                if (symbolTable.count(scCN)) {
                    errors.push_back("Error: Redefinition of symbol '" + scCN + "'");
                }
                expect(AssemblerTokenType::EQUALS, "Expected '=' after constant name");
                uint32_t val = evaluateExpressionAt((int)pos, stmt->scopePrefix);
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                symbolTable[scCN] = {val, false, 2, false, true, val, false, 0, false, 0, ""};
                stmt->size = 0;
            }
            else if (stmt->dir.name == "global") {
                // .global sym1, sym2, ... — mark symbols for export
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                    if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                    std::string sym = expect(AssemblerTokenType::IDENTIFIER, "Expected symbol name after .global").value;
                    globalSymbols.insert(sym);
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "weak") {
                // .weak sym1, sym2, ... — mark symbols as weak exports
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                    if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                    std::string sym = expect(AssemblerTokenType::IDENTIFIER, "Expected symbol name after .weak").value;
                    globalSymbols.insert(sym); // weak symbols are also global (exported)
                    weakSymbols.insert(sym);
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "extern") {
                // .extern sym1, sym2, ... — declare external (imported) symbols
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                    if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                    std::string sym = expect(AssemblerTokenType::IDENTIFIER, "Expected symbol name after .extern").value;
                    if (!externIndex.count(sym)) {
                        uint32_t idx = (uint32_t)externSymbols.size();
                        externSymbols.push_back(sym);
                        externIndex[sym] = idx;
                        // Add extern as a placeholder symbol (value 0, will be resolved by linker)
                        symbolTable[sym] = {0, true, 2, false, false, 0, false, 0, false, 0, "__extern__"};
                    }
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "zp_uses" || stmt->dir.name == "zp_clobbers" || stmt->dir.name == "zp_release") {
                // .zp_uses $03, $04, $05   — ZP slots read as parameters
                // .zp_clobbers $03, $04    — ZP slots written by this function
                // .zp_release $07, $08     — ZP slots consumed (caller need not restore)
                if (!currentProc) {
                    errors.push_back("Error: ." + stmt->dir.name + " outside proc/endproc block");
                } else {
                    uint32_t mask = 0;
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                        if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                        uint32_t addr = evaluateExpressionAt((int)pos, stmt->scopePrefix);
                        while (peek().type != AssemblerTokenType::COMMA && peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                        // Convert ZP address to bit position relative to zeroPageStart+1
                        uint32_t zpStart = getZPStart();
                        if (addr > zpStart) {
                            uint32_t bit = addr - (zpStart + 1);
                            if (bit < 32) mask |= (1u << bit);
                            else errors.push_back("Error: ZP address $" + std::to_string(addr) + " out of range for function attributes");
                        }
                    }
                    if (stmt->dir.name == "zp_uses") currentProc->zpUsesMask |= mask;
                    else if (stmt->dir.name == "zp_clobbers") currentProc->zpClobbersMask |= mask;
                    else currentProc->zpReleaseMask |= mask;
                    currentProc->hasFuncAttrs = true;
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "reg_clobbers") {
                // .reg_clobbers A, X, Y, Z
                if (!currentProc) {
                    errors.push_back("Error: .reg_clobbers outside proc/endproc block");
                } else {
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                        if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                        std::string reg = advance().value;
                        if (reg == "A" || reg == "a") currentProc->regClobbersMask |= 0x01;
                        else if (reg == "X" || reg == "x") currentProc->regClobbersMask |= 0x02;
                        else if (reg == "Y" || reg == "y") currentProc->regClobbersMask |= 0x04;
                        else if (reg == "Z" || reg == "z") currentProc->regClobbersMask |= 0x08;
                        else errors.push_back("Error: unknown register '" + reg + "' in .reg_clobbers");
                    }
                    currentProc->hasFuncAttrs = true;
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "flag_clobbers") {
                // .flag_clobbers C, N, Z, V
                if (!currentProc) {
                    errors.push_back("Error: .flag_clobbers outside proc/endproc block");
                } else {
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                        if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                        std::string flag = advance().value;
                        if (flag == "C" || flag == "c") currentProc->flagClobbersMask |= 0x01;
                        else if (flag == "N" || flag == "n") currentProc->flagClobbersMask |= 0x02;
                        else if (flag == "Z" || flag == "z") currentProc->flagClobbersMask |= 0x04;
                        else if (flag == "V" || flag == "v") currentProc->flagClobbersMask |= 0x08;
                        else errors.push_back("Error: unknown flag '" + flag + "' in .flag_clobbers");
                    }
                    currentProc->hasFuncAttrs = true;
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "func_flags") {
                // .func_flags zp_call, leaf, reentrant, int_safe, stack_call
                if (!currentProc) {
                    errors.push_back("Error: .func_flags outside proc/endproc block");
                } else {
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                        if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                        std::string flag = advance().value;
                        if (flag == "zp_call")        currentProc->funcFlags |= FUNC_FLAG_ZP_CONV;
                        else if (flag == "stack_call") { /* ZP_CONV bit clear; hasFuncAttrs signals known */ }
                        else if (flag == "leaf")       currentProc->funcFlags |= FUNC_FLAG_LEAF;
                        else if (flag == "reentrant")  currentProc->funcFlags |= FUNC_FLAG_REENTRANT;
                        else errors.push_back("Error: unknown flag '" + flag + "' in .func_flags");
                    }
                    currentProc->hasFuncAttrs = true;
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "loc") {
                // .loc "filename", line — source-level debug info marker
                currentSourceFile_ = advance().value;
                if (match(AssemblerTokenType::COMMA)) {
                    currentSourceLine_ = (int)evaluateExpressionAt((int)pos, stmt->scopePrefix);
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                }
                stmt->size = 0;
            }
            else if (stmt->dir.name == "array") {
                // .array name, element_size, dim0 [, dim1 [, dim2 ...]]
                std::string arrName = expect(AssemblerTokenType::IDENTIFIER, "Expected array name").value;
                std::string scName = stmt->scopePrefix + arrName;
                if (peek().type == AssemblerTokenType::COMMA) advance();
                uint32_t elemSize = evaluateExpressionAt((int)pos, stmt->scopePrefix);
                while (peek().type != AssemblerTokenType::COMMA && peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();

                ArrayInfo info;
                info.elementSize = elemSize;
                while (peek().type == AssemblerTokenType::COMMA) {
                    advance(); // consume comma
                    int dimPos = (int)pos;
                    uint32_t dim = evaluateExpressionAt(dimPos, stmt->scopePrefix);
                    while (pos < (size_t)dimPos || (peek().type != AssemblerTokenType::COMMA && peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE)) {
                        if (pos >= tokens.size()) break;
                        advance();
                    }
                    info.dimensions.push_back(dim);
                }
                if (info.dimensions.empty()) {
                    errors.push_back("Error: .array requires at least one dimension");
                    stmt->size = 0;
                } else {
                    // Compute strides: stride[i] = product(dims[i+1..]) * elementSize
                    info.strides.resize(info.dimensions.size());
                    uint32_t stride = elemSize;
                    for (int i = (int)info.dimensions.size() - 1; i >= 0; --i) {
                        info.strides[i] = stride;
                        stride *= info.dimensions[i];
                    }
                    uint32_t totalSize = stride; // product of all dims * elemSize

                    // Define label at current address
                    stmt->label = scName;
                    symbolTable[scName] = {pc, true, 2, false, false, pc, false, 0, false, 0, currentSegment->name};

                    // Define metadata constants in symbol table
                    symbolTable[scName + ".__elsize"] = {elemSize, false, 2, false, true, elemSize, false, 0, false, 0, ""};
                    symbolTable[scName + ".__dims"] = {(uint32_t)info.dimensions.size(), false, 2, false, true, (uint32_t)info.dimensions.size(), false, 0, false, 0, ""};
                    for (size_t i = 0; i < info.dimensions.size(); ++i) {
                        symbolTable[scName + ".__dim" + std::to_string(i)] = {info.dimensions[i], false, 2, false, true, info.dimensions[i], false, 0, false, 0, ""};
                        symbolTable[scName + ".__stride" + std::to_string(i)] = {info.strides[i], false, 2, false, true, info.strides[i], false, 0, false, 0, ""};
                    }

                    arrayInfos[scName] = info;
                    // Reserve storage — keep directive name as "array"
                    // Generator will emit totalSize zero bytes
                    stmt->size = (int)totalSize;
                    stmt->dir.arguments.clear();
                    stmt->dir.arguments.push_back(std::to_string(totalSize));
                }
            }
            else if (stmt->dir.name == "cleanup") {
                stmt->dir.tokenIndex = (int)pos;
                uint32_t val = evaluateExpressionAt((int)pos, stmt->scopePrefix);
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                if (currentProc) currentProc->totalParamSize += val;
                stmt->size = 0;
            }
            else if (stmt->dir.name == "basicUpstart") {
                if (firstOrgAddress == 0xFFFFFFFF) {
                    firstOrgAddress = 0x2001;
                    pc = 0x2001;
                    currentSegment->pc = pc;
                    
                    // Synthesize an org statement so pass2 knows the start address
                    auto orgStmt = std::make_unique<Statement>();
                    orgStmt->type = Statement::DIRECTIVE;
                    orgStmt->dir.name = "org";
                    orgStmt->dir.arguments.push_back("$2001");
                    orgStmt->address = pc;
                    orgStmt->segmentName = currentSegment->name;
                    orgStmt->scopePrefix = stmt->scopePrefix;
                    statements.push_back(std::move(orgStmt));
                    
                    stmt->address = pc;
                }
                stmt->type = Statement::BASIC_UPSTART;
                stmt->basicUpstartTokenIndex = (int)pos;
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                stmt->size = 12;
            }
            else {
                stmt->dir.tokenIndex = (int)pos;
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                    if (peek().type == AssemblerTokenType::COMMA) { advance(); continue; }
                    stmt->dir.arguments.push_back(advance().value);
                }
                if (stmt->dir.name == "org") {
                    if (!stmt->dir.arguments.empty()) pc = parseNumericLiteral(stmt->dir.arguments[0]);
                    if (firstOrgAddress == 0xFFFFFFFF) firstOrgAddress = pc;
                    currentSegment->hasOrg = true;
                    stmt->address = pc;
                    stmt->size = 0;
                }
                else if (stmt->dir.name == "cpu") stmt->size = 0;
                else stmt->size = calculateDirectiveSize(stmt->dir, pc);
            }
        }
        else if (peek().type == AssemblerTokenType::STAR && pos + 1 < tokens.size() && tokens[pos+1].type == AssemblerTokenType::EQUALS) {
            advance(); advance();
            stmt->type = Statement::DIRECTIVE;
            stmt->dir.name = "org";
            stmt->dir.arguments.push_back(advance().value);
            if (!stmt->dir.arguments.empty()) pc = parseNumericLiteral(stmt->dir.arguments[0]);
            if (firstOrgAddress == 0xFFFFFFFF) firstOrgAddress = pc;
            currentSegment->hasOrg = true;
            stmt->address = pc;
            stmt->size = 0;
        }
        else if (match(AssemblerTokenType::INSTRUCTION)) {
          try {
            stmt->type = Statement::INSTRUCTION;
            std::string fullMnemonic = tokens[pos-1].value;
            std::transform(fullMnemonic.begin(), fullMnemonic.end(), fullMnemonic.begin(), ::tolower);
            stmt->instr.mnemonic = fullMnemonic;

            // Mnemonic → type + dispatch function assignment
            // Shorthand: S = AssemblerSimulatedOps
            using S = AssemblerSimulatedOps;
            #define SIMOP(T, F) stmt->type = Statement::T; stmt->emitFn = S::F

            if (fullMnemonic == "add.16") { SIMOP(ADD16, dispatch_AddSub16); }
            else if (fullMnemonic == "sub.16") { SIMOP(SUB16, dispatch_AddSub16); }
            else if (fullMnemonic == "add.s16") { SIMOP(ADDS16, dispatch_AddSub16); }
            else if (fullMnemonic == "sub.s16") { SIMOP(SUBS16, dispatch_AddSub16); }
            else if (fullMnemonic == "and.16") { SIMOP(AND16, dispatch_Bitwise16); }
            else if (fullMnemonic == "ora.16") { SIMOP(ORA16, dispatch_Bitwise16); }
            else if (fullMnemonic == "eor.16") { SIMOP(EOR16, dispatch_Bitwise16); }
            else if (fullMnemonic == "cmp.16" || fullMnemonic == "cpw") { SIMOP(CMP16, dispatch_CMP16); }
            else if (fullMnemonic == "cmp.s16") { SIMOP(CMP_S16, dispatch_CMP_S16); }
            else if (fullMnemonic == "ldw" || fullMnemonic == "ldw.sp") { SIMOP(LDW, dispatch_LDW); }
            else if (fullMnemonic == "stw" || fullMnemonic == "stw.sp") { SIMOP(STW, dispatch_STW); }
            else if (fullMnemonic == "fill" || fullMnemonic == "fill.sp") { SIMOP(FILL, dispatch_Fill); }
            else if (fullMnemonic == "move" || fullMnemonic == "move.sp") { SIMOP(COPY, dispatch_Copy); }
            else if (fullMnemonic == "swap") { SIMOP(SWAP, dispatch_Swap); }
            else if (fullMnemonic == "neg.16") { SIMOP(NEG16, dispatch_NegNot16); }
            else if (fullMnemonic == "not.16") { SIMOP(NOT16, dispatch_NegNot16); }
            else if (fullMnemonic == "neg.s16") { SIMOP(NEG_S16, dispatch_NegNot16); }
            else if (fullMnemonic == "abs.16") { SIMOP(ABS16, dispatch_ABS16); }
            else if (fullMnemonic == "abs.s16") { SIMOP(ABS_S16, dispatch_ABS16); }
            else if (fullMnemonic == "chkzero.8") { SIMOP(CHKZERO8, dispatch_ChkZero); }
            else if (fullMnemonic == "chkzero.16") { SIMOP(CHKZERO16, dispatch_ChkZero); }
            else if (fullMnemonic == "chknonzero.8") { SIMOP(CHKNONZERO8, dispatch_ChkZero); }
            else if (fullMnemonic == "chknonzero.16") { SIMOP(CHKNONZERO16, dispatch_ChkZero); }
            else if (fullMnemonic == "branch.16") { SIMOP(BRANCH16, dispatch_Branch16); }
            else if (fullMnemonic == "select") { SIMOP(SELECT, dispatch_Select); }
            else if (fullMnemonic == "ptrstack") { SIMOP(PTRSTACK, dispatch_PtrStack); }
            else if (fullMnemonic == "ptrderef") { SIMOP(PTRDEREF, dispatch_PtrDeref); }
            else if (fullMnemonic == "phw.sp") { SIMOP(PHW_STACK, dispatch_PHWStack); }
            else if (fullMnemonic == "lda.sp") { SIMOP(LDA_STACK, dispatch_LDA_Stack); }
            else if (fullMnemonic == "sta.sp") { SIMOP(STA_STACK, dispatch_STA_Stack); }
            else if (fullMnemonic == "ldw.f") { SIMOP(LDWF, dispatch_FlatMemory); }
            else if (fullMnemonic == "stw.f") { SIMOP(STWF, dispatch_FlatMemory); }
            else if (fullMnemonic == "inc.f") { SIMOP(INCF, dispatch_FlatMemory); }
            else if (fullMnemonic == "dec.f") { SIMOP(DECF, dispatch_FlatMemory); }
            else if (fullMnemonic == "asw") { SIMOP(ASW, dispatch_ASW); }
            else if (fullMnemonic == "row") { SIMOP(ROW, dispatch_ROW); }
            else if (fullMnemonic == "lsl.16") { SIMOP(LSL16, dispatch_Shift16); }
            else if (fullMnemonic == "lsr.16") { SIMOP(LSR16, dispatch_Shift16); }
            else if (fullMnemonic == "rol.16") { SIMOP(ROL16, dispatch_Shift16); }
            else if (fullMnemonic == "ror.16") { SIMOP(ROR16, dispatch_Shift16); }
            else if (fullMnemonic == "asr.16") { SIMOP(ASR16, dispatch_Shift16); }
            else if (fullMnemonic == "lsl.s16") { SIMOP(LSL_S16, dispatch_Shift16); }
            else if (fullMnemonic == "lsr.s16") { SIMOP(LSR_S16, dispatch_Shift16); }
            else if (fullMnemonic == "rol.s16") { SIMOP(ROL_S16, dispatch_Shift16); }
            else if (fullMnemonic == "ror.s16") { SIMOP(ROR_S16, dispatch_Shift16); }
            else if (fullMnemonic == "asr.s16") { SIMOP(ASR_S16, dispatch_Shift16); }
            else if (fullMnemonic == "sxt.8") {
                SIMOP(SXT8, dispatch_SXT8);
                if (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                }
            }
            else if (fullMnemonic == "sxt.16") { SIMOP(SXT16, dispatch_SXT16); }
            else if (fullMnemonic == "add.32") { SIMOP(ADD32, dispatch_AddSub32); }
            else if (fullMnemonic == "sub.32") { SIMOP(SUB32, dispatch_AddSub32); }
            else if (fullMnemonic == "add.s32") { SIMOP(ADDS32, dispatch_AddSub32); }
            else if (fullMnemonic == "sub.s32") { SIMOP(SUBS32, dispatch_AddSub32); }
            else if (fullMnemonic == "and.32") { SIMOP(AND32, dispatch_Bitwise32); }
            else if (fullMnemonic == "ora.32") { SIMOP(ORA32, dispatch_Bitwise32); }
            else if (fullMnemonic == "eor.32") { SIMOP(EOR32, dispatch_Bitwise32); }
            else if (fullMnemonic == "cmp.32") { SIMOP(CMP32, dispatch_CMP32); }
            else if (fullMnemonic == "cmp.s32") { SIMOP(CMP_S32, dispatch_CMP_S32); }
            else if (fullMnemonic == "neg.32") { SIMOP(NEG32, dispatch_NegNot32); }
            else if (fullMnemonic == "neg.s32") { SIMOP(NEG_S32, dispatch_NegNot32); }
            else if (fullMnemonic == "not.32") { SIMOP(NOT32, dispatch_NegNot32); }
            else if (fullMnemonic == "abs.32") { SIMOP(ABS32, dispatch_ABS32); }
            else if (fullMnemonic == "abs.s32") { SIMOP(ABS_S32, dispatch_ABS32); }
            else if (fullMnemonic == "lsl.32") { SIMOP(LSL32, dispatch_Shift32); }
            else if (fullMnemonic == "lsr.32") { SIMOP(LSR32, dispatch_Shift32); }
            else if (fullMnemonic == "rol.32") { SIMOP(ROL32, dispatch_Shift32); }
            else if (fullMnemonic == "ror.32") { SIMOP(ROR32, dispatch_Shift32); }
            else if (fullMnemonic == "asr.32") { SIMOP(ASR32, dispatch_Shift32); }
            else if (fullMnemonic == "lsl.s32") { SIMOP(LSL_S32, dispatch_Shift32); }
            else if (fullMnemonic == "lsr.s32") { SIMOP(LSR_S32, dispatch_Shift32); }
            else if (fullMnemonic == "rol.s32") { SIMOP(ROL_S32, dispatch_Shift32); }
            else if (fullMnemonic == "ror.s32") { SIMOP(ROR_S32, dispatch_Shift32); }
            else if (fullMnemonic == "asr.s32") { SIMOP(ASR_S32, dispatch_Shift32); }
            else if (fullMnemonic == "push") { SIMOP(PUSH, dispatch_PushPop); }
            else if (fullMnemonic == "pop") { SIMOP(POP, dispatch_PushPop); }
            else if (fullMnemonic == "lda.fp") { SIMOP(LDA_FP, dispatch_LDA_FP); }
            else if (fullMnemonic == "sta.fp") { SIMOP(STA_FP, dispatch_STA_FP); }
            else if (fullMnemonic == "ldax.fp") { SIMOP(LDAX_FP, dispatch_LDAX_FP); }
            else if (fullMnemonic == "stax.fp") { SIMOP(STAX_FP, dispatch_STAX_FP); }
            else if (fullMnemonic == "ldaxyz.fp") { SIMOP(LDAXYZ_FP, dispatch_LDAXYZ_FP); }
            else if (fullMnemonic == "staxyz.fp") { SIMOP(STAXYZ_FP, dispatch_STAXYZ_FP); }
            else if (fullMnemonic == "leax.fp") { SIMOP(LEAX_FP, dispatch_LEAX_FP); }
            else if (fullMnemonic == "move.fp") { SIMOP(MOVE_FP, dispatch_MOVE_FP); }
            else if (fullMnemonic == "bfext") { SIMOP(BFEXT, dispatch_BFExt); }
            else if (fullMnemonic == "bfext16") { SIMOP(BFEXT16, dispatch_BFExt); }
            else if (fullMnemonic == "bfins") { SIMOP(BFINS, dispatch_BFIns); }
            else if (fullMnemonic == "bfins.sp") { SIMOP(BFINS_SP, dispatch_BFIns); }
            else if (fullMnemonic == "bfins.ind") { SIMOP(BFINS_IND, dispatch_BFIns); }
            else if (fullMnemonic == "bfins16") { SIMOP(BFINS16, dispatch_BFIns); }
            else if (fullMnemonic == "bfins16.sp") { SIMOP(BFINS16_SP, dispatch_BFIns); }
            else if (fullMnemonic == "bfins16.ind") { SIMOP(BFINS16_IND, dispatch_BFIns); }
            #undef SIMOP

            if (stmt->instr.mnemonic == "expr") {
                stmt->type = Statement::EXPR; stmt->emitFn = AssemblerSimulatedOps::dispatch_Expr;
                const auto& trg = advance();
                stmt->exprTarget = (trg.type == AssemblerTokenType::REGISTER ? "." : "") + trg.value;
                expect(AssemblerTokenType::COMMA, "Expected ,");
                stmt->exprTokenIndex = (int)pos;
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                std::vector<uint8_t> d;
                emitExpressionCode(d, stmt->exprTarget, stmt->exprTokenIndex, stmt->scopePrefix);
                stmt->size = d.size();
            }
            else if (stmt->instr.mnemonic == "mul.s16" || stmt->instr.mnemonic == "div.s16" ||
                     stmt->instr.mnemonic == "mod.16" || stmt->instr.mnemonic == "mod.s16") {
                std::string m = stmt->instr.mnemonic;
                if (m == "mul.s16") { stmt->type = Statement::MUL_S16; stmt->emitFn = AssemblerSimulatedOps::dispatch_MulS16; }
                else if (m == "div.s16") { stmt->type = Statement::DIV_S16; stmt->emitFn = AssemblerSimulatedOps::dispatch_DivS16; }
                else if (m == "mod.16") { stmt->type = Statement::MOD16; stmt->emitFn = AssemblerSimulatedOps::dispatch_Mod16; }
                else { stmt->type = Statement::MOD_S16; stmt->emitFn = AssemblerSimulatedOps::dispatch_Mod16; }
                const auto& dst = advance();
                stmt->instr.operand = (dst.type == AssemblerTokenType::REGISTER ? "." : "") + dst.value;
                expect(AssemblerTokenType::COMMA, "Expected , after destination");
                stmt->exprTokenIndex = (int)pos;
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                std::vector<uint8_t> d;
                if (stmt->type == Statement::MUL_S16) emitMulS16Code(d, stmt->instr.operand, stmt->exprTokenIndex, stmt->scopePrefix);
                else if (stmt->type == Statement::DIV_S16) emitDivS16Code(d, stmt->instr.operand, stmt->exprTokenIndex, stmt->scopePrefix);
                else emitMod16Code(d, stmt->type == Statement::MOD_S16, stmt->instr.operand, stmt->exprTokenIndex, stmt->scopePrefix);
                stmt->size = d.size();
            }
            else if (stmt->instr.mnemonic.substr(0, 3) == "mul" || stmt->instr.mnemonic.substr(0, 3) == "div") {
                stmt->type = (stmt->instr.mnemonic.substr(0, 3) == "mul") ? Statement::MUL : Statement::DIV;
                stmt->emitFn = (stmt->type == Statement::MUL) ? AssemblerSimulatedOps::dispatch_Mul : AssemblerSimulatedOps::dispatch_Div;
                std::string m = stmt->instr.mnemonic;
                if (m.size() > 4 && m[3] == '.') stmt->mulWidth = std::stoi(m.substr(4));
                else stmt->mulWidth = 8;
                const auto& dst = advance();
                stmt->instr.operand = (dst.type == AssemblerTokenType::REGISTER ? "." : "") + dst.value;
                expect(AssemblerTokenType::COMMA, "Expected , after destination");
                stmt->exprTokenIndex = (int)pos;
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                std::vector<uint8_t> d;
                if (stmt->type == Statement::MUL) emitMulCode(d, stmt->mulWidth, stmt->instr.operand, stmt->exprTokenIndex, stmt->scopePrefix);
                else emitDivCode(d, stmt->mulWidth, stmt->instr.operand, stmt->exprTokenIndex, stmt->scopePrefix);
                stmt->size = d.size();
            }
            else if (stmt->instr.mnemonic == "ldax" || stmt->instr.mnemonic == "lday" || stmt->instr.mnemonic == "ldaz" || stmt->instr.mnemonic == "ldxy" ||
                     stmt->instr.mnemonic == "stax" || stmt->instr.mnemonic == "stay" || stmt->instr.mnemonic == "staz" || stmt->instr.mnemonic == "stxy") {
                std::string m = stmt->instr.mnemonic;
                std::string reg = "." + m.substr(2);
                bool isLoad = (m.substr(0, 2) == "ld");
                stmt->type = isLoad ? Statement::LDW : Statement::STW;
                stmt->emitFn = isLoad ? AssemblerSimulatedOps::dispatch_LDW : AssemblerSimulatedOps::dispatch_STW;
                stmt->instr.operand = reg;
                stmt->exprTokenIndex = (int)pos;
                while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                std::vector<uint8_t> d;
                if (isLoad) emitLDWCode(d, reg, stmt->exprTokenIndex, stmt->scopePrefix);
                else emitSTWCode(d, reg, stmt->exprTokenIndex, stmt->scopePrefix);
                stmt->size = d.size();
            }
            else if (stmt->isSimulatedOp()) {
                if (stmt->type == Statement::SXT8) {
                    // No operand
                } else {
                    stmt->instr.operandTokenIndex = (int)pos;
                    stmt->exprTokenIndex = (int)pos; // Initialize exprTokenIndex too
                    if (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) {
                        if (peek().type == AssemblerTokenType::REGISTER) {
                            stmt->instr.operand = "." + advance().value;
                        } else {
                            if (peek().type == AssemblerTokenType::HASH) {
                                std::string immStr = advance().value;
                                stmt->instr.operandTokenIndex = (int)pos - 1; // HASH
                                stmt->exprTokenIndex = (int)pos;
                                std::string valPart;
                                int tempPos = (int)pos;
                                while(tempPos < (int)tokens.size() && tokens[tempPos].type != AssemblerTokenType::NEWLINE && tokens[tempPos].type != AssemblerTokenType::END_OF_FILE && tokens[tempPos].type != AssemblerTokenType::COMMA) {
                                    valPart += tokens[tempPos].value;
                                    tempPos++;
                                }
                                stmt->instr.operand = immStr + valPart;
                                while(peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE && peek().type != AssemblerTokenType::COMMA) advance();
                            } else {
                                int idx = (int)pos;
                                auto ast = parseExprAST(tokens, idx, symbolTable, stmt->scopePrefix);
                                if (ast) {
                                    if (auto* v = dynamic_cast<VariableNode*>(ast.get())) stmt->instr.operand = v->name;
                                    else stmt->instr.operand = "EXPR";
                                }
                                pos = idx;
                            }
                        }
                        if (match(AssemblerTokenType::COMMA)) {
                            stmt->exprTokenIndex = (int)pos;
                            while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                        }
                    }
                }
                // Unified sizing via emitFn dispatch
                if (stmt->emitFn) {
                    std::vector<uint8_t> d;
                    M65Emitter sizer(d, getZPStart()); sizer.setSpBase(getSpBase()); sizer.setScratchZP(getScratchZP());
                    stmt->emitFn(this, sizer, stmt.get());
                    stmt->size = d.size();
                }
            }
            else {
                size_t dotPos = fullMnemonic.find('.');
                if (dotPos != std::string::npos) {
                    stmt->instr.mnemonic = fullMnemonic.substr(0, dotPos);
                    std::string suffix = fullMnemonic.substr(dotPos + 1);
                    stmt->instr.forceMode = true;
                    if (suffix == "imm") {
                        if (stmt->instr.mnemonic == "phw") stmt->instr.mode = AddressingMode::IMMEDIATE16;
                        else stmt->instr.mode = AddressingMode::IMMEDIATE;
                    }
                    else if (suffix == "zp") stmt->instr.mode = AddressingMode::BASE_PAGE;
                    else if (suffix == "abs") stmt->instr.mode = AddressingMode::ABSOLUTE;
                    else if (suffix == "ind") stmt->instr.mode = AddressingMode::INDIRECT;
                    else if (suffix == "xind") stmt->instr.mode = AddressingMode::BASE_PAGE_X_INDIRECT;
                    else if (suffix == "indy") stmt->instr.mode = AddressingMode::BASE_PAGE_INDIRECT_Y;
                    else if (suffix == "indz") stmt->instr.mode = AddressingMode::BASE_PAGE_INDIRECT_Z;
                    else if (suffix == "accum" || suffix == "a") stmt->instr.mode = AddressingMode::ACCUMULATOR;
                    else if (suffix == "s") stmt->instr.mode = AddressingMode::STACK_RELATIVE;
                    else { stmt->instr.mnemonic = fullMnemonic; stmt->instr.forceMode = false; }
                }
                if (stmt->instr.mnemonic == "nop") throw std::runtime_error("nop disallowed");
                if (stmt->instr.mnemonic == "proc") {
                    if (currentProc != nullptr) {
                        addError("Error: nested 'proc' not allowed (inside '" + currentProc->name + "')");
                    }
                    std::string pN = advance().value;
                    stmt->label = pN;
                    symbolTable[pN] = {pc, true, 2, false, false, pc, false, 0, false, 0, currentSegment->name};
                    auto ctx = std::make_shared<ProcContext>();
                    ctx->name = pN; ctx->totalParamSize = 0;
                    // args: {name, accessSize, stackSize}
                    struct ProcArg { std::string name; int accessSize; int stackSize; };
                    std::vector<ProcArg> args;
                    while (match(AssemblerTokenType::COMMA)) {
                        int pSize = 2; // default: word
                        if (peek().type == AssemblerTokenType::IDENTIFIER && (peek().value == "B" || peek().value == "W" || peek().value == "D")) {
                            std::string sizeSpec = advance().value;
                            if (sizeSpec == "B") pSize = 1;
                            else if (sizeSpec == "D") pSize = 4;
                            match(AssemblerTokenType::HASH);
                        }
                        // C calling convention promotes char (B#) to 16-bit on the stack.
                        // Stack size is always at least 2 bytes; access size stays 1 for B#.
                        int stackSize = (pSize < 2) ? 2 : pSize;
                        args.push_back({advance().value, pSize, stackSize});
                        ctx->totalParamSize += args.back().stackSize;
                    }
                    scopeStack.push_back(pN); stmt->scopePrefix = currentScopePrefix();
                    // Stack-relative offsets: past 2-byte return addr
                    int sOff = 2;
                    for (int i = (int)args.size() - 1; i >= 0; --i) {
                        std::string scA = stmt->scopePrefix + args[i].name;
                        std::string scAN = stmt->scopePrefix + "ARG" + std::to_string(i + 1);
                        ctx->localArgs[args[i].name] = sOff; ctx->localArgs["ARG" + std::to_string(i + 1)] = sOff;
                        Symbol pSym = {(uint32_t)sOff, false, args[i].accessSize, true, false, (uint32_t)sOff, true, sOff, false, 0, ""};
                        symbolTable[scA] = pSym;
                        symbolTable[scAN] = pSym;
                        sOff += args[i].stackSize;
                    }
                    procedures[pc] = ctx; pass1ProcStack.push_back(currentProc);
                    currentProc = ctx; stmt->procCtx = ctx; stmt->size = 0;
                }
                else if (stmt->instr.mnemonic == "endproc") {
                    if (currentProc) {
                        stmt->instr.procParamSize = currentProc->totalParamSize;
                        currentProc = pass1ProcStack.empty() ? nullptr : pass1ProcStack.back();
                        if (!pass1ProcStack.empty()) pass1ProcStack.pop_back();
                    } else {
                        addError("Error: 'endproc' outside of procedure scope");
                    }
                    if (!scopeStack.empty()) scopeStack.pop_back();
                    // RTS (1 byte) or RTS #n (2 bytes)
                    stmt->size = (stmt->instr.procParamSize == 0) ? 1 : 2;
                }
                else if (stmt->instr.mnemonic == "call") {
                    stmt->instr.operand = advance().value;
                    while (match(AssemblerTokenType::COMMA)) {
                        if (match(AssemblerTokenType::HASH)) stmt->instr.callArgs.push_back(std::string("#") + advance().value);
                        else if (peek().type == AssemblerTokenType::IDENTIFIER && (peek().value == "B" || peek().value == "W")) {
                            std::string p = advance().value; match(AssemblerTokenType::HASH);
                            stmt->instr.callArgs.push_back(p + "#" + advance().value);
                        }
                        else stmt->instr.callArgs.push_back(advance().value);
                    }
                    stmt->size = calculateInstructionSize(stmt->instr, pc, stmt->scopePrefix);
                }
                else if (stmt->instr.mnemonic == "zero") {
                    stmt->type = Statement::ZERO; stmt->emitFn = AssemblerSimulatedOps::dispatch_Zero; stmt->instr.operandTokenIndex = (int)pos;
                    while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
                    std::vector<uint8_t> d; emitZeroCode(d, stmt->instr.operandTokenIndex, stmt->scopePrefix);
                    stmt->size = d.size();
                }
                else {
                    if (match(AssemblerTokenType::HASH)) {
                        stmt->instr.operandTokenIndex = (int)pos;
                        std::string o; while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE && peek().type != AssemblerTokenType::COMMA) o += advance().value;
                        stmt->instr.operand = o;
                        if (!stmt->instr.forceMode) {
                            if (stmt->instr.mnemonic == "phw") stmt->instr.mode = AddressingMode::IMMEDIATE16;
                            else stmt->instr.mode = AddressingMode::IMMEDIATE;
                        }
                    }
                    else if (match(AssemblerTokenType::OPEN_PAREN)) {
                        stmt->instr.operandTokenIndex = (int)pos;
                        std::string o; while (peek().type != AssemblerTokenType::CLOSE_PAREN && peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE && peek().type != AssemblerTokenType::COMMA) o += advance().value;
                        stmt->instr.operand = o;
                        if (match(AssemblerTokenType::COMMA)) {
                            std::string r = advance().value;
                            if (r == "X" || r == "x") {
                                expect(AssemblerTokenType::CLOSE_PAREN, "Expected )");
                                if (!stmt->instr.forceMode) {
                                    try {
                                        uint32_t val = parseNumericLiteral(stmt->instr.operand);
                                        if (val > 0xFF || stmt->instr.mnemonic == "jsr" || stmt->instr.mnemonic == "jmp" || stmt->instr.mnemonic == "phw") stmt->instr.mode = AddressingMode::ABSOLUTE_X_INDIRECT;
                                        else stmt->instr.mode = AddressingMode::BASE_PAGE_X_INDIRECT;
                                    } catch(...) { stmt->instr.mode = AddressingMode::ABSOLUTE_X_INDIRECT; }
                                }
                            } else if (r == "SP" || r == "sp") {
                                expect(AssemblerTokenType::CLOSE_PAREN, "Expected )"); expect(AssemblerTokenType::COMMA, "Expected ,"); advance();
                                if (!stmt->instr.forceMode) stmt->instr.mode = AddressingMode::BASE_PAGE_INDIRECT_SP_Y;
                            }
                        } else {
                            expect(AssemblerTokenType::CLOSE_PAREN, "Expected )");
                            if (match(AssemblerTokenType::COMMA)) {
                                std::string r = advance().value;
                                if (!stmt->instr.forceMode) {
                                    if (r == "Y" || r == "y") stmt->instr.mode = AddressingMode::BASE_PAGE_INDIRECT_Y;
                                    else if (r == "Z" || r == "z") stmt->instr.mode = AddressingMode::BASE_PAGE_INDIRECT_Z;
                                }
                            } else {
                                if (!stmt->instr.forceMode) {
                                    try {
                                        uint32_t val = parseNumericLiteral(stmt->instr.operand);
                                        if (val > 0xFF || stmt->instr.mnemonic == "jsr" || stmt->instr.mnemonic == "jmp" || stmt->instr.mnemonic == "phw") stmt->instr.mode = AddressingMode::ABSOLUTE_INDIRECT;
                                        else stmt->instr.mode = AddressingMode::INDIRECT;
                                    } catch(...) { stmt->instr.mode = AddressingMode::ABSOLUTE_INDIRECT; }
                                }
                            }
                        }
                    }
                    else if (match(AssemblerTokenType::OPEN_BRACKET)) {
                        stmt->instr.operandTokenIndex = (int)pos;
                        std::string o; while (peek().type != AssemblerTokenType::CLOSE_BRACKET && peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE && peek().type != AssemblerTokenType::COMMA) o += advance().value;
                        stmt->instr.operand = o; expect(AssemblerTokenType::CLOSE_BRACKET, "Expected ]");
                        if (match(AssemblerTokenType::COMMA)) {
                            advance(); if (!stmt->instr.forceMode) stmt->instr.mode = AddressingMode::FLAT_INDIRECT_Z;
                        }
                    }
                    else if ((peek().type == AssemblerTokenType::REGISTER || peek().type == AssemblerTokenType::IDENTIFIER) && (peek().value == "A" || peek().value == "a") && (pos + 1 >= tokens.size() || tokens[pos+1].type == AssemblerTokenType::NEWLINE || tokens[pos+1].type == AssemblerTokenType::END_OF_FILE)) {
                        advance(); if (!stmt->instr.forceMode) stmt->instr.mode = AddressingMode::ACCUMULATOR;
                    }
                    else {
                        stmt->instr.operandTokenIndex = (int)pos;
                        std::string o; while (peek().type != AssemblerTokenType::COMMA && peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) o += advance().value;
                        stmt->instr.operand = o;
                        if (peek().type == AssemblerTokenType::COMMA) {
                            advance(); std::string r = advance().value;
                            if (!stmt->instr.forceMode) {
                                if (r == "X" || r == "x") stmt->instr.mode = AddressingMode::ABSOLUTE_X;
                                else if (r == "Y" || r == "y") stmt->instr.mode = AddressingMode::ABSOLUTE_Y;
                                else if (r == "S" || r == "s" || r == "SP" || r == "sp") stmt->instr.mode = AddressingMode::STACK_RELATIVE;
                                else { stmt->instr.bitBranchTarget = r; stmt->instr.mode = AddressingMode::BASE_PAGE_RELATIVE; }
                            }
                        } else if (!stmt->instr.forceMode && !o.empty()) {
                            try {
                                uint32_t val = parseNumericLiteral(o);
                                if (val > 0xFF || stmt->instr.mnemonic == "jsr" || stmt->instr.mnemonic == "jmp" || stmt->instr.mnemonic == "phw") stmt->instr.mode = AddressingMode::ABSOLUTE;
                                else stmt->instr.mode = AddressingMode::BASE_PAGE;
                            } catch(...) { stmt->instr.mode = AddressingMode::ABSOLUTE; }
                        }
                    }
                    if ((stmt->instr.mnemonic == "inw" || stmt->instr.mnemonic == "dew") && stmt->instr.mode == AddressingMode::STACK_RELATIVE) {
                        stmt->type = (stmt->instr.mnemonic == "inw") ? Statement::STACK_INC : Statement::STACK_DEC;
                        stmt->emitFn = AssemblerSimulatedOps::dispatch_StackIncDec;
                        stmt->size = (stmt->type == Statement::STACK_INC) ? 9 : 12;
                    } else if ((stmt->instr.mnemonic == "inc" || stmt->instr.mnemonic == "dec") && stmt->instr.mode == AddressingMode::STACK_RELATIVE) {
                        stmt->type = (stmt->instr.mnemonic == "inc") ? Statement::STACK_INC8 : Statement::STACK_DEC8;
                        stmt->emitFn = AssemblerSimulatedOps::dispatch_StackIncDec8;
                        stmt->size = 5;
                    } else if (stmt->instr.mode == AddressingMode::STACK_RELATIVE) {
                        if (stmt->instr.mnemonic == "lda") { stmt->type = Statement::LDA_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_LDA_Stack; }
                        else if (stmt->instr.mnemonic == "sta") { stmt->type = Statement::STA_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_STA_Stack; }
                        else if (stmt->instr.mnemonic == "ldx") { stmt->type = Statement::LDX_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_LDX_Stack; }
                        else if (stmt->instr.mnemonic == "ldy") { stmt->type = Statement::LDY_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_LDY_Stack; }
                        else if (stmt->instr.mnemonic == "ldz") { stmt->type = Statement::LDZ_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_LDZ_Stack; }
                        else if (stmt->instr.mnemonic == "stx") { stmt->type = Statement::STX_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_STX_Stack; }
                        else if (stmt->instr.mnemonic == "sty") { stmt->type = Statement::STY_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_STY_Stack; }
                        else if (stmt->instr.mnemonic == "stz") { stmt->type = Statement::STZ_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_STZ_Stack; }
                    }

                    if (stmt->type != Statement::INSTRUCTION) {
                        // Already handled
                    } else if (stmt->instr.mnemonic == "phw" && stmt->instr.mode == AddressingMode::STACK_RELATIVE) {
                        stmt->type = Statement::PHW_STACK; stmt->emitFn = AssemblerSimulatedOps::dispatch_PHWStack;
                        std::vector<uint8_t> d; emitPHWStackCode(d, stmt->instr.operandTokenIndex, stmt->scopePrefix);
                        stmt->size = d.size();
                    } else {
                        stmt->size = calculateInstructionSize(stmt->instr, pc, stmt->scopePrefix);
                    }
                }
            }
          } catch (const std::exception& ex) {
            int errLine = stmt->line;
            errors.push_back("line " + std::to_string(errLine) + ": Error parsing instruction '" + stmt->instr.mnemonic + "': " + ex.what());
            while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
            continue;
          }
        }
        else if (stmt->label.empty()) {
            std::string badToken = peek().value;
            int badLine = peek().line;
            errors.push_back("line " + std::to_string(badLine) + ": Unknown instruction '" + badToken + "'");
            while (peek().type != AssemblerTokenType::NEWLINE && peek().type != AssemblerTokenType::END_OF_FILE) advance();
            continue;
        }
        pc += stmt->size;
        statements.push_back(std::move(stmt));
    }

    if (currentProc) {
        addError("Error: unclosed 'proc " + currentProc->name + "' at end of file");
    }
}


void AssemblerParser::emitASWCode(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitASWCode(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitROWCode(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitROWCode(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLSL16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLSL16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLSR16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLSR16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitROL16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitROL16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitROR16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitROR16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitASR16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitASR16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSXT8Code(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSXT8Code(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitSXT16Code(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSXT16Code(this, e, tokenIndex, scopePrefix);
}

void AssemblerParser::emitAddSub32Code(std::vector<uint8_t>& binary, bool isAdd, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitAddSub32Code(this, e, isAdd, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitBitwise32Code(std::vector<uint8_t>& binary, const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitBitwise32Code(this, e, mnemonic, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitCMP32Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitCMP32Code(this, e, src1, tokenIndex, scopePrefix);
}

void AssemblerParser::emitCMP_S32Code(std::vector<uint8_t>& binary, const std::string& src1, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitCMP_S32Code(this, e, src1, tokenIndex, scopePrefix);
}

void AssemblerParser::emitNegNot32Code(std::vector<uint8_t>& binary, bool isNeg, const std::string& operand, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitNegNot32Code(this, e, isNeg, operand, tokenIndex, scopePrefix);
}

void AssemblerParser::emitABS32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitABS32Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLSL32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLSL32Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLSR32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLSR32Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitROL32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitROL32Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitROR32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitROR32Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitASR32Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitASR32Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitPushPopCode(std::vector<uint8_t>& binary, bool isPush, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary); e.setSpBase(getSpBase());
    std::string reg = tokens[tokenIndex].value;
    if (tokens[tokenIndex].type == AssemblerTokenType::REGISTER) reg = "." + reg;
    AssemblerSimulatedOps::emitPushPopCode(this, e, isPush, reg, tokenIndex, scopePrefix);
}

void AssemblerParser::emitMulS16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitMulS16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitDivS16Code(std::vector<uint8_t>& binary, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitDivS16Code(this, e, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitMod16Code(std::vector<uint8_t>& binary, bool isSigned, const std::string& dest, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitMod16Code(this, e, isSigned, dest, tokenIndex, scopePrefix);
}

void AssemblerParser::emitLDA_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDA_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitSTA_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTA_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitLDAX_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDAX_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitSTAX_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTAX_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitLDAXYZ_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLDAXYZ_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitSTAXYZ_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitSTAXYZ_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitLEAX_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitLEAX_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitMOVE_FPCode(std::vector<uint8_t>& binary, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase());
    AssemblerSimulatedOps::emitMOVE_FPCode(this, e, tokenIndex, scopePrefix);
}
void AssemblerParser::emitBFExtCode(std::vector<uint8_t>& binary, bool is16, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase()); e.setScratchZP(getScratchZP());
    AssemblerSimulatedOps::emitBFExtCode(this, e, is16, tokenIndex, scopePrefix);
}
void AssemblerParser::emitBFInsCode(std::vector<uint8_t>& binary, bool is16, int mode, int tokenIndex, const std::string& scopePrefix) {
    M65Emitter e(binary, getZPStart()); e.setSpBase(getSpBase()); e.setScratchZP(getScratchZP());
    AssemblerSimulatedOps::emitBFInsCode(this, e, is16, mode, tokenIndex, scopePrefix);
}

int AssemblerParser::calculateInstructionSize(const Instruction& instr, uint32_t currentAddr, const std::string& scopePrefix) {
    if (instr.mnemonic == "proc") return 0;
    if (instr.mnemonic == "endproc") return (instr.procParamSize == 0) ? 1 : 2;
    if (instr.mnemonic == "push" || instr.mnemonic == "pop") {
        return AssemblerSimulatedOps::getPushPopSize(this, instr.mnemonic == "push", instr.operand, instr.operandTokenIndex, scopePrefix);
    }

    AddressingMode resolvedMode = instr.mode;
    
    // Auto-promote BASE_PAGE to ABSOLUTE if it doesn't fit in 8 bits.
    // Or promote identifiers if they don't have a fixed mode.
    if (!instr.forceMode && instr.operandTokenIndex != -1 && (instr.mode == AddressingMode::BASE_PAGE || instr.mode == AddressingMode::ABSOLUTE || 
        instr.mode == AddressingMode::BASE_PAGE_X || instr.mode == AddressingMode::ABSOLUTE_X ||
        instr.mode == AddressingMode::BASE_PAGE_Y || instr.mode == AddressingMode::ABSOLUTE_Y)) {
        
        try {
            uint32_t val = evaluateExpressionAt(instr.operandTokenIndex, scopePrefix);
            bool fitsIn8 = (val <= 0xFF);
            bool forceAbs = (instr.mnemonic == "jsr" || instr.mnemonic == "jmp");

            // If the operand's primary symbol is in a relocatable segment (BSS, extern),
            // its address is not yet known — force absolute addressing to be safe.
            if (fitsIn8 && !forceAbs && instr.operandTokenIndex < (int)tokens.size() &&
                tokens[instr.operandTokenIndex].type == AssemblerTokenType::IDENTIFIER) {
                std::string symName = scopePrefix + tokens[instr.operandTokenIndex].value;
                // Try scoped name first, then bare name
                if (!symbolTable.count(symName)) symName = tokens[instr.operandTokenIndex].value;
                if (isRelocatableSymbol(symName)) forceAbs = true;
            }

            if (instr.mode == AddressingMode::BASE_PAGE || instr.mode == AddressingMode::ABSOLUTE) {
                bool supportsBP = AssemblerOpcodeDatabase::isValidMode(instr.mnemonic, AddressingMode::BASE_PAGE);
                resolvedMode = (fitsIn8 && !forceAbs && supportsBP) ? AddressingMode::BASE_PAGE : AddressingMode::ABSOLUTE;
            } else if (instr.mode == AddressingMode::BASE_PAGE_X || instr.mode == AddressingMode::ABSOLUTE_X) {
                bool supportsBPX = AssemblerOpcodeDatabase::isValidMode(instr.mnemonic, AddressingMode::BASE_PAGE_X);
                resolvedMode = (fitsIn8 && !forceAbs && supportsBPX) ? AddressingMode::BASE_PAGE_X : AddressingMode::ABSOLUTE_X;
            } else if (instr.mode == AddressingMode::BASE_PAGE_Y || instr.mode == AddressingMode::ABSOLUTE_Y) {
                bool supportsBPY = AssemblerOpcodeDatabase::isValidMode(instr.mnemonic, AddressingMode::BASE_PAGE_Y);
                resolvedMode = (fitsIn8 && !forceAbs && supportsBPY) ? AddressingMode::BASE_PAGE_Y : AddressingMode::ABSOLUTE_Y;
            }
        } catch(...) {
            // If we can't evaluate yet, assume ABSOLUTE to be safe
            if (instr.mode == AddressingMode::BASE_PAGE) resolvedMode = AddressingMode::ABSOLUTE;
            else if (instr.mode == AddressingMode::BASE_PAGE_X) resolvedMode = AddressingMode::ABSOLUTE_X;
            else if (instr.mode == AddressingMode::BASE_PAGE_Y) resolvedMode = AddressingMode::ABSOLUTE_Y;
        }
    }

    int size = 1;
    bool isQuad = (instr.mnemonic.size() > 1 && instr.mnemonic.back() == 'q' && instr.mnemonic != "beq" && instr.mnemonic != "bne" && instr.mnemonic != "bra" && instr.mnemonic != "bsr");

    if (isQuad) size += 2;
    if (resolvedMode == AddressingMode::FLAT_INDIRECT_Z) size += 1;

    if (instr.mnemonic == "beq" || instr.mnemonic == "bne" || instr.mnemonic == "bra" || instr.mnemonic == "bcc" || instr.mnemonic == "bcs" || instr.mnemonic == "bpl" || instr.mnemonic == "bmi" || instr.mnemonic == "bvc" || instr.mnemonic == "bvs") {
        try {
            uint32_t target = evaluateExpressionAt(instr.operandTokenIndex, scopePrefix);
            int32_t diff = (int32_t)target - (int32_t)(currentAddr + 2);
            if (diff >= -128 && diff <= 127) return 2;
        } catch (...) {}
        return 3;
    }

    if (instr.mnemonic == "bsr") return 3;

    if (instr.mnemonic == "call") {
        int s = 0;
        for (const auto& arg : instr.callArgs) {
            bool isB = (arg.size() >= 2 && arg.substr(0, 2) == "B#");
            if (isB) s += 2;
            else {
                Symbol* sym = resolveSymbol(arg[0] == '#' ? arg.substr(1) : arg, scopePrefix);
                if (sym && sym->size == 1) s += 2;
                else s += 3;
            }
        }
        return s + 3;
    }

    if (instr.mnemonic == "rtn") {
        // rtn #0 optimizes to just RTS (1 byte); rtn #N is RTS + immediate (2 bytes)
        if (!instr.operand.empty()) {
            Symbol* sym = resolveSymbol(instr.operand, scopePrefix);
            uint32_t v = sym ? sym->value : 0;
            if (!sym) { try { v = std::stoul(instr.operand); } catch(...) { v = 0; } }
            if (v == 0) return 1;
        }
        return 2;
    }

    switch (resolvedMode) {
        case AddressingMode::IMPLIED: case AddressingMode::ACCUMULATOR: return size;
        case AddressingMode::IMMEDIATE: case AddressingMode::BASE_PAGE: case AddressingMode::BASE_PAGE_X: case AddressingMode::BASE_PAGE_Y: case AddressingMode::INDIRECT: case AddressingMode::BASE_PAGE_X_INDIRECT: case AddressingMode::BASE_PAGE_INDIRECT_Y: case AddressingMode::BASE_PAGE_INDIRECT_Z: case AddressingMode::BASE_PAGE_INDIRECT_SP_Y: case AddressingMode::STACK_RELATIVE: return size + 1;
        case AddressingMode::ABSOLUTE: case AddressingMode::ABSOLUTE_X: case AddressingMode::ABSOLUTE_Y: case AddressingMode::ABSOLUTE_INDIRECT: case AddressingMode::ABSOLUTE_X_INDIRECT: case AddressingMode::IMMEDIATE16: return size + 2;
        case AddressingMode::BASE_PAGE_RELATIVE: return size + 2;
        case AddressingMode::FLAT_INDIRECT_Z: return size + 1;
        default: return size + 2;
    }
}

int AssemblerParser::calculateDirectiveSize(const Directive& dir, uint32_t currentAddr) {
    if (dir.name == "byte") return (int)dir.arguments.size();
    if (dir.name == "word") return (int)dir.arguments.size() * 2;
    if (dir.name == "dword" || dir.name == "long") return (int)dir.arguments.size() * 4;
    if (dir.name == "float") return (int)dir.arguments.size() * 5;
    if (dir.name == "text" || dir.name == "ascii") {
        if (dir.arguments.empty()) return 0;
        return (int)dir.arguments[0].size();
    }
    if (dir.name == "res") {
        if (dir.arguments.empty()) return 0;
        return (int)evaluateExpressionAt(dir.tokenIndex, "");
    }
    if (dir.name == "array") {
        if (dir.arguments.empty()) return 0;
        return (int)parseNumericLiteral(dir.arguments[0]);
    }
    if (dir.name == "align" || dir.name == "balign") {
        if (dir.arguments.empty()) return 0;
        uint32_t align = parseNumericLiteral(dir.arguments[0]);
        if (align == 0) return 0;
        return (align - (currentAddr % align)) % align;
    }
    return 0;
}

std::vector<uint8_t> AssemblerParser::pass2(bool isPrg) {
    isPass1_ = false;
    bool overallChanged;
    int pass2Iterations = 0;
    const int maxPass2Iterations = 100;
    do {
        if (++pass2Iterations > maxPass2Iterations) {
            errors.push_back("Error: pass2 failed to converge after " + std::to_string(maxPass2Iterations) + " iterations (possible duplicate labels or size oscillation)");
            break;
        }
        overallChanged = false;
        lineMap_.clear();  // rebuild each iteration; final iteration's map is kept
        bool optimizerMadeChanges = optimize();
        if (optimizerMadeChanges) overallChanged = true;
        std::deque<std::unique_ptr<Statement>> newStatements;
        for (auto& s : statements) {
            if (!s->deleted) newStatements.push_back(std::move(s));
            else overallChanged = true;
        }
        statements = std::move(newStatements);
        // Reset variables to initial values before each pass2 iteration
        for (auto& [name, symbol] : symbolTable) if (symbol.isVariable) symbol.value = symbol.initialValue;
        moveDmaFirstCopyAddr_ = 0xFFFFFFFF;
        moveDmaListAddr_      = 0xFFFFFFFF;
        fillDmaFirstFillAddr_ = 0xFFFFFFFF;
        fillDmaListAddr_      = 0xFFFFFFFF;

        bool addressRecalculationMadeChanges = false;
        std::map<std::string, uint32_t> pass2PCs;
        for (auto const& [name, seg] : segments) {
            pass2PCs[name] = 0xFFFFFFFF; // sentinel = never visited
            seg->startAddress = 0xFFFFFFFF; // reset for recalculation
        }
        std::string activeSegment = "default"; uint32_t cP = 0; bool isDeadCode = false;
        currentLocalScope_ = "";
        pass2PCs["default"] = 0; // default segment starts at 0
        for (auto& s : statements) {
            // ... (rest of loop)
            if (s->type == Statement::DIRECTIVE) {
                if (s->dir.name == "var" || s->dir.name == "local") {
                    if (s->dir.varType == Directive::ASSIGN) {
                        uint32_t oldVal = symbolTable[s->dir.varName].value;
                        uint32_t newVal = evaluateExpressionAt(s->dir.tokenIndex, s->scopePrefix);
                        if (oldVal != newVal) {
                            symbolTable[s->dir.varName].value = newVal;
                            addressRecalculationMadeChanges = true;
                        }
                    }
                    else if (s->dir.varType == Directive::INC) { symbolTable[s->dir.varName].value++; addressRecalculationMadeChanges = true; }
                    else if (s->dir.varType == Directive::DEC) { symbolTable[s->dir.varName].value--; addressRecalculationMadeChanges = true; }
                }
            }
            if (s->type == Statement::DIRECTIVE && (s->dir.name == "segment" || s->dir.name == "code" || s->dir.name == "data" || s->dir.name == "bss")) {
                pass2PCs[activeSegment] = cP; activeSegment = s->segmentName;
                uint32_t savedPC = pass2PCs[activeSegment];
                if (savedPC != 0xFFFFFFFF) cP = savedPC; // restore saved PC for revisited segments
                // else: first visit to this segment — carry cP forward (no .org means it follows previous segment)
            }
            if (s->type == Statement::DIRECTIVE && s->dir.name == "org") {
                if (!s->dir.arguments.empty()) {
                    cP = parseNumericLiteral(s->dir.arguments[0]);
                    if (segments[activeSegment]->startAddress == 0xFFFFFFFF) segments[activeSegment]->startAddress = cP;
                }
            }
            if (segments[activeSegment]->startAddress == 0xFFFFFFFF && (s->size > 0 || s->type == Statement::INSTRUCTION)) segments[activeSegment]->startAddress = cP;
            pc = cP; s->address = cP;
            // Restore auto-scope for @ label resolution from per-statement data
            currentLocalScope_ = s->localLabelScope;
            if (!s->label.empty()) {
                isDeadCode = false;
                if (symbolTable[s->label].value != cP) { symbolTable[s->label].value = cP; addressRecalculationMadeChanges = true; }
            }
            if (s->type == Statement::DIRECTIVE && s->dir.name == "org") {
                if (!s->dir.arguments.empty()) cP = parseNumericLiteral(s->dir.arguments[0]);
                s->address = cP;
            }
            int oS = s->size;
            if (s->type == Statement::INSTRUCTION && s->instr.mnemonic == "proc") s->size = 0;
            else if (s->type == Statement::INSTRUCTION && s->instr.mnemonic == "endproc") {
                if (isDeadCode) s->size = 0; else s->size = (s->instr.procParamSize == 0) ? 1 : 2;
                isDeadCode = false;
            } else if (isDeadCode && s->type != Statement::DIRECTIVE && s->type != Statement::BASIC_UPSTART) s->size = 0;
            else {
                if (s->type == Statement::INSTRUCTION) s->size = calculateInstructionSize(s->instr, cP, s->scopePrefix);
                else if (s->isSimulatedOp()) {
                    // DMA buffer allocation for FILL/COPY (must happen before sizing)
                    if (s->type == Statement::FILL && fillDmaFirstFillAddr_ == 0xFFFFFFFF) {
                        fillDmaFirstFillAddr_ = cP;
                        if (segments.find("bss") == segments.end()) segments["bss"] = std::make_shared<Segment>();
                        if (segments["bss"]->startAddress == 0xFFFFFFFF) segments["bss"]->startAddress = 0x1000;
                        uint32_t bssCurrentSize = 0;
                        for (const auto& stmt : statements) if (stmt && stmt->segmentName == "bss" && !stmt->deleted) bssCurrentSize += stmt->size;
                        fillDmaListAddr_ = segments["bss"]->startAddress + bssCurrentSize;
                        symbolTable["__fill_dma_buf"] = {fillDmaListAddr_, true, 2, false, false, fillDmaListAddr_, false, 0, false, 0, "bss"};
                    }
                    if (s->type == Statement::COPY && moveDmaFirstCopyAddr_ == 0xFFFFFFFF) {
                        moveDmaFirstCopyAddr_ = cP;
                        if (segments.find("bss") == segments.end()) segments["bss"] = std::make_shared<Segment>();
                        if (segments["bss"]->startAddress == 0xFFFFFFFF) segments["bss"]->startAddress = 0x1000;
                        uint32_t bssCurrentSize = 0;
                        for (const auto& stmt : statements) if (stmt && stmt->segmentName == "bss" && !stmt->deleted) bssCurrentSize += stmt->size;
                        moveDmaListAddr_ = segments["bss"]->startAddress + bssCurrentSize;
                        symbolTable["__move_dma_buf"] = {moveDmaListAddr_, true, 2, false, false, moveDmaListAddr_, false, 0, false, 0, "bss"};
                    }
                    // Unified sizing via emitFn dispatch
                    std::vector<uint8_t> d;
                    M65Emitter sizer(d, getZPStart()); sizer.setSpBase(getSpBase()); sizer.setScratchZP(getScratchZP());
                    s->emitFn(this, sizer, s.get());
                    s->size = (int)d.size();
                }
                else if (s->type == Statement::DIRECTIVE) {
                    if (s->dir.name == "var" || s->dir.name == "local") {
                        if (s->dir.varType == Directive::ASSIGN) {
                            uint32_t oldVal = symbolTable[s->dir.varName].value;
                            uint32_t newVal = evaluateExpressionAt(s->dir.tokenIndex, s->scopePrefix);
                            if (oldVal != newVal) {
                                symbolTable[s->dir.varName].value = newVal;
                                addressRecalculationMadeChanges = true;
                            }
                        }
                        else if (s->dir.varType == Directive::INC) { symbolTable[s->dir.varName].value++; addressRecalculationMadeChanges = true; }
                        else if (s->dir.varType == Directive::DEC) { symbolTable[s->dir.varName].value--; addressRecalculationMadeChanges = true; }
                    }
                    s->size = calculateDirectiveSize(s->dir, cP);
                }
                else if (s->type == Statement::BASIC_UPSTART) s->size = 12;
            }
            if (s->size != oS) addressRecalculationMadeChanges = true;
            // Build source-level line map (only on last iteration)
            if (!s->sourceFile.empty() && s->sourceLine > 0 && s->size > 0 && !s->deleted) {
                if (lineMap_.empty() || lineMap_.back().address != s->address
                    || lineMap_.back().line != s->sourceLine
                    || lineMap_.back().file != s->sourceFile) {
                    lineMap_.push_back({s->address, s->sourceFile, s->sourceLine});
                }
            }
            cP += s->size;
            if (s->type == Statement::INSTRUCTION) {
                if (s->instr.mnemonic == "rts" || s->instr.mnemonic == "rtn" || s->instr.mnemonic == "rti") isDeadCode = true;
            }
        }
        // Update segment end addresses from this pass
        pass2PCs[activeSegment] = cP;
        for (auto& [name, seg] : segments) {
            if (pass2PCs.count(name) && pass2PCs[name] != 0xFFFFFFFF) {
                seg->pc = pass2PCs[name];
            }
        }

        if (addressRecalculationMadeChanges) overallChanged = true;
    } while (overallChanged);
    return AssemblerGenerator::generate(this, isPrg);
}

uint32_t AssemblerParser::getExternIndex(const std::string& name) const {
    auto it = externIndex.find(name);
    if (it != externIndex.end()) return it->second;
    return (uint32_t)-1;
}

std::vector<AssemblerParser::SegmentView> AssemblerParser::getSegmentViews() const {
    std::vector<SegmentView> views;
    for (const auto& seg : segmentOrder) {
        SegmentView sv;
        sv.name = seg->name;
        sv.startAddress = seg->startAddress;
        sv.endAddress = seg->pc;
        sv.size = (seg->startAddress != 0xFFFFFFFF) ? (seg->pc - seg->startAddress) : 0;
        views.push_back(sv);
    }
    return views;
}
