#pragma once
#include "Stage.hpp"
#include <string>
#include <vector>

// Assembly stage - converts IR assembly to object files
class AssemblyStage : public Stage {
public:
    AssemblyStage(const std::string& assemblyInput,
                 const std::string& outputFile = "",
                 int verboseLevel = 0,
                 bool relocatable = false,
                 const std::string& toolDir = "")
        : assemblyInput_(assemblyInput),
          outputFile_(outputFile),
          verboseLevel_(verboseLevel),
          relocatable_(relocatable),
          toolDir_(toolDir) {}

    Result execute() override;
    std::string getName() const override { return "Assembly (ca45)"; }

    // Get path to generated object file
    std::string getObjectFile() const { return objectFile_; }

private:
    std::string assemblyInput_;
    std::string outputFile_;
    int verboseLevel_;
    bool relocatable_;
    std::string objectFile_;
    std::string toolDir_;

    // Helper methods
    int invokeAssembler();
};
