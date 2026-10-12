// BasicFormatter — basic45 --format: the source with consistent layout
//
// Keywords in lower case, one space between words outside strings, code
// indented two spaces plus two per open block (if ... then / else / endif,
// proc / endproc, select / case / endselect, for / next, do / loop), labels
// in column 0. Strings, REM and DATA text, # lines and asm blocks are kept
// as written.
#pragma once

#include <string>

std::string formatBasic(const std::string& source);
