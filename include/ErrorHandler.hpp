#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <map>

// Phase 5c: Comprehensive error handling and recovery
class ErrorHandler {
public:
    enum class ErrorLevel {
        INFO,
        WARNING,
        ERROR,
        FATAL
    };

    struct ErrorRecord {
        ErrorLevel level;
        std::string filename;
        std::string message;
        std::string details;
        int lineNumber = 0;
        bool recovered = false;
    };

private:
    std::vector<ErrorRecord> errors;
    bool skipOnError = false;
    bool verboseErrors = false;

public:
    ErrorHandler(bool skipOnError = false, bool verbose = false)
        : skipOnError(skipOnError), verboseErrors(verbose) {}

    void report(ErrorLevel level, const std::string& filename, const std::string& message,
                const std::string& details = "", int lineNumber = 0) {
        ErrorRecord record{level, filename, message, details, lineNumber, false};
        errors.push_back(record);

        // Print immediately if verbose
        if (verboseErrors) {
            printError(record);
        }
    }

    void reportWarning(const std::string& filename, const std::string& message) {
        report(ErrorLevel::WARNING, filename, message);
    }

    void reportError(const std::string& filename, const std::string& message,
                     const std::string& details = "") {
        report(ErrorLevel::ERROR, filename, message, details);
    }

    void reportFatal(const std::string& filename, const std::string& message,
                     const std::string& details = "") {
        report(ErrorLevel::FATAL, filename, message, details);
    }

    bool hasErrors() const {
        for (const auto& err : errors) {
            if (err.level == ErrorLevel::ERROR || err.level == ErrorLevel::FATAL) {
                return true;
            }
        }
        return false;
    }

    bool hasFatalErrors() const {
        for (const auto& err : errors) {
            if (err.level == ErrorLevel::FATAL) {
                return true;
            }
        }
        return false;
    }

    int getErrorCount() const {
        int count = 0;
        for (const auto& err : errors) {
            if (err.level == ErrorLevel::ERROR || err.level == ErrorLevel::FATAL) {
                count++;
            }
        }
        return count;
    }

    int getWarningCount() const {
        int count = 0;
        for (const auto& err : errors) {
            if (err.level == ErrorLevel::WARNING) {
                count++;
            }
        }
        return count;
    }

    void printSummary() const {
        int errors = getErrorCount();
        int warnings = getWarningCount();

        if (errors == 0 && warnings == 0) {
            std::cout << "\n✓ No errors or warnings.\n";
            return;
        }

        std::cout << "\n=== Error Summary ===\n";

        if (errors > 0) {
            std::cout << "Errors:   " << errors << "\n";
        }
        if (warnings > 0) {
            std::cout << "Warnings: " << warnings << "\n";
        }

        std::cout << "\nDetails:\n";
        for (const auto& err : errors) {
            printError(err);
        }
    }

    void printSummaryCompact() const {
        int errors = getErrorCount();
        int warnings = getWarningCount();

        if (errors > 0) {
            std::cerr << errors << " error" << (errors != 1 ? "s" : "");
        }
        if (warnings > 0) {
            if (errors > 0) std::cerr << ", ";
            std::cerr << warnings << " warning" << (warnings != 1 ? "s" : "");
        }
        if (errors > 0 || warnings > 0) {
            std::cerr << "\n";
        }
    }

    void clear() {
        errors.clear();
    }

    const std::vector<ErrorRecord>& getErrors() const {
        return errors;
    }

    bool shouldSkipOnError() const {
        return skipOnError;
    }

private:
    void printError(const ErrorRecord& err) const {
        std::string levelStr;
        std::string prefix;

        switch (err.level) {
            case ErrorLevel::INFO:
                levelStr = "INFO";
                prefix = "ℹ ";
                break;
            case ErrorLevel::WARNING:
                levelStr = "WARNING";
                prefix = "⚠ ";
                break;
            case ErrorLevel::ERROR:
                levelStr = "ERROR";
                prefix = "✗ ";
                break;
            case ErrorLevel::FATAL:
                levelStr = "FATAL";
                prefix = "✗ ";
                break;
        }

        std::cerr << prefix << err.filename;
        if (err.lineNumber > 0) {
            std::cerr << ":" << err.lineNumber;
        }
        std::cerr << ": " << levelStr << ": " << err.message;

        if (!err.details.empty() && verboseErrors) {
            std::cerr << "\n  Details: " << err.details;
        }
        std::cerr << "\n";
    }
};
