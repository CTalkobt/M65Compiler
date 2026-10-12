#include "BasicTooling.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <unistd.h>
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

std::string EmulatorLauncher::findXemu() { return findOnPath("xemu-xmega65"); }

int EmulatorLauncher::launchAsync(const std::string& programFile, const std::string& emulatorPath,
                                  const std::string& diskImage) {
    std::string xemu = emulatorPath.empty() ? findXemu() : emulatorPath;
    if (xemu.empty()) {
        std::cerr << "Emulator: xemu-xmega65 not found on PATH (--emulator-path <file>)" << std::endl;
        return -1;
    }
    std::string prg = fs::absolute(programFile).string(), disk = diskImage.empty() ? "" : fs::absolute(diskImage).string();
    pid_t pid = fork();
    if (pid == 0) {
        if (disk.empty())
            execl(xemu.c_str(), xemu.c_str(), "-prgmode", "65", "-prg", prg.c_str(), (char*)nullptr);
        else
            execl(xemu.c_str(), xemu.c_str(), "-prgmode", "65", "-prg", prg.c_str(), "-8", disk.c_str(), (char*)nullptr);
        _exit(127);
    }
    return pid;
}

std::string EmulatorLauncher::findOnPath(const std::string& name) {
    const char* path = getenv("PATH");
    std::string p = path ? path : "/usr/local/bin:/usr/bin";
    size_t start = 0;
    while (start <= p.size()) {
        size_t end = p.find(':', start);
        if (end == std::string::npos) end = p.size();
        std::string dir = p.substr(start, end - start);
        if (dir.empty()) dir = ".";
        fs::path cand = fs::path(dir) / name;
        if (fs::exists(cand)) return cand.string();
        start = end + 1;
    }
    return "";
}

bool EmulatorLauncher::launch(const std::string& programFile, const std::string& emulatorPath,
                              const std::string& diskImage) {
    if (!fs::exists(programFile)) {
        std::cerr << "Emulator: program file not found: " << programFile << std::endl;
        return false;
    }
    std::string xemu = emulatorPath.empty() ? findXemu() : emulatorPath;
    if (xemu.empty()) {
        std::cerr << "Emulator: xemu-xmega65 not found on PATH (--emulator-path <file>)" << std::endl;
        return false;
    }
    auto quote = [](const std::string& s) {
        std::string q = "'";
        for (char c : s) { if (c == '\'') q += "'\\''"; else q += c; }
        return q + "'";
    };
    // -prgmode 65: load as a BASIC 65 program (RUN is typed by xemu)
    std::string cmd = quote(xemu) + " -prgmode 65 -prg " + quote(fs::absolute(programFile).string());
    if (!diskImage.empty()) cmd += " -8 " + quote(fs::absolute(diskImage).string());
    std::cout << "Launching: " << cmd << std::endl;
    int result = system(cmd.c_str());
    return result == 0;
}
