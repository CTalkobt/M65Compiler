#pragma once
#include "AsmIR.hpp"
#include <string>
#include <memory>

// Abstract base class for assembler format parsers
class AsmParser {
public:
    virtual ~AsmParser() = default;

    virtual AsmIR::Module parse(const std::string& source) = 0;

    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getWarnings() const { return warnings_; }
    bool hasErrors() const { return !errors_.empty(); }

protected:
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;

    void addError(const std::string& msg) {
        errors_.push_back(msg);
    }

    void addWarning(const std::string& msg) {
        warnings_.push_back(msg);
    }
};

// Factory function signature
using ParserFactory = AsmParser*(*)();

// Registry for available parsers
class ParserRegistry {
public:
    static ParserRegistry& getInstance();

    void registerParser(const std::string& format, ParserFactory factory);
    std::unique_ptr<AsmParser> createParser(const std::string& format);
    std::vector<std::string> getAvailableParsers() const;

private:
    std::map<std::string, ParserFactory> parsers_;

    ParserRegistry() = default;
    ParserRegistry(const ParserRegistry&) = delete;
    ParserRegistry& operator=(const ParserRegistry&) = delete;
};
