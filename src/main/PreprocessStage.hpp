#pragma once
#include "Stage.hpp"
#include <string>
#include <vector>
#include <map>

// C Preprocessor stage
class PreprocessStage : public Stage {
public:
    PreprocessStage(const std::string& inputFile,
                   const std::string& sourceRaw,
                   int verboseLevel,
                   const std::vector<std::string>& includePaths,
                   const std::map<std::string, std::string>& symbols,
                   const std::string& outputFile = "")
        : inputFile_(inputFile),
          sourceRaw_(sourceRaw),
          verboseLevel_(verboseLevel),
          includePaths_(includePaths),
          symbols_(symbols),
          outputFile_(outputFile) {}

    Result execute() override;
    std::string getName() const override { return "Preprocess"; }

    // Get preprocessed source
    std::string getPreprocessedSource() const { return preprocessedSource_; }

private:
    std::string inputFile_;
    std::string sourceRaw_;
    int verboseLevel_;
    std::vector<std::string> includePaths_;
    std::map<std::string, std::string> symbols_;
    std::string outputFile_;
    std::string preprocessedSource_;
};
