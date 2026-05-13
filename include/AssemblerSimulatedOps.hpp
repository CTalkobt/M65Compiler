#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "AssemblerTypes.hpp"
#include "AssemblerParser.hpp"

class M65Emitter;

class AssemblerSimulatedOps {
public:
    // Unified dispatch wrappers — each adapts the (parser, emitter, stmt) signature
    // to the existing emitXxxCode functions. Used as Statement::emitFn values.
    using Stmt = AssemblerParser::Statement;
    static void dispatch_Expr(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Mul(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Div(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_StackIncDec(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_StackIncDec8(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_AddSub16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Bitwise16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_CMP16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_CMP_S16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LDW(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_STW(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Fill(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Copy(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Swap(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_NegNot16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_ABS16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_ChkZero(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Branch16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Select(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_PtrStack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_PtrDeref(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_FlatMemory(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_PHWStack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_ASW(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_ROW(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Shift16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_SXT8(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_SXT16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_AddSub32(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Bitwise32(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_CMP32(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_CMP_S32(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_NegNot32(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_ABS32(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Shift32(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_PushPop(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LDA_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_STA_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LDX_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LDY_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LDZ_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_STX_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_STY_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_STZ_Stack(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Zero(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LDA_FP(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_STA_FP(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LDAX_FP(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_STAX_FP(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_LEAX_FP(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_MOVE_FP(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_BFExt(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_BFIns(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_MulS16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_DivS16(AssemblerParser* p, M65Emitter& e, Stmt* s);
    static void dispatch_Mod16(AssemblerParser* p, M65Emitter& e, Stmt* s);

    // Original emit functions (implementations unchanged)
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
    static void emitSXT16Code(AssemblerParser* parser, M65Emitter& e, int tokenIndex, const std::string& scopePrefix);

    // 32-bit simulated ops
    static void emitAddSub32Code(AssemblerParser* parser, M65Emitter& e, bool isAdd, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitBitwise32Code(AssemblerParser* parser, M65Emitter& e, const std::string& mnemonic, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitCMP32Code(AssemblerParser* parser, M65Emitter& e, const std::string& src1, int tokenIndex, const std::string& scopePrefix);
    static void emitCMP_S32Code(AssemblerParser* parser, M65Emitter& e, const std::string& src1, int tokenIndex, const std::string& scopePrefix);
    static void emitNegNot32Code(AssemblerParser* parser, M65Emitter& e, bool isNeg, const std::string& operand, int tokenIndex, const std::string& scopePrefix);
    static void emitABS32Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitLSL32Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitLSR32Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitROL32Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitROR32Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitASR32Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitPushPopCode(AssemblerParser* parser, M65Emitter& e, bool isPush, const std::string& reg, int tokenIndex, const std::string& scopePrefix);
    static int getPushPopSize(AssemblerParser* parser, bool isPush, const std::string& reg, int tokenIndex, const std::string& scopePrefix);
    static void emitMulS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitDivS16Code(AssemblerParser* parser, M65Emitter& e, const std::string& dest, int tokenIndex, const std::string& scopePrefix);
    static void emitMod16Code(AssemblerParser* parser, M65Emitter& e, bool isSigned, const std::string& dest, int tokenIndex, const std::string& scopePrefix);

    // Bitfield pseudo-ops
    static void emitBFExtCode(AssemblerParser* parser, M65Emitter& e, bool is16, int tokenIndex, const std::string& scopePrefix);
    static void emitBFInsCode(AssemblerParser* parser, M65Emitter& e, bool is16, int mode, int tokenIndex, const std::string& scopePrefix);
    // mode: 0=absolute, 1=stack-relative, 2=indirect

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
