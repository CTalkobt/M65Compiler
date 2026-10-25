// Logger.cpp — the toolchain's diagnostic log (see Logger.hpp)
#include "Logger.hpp"
#include <iostream>
#include <streambuf>

LogLevel Logger::current_ = LogLevel::OFF;

namespace {

std::string& toolName() {
    static std::string name = "cc45";
    return name;
}

// stderr with "[tool:LEVEL] " at the start of each line
class PrefixBuf : public std::streambuf {
public:
    explicit PrefixBuf(LogLevel level) : level_(level) {}
protected:
    int overflow(int c) override {
        if (c == EOF) return 0;
        if (atLineStart_) {
            std::cerr << '[' << toolName() << ':' << Logger::levelName(level_) << "] ";
            atLineStart_ = false;
        }
        std::cerr.put((char)c);
        if (c == '\n') atLineStart_ = true;
        return c;
    }
    int sync() override { std::cerr.flush(); return 0; }
private:
    LogLevel level_;
    bool atLineStart_ = true;
};

class NullBuf : public std::streambuf {
protected:
    int overflow(int c) override { return c == EOF ? 0 : c; }
};

} // namespace

void Logger::setTool(const std::string& tool) { toolName() = tool; }
const std::string& Logger::tool() { return toolName(); }

void Logger::setLevel(LogLevel level) { current_ = level; }
void Logger::setLevel(int level) {
    if (level < 0) level = 0;
    if (level > 4) level = 4;
    current_ = (LogLevel)level;
}
LogLevel Logger::getLevel() { return current_; }
int Logger::level() { return (int)current_; }

const char* Logger::levelName(LogLevel level) {
    switch (level) {
        case LogLevel::INFO:  return "INFO";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::DEEP:  return "DEEP";
        case LogLevel::TRACE: return "TRACE";
        default:              return "OFF";
    }
}

void Logger::log(LogLevel level, const std::string& tool, const std::string& message) {
    if (!isEnabled(level)) return;
    std::cerr << '[' << tool << ':' << levelName(level) << "] " << message << '\n';
}

std::ostream& Logger::stream(LogLevel level) {
    static PrefixBuf infoBuf(LogLevel::INFO), debugBuf(LogLevel::DEBUG), deepBuf(LogLevel::DEEP),
                     traceBuf(LogLevel::TRACE);
    static std::ostream info(&infoBuf), debug(&debugBuf), deep(&deepBuf), trace(&traceBuf);
    static NullBuf nullBuf;
    static std::ostream null(&nullBuf);
    if (!isEnabled(level)) return null;
    switch (level) {
        case LogLevel::INFO:  return info;
        case LogLevel::DEBUG: return debug;
        case LogLevel::DEEP:  return deep;
        default:              return trace;
    }
}

bool Logger::parseFlag(const std::string& arg, int& level) {
    if (arg == "-v0") { level = 0; return true; }
    if (arg.size() < 2 || arg[0] != '-' || arg.find_first_not_of('v', 1) != std::string::npos) return false;
    level += (int)arg.size() - 1;
    if (level > 4) level = 4;
    return true;
}

int Logger::takeFlags(int argc, char** argv, int first) {
    int level = Logger::level(), out = first;
    for (int i = first; i < argc; i++) {
        if (parseFlag(argv[i], level)) continue;
        argv[out++] = argv[i];
    }
    setLevel(level);
    return out;
}

std::string Logger::flagFor(int level) {
    if (level <= 0) return "";
    return "-" + std::string(level > 4 ? 4 : level, 'v');
}
