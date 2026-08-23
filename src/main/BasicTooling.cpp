#include "BasicTooling.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <thread>

namespace fs = std::filesystem;

// ==================== DryRunMode ====================

std::string DryRunMode::simulateCompile(
    const std::string& inputFile,
    const std::string& sourceCode,
    size_t outputSize
) {
    std::ostringstream report;
    report << "=== DRY RUN MODE ===" << "\n";
    report << "Input:  " << inputFile << "\n";
    report << "Size:   " << sourceCode.length() << " bytes of BASIC source" << "\n";
    report << "Output: " << outputSize << " bytes (would be written)" << "\n";
    report << "\nActions that would be performed:\n";

    for (const auto& [action, target] : actions) {
        report << "  [" << action << "] " << target << "\n";
    }

    if (actions.empty()) {
        report << "  (no files would be modified)\n";
    }

    report << "\nNo files were actually written (dry-run mode)\n";
    return report.str();
}

void DryRunMode::reportAction(const std::string& action, const std::string& target) {
    actions.emplace_back(action, target);
}

// ==================== WatchMode ====================

std::chrono::system_clock::time_point WatchMode::getFileModTime(const std::string& filename) {
    try {
        auto lastWrite = fs::last_write_time(filename);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastWrite - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        return sctp;
    } catch (const std::exception&) {
        return std::chrono::system_clock::now();
    }
}

void WatchMode::watch(
    const std::string& inputFile,
    CompileCallback onFileChange,
    int pollIntervalMs
) {
    if (!fs::exists(inputFile)) {
        std::cerr << "Watch: file not found: " << inputFile << std::endl;
        return;
    }

    std::cout << "Watch mode enabled for: " << inputFile << std::endl;
    std::cout << "Press Ctrl+C to stop watching..." << std::endl;

    auto lastModTime = getFileModTime(inputFile);
    int consecutiveErrors = 0;

    while (!shouldStop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(pollIntervalMs));

        try {
            auto currentModTime = getFileModTime(inputFile);

            if (currentModTime > lastModTime) {
                std::cout << "\n[" << std::chrono::system_clock::now().time_since_epoch().count() << "] "
                         << "File changed, recompiling..." << std::endl;

                lastModTime = currentModTime;
                consecutiveErrors = 0;

                // Read file and compile
                std::ifstream file(inputFile);
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string sourceCode = buffer.str();

                // Call the compile callback
                if (!onFileChange(sourceCode)) {
                    consecutiveErrors++;
                    if (consecutiveErrors >= 5) {
                        std::cerr << "Too many compilation errors, stopping watch mode" << std::endl;
                        shouldStop = true;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Watch error: " << e.what() << std::endl;
            consecutiveErrors++;
            if (consecutiveErrors >= 5) {
                shouldStop = true;
            }
        }
    }

    std::cout << "\nWatch mode stopped" << std::endl;
}

// ==================== SourceMapGenerator ====================

void SourceMapGenerator::addMapping(const SourceMapping& mapping) {
    mappings.push_back(mapping);
}

std::string SourceMapGenerator::generateJSON() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"version\": 3,\n";
    json << "  \"file\": \"compiled.prg\",\n";
    json << "  \"mappings\": [\n";

    for (size_t i = 0; i < mappings.size(); i++) {
        const auto& m = mappings[i];
        json << "    {\n";
        json << "      \"compiledLine\": " << m.compiledLineNum << ",\n";
        json << "      \"sourceLine\": " << m.sourceLineNum << ",\n";
        json << "      \"sourceFile\": \"" << m.sourceFile << "\",\n";
        json << "      \"sourceCode\": \"" << m.sourceLine << "\"\n";
        json << "    }";
        if (i < mappings.size() - 1) json << ",";
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";
    return json.str();
}

std::string SourceMapGenerator::generateText() const {
    std::ostringstream text;
    text << "Source Map: Compiled Line → Source Line\n";
    text << "=====================================\n\n";

    for (const auto& m : mappings) {
        text << "Line " << m.compiledLineNum << " ← Line " << m.sourceLineNum;
        text << " (" << m.sourceFile << ")\n";
        text << "  Source: " << m.sourceLine << "\n";
    }

    return text.str();
}

// ==================== EmulatorLauncher ====================

bool EmulatorLauncher::launch(
    const std::string& programFile,
    Emulator emulator,
    const std::string& emulatorPath
) {
    if (!fs::exists(programFile)) {
        std::cerr << "Emulator: program file not found: " << programFile << std::endl;
        return false;
    }

    // Auto-detect if requested
    if (emulator == Emulator::AUTO) {
        if (isAvailable(Emulator::MMEMU)) {
            emulator = Emulator::MMEMU;
        } else if (isAvailable(Emulator::VICE)) {
            emulator = Emulator::VICE;
        } else {
            std::cerr << "Emulator: no compatible emulator found" << std::endl;
            return false;
        }
    }

    switch (emulator) {
        case Emulator::MMEMU:
            return launchMMemu(programFile, emulatorPath);
        case Emulator::VICE:
            return launchVice(programFile, emulatorPath);
        default:
            std::cerr << "Emulator: unsupported emulator" << std::endl;
            return false;
    }
}

bool EmulatorLauncher::isAvailable(Emulator emulator) {
    std::string path = getEmulatorPath(emulator);
    return !path.empty() && fs::exists(path);
}

std::string EmulatorLauncher::getEmulatorPath(Emulator emulator) {
    switch (emulator) {
        case Emulator::MMEMU:
            // Check common locations for mmemu-cli
            if (fs::exists("/usr/local/bin/mmemu-cli")) return "/usr/local/bin/mmemu-cli";
            if (fs::exists("/usr/bin/mmemu-cli")) return "/usr/bin/mmemu-cli";
            if (fs::exists("./mmemu-cli")) return "./mmemu-cli";
            return "";

        case Emulator::VICE:
            // Check common locations for VICE
            if (fs::exists("/usr/local/bin/x64")) return "/usr/local/bin/x64";
            if (fs::exists("/usr/bin/x64")) return "/usr/bin/x64";
            if (fs::exists("./x64")) return "./x64";
            return "";

        default:
            return "";
    }
}

bool EmulatorLauncher::launchMMemu(const std::string& programFile, const std::string& emulatorPath) {
    std::string mmemu = emulatorPath.empty() ? getEmulatorPath(Emulator::MMEMU) : emulatorPath;

    if (mmemu.empty() || !fs::exists(mmemu)) {
        std::cerr << "Emulator: mmemu-cli not found" << std::endl;
        return false;
    }

    std::string cmd = mmemu + " " + programFile;
    std::cout << "Launching: " << cmd << std::endl;

    int result = system(cmd.c_str());
    return result == 0;
}

bool EmulatorLauncher::launchVice(const std::string& programFile, const std::string& emulatorPath) {
    std::string vice = emulatorPath.empty() ? getEmulatorPath(Emulator::VICE) : emulatorPath;

    if (vice.empty() || !fs::exists(vice)) {
        std::cerr << "Emulator: x64 (VICE) not found" << std::endl;
        return false;
    }

    std::string cmd = vice + " " + programFile;
    std::cout << "Launching: " << cmd << std::endl;

    int result = system(cmd.c_str());
    return result == 0;
}
