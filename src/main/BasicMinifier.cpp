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
    std::vector<int> lineFrom;
    int inLine = 0;
    origins_.clear();

    while (std::getline(iss, line)) {
        inLine++;
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
        lineFrom.push_back(inLine);
    }

    // Merge statements if enabled
    if (mergeStatements && !minifiedLines.empty()) {
        // Join lines into the line before them, up to a labeled line (a jump
        // target starts a line of its own). Never onto a line with IF: the
        // statements after THEN on a line run only when the condition holds.
        // A joined line stays at most maxMergedLength characters.
        const size_t maxMergedLength = 160;
        std::vector<std::string> merged;
        std::vector<int> mergedFrom;
        std::string currentGroup;
        int groupFrom = 0;
        std::regex labelLine(R"(^[A-Za-z_]\w*\s*:)");

        for (size_t i = 0; i < minifiedLines.size(); i++) {
            const auto& stmt = minifiedLines[i];
            bool isLabel = std::regex_search(stmt, labelLine);
            bool canJoin = !isLabel && !currentGroup.empty() && !hasWordOutsideStrings(currentGroup, "IF") &&
                           currentGroup.size() + 1 + stmt.size() <= maxMergedLength;
            if (canJoin) {
                // a label alone ("done:") already ends with its colon
                currentGroup += (currentGroup.back() == ':') ? stmt : ":" + stmt;
            } else {
                if (!currentGroup.empty()) { merged.push_back(currentGroup); mergedFrom.push_back(groupFrom); }
                currentGroup = stmt;
                groupFrom = lineFrom[i];
            }
        }

        if (!currentGroup.empty()) {
            merged.push_back(currentGroup);
            mergedFrom.push_back(groupFrom);
        }

        minifiedLines = merged;
        lineFrom = mergedFrom;
    }
    origins_ = lineFrom;

    for (const auto& line : minifiedLines) {
        result << line << "\n";
    }

    return result.str();
}

// Whether word (upper case) appears as a whole word outside strings
bool BasicMinifier::hasWordOutsideStrings(const std::string& text, const std::string& word) const {
    bool inString = false;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '"') { inString = !inString; continue; }
        if (inString) continue;
        if (i > 0 && (std::isalnum((unsigned char)text[i - 1]) || text[i - 1] == '_' || text[i - 1] == '$')) continue;
        if (text.size() - i < word.size()) break;
        bool match = true;
        for (size_t k = 0; k < word.size(); k++)
            if (std::toupper((unsigned char)text[i + k]) != word[k]) { match = false; break; }
        if (!match) continue;
        size_t e = i + word.size();
        if (e < text.size() && (std::isalnum((unsigned char)text[e]) || text[e] == '_' || text[e] == '$')) continue;
        return true;
    }
    return false;
}

// Drop a REM statement and everything after it (outside strings). A label
// before it ("loop: rem ...") is kept, so the label still names this line.
std::string BasicMinifier::removeRemComments(const std::string& line) {
    bool inString = false;
    size_t stmtStart = 0;
    for (size_t i = 0; i <= line.size(); i++) {
        if (i < line.size() && line[i] == '"') { inString = !inString; continue; }
        if (inString) continue;
        if (i == line.size() || line[i] == ':') {
            std::string stmt = line.substr(stmtStart, i - stmtStart);
            size_t b = stmt.find_first_not_of(" \t");
            if (b != std::string::npos) {
                std::string head = stmt.substr(b, 3);
                std::transform(head.begin(), head.end(), head.begin(), ::toupper);
                bool after = b + 3 >= stmt.size() || !std::isalnum((unsigned char)stmt[b + 3]);
                if (head == "REM" && after) {
                    std::string kept = line.substr(0, stmtStart);
                    size_t e = kept.find_last_not_of(" \t");
                    kept = e == std::string::npos ? "" : kept.substr(0, e + 1);
                    // "x = 1:" -> "x = 1"; a lone label keeps its colon
                    static const std::regex lone(R"(^\s*[A-Za-z_]\w*\s*:$)");
                    if (!kept.empty() && kept.back() == ':' && !std::regex_match(kept, lone)) kept.pop_back();
                    return kept;
                }
            }
            stmtStart = i + 1;
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
