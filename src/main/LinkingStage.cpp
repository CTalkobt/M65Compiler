#include "LinkingStage.hpp"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

Stage::Result LinkingStage::execute() {
    if (verboseLevel_ >= 1) {
        std::cout << "Linking..." << std::endl;
    }

    try {
        if (verboseLevel_ >= 2) {
            std::cout << "  Linking object file to " << outputFile_ << std::endl;
        }

        // Invoke ln45 linker
        int exitCode = invokeLinker();

        if (exitCode != 0) {
            return {false, "Linker failed with exit code " + std::to_string(exitCode), exitCode};
        }

        if (verboseLevel_ >= 1) {
            std::cout << "Linking complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}

int LinkingStage::invokeLinker() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Invoking ln45..." << std::endl;
    }

    // Build ln45 command line
    // Format: ln45 input.o45 -o output.prg --base address [--lib path...]
    std::string cmd = toolDir_ + "ln45 " + objectFile_;

    // Add output file
    cmd += " -o " + outputFile_;

    // Add PRG base address
    cmd += " --base " + prgBase_;

    // Add library paths
    for (const auto& libPath : libraryPaths_) {
        cmd += " --lib " + libPath;
    }

    // Add verbosity flags
    if (verboseLevel_ >= 2) {
        cmd += " -v";
    }

    if (verboseLevel_ >= 3) {
        std::cout << "    Command: " << cmd << std::endl;
    }

    // Execute linker
    int exitCode = system(cmd.c_str());

    // Extract actual exit code from system() return value
    if (WIFEXITED(exitCode)) {
        return WEXITSTATUS(exitCode);
    }

    return exitCode;
}
