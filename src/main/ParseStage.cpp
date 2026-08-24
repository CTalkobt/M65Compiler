#include "ParseStage.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"  // Include full definition for Parser
#include "Token.hpp"
#include <iostream>

void ParseStage::printTokens(const std::vector<Token>& tokens) const {
    for (const auto& token : tokens) {
        std::cout << "Token: " << token.typeToString() << " ("
                 << token.value << ") at " << token.line << ":"
                 << token.column << std::endl;
    }
}

Stage::Result ParseStage::execute() {
    if (verboseLevel_ >= 1) {
        std::cout << "Lexing..." << std::endl;
    }

    try {
        // Lex the source code
        Lexer lexer(source_);
        std::vector<Token> tokens = lexer.tokenize();
        auto lexerLineMap = lexer.getLineToFileMap();

        // Convert Lexer's FileContext map to our format
        // Each entry: (lineNumber, (filename, lineOffset))
        for (const auto& entry : lexerLineMap) {
            lineMap_.push_back({entry.first, {entry.second.filename, entry.second.lineOffset}});
        }

        if (verboseLevel_ >= 2) {
            printTokens(tokens);
        }

        if (verboseLevel_ >= 1) {
            std::cout << "Parsing..." << std::endl;
        }

        // Parse the tokens into an AST
        Parser parser(tokens);
        ast_ = parser.parse();

        // Phase 102: Extract typedef information from parser for IR generation
        // Build a simplified typedef map for later use
        const auto& parserTypedefs = parser.getTypedefs();
        for (const auto& [typedefName, typeAlias] : parserTypedefs) {
            typedefInfo_[typedefName] = {typeAlias.baseType, typeAlias.pointerLevel};
        }

        if (verboseLevel_ >= 1) {
            std::cout << "Parsing complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}
