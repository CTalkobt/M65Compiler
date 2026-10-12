// LinkerLog.hpp — ln45's diagnostic verbosity
//
// ln45 prints only errors, warnings that need action, and its summary by
// default; -v adds the analysis summaries and informational warnings
// (ln45info), -vv also traces every relocation (ln45debug). Both return
// std::cerr or a stream that discards its output.
#pragma once
#include <ostream>

extern int g_ln45Verbosity;   // 0 (default), 1 (-v), 2 (-vv)
std::ostream& ln45info();
std::ostream& ln45debug();
