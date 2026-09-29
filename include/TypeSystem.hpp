#pragma once

#include <string>
#include <map>
#include <memory>
#include <stdexcept>

// Unified type system for size calculation across compiler phases
class TypeSystem {
public:
    // Aggregate (struct/union) information
    struct AggregateInfo {
        std::string name;
        int totalSize;
    };

    // Get the byte size of a type
    // Parameters:
    //   typeName: Type name ("int", "char", "struct Point", "unsigned long", etc.)
    //   ptrLevel: Pointer indirection level (0 for non-pointer, 1+ for pointers)
    //   aggregates: Map of struct/union names to their aggregate info
    // Returns: Size in bytes (1, 2, 4, 5 for float, etc.)
    static int getTypeSize(
        const std::string& typeName,
        int ptrLevel,
        const std::map<std::string, AggregateInfo>& aggregates = {}
    );

    // Parse "struct NameHere" or "union NameHere" to extract the aggregate name
    static std::string getAggregateName(const std::string& typeName);

    // Check if a type is a struct or union
    static bool isAggregate(const std::string& typeName);
};
