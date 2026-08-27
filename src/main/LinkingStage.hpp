#pragma once
#include "Stage.hpp"
#include <string>
#include <vector>

// Linking stage - combines object files into executable
class LinkingStage : public Stage {
public:
    LinkingStage(const std::string& objectFile,
                const std::string& outputFile,
                int verboseLevel = 0,
                const std::string& prgBase = "0x0801",
                const std::vector<std::string>& libraryPaths = {},
                const std::string& toolDir = "")
        : objectFile_(objectFile),
          outputFile_(outputFile),
          verboseLevel_(verboseLevel),
          prgBase_(prgBase),
          libraryPaths_(libraryPaths),
          toolDir_(toolDir) {}

    Result execute() override;
    std::string getName() const override { return "Linking (ln45)"; }

    // Get path to generated executable
    std::string getExecutable() const { return outputFile_; }

private:
    std::string objectFile_;
    std::string outputFile_;
    int verboseLevel_;
    std::string prgBase_;
    std::vector<std::string> libraryPaths_;
    std::string toolDir_;

    // Helper methods
    int invokeLinker();
};
