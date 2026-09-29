#include "TypeSystem.hpp"
#include <algorithm>

std::string TypeSystem::getAggregateName(const std::string& typeName) {
    if (typeName.rfind("struct ", 0) == 0) {
        return typeName.substr(7);
    }
    if (typeName.rfind("union ", 0) == 0) {
        return typeName.substr(6);
    }
    return typeName;
}

bool TypeSystem::isAggregate(const std::string& typeName) {
    return typeName.rfind("struct ", 0) == 0 || typeName.rfind("union ", 0) == 0;
}

int TypeSystem::getTypeSize(
    const std::string& typeName,
    int ptrLevel,
    const std::map<std::string, AggregateInfo>& aggregates
) {
    // Pointers are always 2 bytes (16-bit on 6502)
    if (ptrLevel > 0) {
        return 2;
    }

    // Single-byte types
    if (typeName == "char" || typeName == "unsigned char" || typeName == "_Bool") {
        return 1;
    }

    // Two-byte types (16-bit)
    if (typeName == "int" || typeName == "unsigned int" ||
        typeName == "short" || typeName == "unsigned short" ||
        typeName == "unsigned") {
        return 2;
    }

    // Four-byte types (32-bit)
    if (typeName == "long" || typeName == "unsigned long") {
        return 4;
    }

    // Floating point (40-bit Commodore format, stored as 5 bytes)
    if (typeName == "float" || typeName == "double" || typeName == "long double") {
        return 5;
    }

    // Enum types (treated as int = 2 bytes)
    if (typeName.length() >= 5 && typeName.substr(0, 5) == "enum ") {
        return 2;
    }

    // Arbitrary-width integers: __int(N) / __uint(N)
    // Extract the bit width from the type name and convert to bytes
    if (typeName.substr(0, 5) == "__int" || typeName.substr(0, 6) == "__uint") {
        size_t numStart = typeName.find_first_of("0123456789");
        if (numStart != std::string::npos) {
            int bits = std::stoi(typeName.substr(numStart));
            return (bits + 7) / 8;  // Round up to nearest byte
        }
        return 4;  // Default to 4 bytes if malformed
    }

    // Struct and union types
    if (isAggregate(typeName)) {
        std::string aggregateName = getAggregateName(typeName);
        auto it = aggregates.find(aggregateName);
        if (it != aggregates.end()) {
            return it->second.totalSize;
        }
        // If struct/union not found, throw error
        throw std::runtime_error("Unknown struct/union type: " + aggregateName);
    }

    // Unknown types default to 2 bytes (int size)
    // This handles cases where the type system isn't fully initialized
    return 2;
}
