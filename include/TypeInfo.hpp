#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

// Forward declaration to avoid circular dependency
struct FuncPtrSignature;

// Lightweight type utilities used by constant folder and other optimization passes.
// This extracted the essential type-related data structures from the legacy CodeGenerator.

class TypeInfo {
public:
    struct MemberInfo {
        std::string type;
        int pointerLevel;
        bool isSigned = false;
        bool isConst = false;
        int offset;
        int alignment = 1;
        std::vector<int> arrayDims;
        int arraySize() const {
            if (arrayDims.empty()) return -1;
            int s = 1;
            for (int d : arrayDims) s *= d;
            return s;
        }
        int bitWidth = 0;   // 0 = not a bitfield; >0 = bitfield width in bits
        int bitOffset = 0;  // bit offset within the storage unit
    };

    struct StructInfo {
        std::string name;
        std::map<std::string, MemberInfo> members;
        int totalSize;
        int alignment = 1;
    };

    struct VarInfo {
        std::string type;
        int pointerLevel;
        bool isSigned = false;
        bool isVolatile = false;
        bool isConst = false;
        bool isPointerConst = false;
        bool isRegister = false;
        std::vector<int> arrayDims;
        int arraySize() const {
            if (arrayDims.empty()) return -1;
            int s = 1;
            for (int d : arrayDims) s *= d;
            return s;
        }
        bool isFunctionPointer = false;
        std::shared_ptr<FuncPtrSignature> funcPtrSig;
    };

    // Compute byte size of a type given its components
    static int getTypeSize(const std::string& type, int ptrLevel, int arraySize,
                          const std::map<std::string, std::shared_ptr<StructInfo>>& structs);

    static bool is8BitType(const std::string& type) {
        return type == "char" || type == "_Bool";
    }

    static bool is32BitType(const std::string& type) {
        return type == "long";
    }
};
