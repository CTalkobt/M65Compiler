#pragma once
// The suite's version, and the flags every tool answers the same way (#641):
//   -?  --help  (-h where the tool has no -h of its own)   the usage, exit 0
//   -V  --version                                           "<tool> v<version> (<commit>)", exit 0
// anywhere among the arguments (before a "--").
#include <iostream>
#include <string>

#ifndef SUITE_VERSION
#define SUITE_VERSION "1.5.0"
#endif

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

inline std::string suiteVersionString(const char* toolName) {
    return std::string(toolName) + " v" + SUITE_VERSION + " (" + GIT_HASH + ")";
}

// The version as a number, major * 10000 + minor * 100 + patch (1.5.0 →
// 10500): cp45's __CC45_VERSION__
inline long suiteVersionNumber() {
    long part[3] = {0, 0, 0};
    const char* p = SUITE_VERSION;
    for (int i = 0; i < 3 && *p >= '0' && *p <= '9'; ++i) {
        while (*p >= '0' && *p <= '9') part[i] = part[i] * 10 + (*p++ - '0');
        if (*p != '.') break;
        ++p;
    }
    return part[0] * 10000 + part[1] * 100 + part[2];
}

enum class StdFlag { None, Help, Version };

// What one argument asks for. hIsHelp: false in a tool whose -h is an option
// of its own (nm45, objdump45: headers).
inline StdFlag standardFlag(const std::string& arg, bool hIsHelp = true) {
    if (arg == "-?" || arg == "--help" || (hIsHelp && arg == "-h")) return StdFlag::Help;
    if (arg == "-V" || arg == "--version") return StdFlag::Version;
    return StdFlag::None;
}

// The first such flag among a tool's arguments, answered: the version
// printed, or usage() called. True when there was one (the tool then
// returns 0).
template <class Usage>
inline bool answerStandardFlags(int argc, char** argv, const char* toolName, Usage usage, bool hIsHelp = true) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--") break;
        switch (standardFlag(arg, hIsHelp)) {
            case StdFlag::Help: usage(); return true;
            case StdFlag::Version: std::cout << suiteVersionString(toolName) << std::endl; return true;
            case StdFlag::None: break;
        }
    }
    return false;
}
