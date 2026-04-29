#pragma once
#include <string>

#ifndef SUITE_VERSION
#define SUITE_VERSION "0.99.dev"
#endif

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

inline std::string suiteVersionString(const char* toolName) {
    return std::string(toolName) + " v" + SUITE_VERSION + " (" + GIT_HASH + ")";
}
