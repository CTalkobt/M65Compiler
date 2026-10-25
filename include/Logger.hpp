// Logger.hpp — the toolchain's diagnostic log (cc45, ca45, ln45)
//
// One level for the program, from its -v flags: -v INFO (phases, summaries),
// -vv DEBUG (pass activity, layout), -vvv DEEP (internal structures,
// relocation records), -vvvv TRACE (token streams, byte patches). Every
// message goes to stderr as "[tool:LEVEL] message", so standard output keeps
// only what the program was asked to write. Reports a flag requests
// (-Roptir, -Roptimizer, a map) are output, not log, and stay as they are.
//
//   LOG_INFO("cc45", "Parsing " << file);    // built only when enabled
//   Logger::stream(LogLevel::DEBUG) << "..." << std::endl;   // each line prefixed
#pragma once
#include <ostream>
#include <sstream>
#include <string>

enum class LogLevel {
    OFF = 0,
    INFO = 1,     // -v
    DEBUG = 2,    // -vv
    DEEP = 3,     // -vvv
    TRACE = 4     // -vvvv
};

class Logger {
public:
    static void setTool(const std::string& tool);   // the tag of stream()'s lines
    static const std::string& tool();
    static void setLevel(LogLevel level);
    static void setLevel(int level);                 // 0..4 (capped)
    static LogLevel getLevel();
    static int level();
    static bool isEnabled(LogLevel level) { return (int)current_ >= (int)level && level != LogLevel::OFF; }
    static const char* levelName(LogLevel level);

    static void log(LogLevel level, const std::string& tool, const std::string& message);
    static void info(const std::string& tool, const std::string& message) { log(LogLevel::INFO, tool, message); }
    static void debug(const std::string& tool, const std::string& message) { log(LogLevel::DEBUG, tool, message); }
    static void deep(const std::string& tool, const std::string& message) { log(LogLevel::DEEP, tool, message); }
    static void trace(const std::string& tool, const std::string& message) { log(LogLevel::TRACE, tool, message); }

    // A stream for code that writes in pieces: stderr with each line
    // prefixed "[tool:LEVEL] " when the level is enabled, else discarded
    static std::ostream& stream(LogLevel level);

    // A verbosity flag: -v, -vv, ... (each v one level, cumulative over
    // several flags, at most 4) or -v0 (off). True when arg is one.
    static bool parseFlag(const std::string& arg, int& level);
    // The flag that gives a child process the same level ("" at 0)
    static std::string flagFor(int level);
    // For tools with positional arguments: the verbosity flags from
    // argv[first..] taken out (argv compacted, the level set); the new argc
    static int takeFlags(int argc, char** argv, int first = 1);

private:
    static LogLevel current_;
};

#define LOG_AT_(lvl, tool, expr) do { \
    if (Logger::isEnabled(lvl)) { \
        std::ostringstream log_oss_; log_oss_ << expr; \
        Logger::log(lvl, tool, log_oss_.str()); \
    } \
} while (0)
#define LOG_INFO(tool, expr)  LOG_AT_(LogLevel::INFO, tool, expr)
#define LOG_DEBUG(tool, expr) LOG_AT_(LogLevel::DEBUG, tool, expr)
#define LOG_DEEP(tool, expr)  LOG_AT_(LogLevel::DEEP, tool, expr)
#define LOG_TRACE(tool, expr) LOG_AT_(LogLevel::TRACE, tool, expr)
