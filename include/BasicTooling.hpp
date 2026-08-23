#ifndef BASICTOOLING_HPP
#define BASICTOOLING_HPP

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>

// Dry-run mode - compile but don't write files
class DryRunMode {
public:
    DryRunMode() = default;

    // Simulate compilation without writing output
    std::string simulateCompile(
        const std::string& inputFile,
        const std::string& sourceCode,
        size_t outputSize
    );

    // Report what would be written
    void reportAction(const std::string& action, const std::string& target);

private:
    std::vector<std::pair<std::string, std::string>> actions;  // action, target
};

// Watch mode - monitor file changes and recompile
class WatchMode {
public:
    using CompileCallback = std::function<bool(const std::string&)>;

    WatchMode() = default;

    // Start watching file and recompiling on changes
    void watch(const std::string& inputFile, CompileCallback onFileChange, int pollIntervalMs = 1000);

    // Get last modification time
    std::chrono::system_clock::time_point getFileModTime(const std::string& filename);

    // Stop watching (can be set from callback)
    void stop() { shouldStop = true; }

    bool isStopped() const { return shouldStop; }

private:
    bool shouldStop = false;
};

// Source map generation - map compiled lines back to source
class SourceMapGenerator {
public:
    struct SourceMapping {
        uint16_t compiledLineNum;    // BASIC line number in compiled code
        uint16_t sourceLineNum;      // Original source line number
        std::string sourceFile;      // Original source filename
        std::string sourceLine;      // Source code at that line
    };

    SourceMapGenerator() = default;

    // Add a mapping entry
    void addMapping(const SourceMapping& mapping);

    // Generate source map in JSON format
    std::string generateJSON() const;

    // Generate source map in text format
    std::string generateText() const;

    // Get all mappings
    const std::vector<SourceMapping>& getMappings() const { return mappings; }

private:
    std::vector<SourceMapping> mappings;
};

// Emulator launcher - execute compiled program in emulator
class EmulatorLauncher {
public:
    EmulatorLauncher() = default;

    // Supported emulators
    enum class Emulator {
        MMEMU,      // mmemu-cli (M65 emulator)
        VICE,       // VICE C64 emulator
        DFPLAYER,   // dfplayer (hypothetical)
        AUTO        // Auto-detect available emulator
    };

    // Launch emulator with compiled program
    bool launch(
        const std::string& programFile,
        Emulator emulator = Emulator::AUTO,
        const std::string& emulatorPath = ""
    );

    // Check if emulator is available
    static bool isAvailable(Emulator emulator);

    // Get path to emulator executable
    static std::string getEmulatorPath(Emulator emulator);

private:
    bool launchMMemu(const std::string& programFile, const std::string& emulatorPath);
    bool launchVice(const std::string& programFile, const std::string& emulatorPath);
};

#endif  // BASICTOOLING_HPP
