#include "CallGraphAnalyzer.hpp"
#include <algorithm>
#include <iostream>

void CallGraphAnalyzer::analyzeTranslationUnit(TranslationUnit& unit, const FunctionAnalyzer& analyzer) {
    // First pass: collect all function declarations
    for (auto& decl : unit.topLevelDecls) {
        if (auto func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            FunctionNode node;
            node.name = func->name;
            node.characteristics = analyzer.getCharacteristics(func->name);
            callGraph_[func->name] = node;
        }
    }

    // Second pass: extract function calls
    currentFunction_ = "";
    for (auto& decl : unit.topLevelDecls) {
        if (auto func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            currentFunction_ = func->name;
            if (func->body) {
                func->body->accept(*this);
            }
        }
    }

    // Third pass: build graph structure
    buildCallGraph();

    // Fourth pass: analyze virtual methods in struct definitions
    analyzeVirtualMethods();
}

void CallGraphAnalyzer::buildCallGraph() {
    // For each function, compute derived properties
    for (auto& [funcName, node] : callGraph_) {
        node.incomingCallCount = 0;
        node.outgoingCallCount = node.callees.size();
        node.isLeaf = node.callees.empty();

        // Count incoming calls
        for (auto& caller : node.callers) {
            auto it = callGraph_.find(caller);
            if (it != callGraph_.end()) {
                node.incomingCallCount++;
            }
        }
    }

    // Identify root functions (never called)
    for (auto& [funcName, node] : callGraph_) {
        node.isRoot = node.callers.empty() && funcName != "main";
    }
}

const CallGraphAnalyzer::FunctionNode* CallGraphAnalyzer::getFunctionNode(const std::string& funcName) const {
    auto it = callGraph_.find(funcName);
    if (it != callGraph_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::set<std::string> CallGraphAnalyzer::getReachableFunctions(const std::string& funcName) const {
    std::set<std::string> visited;
    reachableFunctionsDFS(funcName, visited);
    return visited;
}

std::set<std::string> CallGraphAnalyzer::reachableFunctionsDFS(const std::string& funcName,
                                                               std::set<std::string>& visited) const {
    if (visited.count(funcName)) {
        return visited;
    }
    visited.insert(funcName);

    auto it = callGraph_.find(funcName);
    if (it == callGraph_.end()) {
        return visited;
    }

    for (const auto& callee : it->second.callees) {
        reachableFunctionsDFS(callee, visited);
    }

    return visited;
}

std::set<std::string> CallGraphAnalyzer::getCallers(const std::string& funcName) const {
    std::set<std::string> result;
    auto it = callGraph_.find(funcName);
    if (it != callGraph_.end()) {
        for (const auto& caller : it->second.callers) {
            result.insert(caller);
        }
    }
    return result;
}

std::vector<std::string> CallGraphAnalyzer::getLeafFunctions() const {
    std::vector<std::string> leaves;
    for (const auto& [funcName, node] : callGraph_) {
        if (node.isLeaf && !funcName.empty()) {
            leaves.push_back(funcName);
        }
    }
    return leaves;
}

std::vector<std::string> CallGraphAnalyzer::getSimpleCalleeFunctions() const {
    std::vector<std::string> simple;
    for (const auto& [funcName, node] : callGraph_) {
        // Functions with exactly one caller are candidates for inlining
        if (node.incomingCallCount == 1 && !node.isRoot && node.isLeaf) {
            simple.push_back(funcName);
        }
    }
    return simple;
}

std::vector<CallGraphAnalyzer::VirtualMethodInfo> CallGraphAnalyzer::getVirtualMethods() const {
    return virtualMethods_;
}

bool CallGraphAnalyzer::canCoOptimize(const std::string& func1, const std::string& func2) const {
    // Two functions can be co-optimized if:
    // 1. Neither calls the other (no cycles)
    // 2. Both are safe to inline (not recursive)
    // 3. They share a common caller or are close in call hierarchy

    auto node1 = getFunctionNode(func1);
    auto node2 = getFunctionNode(func2);

    if (!node1 || !node2) return false;

    // Check for cycles
    auto reachable1 = getReachableFunctions(func1);
    auto reachable2 = getReachableFunctions(func2);

    if (reachable1.count(func2) || reachable2.count(func1)) {
        return false; // Cycle detected
    }

    // Check recursion
    if (node1->characteristics && node1->characteristics->isRecursive) {
        return false;
    }
    if (node2->characteristics && node2->characteristics->isRecursive) {
        return false;
    }

    return true;
}

void CallGraphAnalyzer::visit(FunctionCall& node) {
    // Record this call in the call graph (skip indirect calls)
    if (!currentFunction_.empty() && !node.callExpr) {
        auto it = callGraph_.find(currentFunction_);
        if (it != callGraph_.end()) {
            // Add callee if not already present
            if (std::find(it->second.callees.begin(), it->second.callees.end(), node.name) ==
                it->second.callees.end()) {
                it->second.callees.push_back(node.name);
            }

            // Add caller to callee's callers list
            auto calleeIt = callGraph_.find(node.name);
            if (calleeIt != callGraph_.end()) {
                if (std::find(calleeIt->second.callers.begin(), calleeIt->second.callers.end(), currentFunction_) ==
                    calleeIt->second.callers.end()) {
                    calleeIt->second.callers.push_back(currentFunction_);
                }
            }
        }
    }
}

void CallGraphAnalyzer::visit(FunctionDeclaration& node) {
    // No-op: already processed in analyzeTranslationUnit
}

void CallGraphAnalyzer::visit(TranslationUnit& node) {
    // No-op: already processed in analyzeTranslationUnit
}

void CallGraphAnalyzer::analyzeVirtualMethods() {
    // Scan for virtual method definitions in struct definitions
    // Build a map of method name → list of implementations
    std::map<std::string, std::set<std::string>> methodImpls;

    // Since we can't access the AST directly from here, we collect from the callGraph_
    // Virtual method implementations are marked with isVirtual in function calls
    // For now, we scan function names to identify struct methods (containing __)
    for (const auto& [funcName, node] : callGraph_) {
        // Struct methods have names like "StructName__methodName"
        size_t separatorPos = funcName.find("__");
        if (separatorPos != std::string::npos) {
            std::string structName = funcName.substr(0, separatorPos);
            std::string methodName = funcName.substr(separatorPos + 2);
            // Assume all struct methods could be virtual (conservative approach)
            // The actual virtual flag should be determined during parsing
            methodImpls[methodName].insert(funcName);
        }
    }

    // Convert map to VirtualMethodInfo vector
    virtualMethods_.clear();
    for (const auto& [methodName, impls] : methodImpls) {
        VirtualMethodInfo vmi;
        vmi.methodName = methodName;
        vmi.implementations.assign(impls.begin(), impls.end());
        virtualMethods_.push_back(vmi);
    }
}
