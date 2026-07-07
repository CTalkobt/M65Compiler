#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>

// Minimal CodeGenerator header - contains only type definitions
// The CodeGenerator visitor implementation has been removed and replaced by IRBuilder
// These types are retained for use by ConstantFolder

// Forward declare FuncPtrSignature
struct FuncPtrSignature;

// Type metadata for variables
struct CodeGeneratorVarInfo {
    std::string type;
    int pointerLevel = 0;
    bool isSigned = false;
    bool isVolatile = false;
    bool isConst = false;         // base type is const (prevents *p = x)
    bool isPointerConst = false;  // pointer itself is const (prevents p = x)
    bool isRegister = false;      // allocated in zero page
    std::vector<int> arrayDims;   // empty = not array; {3,4} = int[3][4]
    int arraySize() const {
        if (arrayDims.empty()) return -1;
        int s = 1;
        for (int d : arrayDims) s *= d;
        return s;
    }
    bool isFunctionPointer = false;
    std::shared_ptr<FuncPtrSignature> funcPtrSig;
};

// Member information for struct members
struct CodeGeneratorMemberInfo {
    std::string type;
    int pointerLevel = 0;
    bool isConst = false;
    bool isSigned = false;
    int alignment = 1;
    std::vector<int> arrayDims;
    int offset = 0;
    int size = 0;
};

// Struct metadata
struct CodeGeneratorStructInfo {
    std::string name;
    std::map<std::string, CodeGeneratorMemberInfo> members;
    int totalSize = 0;
    int alignment = 1;
};

// Namespace wrapper for compatibility with existing code
namespace CodeGenerator {
    using VarInfo = CodeGeneratorVarInfo;
    using MemberInfo = CodeGeneratorMemberInfo;
    using StructInfo = CodeGeneratorStructInfo;
}
