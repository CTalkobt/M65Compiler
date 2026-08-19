#include "DevirtualizationHints.hpp"

void DevirtualizationHints::markDevirtualizable(const std::string& className,
                                                const std::string& methodName,
                                                const std::string& implementation) {
    auto key = std::make_pair(className, methodName);
    devirtualizations_[key] = implementation;

    // Also mark the mangled name as a direct call
    std::string mangled = className + "__" + methodName;
    directCalls_.insert(mangled);
}

bool DevirtualizationHints::canDevirtualize(const std::string& className,
                                           const std::string& methodName) const {
    auto key = std::make_pair(className, methodName);
    return devirtualizations_.find(key) != devirtualizations_.end();
}

std::string DevirtualizationHints::getDirectImplementation(const std::string& className,
                                                          const std::string& methodName) const {
    auto key = std::make_pair(className, methodName);
    auto it = devirtualizations_.find(key);
    if (it != devirtualizations_.end()) {
        return it->second;
    }
    return "";
}

bool DevirtualizationHints::isDirectCall(const std::string& mangledName) const {
    return directCalls_.find(mangledName) != directCalls_.end();
}
