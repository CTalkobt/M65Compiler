#pragma once
#include <string>
#include <optional>

// Format auto-detection utility
// Detects assembler format from file extension or vim/emacs modeline
class FormatDetection {
public:
    // Detect format from file path (extension or modeline)
    static std::optional<std::string> detectFormat(const std::string& filePath);

    // Detect format from file content (modeline detection)
    static std::optional<std::string> detectFromModeline(const std::string& content);

    // Detect format from file extension
    static std::optional<std::string> detectFromExtension(const std::string& filePath);

    // Map format aliases to canonical names
    static std::string canonicalizeFormat(const std::string& format);

private:
    // Extract vim/emacs modeline from file content
    // Looks for patterns like: vim: ft=ca65 or emacs: -*- mode: ca65 -*-
    static std::optional<std::string> extractModeline(const std::string& content);
};
