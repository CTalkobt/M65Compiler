#pragma once

#include "AST.hpp"
#include <functional>
#include <map>
#include <set>
#include <string>

// Loop interchange (-floop-interchange): swaps a perfectly nested pair of
// for loops when array subscripts put the inner loop's variable in an earlier
// (outer) dimension than the outer loop's variable, so that after the swap
// the row address of `a[j][i]` is invariant in the inner loop (LICM then
// computes it once per row instead of once per element). There is no cache
// on the 45GS02; the gain is the hoisted address arithmetic.
//
// The swap moves the loop headers (initializer, condition, increment)
// between the two ForStatements; the body is not touched or copied.
//
// It is done only when it cannot change the program's behaviour (all checks
// are on the AST, by name, and conservative):
//   - a perfect nest: the outer body is the inner for statement alone
//   - rectangular, side-effect-free headers: each loop's initializer,
//     condition and increment use its own variable plus names that the nest
//     does not assign, and contain no calls, assignments to other variables
//     or volatile objects
//   - a simple body: no break/continue/goto/return/labels/switch/asm/repeat,
//     no calls, no declarations that shadow a loop or bound variable, no
//     assignment to the loop variables or to names used in the headers
//   - memory: every store is to an element of a declared (non-parameter,
//     non-pointer, non-volatile) array whose every access in the body uses the
//     same subscripts, which mention both loop variables (each iteration owns
//     its element); scalars may only be integer reductions (+= -= ^= |= &=)
//     not otherwise used in the body; when the body stores anything, it reads
//     nothing through a pointer (*p, p->m, p[i] with p not a declared array)
//   - the loop variables are declared in their for statements, or are not
//     used elsewhere in the function (their values after the nest differ)
// #pragma cc45 optimize(no-loop-interchange) (in the AST as the directive
// ".optimize no-loop-interchange") turns it off for the function it is in,
// or at file scope for the functions after it.
class LoopInterchange {
public:
    // trace(function, line, message) is called for each interchange (-Roptir)
    std::function<void(const std::string&, int, const std::string&)> trace;

    // Returns the number of loop nests interchanged
    int optimizeTranslationUnit(TranslationUnit& unit, bool enabledByDefault);

private:
    struct VarInfo {
        bool isArray = false;      // declared with array dimensions (not a parameter)
        bool isVolatile = false;
        bool isFloat = false;
        bool isPointer = false;
    };
    std::map<std::string, VarInfo> globals_;
    std::map<std::string, VarInfo> locals_;   // current function: parameters and locals
    std::string function_;
    FunctionDeclaration* currentFn_ = nullptr;
    int count_ = 0;

    const VarInfo* lookup(const std::string& name) const;
    void collectLocals(Statement* s);
    void walk(std::unique_ptr<Statement>& s, bool& enabled);
    bool tryInterchange(ForStatement& outer);
};
