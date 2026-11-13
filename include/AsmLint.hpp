#pragma once
// al45: ca45's lint mode (#64, #199; doc/architecture/lint.md). The rules
// over a parsed (pass 1 and 2, no optimizer) assembly file.

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
void run(lint::Engine& e, AssemblerParser& p, const std::string& mainFile);

}  // namespace asmlint
