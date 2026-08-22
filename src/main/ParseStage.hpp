#pragma once
#include "Stage.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

class TranslationUnit;
struct Token;

// Lexing and Parsing stage
class ParseStage : public Stage {
public:
    ParseStage(const std::string& source,
              int verboseLevel = 0)
        : source_(source),
          verboseLevel_(verboseLevel) {}

    Result execute() override;
    std::string getName() const override { return "Parse"; }

    // Get parsed AST
    std::shared_ptr<TranslationUnit> getAST() const { return ast_; }

    // Get line-to-file mapping (for source attribution)
    std::vector<std::pair<int, std::pair<std::string, int>>> getLineMap() const { return lineMap_; }

private:
    std::string source_;
    int verboseLevel_;
    std::shared_ptr<TranslationUnit> ast_;
    std::vector<std::pair<int, std::pair<std::string, int>>> lineMap_;

    // Helper to print tokens if verbose
    void printTokens(const std::vector<Token>& tokens) const;
};
