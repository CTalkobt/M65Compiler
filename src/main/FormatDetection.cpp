#include "FormatDetection.hpp"
#include <regex>
#include <algorithm>
#include <sstream>
#include <fstream>

std::optional<std::string> FormatDetection::detectFormat(const std::string& filePath) {
    // Try modeline detection first (more specific)
    std::ifstream file(filePath);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        auto modeline = detectFromModeline(buffer.str());
        if (modeline) {
            return canonicalizeFormat(*modeline);
        }
    }

    // Fall back to extension detection
    return detectFromExtension(filePath);
}

std::optional<std::string> FormatDetection::detectFromExtension(const std::string& filePath) {
    // Find the last dot
    size_t dotPos = filePath.find_last_of('.');
    if (dotPos == std::string::npos) {
        return std::nullopt;
    }

    std::string ext = filePath.substr(dotPos + 1);
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // Map extensions to formats
    if (ext == "s45" || ext == "asm" || ext == "s") return "ca45";
    if (ext == "s65" || ext == "c65") return "ca65";
    if (ext == "a65") return "acme";
    if (ext == "m64" || ext == "m6502") return "merlin64";
    if (ext == "s6502") return "oscar";
    if (ext == "x65") return "x65";
    if (ext == "asm" || ext == "ka") return "kickassembler";  // .asm is ambiguous, treat as kickassembler

    return std::nullopt;
}

std::optional<std::string> FormatDetection::detectFromModeline(const std::string& content) {
    // Look for vim modelines: vim: ft=format or vim: filetype=format
    // Example: # vim: ft=ca65
    std::regex vim_pattern(R"(vim:\s+(?:filetype|ft)=(\w+))", std::regex::icase);
    std::smatch match;

    // Search in first and last few lines (vim checks these)
    std::istringstream iss(content);
    std::string line;
    int lineNum = 0;
    const int CHECK_LINES = 5;

    while (std::getline(iss, line) && lineNum < CHECK_LINES) {
        if (std::regex_search(line, match, vim_pattern)) {
            return match[1].str();
        }
        lineNum++;
    }

    // Also check last few lines
    std::istringstream iss2(content);
    std::vector<std::string> lines;
    while (std::getline(iss2, line)) {
        lines.push_back(line);
    }

    for (int i = std::max(0, (int)lines.size() - CHECK_LINES); i < (int)lines.size(); ++i) {
        if (std::regex_search(lines[i], match, vim_pattern)) {
            return match[1].str();
        }
    }

    // Look for emacs modelines: -*- mode: format -*-
    // Example: ; -*- mode: ca65 -*-
    std::regex emacs_pattern(R"(-\*-\s+mode:\s*(\w+)\s+-\*-)", std::regex::icase);

    iss.clear();
    iss.seekg(0);
    lineNum = 0;
    while (std::getline(iss, line) && lineNum < CHECK_LINES) {
        if (std::regex_search(line, match, emacs_pattern)) {
            return match[1].str();
        }
        lineNum++;
    }

    return std::nullopt;
}

std::string FormatDetection::canonicalizeFormat(const std::string& format) {
    std::string fmt = format;
    // Convert to lowercase
    std::transform(fmt.begin(), fmt.end(), fmt.begin(), ::tolower);

    // Map aliases to canonical names
    if (fmt == "ca45" || fmt == "45gs02" || fmt == "mega65") return "ca45";
    if (fmt == "ca65" || fmt == "cc65" || fmt == "c65") return "ca65";
    if (fmt == "acme" || fmt == "a65") return "acme";
    if (fmt == "merlin64" || fmt == "merlin" || fmt == "m64" || fmt == "m6502") return "merlin64";
    if (fmt == "oscar" || fmt == "commodore") return "oscar";
    if (fmt == "x65") return "x65";
    if (fmt == "kickassembler" || fmt == "kickass" || fmt == "ka") return "kickassembler";

    // If no alias matches, return as-is (might be a valid format)
    return fmt;
}
