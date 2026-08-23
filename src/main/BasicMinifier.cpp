#include "BasicMinifier.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

std::string BasicMinifier::minify(const std::string& source) {
    std::istringstream iss(source);
    std::string line;
    std::ostringstream result;
    std::vector<std::string> minifiedLines;

    while (std::getline(iss, line)) {
        // Remove REM comments if enabled
        if (removeComments) {
            line = removeRemComments(line);
        }

        // Strip whitespace if enabled
        if (stripWhitespace) {
            line = stripWhitespaceInLine(line);
        }

        // Skip empty lines
        if (line.empty() || (line.find_first_not_of(" \t") == std::string::npos)) {
            continue;
        }

        minifiedLines.push_back(line);
    }

    // Merge statements if enabled
    if (mergeStatements && !minifiedLines.empty()) {
        // Group statements between labels
        std::vector<std::string> merged;
        std::string currentGroup;

        for (size_t i = 0; i < minifiedLines.size(); i++) {
            const auto& stmt = minifiedLines[i];

            // Check if this line starts with a label (contains ':' at start)
            bool isLabel = false;
            for (size_t j = 0; j < stmt.length(); j++) {
                if (stmt[j] == ':') {
                    isLabel = true;
                    break;
                } else if (stmt[j] == ' ' || stmt[j] == '\t') {
                    break;
                }
            }

            if (isLabel) {
                // Start a new group with this label
                if (!currentGroup.empty()) {
                    merged.push_back(currentGroup);
                }
                currentGroup = stmt;
            } else {
                // Add to current group (colon-separate multiple statements)
                if (!currentGroup.empty()) {
                    currentGroup += ": " + stmt;
                } else {
                    currentGroup = stmt;
                }
            }
        }

        if (!currentGroup.empty()) {
            merged.push_back(currentGroup);
        }

        minifiedLines = merged;
    }

    // Output minified lines
    for (const auto& line : minifiedLines) {
        result << line << "\n";
    }

    return result.str();
}

std::string BasicMinifier::removeRemComments(const std::string& line) {
    // Find REM keyword (case-insensitive)
    size_t pos = 0;

    // First, skip label if present
    size_t colonPos = line.find(':');
    if (colonPos != std::string::npos) {
        pos = colonPos + 1;
    }

    // Look for REM after the colon/start of line
    std::string upperLine = line;
    std::transform(upperLine.begin(), upperLine.end(), upperLine.begin(), ::toupper);

    size_t remPos = upperLine.find("REM", pos);

    if (remPos != std::string::npos) {
        // Check if REM is preceded by space/colon/start and followed by space or end
        bool isValidREM = false;

        if (remPos == 0 || upperLine[remPos - 1] == ':' || std::isspace(upperLine[remPos - 1])) {
            if (remPos + 3 >= upperLine.length() || std::isspace(upperLine[remPos + 3])) {
                isValidREM = true;
            }
        }

        if (isValidREM) {
            // Remove the REM and everything after it
            // But keep the label and colon if present
            if (colonPos != std::string::npos) {
                return line.substr(0, colonPos + 1);
            } else {
                return "";
            }
        }
    }

    return line;
}

std::string BasicMinifier::stripWhitespaceInLine(const std::string& line) {
    std::string result;
    bool inString = false;
    char stringChar = 0;

    for (size_t i = 0; i < line.length(); i++) {
        char c = line[i];

        // Handle string literals
        if ((c == '"' || c == '\'') && (i == 0 || line[i-1] != '\\')) {
            if (!inString) {
                inString = true;
                stringChar = c;
                result += c;
            } else if (c == stringChar) {
                inString = false;
                result += c;
            } else {
                result += c;
            }
            continue;
        }

        if (inString) {
            result += c;
        } else if (c == ' ' || c == '\t') {
            // Collapse multiple spaces to single space, but preserve meaningful spaces
            if (!result.empty() && result.back() != ' ' && result.back() != ':' &&
                i + 1 < line.length() && line[i + 1] != ':' &&
                !std::isspace(line[i + 1])) {
                // Add space only if it's between tokens
                if (std::isalnum(result.back()) && std::isalnum(line[i + 1])) {
                    result += ' ';
                }
            }
        } else {
            result += c;
        }
    }

    // Trim leading and trailing whitespace
    size_t start = result.find_first_not_of(" \t");
    size_t end = result.find_last_not_of(" \t");

    if (start != std::string::npos) {
        return result.substr(start, end - start + 1);
    }

    return "";
}

bool BasicMinifier::isInString(const std::string& text, size_t pos) const {
    bool inString = false;
    char stringChar = 0;

    for (size_t i = 0; i < pos && i < text.length(); i++) {
        char c = text[i];

        if ((c == '"' || c == '\'') && (i == 0 || text[i-1] != '\\')) {
            if (!inString) {
                inString = true;
                stringChar = c;
            } else if (c == stringChar) {
                inString = false;
            }
        }
    }

    return inString;
}

std::string BasicMinifier::extractStringLiteral(const std::string& text, size_t& pos) const {
    if (pos >= text.length() || (text[pos] != '"' && text[pos] != '\'')) {
        return "";
    }

    char quote = text[pos];
    std::string result;
    result += quote;
    pos++;

    while (pos < text.length()) {
        char c = text[pos];
        result += c;

        if (c == quote && (pos == 0 || text[pos - 1] != '\\')) {
            pos++;
            break;
        }

        pos++;
    }

    return result;
}
