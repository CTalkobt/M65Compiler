#pragma once
#include <vector>
#include <string>
#include <stack>
#include <map>
#include "Token.hpp"

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

    // Access to line number mappings for CodeGenerator
    struct FileContext {
        std::string filename;
        int lineOffset;      // Line number from #line directive
        int startAbsLine;    // Absolute line in expanded source where this context starts
    };

    const std::map<int, FileContext>& getLineToFileMap() const { return lineToFileMap; }

private:
    std::string source;
    size_t pos;
    int line;
    int column;
    int absLine;  // Absolute line number in expanded source
    std::string sourceFile;  // Current source file from #line directives

    // Stack to save/restore file contexts during includes
    std::stack<FileContext> fileContextStack;
    std::map<int, FileContext> lineToFileMap;  // Maps absolute line to file context

    char peek() const;
    char peekNext() const;
    char get();
    void skipWhitespace();
    void handleLineDirective(int newLine, const std::string& newFile);
    Token nextToken();

    Token lexIdentifierOrKeyword();
    Token lexNumber();
    Token lexString(bool ascii = false);
    Token lexChar(bool ascii = false);
};
