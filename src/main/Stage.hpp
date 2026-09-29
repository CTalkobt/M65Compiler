#pragma once
#include <memory>
#include <string>
#include <vector>

namespace ir { struct Module; }
class TranslationUnit;

// Base interface for compilation pipeline stages
class Stage {
public:
    virtual ~Stage() = default;

    // Stage result (success/failure)
    struct Result {
        bool success = false;
        std::string error;
        int exitCode = 0;
    };

    // Execute the stage
    virtual Result execute() = 0;

    // Stage name for logging
    virtual std::string getName() const = 0;
};
