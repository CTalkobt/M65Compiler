#ifndef BASICMINIFIER_HPP
#define BASICMINIFIER_HPP

#include <string>
#include <vector>
#include <memory>

class BasicMinifier {
public:
    BasicMinifier() = default;

    // Minify BASIC source code
    std::string minify(const std::string& source);
    // After minify: output line n (1-based) came from input line origins()[n-1]
    const std::vector<int>& origins() const { return origins_; }

    // Configuration options
    void setRemoveComments(bool enable) { removeComments = enable; }
    void setStripWhitespace(bool enable) { stripWhitespace = enable; }
    void setMergeStatements(bool enable) { mergeStatements = enable; }
    void setPreserveStringLiterals(bool enable) { preserveStringLiterals = enable; }

private:
    std::vector<int> origins_;
    bool removeComments = true;
    bool stripWhitespace = true;
    bool mergeStatements = true;
    bool preserveStringLiterals = true;

    struct BasicStatement {
        std::string label;          // label: (if any)
        std::string code;           // the statement
        int sourceLineNum;          // original line number for error reporting
    };

    // Parse source into statements
    std::vector<BasicStatement> parseStatements(const std::string& source);

    // Remove REM statements and inline comments
    std::string removeRemComments(const std::string& line);
    bool hasWordOutsideStrings(const std::string& text, const std::string& word) const;

    // Strip unnecessary whitespace while preserving strings
    std::string stripWhitespaceInLine(const std::string& line);

    // Check if character is inside a string literal
    bool isInString(const std::string& text, size_t pos) const;

    // Extract string literal at position (returns with quotes)
    std::string extractStringLiteral(const std::string& text, size_t& pos) const;
};

#endif  // BASICMINIFIER_HPP
