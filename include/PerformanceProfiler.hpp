#pragma once
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>

// Phase 5d: Detailed performance profiling and analysis
class PerformanceProfiler {
public:
    struct FileProfile {
        std::string filename;
        std::chrono::milliseconds readTime{0};
        std::chrono::milliseconds parseTime{0};
        std::chrono::milliseconds expandTime{0};
        std::chrono::milliseconds writeTime{0};
        size_t fileSize = 0;
        bool success = true;

        std::chrono::milliseconds totalTime() const {
            return readTime + parseTime + expandTime + writeTime;
        }

        double getMBPerSecond() const {
            if (totalTime().count() == 0) return 0.0;
            return (fileSize / (1024.0 * 1024.0)) / (totalTime().count() / 1000.0);
        }
    };

private:
    std::vector<FileProfile> profiles;
    std::map<std::string, std::chrono::high_resolution_clock::time_point> timers;
    bool enableProfiling = false;

public:
    PerformanceProfiler(bool enable = false) : enableProfiling(enable) {}

    void startFile(const std::string& filename) {
        if (!enableProfiling) return;
        timers["__file_start"] = std::chrono::high_resolution_clock::now();
    }

    void startPhase(const std::string& phase) {
        if (!enableProfiling) return;
        timers["__" + phase + "_start"] = std::chrono::high_resolution_clock::now();
    }

    void endPhase(const std::string& phase, FileProfile& profile) {
        if (!enableProfiling) return;

        auto key = "__" + phase + "_start";
        if (timers.find(key) == timers.end()) return;

        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::high_resolution_clock::now() - timers[key]);

        if (phase == "read") {
            profile.readTime = duration;
        } else if (phase == "parse") {
            profile.parseTime = duration;
        } else if (phase == "expand") {
            profile.expandTime = duration;
        } else if (phase == "write") {
            profile.writeTime = duration;
        }

        timers.erase(key);
    }

    void endFile(const std::string& filename, FileProfile& profile) {
        if (!enableProfiling) return;
        profile.filename = filename;
        profiles.push_back(profile);
        timers.clear();
    }

    void printReport() const {
        if (profiles.empty()) return;

        std::cout << "\n=== Performance Profile Report ===\n";
        std::cout << std::left << std::setw(30) << "File"
                  << std::setw(10) << "Total"
                  << std::setw(10) << "Read"
                  << std::setw(10) << "Parse"
                  << std::setw(10) << "Expand"
                  << std::setw(10) << "Write"
                  << std::setw(12) << "MB/s\n";
        std::cout << std::string(92, '-') << "\n";

        for (const auto& profile : profiles) {
            std::string fname = profile.filename;
            if (fname.length() > 28) {
                fname = "..." + fname.substr(fname.length() - 25);
            }

            std::cout << std::left << std::setw(30) << fname
                      << std::setw(10) << (std::to_string(profile.totalTime().count()) + "ms")
                      << std::setw(10) << (std::to_string(profile.readTime.count()) + "ms")
                      << std::setw(10) << (std::to_string(profile.parseTime.count()) + "ms")
                      << std::setw(10) << (std::to_string(profile.expandTime.count()) + "ms")
                      << std::setw(10) << (std::to_string(profile.writeTime.count()) + "ms")
                      << std::fixed << std::setprecision(2) << std::setw(10)
                      << profile.getMBPerSecond() << " MB/s\n";
        }

        std::cout << std::string(92, '-') << "\n";

        // Totals
        auto totalRead = std::chrono::milliseconds(0);
        auto totalParse = std::chrono::milliseconds(0);
        auto totalExpand = std::chrono::milliseconds(0);
        auto totalWrite = std::chrono::milliseconds(0);
        size_t totalSize = 0;

        for (const auto& profile : profiles) {
            totalRead += profile.readTime;
            totalParse += profile.parseTime;
            totalExpand += profile.expandTime;
            totalWrite += profile.writeTime;
            totalSize += profile.fileSize;
        }

        auto grandTotal = totalRead + totalParse + totalExpand + totalWrite;
        double grandMBPerSec = (totalSize / (1024.0 * 1024.0)) / (grandTotal.count() / 1000.0);

        std::cout << std::left << std::setw(30) << "TOTAL (" + std::to_string(profiles.size()) + " files)"
                  << std::setw(10) << (std::to_string(grandTotal.count()) + "ms")
                  << std::setw(10) << (std::to_string(totalRead.count()) + "ms")
                  << std::setw(10) << (std::to_string(totalParse.count()) + "ms")
                  << std::setw(10) << (std::to_string(totalExpand.count()) + "ms")
                  << std::setw(10) << (std::to_string(totalWrite.count()) + "ms")
                  << std::fixed << std::setprecision(2) << std::setw(10)
                  << grandMBPerSec << " MB/s\n\n";

        // Bottleneck analysis
        std::cout << "Bottleneck Analysis:\n";
        auto phases = std::map<std::string, std::chrono::milliseconds>{
            {"Read", totalRead},
            {"Parse", totalParse},
            {"Expand", totalExpand},
            {"Write", totalWrite}
        };

        auto maxPhase = std::max_element(phases.begin(), phases.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        std::cout << "  Slowest phase: " << maxPhase->first << " (" << maxPhase->second.count() << "ms)\n";
        std::cout << "  Percentage: " << std::fixed << std::setprecision(1)
                  << (maxPhase->second.count() * 100.0 / grandTotal.count()) << "%\n";

        std::cout << "\n";
    }

    bool isEnabled() const { return enableProfiling; }
    size_t getProfileCount() const { return profiles.size(); }
};
