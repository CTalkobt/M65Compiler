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
        return {false, e.what(), 211};  // Exit code 211-252: linker error
    }
}

int LinkingStage::invokeLinker() {
    if (verboseLevel_ >= 2) {
        std::cout << "  Invoking ln45..." << std::endl;
    }

    // Build ln45 command line: the object, the extra libraries, then the
    // standard library (its crt0 member is the startup code; ln45 adds the
    // BASIC "SYS" line of a -basic PRG itself)
    std::string cmd = toolDir_ + "ln45";
    if (prgBase_.empty()) cmd += " -basic";
    else cmd += " -prg -t 0x" + prgBase_;
    cmd += " -o " + outputFile_ + " " + objectFile_;
    for (const auto& libPath : libraryPaths_) cmd += " " + libPath;
    cmd += " " + standardLibrary();

    if (verboseLevel_ >= 3) {
        std::cout << "    Command: " << cmd << std::endl;
    }
    // ln45's stdout is its "Linked ..." summary; errors and warnings go to
    // stderr and are always shown
    if (verboseLevel_ < 1) cmd += " >/dev/null";

    // Execute linker
    int exitCode = system(cmd.c_str());

    // Extract actual exit code from system() return value
    if (WIFEXITED(exitCode)) {
        return WEXITSTATUS(exitCode);
    }

    return exitCode;
}

// The standard library for the calling convention: c45.lib (ZP) or
// c45_stack.lib, from the build tree (bin/../lib/build) or an installed
// tree (bin/../lib/cc45); else by name, which ln45 looks up in CC45_LIB
std::string LinkingStage::standardLibrary() const {
    std::string name = zpCallMode_ ? "c45.lib" : "c45_stack.lib";
    for (const char* dir : {"../lib/build/", "../lib/cc45/"}) {
        std::string path = toolDir_ + dir + name;
        if (access(path.c_str(), R_OK) == 0) return path;
    }
    return "-l " + name;
}
