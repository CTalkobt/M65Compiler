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
                const std::string& prgBase = "",
                const std::vector<std::string>& libraryPaths = {},
                const std::string& toolDir = "",
                bool zpCallMode = true)
        : objectFile_(objectFile),
          outputFile_(outputFile),
          verboseLevel_(verboseLevel),
          prgBase_(prgBase),
          libraryPaths_(libraryPaths),
          toolDir_(toolDir),
          zpCallMode_(zpCallMode) {}

    Result execute() override;
    std::string getName() const override { return "Linking (ln45)"; }

    // Get path to generated executable
    std::string getExecutable() const { return outputFile_; }

    // -ffunction-sections: ln45 also drops the program's own unused sections
    void setGcInputs(bool on) { gcInputs_ = on; }
    // -g: ln45 writes the program's debug information, <program>.dbg
    void setDebugFile(bool on) { debugFile_ = on; }
private:
    bool gcInputs_ = false;
    bool debugFile_ = false;
    std::string objectFile_;
    std::string outputFile_;
    int verboseLevel_;
    std::string prgBase_;           // hex load address of a plain PRG; "" = BASIC-startable PRG
    std::vector<std::string> libraryPaths_;
    std::string toolDir_;
    bool zpCallMode_;

    std::string standardLibrary() const;

    // Helper methods
    int invokeLinker();
};
