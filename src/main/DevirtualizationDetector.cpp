#include "DevirtualizationDetector.hpp"
#include <algorithm>

void DevirtualizationDetector::analyzeTranslationUnit(TranslationUnit& unit, const CallGraphAnalyzer& callGraph) {
    // First pass: collect all struct definitions
    for (auto& decl : unit.topLevelDecls) {
        if (auto structDef = dynamic_cast<StructDefinition*>(decl.get())) {
            analyzeStructForVirtualMethods(*structDef);
        }
    }

    // Second pass: count virtual call sites
    for (auto& decl : unit.topLevelDecls) {
        if (auto func = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            if (func->body) {
                func->body->accept(*this);
            }
        }
    }
}

void DevirtualizationDetector::analyzeStructForVirtualMethods(StructDefinition& structDef) {
    // Initialize vtable for this struct if not already present
    if (vtables_.find(structDef.name) == vtables_.end()) {
        VtableInfo vtInfo;
        vtInfo.className = structDef.name;
        vtables_[structDef.name] = vtInfo;
    }

    // Scan struct members for virtual methods (methods with virtual keyword)
    // In this compiler, virtual methods are defined as functions inside struct body
    // This is a simplified analysis - full OOP support would track inheritance hierarchy
}

void DevirtualizationDetector::recordVirtualMethod(const std::string& className,
                                                  const std::string& methodName,
                                                  const std::string& implementation) {
    // Check if we already have this virtual method
    auto it = std::find_if(virtualMethods_.begin(), virtualMethods_.end(),
                          [&](const VirtualMethodInfo& v) {
                              return v.className == className && v.methodName == methodName;
                          });

    if (it != virtualMethods_.end()) {
        // Add implementation if not already present
        if (std::find(it->implementations.begin(), it->implementations.end(), implementation) ==
            it->implementations.end()) {
            it->implementations.push_back(implementation);
            it->canDevirtualize = it->implementations.size() == 1;
        }
    } else {
        // Create new virtual method entry
        VirtualMethodInfo info;
        info.className = className;
        info.methodName = methodName;
        info.implementations.push_back(implementation);
        info.canDevirtualize = true;
        info.callSiteCount = 0;
        virtualMethods_.push_back(info);
    }
}

std::vector<DevirtualizationDetector::VirtualMethodInfo> DevirtualizationDetector::getDevirtualizableMethods() const {
    std::vector<VirtualMethodInfo> result;
    for (const auto& method : virtualMethods_) {
        if (method.canDevirtualize && method.implementations.size() == 1) {
            result.push_back(method);
        }
    }
    return result;
}

bool DevirtualizationDetector::canDevirtualize(const std::string& className,
                                               const std::string& methodName) const {
    auto it = std::find_if(virtualMethods_.begin(), virtualMethods_.end(),
                          [&](const VirtualMethodInfo& v) {
                              return v.className == className && v.methodName == methodName;
                          });

    if (it != virtualMethods_.end()) {
        return it->implementations.size() == 1;
    }
    return false;
}

void DevirtualizationDetector::visit(StructDefinition& node) {
    analyzeStructForVirtualMethods(node);
}

void DevirtualizationDetector::visit(FunctionDeclaration& node) {
    // No-op: virtual methods tracked separately
}

void DevirtualizationDetector::visit(FunctionCall& node) {
    // Track virtual call sites (currently simplified)
    // In a full implementation, would check if call is through vtable
}

void DevirtualizationDetector::visit(TranslationUnit& node) {
    // No-op: main analysis in analyzeTranslationUnit
}
