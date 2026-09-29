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

    // Scan struct methods for virtual methods (stored in methods vector)
    if (!structDef.methods.empty()) {
        for (const auto& method : structDef.methods) {
            if (method->isVirtual) {
                auto& vtInfo = vtables_[structDef.name];
                vtInfo.methods.push_back(method->name);
                vtInfo.methodCounts.push_back(1); // One implementation in this struct
                recordVirtualMethod(structDef.name, method->name, method->name);
            }
        }
    }
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

    // Methods with exactly one implementation can be devirtualized
    for (const auto& method : virtualMethods_) {
        if (method.implementations.size() == 1) {
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
