#pragma once

#include <map>
#include <set>
#include <string>

// Phase 87: Devirtualization hints for code generation
// Maps virtual method calls to direct implementations when safe

class DevirtualizationHints {
public:
    // Record a virtual method that can be safely devirtualized
    void markDevirtualizable(const std::string& className,
                            const std::string& methodName,
                            const std::string& implementation);

    // Check if a virtual method call can be devirtualized
    bool canDevirtualize(const std::string& className,
                        const std::string& methodName) const;

    // Get the direct implementation for a devirtualizable method
    std::string getDirectImplementation(const std::string& className,
                                       const std::string& methodName) const;

    // Check if a method is marked for direct call
    bool isDirectCall(const std::string& mangledName) const;

private:
    // Map of (className, methodName) -> implementation
    std::map<std::pair<std::string, std::string>, std::string> devirtualizations_;

    // Set of mangled function names that should be called directly
    std::set<std::string> directCalls_;
};
