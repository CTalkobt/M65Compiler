#pragma once
// cl45: cc45's lint mode (#199; doc/architecture/lint.md). The C rules over
// what cc45's front end made of a file: the AST as parsed, the IR as the
// IRBuilder wrote it (no pass run), the frames the code generator laid out.
// Rules that need an expression's type are reported by the IRBuilder itself
// (IRBuilder::lintReport), where the types are known.

#include <map>
#include <set>
#include <string>
#include <vector>
#include "Lint.hpp"

class TranslationUnit;
namespace ir { struct Module; }

namespace clint {

struct Context {
    lint::Engine* engine = nullptr;
    // headers in these directories (the system headers) are not reported
    std::vector<std::string> systemDirs;
    bool staticAlloc = true;         // -fstaticalloc in force
    // #pragma cc45 lint(ignore | restore, id) as the preprocessor met them
    struct Pragma { std::string file; int line; std::string id; bool restore; };
    void addPragmas(const std::vector<Pragma>& ps);

    // a finding: dropped in a system header, where a NOLINT comment or a
    // pragma covers it, or when the same one was reported before (a function
    // can be lowered twice)
    void report(const std::string& id, const std::string& file, int line, int col, const std::string& message);

private:
    std::set<std::string> scanned_, seen_;
};

// cl45's rules into the engine (ids, default levels, profiles, texts)
void registerRules(lint::Engine& e);

// The AST as parsed (before constant folding: casts are still casts):
// mega65-io-not-volatile on constant addresses
void runAST(Context& c, TranslationUnit& unit);

// The IR as the IRBuilder wrote it, and the AST for what only it has
// (declarations): DMA trigger order, the I/O knock, recursion, what
// interrupt handlers reach, long arithmetic that fits an int, variables
// placed in I/O
void runIR(Context& c, const ir::Module& m, TranslationUnit& unit);

// The stack frames the code generator laid out: function -> bytes
void runFrames(Context& c, const ir::Module& m, const std::map<std::string, int>& frames);

}  // namespace clint
