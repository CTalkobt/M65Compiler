#include "ScopeManager.hpp"
#include <stdexcept>

void ScopeManager::pushGlobalScope() {
    if (!scopes_.empty()) throw std::runtime_error("Global scope already exists");
    scopes_.push_back({0, {}});
}

void ScopeManager::pushFunctionScope() {
    if (scopes_.empty()) throw std::runtime_error("Cannot push function scope without global scope");
    scopes_.push_back({1, {}});
}

void ScopeManager::pushBlockScope() {
    if (scopes_.empty()) throw std::runtime_error("Cannot push block scope without global scope");
    int newLevel = scopes_.back().level + 1;
    scopes_.push_back({newLevel, {}});
}

void ScopeManager::popScope() {
    if (scopes_.size() <= 1) throw std::runtime_error("Cannot pop global scope");
    scopes_.pop_back();
}

void ScopeManager::declareVariable(const std::string& name, const VarInfo& info) {
    currentScope().symbols[name] = info;
}

bool ScopeManager::isDeclared(const std::string& name) const {
    return lookupInScopes(name) != nullptr;
}

ScopeManager::VarInfo* ScopeManager::lookup(const std::string& name) {
    return lookupInScopes(name);
}

const ScopeManager::VarInfo* ScopeManager::lookup(const std::string& name) const {
    return lookupInScopes(name);
}

ScopeManager::VarInfo ScopeManager::lookupRequired(const std::string& name) {
    auto* info = lookupInScopes(name);
    if (!info) throw std::runtime_error("Variable not found: " + name);
    return *info;
}

std::string ScopeManager::resolveName(const std::string& baseName) {
    // For a bare name, add appropriate prefix and check if it exists
    std::string pName = "_p_" + baseName;
    if (isDeclared(pName)) return pName;
    std::string lName = "_l_" + baseName;
    if (isDeclared(lName)) return lName;
    std::string gName = "_" + baseName;
    if (isDeclared(gName)) return gName;
    return baseName;  // Not found, return original
}

std::string ScopeManager::resolveVarName(const std::string& name) {
    return resolveName(name);
}

std::vector<std::pair<std::string, ScopeManager::VarInfo>> ScopeManager::currentScopeVariables() const {
    std::vector<std::pair<std::string, VarInfo>> result;
    for (const auto& [name, info] : currentScope().symbols) {
        result.push_back({name, info});
    }
    return result;
}

void ScopeManager::clear() {
    scopes_.clear();
}

ScopeManager::Scope& ScopeManager::currentScope() {
    if (scopes_.empty()) throw std::runtime_error("No active scope");
    return scopes_.back();
}

const ScopeManager::Scope& ScopeManager::currentScope() const {
    if (scopes_.empty()) throw std::runtime_error("No active scope");
    return scopes_.back();
}

ScopeManager::VarInfo* ScopeManager::lookupInScopes(const std::string& name) {
    // Search from innermost (current) scope outward
    for (int i = (int)scopes_.size() - 1; i >= 0; i--) {
        auto it = scopes_[i].symbols.find(name);
        if (it != scopes_[i].symbols.end()) {
            return &it->second;
        }
    }
    return nullptr;
}

const ScopeManager::VarInfo* ScopeManager::lookupInScopes(const std::string& name) const {
    // Search from innermost (current) scope outward
    for (int i = (int)scopes_.size() - 1; i >= 0; i--) {
        auto it = scopes_[i].symbols.find(name);
        if (it != scopes_[i].symbols.end()) {
            return &it->second;
        }
    }
    return nullptr;
}
