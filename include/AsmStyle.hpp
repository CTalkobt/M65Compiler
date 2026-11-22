#pragma once
// al45's style rules (mega65-style-*, off by default) and af45, the
// formatter that lays a file out as they describe (#611; doc/bin/al45.md,
// doc/bin/af45.md). Both work on the source text, line by line: ca45's
// statements keep no columns. Whitespace only: nothing ca45 assembles
// changes (labels need their colon, so indentation means nothing to it).

#include <string>
#include "Lint.hpp"

namespace asmstyle {

// The layout and the names, from the rules' options
struct Options {
    int instrCol = 4;      // mega65-style-instruction-column: spaces before an instruction
    int operandCol = 0;    // mega65-style-operand-column: 0 = one space after the mnemonic
    int commentCol = 28;   // mega65-style-comment-column: where a trailing comment starts
    std::string labelName = "^[A-Za-z_.][A-Za-z0-9_.]*$";   // mega65-style-label-name
    std::string localName = "^@[A-Za-z0-9_]+$";             // mega65-style-local-name
    std::string constName = "^[A-Za-z_][A-Za-z0-9_.]*$";    // mega65-style-constant-name
};
// false (and err) on an option that is not a number / not a regular expression
bool optionsFrom(const lint::Config& c, Options& o, std::string& err);

// The rules into al45's engine (each off until enabled)
void registerRules(lint::Engine& e);

// The rules over a file's text (al45: every file its statements come from)
void check(lint::Engine& e, const std::string& file, const std::string& text);

// af45: the text laid out by the options (idempotent)
std::string format(const std::string& text, const Options& o);

// af45's main (ca45 run by the name af45)
int formatterMain(int argc, char** argv);

}  // namespace asmstyle
