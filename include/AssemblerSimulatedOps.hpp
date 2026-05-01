#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "AssemblerTypes.hpp"

class AssemblerParser;
class M65Emitter;

class AssemblerSimulatedOps {
public:
    static void emitExpressionCode(AssemblerParser* parser, M65Emitter& e, const std::string& target, int tokenIndex, const std::string& scopePrefix);
    static void emitMulCode(AssemblerParser* parser, M65Emitter& e, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitDivCode(AssemblerParser* parser, M65Emitter& e, int width, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitStackIncDecCode(AssemblerParser* parser, M65Emitter& e, bool isInc, int tokenIndex, const std::string& scopePrefix);
    static void emitStackIncDec8Code(AssemblerParser* parser, M65Emitter& e, bool isInc, int tokenIndex, const std::string& scopePrefix);
    static void emitAddSub16Code(AssemblerParser* parser, M65Emitter& e, bool isAdd, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitBitwise16Code(AssemblerParser* parser, M65Emitter& e, const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitCMP16Code(AssemblerParser* parser, M65Emitter& e, const std::string& src1, int tokenIndex, const std::string& scopePrefix);
    static void emitCMP_S16Code(AssemblerParser* parser, M65Emitter& e, const std::string& src1, int tokenIndex, const std::string& scopePrefix);
    static void emitLDWCode(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix, bool forceStack = false);
    static void emitSTWCode(AssemblerParser* parser, M65Emitter& e, const std::string& src, int tokenIndex, const std::string& scopePrefix, bool forceStack = false);
    static void emitLDA_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSTA_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitLDX_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitLDY_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitLDZ_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSTX_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSTY_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSTZ_StackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSwapCode(AssemblerParser* parser, M65Emitter& e, const std::string& r1, int tokenIndex, const std::string& scopePrefix);
    static void emitZeroCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitNegNot16Code(AssemblerParser* parser, M65Emitter& e, bool isNeg, const std::string& operand, int tokenIndex, const std::string& scopePrefix);
    static void emitABS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitChkZeroCode(AssemblerParser* parser, M65Emitter& e, bool is16, bool isInverse, int tokenIndex, const std::string& scopePrefix);
    static void emitBranch16Code(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSelectCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitPtrStackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitPtrDerefCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitFillCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix, bool forceStack = false);
    static void emitMoveCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix, bool forceStack = false);
    static void emitFlatMemoryCode(AssemblerParser* parser, M65Emitter& e, const std::string& mnemonic, int tokenIndex, const std::string& scopePrefix);
    static void emitPHWStackCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitASWCode(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitROWCode(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitLSL16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitLSR16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitROL16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitROR16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitASR16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitSXT8Code(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitPushPopCode(AssemblerParser* parser, M65Emitter& e, bool isPush, const std::string& reg, int tokenIndex, const std::string& scopePrefix);
    static int getPushPopSize(AssemblerParser* parser, bool isPush, const std::string& reg, int tokenIndex, const std::string& scopePrefix);
    static void emitMulS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitDivS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitMod16Code(AssemblerParser* parser, M65Emitter& e, bool isSigned, const std::string& dest, int tokenIndex, const std::string& scopePrefix);

    // Frame-pointer pseudo-ops
    static void emitLDA_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSTA_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitLDAX_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitSTAX_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitLEAX_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
    static void emitMOVE_FPCode(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);
private:
    static uint32_t resolveAbsAddr(AssemblerParser* parser, int tokenIndex, const std::string& scopePrefix);
    static void emitSignedMathOp(AssemblerParser* parser, M65Emitter& e, int op, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
};
