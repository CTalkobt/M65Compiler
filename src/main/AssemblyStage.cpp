#include "AssemblyStage.hpp"
#include "Logger.hpp"
#include "CcTemp.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>

Stage::Result AssemblyStage::execute() {
    if (verboseLevel_ >= 1) {
        Logger::stream(LogLevel::INFO) << "Assembling..." << std::endl;
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
            Logger::stream(LogLevel::DEBUG) << "Assembling to " << objectFile_ << std::endl;
        }

        // Write assembly input to temporary file
        // (--save-temps: in the current directory, else cc45's temp directory, #367)
        tempAsmFile_ = keepTemp_ ? "temp_" + std::to_string(getpid()) + ".s45" : cctemp::path("temp.s45");
        const std::string& tempAsmFile = tempAsmFile_;
        std::ofstream asmOut(tempAsmFile);
        if (!asmOut) {
            return {false, "Failed to write temporary assembly file"};
        }
        asmOut << assemblyInput_;
        asmOut.close();

        if (verboseLevel_ >= 3) {
            Logger::stream(LogLevel::DEEP) << "Wrote assembly to " << tempAsmFile << std::endl;
        }

        // Invoke ca45 assembler
        int exitCode = invokeAssembler();

        // Clean up temporary file (kept with --save-temps)
        if (keepTemp_) std::cerr << "cc45: kept " << tempAsmFile << std::endl;
        else remove(tempAsmFile.c_str());

        if (exitCode != 0) {
            return {false, "Assembler failed with exit code " + std::to_string(exitCode), exitCode};
        }

        if (verboseLevel_ >= 1) {
            Logger::stream(LogLevel::INFO) << "Assembly complete." << std::endl;
        }

        return {true};

    } catch (const std::exception& e) {
        return {false, e.what(), 169};  // Exit code 169-210: assembler error
    }
}

int AssemblyStage::invokeAssembler() {
    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Invoking ca45..." << std::endl;
    }

    // Build ca45 command line
    // Format: ca45 input.s45 [-c] -o output.o45
    std::string cmd = toolDir_ + "ca45 " + tempAsmFile_;

    // Add -c flag if relocatable object requested
    if (relocatable_) {
        cmd += " -c";
    }

    // Add output file
    cmd += " -o " + objectFile_;
    if (!extraArgs_.empty()) cmd += " " + extraArgs_;

    // ca45 logs at cc45's level
    if (verboseLevel_ >= 1) cmd += " " + Logger::flagFor(verboseLevel_);

    if (verboseLevel_ >= 2) {
        Logger::stream(LogLevel::DEBUG) << "Command: " << cmd << std::endl;
    }
    // ca45's stdout is its "Object file: ..." summary; errors and warnings
    // go to stderr and are always shown
    if (verboseLevel_ < 1) cmd += " >/dev/null";

    // Execute assembler
    int exitCode = system(cmd.c_str());

    // Extract actual exit code from system() return value
    if (WIFEXITED(exitCode)) {
        return WEXITSTATUS(exitCode);
    }

    return exitCode;
}
