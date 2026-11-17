#pragma once
// al45: ca45's lint mode (#64, #199; doc/architecture/lint.md). The rules
// over a parsed (pass 1 and 2, no optimizer) assembly file.

#include <iosfwd>
#include <set>
#include <string>
#include "Lint.hpp"

class AssemblerParser;

namespace asmlint {

// al45's rules into the engine (ids, default levels, profiles, texts)
void registerRules(lint::Engine& e);

// The default profile when the configuration and the command line set none:
// cc45 for a relocatable (-c) object, standalone for a flat binary
std::string defaultProfile(const AssemblerParser& p, bool relocMode);

// Run the rules over the parser's statements (mainFile: the source's name,
// for statements not from an #include)
// suggest: also print each proc's .reg_clobbers / .flag_clobbers as al45
// works them out (--suggest-clobbers), on standard output
void run(lint::Engine& e, AssemblerParser& p, const std::string& mainFile, bool suggest = false);

// The rules on data flow (AsmLintFlow.cpp; phase 3): stale flags, stack
// balance, dead register writes, redundant loads, dead base-page stores,
// the parameter block and library temporaries across calls, declared
// clobbers. patched: the statements a store patches (self-modified).
void registerFlowRules(lint::Engine& e);
void runFlow(lint::Engine& e, AssemblerParser& p, const std::string& mainFile,
             const std::set<const void*>& patched, bool suggestClobbers, std::ostream* suggestOut);

}  // namespace asmlint
