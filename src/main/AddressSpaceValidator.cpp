#include "AddressSpaceValidator.hpp"
#include <algorithm>
#include <sstream>

bool AddressSpaceValidator::validateVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors) {
    int space = var.addressSpace;

    switch (space) {
        case 1:  // ZP
            return validateZPVariable(var, errors);
        case 2:  // ABS
            return validateABSVariable(var, errors);
        case 3:  // FAR
            return validateFARVariable(var, errors);
        case 0:  // DEFAULT
        default:
            return true;  // No special validation for default/absolute addressing
    }
}

bool AddressSpaceValidator::validateZPVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors) {
    // ZP space constraint: total size must not exceed 256 bytes
    int totalSize = calculateVariableSize(var);

    if (totalSize > 256) {
        AddressSpaceError err;
        err.file = var.sourceFile;
        err.line = var.line;
        err.variable = var.name;

        std::ostringstream oss;
        oss << "ZP variable exceeds 256-byte limit (size: " << totalSize << " bytes)";
        err.message = oss.str();

        errors.push_back(err);
        return false;
    }

    return true;
}

bool AddressSpaceValidator::validateABSVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors) {
    // ABS space has no compile-time size constraints
    // Standard 16-bit addressing allows up to 64KB per variable
    return true;
}

bool AddressSpaceValidator::validateFARVariable(const VariableDeclaration& var, std::vector<AddressSpaceError>& errors) {
    // FAR space for cross-bank addressing
    // No compile-time constraints; bank switching handles overflow
    return true;
}

bool AddressSpaceValidator::validateStructDefinition(const StructDefinition& structDef, std::vector<AddressSpaceError>& errors) {
    // Phase 97.2+: Validate struct members with address space qualifiers
    // This is a placeholder for Phase 97.2+
    return true;
}

bool AddressSpaceValidator::canConvertPointer(int fromSpace, int toSpace) const {
    // Pointer conversion rules
    if (fromSpace == toSpace) return true;  // Same space, always OK

    // Allow safe conversions
    if (fromSpace == 1 && toSpace == 2) return true;   // ZP → ABS (safe, smaller to larger)
    if (fromSpace == 2 && toSpace == 3) return true;   // ABS → FAR (safe, single-bank to multi-bank)
    if (fromSpace == 1 && toSpace == 3) return true;   // ZP → FAR (safe, can cross banks)

    // Forbid unsafe conversions
    if (fromSpace == 2 && toSpace == 1) return false;  // ABS → ZP (unsafe, larger to smaller)
    if (fromSpace == 3 && toSpace == 1) return false;  // FAR → ZP (unsafe, multi-bank to single)
    if (fromSpace == 3 && toSpace == 2) return false;  // FAR → ABS (unsafe, multi-bank to single-bank)

    return false;
}

std::string AddressSpaceValidator::getAddressingMode(int addressSpace) const {
    switch (addressSpace) {
        case 1:  return "8-bit (zero-page)";
        case 2:  return "16-bit (absolute)";
        case 3:  return "32-bit (far/banking)";
        case 0:
        default: return "16-bit (absolute)";
    }
}

int AddressSpaceValidator::calculateVariableSize(const VariableDeclaration& var) const {
    int typeSize = getTypeSize(var.type);

    // Handle pointers
    if (var.pointerLevel > 0) {
        return 2 * var.pointerLevel;  // Each pointer level is 2 bytes
    }

    // Handle arrays
    if (!var.arrayDims.empty()) {
        int totalElements = 1;
        for (int dim : var.arrayDims) {
            if (dim > 0) totalElements *= dim;
        }
        return typeSize * totalElements;
    }

    return typeSize;
}

int AddressSpaceValidator::getTypeSize(const std::string& type) const {
    // Base type sizes (in bytes)
    if (type == "char" || type == "_Bool") return 1;
    if (type == "short") return 2;
    if (type == "int") return 2;
    if (type == "long") return 4;
    if (type == "float" || type == "double") return 5;  // CBM 40-bit float
    if (type == "struct __int64") return 8;
    if (type == "struct __int128") return 16;

    // Default to 2 bytes for unknown types (typically pointers or struct types)
    return 2;
}

std::string AddressSpaceValidator::formatError(const VariableDeclaration& var, const std::string& message) const {
    std::ostringstream oss;
    oss << var.sourceFile << ":" << var.line << ": error: " << message;
    return oss.str();
}
