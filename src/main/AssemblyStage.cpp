#include "AssemblyStage.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

Stage::Result AssemblyStage::execute() {
    if (verboseLevel_ >= 1) {
        std::cout << "Assembling..." << std::endl;
    }

    try {
        // Determine output file
        if (outputFile_.empty()) {
            // Default to .o45 object file
            objectFile_ = "output.o45";
        } else if (relocatable_) {
            // User requested object file (-c flag)
            objectFile_ = outputFile_;
        } else {
            // User requested binary (no extension change needed by us,
            // assembler will determine output format)
            objectFile_ = outputFile_;
        }

        if (verboseLevel_ >= 2) {
            std::cout << "  Assembling to " << objectFile_ << std::endl;
        }

        // Write assembly input to temporary file
        std::string tempAsmFile = "temp_" + std::to_string(getpid()) + ".s45";
        std::ofstream asmOut(tempAsmFile);
        if (!asmOut) {
            return {false, "Failed to write temporary assembly file"};
        }
        asmOut << assemblyInput_;
        asmOut.close();

        if (verboseLevel_ >= 3) {
            std::cout << "    Wrote assembly to " << tempAsmFile << std::endl;
        }

        // Invoke ca45 assembler
        int exitCode = invokeAssembler();

        // Clean up temporary file
        remove(tempAsmFile.c_str());

        if (exitCode != 0) {
            return {false, "Assembler failed with exit code " + std::to_string(exitCode), exitCode};
        }

        if (verboseLevel_ >= 1) {
            std::cout << "Assembly complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what()};
    }
}

int AssemblyStage::invokeAssembler() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Invoking ca45..." << std::endl;
    }

    // Build ca45 command line
    // Format: ca45 input.s45 [-c] -o output.o45
    std::string cmd = "ca45 temp_" + std::to_string(getpid()) + ".s45";

    // Add -c flag if relocatable object requested
    if (relocatable_) {
        cmd += " -c";
    }

    // Add output file
    cmd += " -o " + objectFile_;

    // Add verbosity flags
    if (verboseLevel_ >= 2) {
        cmd += " -v";
    }

    if (verboseLevel_ >= 3) {
        std::cout << "    Command: " << cmd << std::endl;
    }

    // Execute assembler
    int exitCode = system(cmd.c_str());

    // Extract actual exit code from system() return value
    if (WIFEXITED(exitCode)) {
        return WEXITSTATUS(exitCode);
    }

    return exitCode;
}
