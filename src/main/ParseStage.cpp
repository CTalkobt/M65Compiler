#include "ParseStage.hpp"
#include "Logger.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"  // Include full definition for Parser
#include "Token.hpp"
#include "ComplexBuiltin.hpp"
#include <iostream>

void ParseStage::printTokens(const std::vector<Token>& tokens) const {
    for (const auto& token : tokens) {
        Logger::stream(LogLevel::TRACE) << "Token: " << token.typeToString() << " ("
                 << token.value << ") at " << token.line << ":"
                 << token.column << std::endl;
    }
}

Stage::Result ParseStage::execute() {
    if (verboseLevel_ >= 1) {
        Logger::stream(LogLevel::INFO) << "Lexing..." << std::endl;
    }

    try {
        // Lex the source code
        Lexer lexer(source_);
        lexer.setSourceFile(filename_);  // Set filename for error reporting
        std::vector<Token> tokens = lexer.tokenize();
        auto lexerLineMap = lexer.getLineToFileMap();

        // Convert Lexer's FileContext map to our format
        // Each entry: (lineNumber, (filename, lineOffset))
        for (const auto& entry : lexerLineMap) {
            lineMap_.push_back({entry.first, {entry.second.filename, entry.second.lineOffset}});
        }

        if (verboseLevel_ >= 4) {
            printTokens(tokens);
        }

        if (verboseLevel_ >= 1) {
            Logger::stream(LogLevel::INFO) << "Parsing..." << std::endl;
        }

        // _Complex types are structs with overloaded operators; add their
        // definitions when the unit uses them and doesn't define them itself
        bool usesComplex = false, definesComplex = false;
        for (size_t i = 0; i < tokens.size(); i++) {
            const Token& t = tokens[i];
            if (t.type == TokenType::COMPLEX || t.type == TokenType::IMAGINARY_LITERAL ||
                (t.type == TokenType::IDENTIFIER && (t.value == "_Complex_float" || t.value == "_Complex_int")))
                usesComplex = true;
            if (t.type == TokenType::STRUCT && i + 2 < tokens.size() &&
                tokens[i + 1].value == "_Complex_float" && tokens[i + 2].type == TokenType::OPEN_BRACE)
                definesComplex = true;
        }
        if (usesComplex && !definesComplex) {
            Lexer clex(cc45::complexBuiltinSource());
            clex.setSourceFile("<complex>");
            std::vector<Token> ctoks = clex.tokenize();
            while (!ctoks.empty() && ctoks.back().type == TokenType::END_OF_FILE) ctoks.pop_back();
            for (auto& t : ctoks) { t.sourceFile = "<complex>"; t.line = 0; }
            tokens.insert(tokens.begin(), ctoks.begin(), ctoks.end());
        }

        // Parse the tokens into an AST
        Parser parser(tokens);
        parser.signedDefault = signedDefault_;
        parser.unsignedChar = unsignedChar_;
        ast_ = parser.parse();

        // Phase 102: Extract typedef information from parser for IR generation
        // Build a simplified typedef map for later use
        const auto& parserTypedefs = parser.getTypedefs();
        for (const auto& [typedefName, typeAlias] : parserTypedefs) {
            typedefInfo_[typedefName] = {typeAlias.baseType, typeAlias.pointerLevel};
        }

        if (verboseLevel_ >= 1) {
            Logger::stream(LogLevel::INFO) << "Parsing complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what(), 43};  // Exit code 43-84: parser/syntax error
    }
}
