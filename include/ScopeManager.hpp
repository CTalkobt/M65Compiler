#pragma once
#include <string>
#include <map>
#include <vector>
#include <deque>
#include <memory>

// Unified symbol table for CodeGenerator.
// Manages nested scopes (global, function, block) and variable lookup across scope boundaries.
class ScopeManager {
public:
    struct VarInfo {
        std::string type;
        int pointerLevel;
        bool isSigned = false;
        bool isVolatile = false;
        bool isConst = false;
        bool isPointerConst = false;
        bool isRegister = false;
        std::vector<int> arrayDims;
        bool isFunctionPointer = false;
        std::shared_ptr<void> funcPtrSig;  // opaque; holds FuncPtrSignature*

        int arraySize() const {
            if (arrayDims.empty()) return -1;
            int s = 1;
            for (int d : arrayDims) s *= d;
            return s;
        }
    };

    ScopeManager() = default;

    // Scope management
    void pushGlobalScope();       // Enter global scope (called at start of translation unit)
    void pushFunctionScope();     // Enter function scope
    void pushBlockScope();        // Enter block scope (compound statement)
    void popScope();              // Exit current scope

    // Variable declaration and lookup
    void declareVariable(const std::string& name, const VarInfo& info);
    bool isDeclared(const std::string& name) const;
    VarInfo* lookup(const std::string& name);        // Returns ptr for modification; null if not found
    const VarInfo* lookup(const std::string& name) const;
    VarInfo lookupRequired(const std::string& name); // Throws if not found

    // Special handling for mangled names (_l_, _p_, __)
    std::string resolveName(const std::string& baseName);  // Add appropriate prefix
    std::string resolveVarName(const std::string& name);   // Legacy method name

    // Enumerate all variables in current scope (for .var emission, etc.)
    std::vector<std::pair<std::string, VarInfo>> currentScopeVariables() const;

    // Clear all scopes (for new translation unit or function)
    void clear();

    // Scope depth tracking
    int depth() const { return (int)scopes_.size(); }
    bool isGlobalScope() const { return depth() == 1; }

private:
    struct Scope {
        int level;  // 0=global, 1=function, 2+=block
        std::map<std::string, VarInfo> symbols;  // Local symbols in this scope
    };

    std::deque<Scope> scopes_;  // Stack of scopes (global at [0])

    Scope& currentScope();
    const Scope& currentScope() const;
    VarInfo* lookupInScopes(const std::string& name);
    const VarInfo* lookupInScopes(const std::string& name) const;
};
