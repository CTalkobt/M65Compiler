// LinkerLog.hpp — ln45's diagnostic streams (Logger.hpp, tag "ln45")
//
// ln45 prints only errors, warnings that need action, and its summary by
// default; -v adds the analysis summaries and informational warnings
// (ln45info, INFO), -vv the call graph's indirect calls (DEBUG), -vvv the
// relocation records and patches (ln45debug, DEEP). Each line goes to
// stderr as "[ln45:LEVEL] ..."; disabled levels discard their output.
#pragma once
#include "Logger.hpp"

inline std::ostream& ln45info() { return Logger::stream(LogLevel::INFO); }
inline std::ostream& ln45debug() { return Logger::stream(LogLevel::DEEP); }
