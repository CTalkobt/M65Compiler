#pragma once

#include "AST.hpp"
#include "TypeInfo.hpp"
#include <string>
#include <vector>
#include <map>

// Phase 97.2: Address Space Validation
// Validates address space qualifiers during compilation
// AddressSpace enum defined in TypeInfo.hpp

struct AddressSpaceError {
    std::string file;
    int line;
    std::string message;
    std::string variable;
};

class AddressSpaceValidator {
public:
    AddressSpaceValidator() = default;
    ~AddressSpaceValidator() = default;

    // Validate a variable declaration
    bool validateVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors);

    // Validate a struct definition for address space usage in members
    bool validateStructDefinition(const StructDefinition& structDef, std::vector<AddressSpaceError>& errors);

    // Check if pointer conversion is allowed between address spaces
    bool canConvertPointer(int fromSpace, int toSpace) const;

    // Get addressing mode for a variable based on address space
    std::string getAddressingMode(int addressSpace) const;

private:
    // Validation helpers
    bool validateZPVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors);
    bool validateABSVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors);
    bool validateFARVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors);

    // Size calculation
    int calculateVariableSize(const VariableDeclaration& var) const;
    int getTypeSize(const std::string& type) const;

    // Error message generation
    std::string formatError(const VariableDeclaration& var, const std::string& message) const;
};
