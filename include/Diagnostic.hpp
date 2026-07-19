#pragma once

#include <string>

enum class Severity { Error, Warning, Note };

inline std::string formatDiagnostic(const std::string& file, int line, int col,
                                     Severity sev, const std::string& msg) {
    const char* s = sev == Severity::Error ? "error" :
                    sev == Severity::Warning ? "warning" : "note";
    return file + ":" + std::to_string(line) + ":" + std::to_string(col) +
           ": " + s + ": " + msg;
}
