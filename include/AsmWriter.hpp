#pragma once
#include "AsmIR.hpp"
#include <string>
#include <memory>

// Abstract base class for assembler format writers
class AsmWriter {
public:
    virtual ~AsmWriter() = default;

    virtual std::string write(const AsmIR::Module& module) = 0;

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
using WriterFactory = AsmWriter*(*)();

// Registry for available writers
class WriterRegistry {
public:
    static WriterRegistry& getInstance();

    void registerWriter(const std::string& format, WriterFactory factory);
    std::unique_ptr<AsmWriter> createWriter(const std::string& format);
    std::vector<std::string> getAvailableWriters() const;

private:
    std::map<std::string, WriterFactory> writers_;

    WriterRegistry() = default;
    WriterRegistry(const WriterRegistry&) = delete;
    WriterRegistry& operator=(const WriterRegistry&) = delete;
};
