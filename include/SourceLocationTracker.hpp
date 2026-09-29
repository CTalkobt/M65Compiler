#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Tracks source location information through compilation
class SourceLocationTracker {
public:
    struct SourceLocation {
        std::string filename;
        uint32_t line = 0;
        uint32_t column = 0;
        uint32_t endLine = 0;
        uint32_t endColumn = 0;

        SourceLocation() = default;
        SourceLocation(const std::string& file, uint32_t l, uint32_t c = 0)
            : filename(file), line(l), column(c), endLine(l), endColumn(c) {}

        bool isValid() const { return line > 0; }
    };

    struct FunctionInfo {
        std::string name;
        SourceLocation declLocation;
        SourceLocation defLocation;
        uint64_t lowPC = 0;
        uint64_t highPC = 0;
        std::vector<std::string> parameters;
    };

    struct VariableInfo {
        std::string name;
        SourceLocation location;
        std::string type;
        uint32_t frameOffset = 0;  // Stack frame offset
        bool isParameter = false;
    };

    struct ScopeInfo {
        SourceLocation entry;
        SourceLocation exit;
        std::vector<VariableInfo> variables;
        uint32_t scopeId = 0;
    };

    // Constructor
    SourceLocationTracker() = default;

    // Set current source file
    void setCurrentFile(const std::string& filename) {
        currentFile_ = filename;
    }

    // Get current source file
    const std::string& getCurrentFile() const {
        return currentFile_;
    }

    // Track function definition
    void trackFunctionDef(const std::string& name,
                        uint32_t line, uint32_t column,
                        uint64_t lowPC, uint64_t highPC) {
        FunctionInfo info;
        info.name = name;
        info.declLocation = SourceLocation(currentFile_, line, column);
        info.defLocation = SourceLocation(currentFile_, line, column);
        info.lowPC = lowPC;
        info.highPC = highPC;
        functions_[name] = info;
    }

    // Track variable declaration
    void trackVariableDecl(const std::string& varName,
                         uint32_t line, uint32_t column,
                         const std::string& type,
                         uint32_t frameOffset = 0,
                         bool isParam = false) {
        VariableInfo info;
        info.name = varName;
        info.location = SourceLocation(currentFile_, line, column);
        info.type = type;
        info.frameOffset = frameOffset;
        info.isParameter = isParam;
        variables_[varName] = info;
    }

    // Track scope entry/exit
    void enterScope(uint32_t line, uint32_t column) {
        ScopeInfo scope;
        scope.entry = SourceLocation(currentFile_, line, column);
        scope.scopeId = nextScopeId_++;
        scopeStack_.push_back(scope);
    }

    void exitScope(uint32_t line, uint32_t column) {
        if (!scopeStack_.empty()) {
            scopeStack_.back().exit = SourceLocation(currentFile_, line, column);
            scopes_.push_back(scopeStack_.back());
            scopeStack_.pop_back();
        }
    }

    // Get function info
    const FunctionInfo* getFunction(const std::string& name) const {
        auto it = functions_.find(name);
        return (it != functions_.end()) ? &it->second : nullptr;
    }

    // Get variable info
    const VariableInfo* getVariable(const std::string& name) const {
        auto it = variables_.find(name);
        return (it != variables_.end()) ? &it->second : nullptr;
    }

    // Get all functions
    const std::map<std::string, FunctionInfo>& getFunctions() const {
        return functions_;
    }

    // Get all variables
    const std::map<std::string, VariableInfo>& getVariables() const {
        return variables_;
    }

    // Get all scopes
    const std::vector<ScopeInfo>& getScopes() const {
        return scopes_;
    }

    // Map instruction address to source location
    void mapAddressToSource(uint64_t address, const SourceLocation& loc) {
        addressToSource_[address] = loc;
    }

    // Get source location for an address
    const SourceLocation* getSourceForAddress(uint64_t address) const {
        auto it = addressToSource_.find(address);
        return (it != addressToSource_.end()) ? &it->second : nullptr;
    }

    // Get address range for a source line
    std::pair<uint64_t, uint64_t> getAddressRange(const std::string& file,
                                                   uint32_t line) const {
        uint64_t minAddr = ~0UL;
        uint64_t maxAddr = 0;
        bool found = false;

        for (const auto& [addr, loc] : addressToSource_) {
            if (loc.filename == file && loc.line == line) {
                found = true;
                minAddr = std::min(minAddr, addr);
                maxAddr = std::max(maxAddr, addr);
            }
        }

        if (found) {
            return std::make_pair(minAddr, maxAddr);
        } else {
            return std::make_pair(0UL, 0UL);
        }
    }

    // Clear all tracking data
    void clear() {
        functions_.clear();
        variables_.clear();
        scopes_.clear();
        scopeStack_.clear();
        addressToSource_.clear();
        nextScopeId_ = 0;
    }

private:
    std::string currentFile_;
    std::map<std::string, FunctionInfo> functions_;
    std::map<std::string, VariableInfo> variables_;
    std::vector<ScopeInfo> scopes_;
    std::vector<ScopeInfo> scopeStack_;  // Stack for nested scopes
    std::map<uint64_t, SourceLocation> addressToSource_;  // Maps instruction addresses to source
    uint32_t nextScopeId_ = 0;
};
