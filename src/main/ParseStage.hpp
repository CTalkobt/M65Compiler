#pragma once
#include "Stage.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

class TranslationUnit;
struct Token;
class Parser;  // Forward declaration

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

    // Phase 102: Get typedef information for IR generation
    // Returns a simplified map for struct/union typedefs
    const std::map<std::string, std::pair<std::string, int>>& getTypedefInfo() const {
        return typedefInfo_;
    }

private:
    std::string source_;
    int verboseLevel_;
    std::shared_ptr<TranslationUnit> ast_;
    std::vector<std::pair<int, std::pair<std::string, int>>> lineMap_;

    // Phase 102: Store simplified typedef information from parser
    // Map: typedef_name → (baseType, pointerLevel)
    std::map<std::string, std::pair<std::string, int>> typedefInfo_;

    // Helper to print tokens if verbose
    void printTokens(const std::vector<Token>& tokens) const;
};
