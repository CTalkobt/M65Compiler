#ifndef BASICSTRUCTURES_HPP
#define BASICSTRUCTURES_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

// Struct field definition
class StructField {
public:
    StructField(const std::string& name, const std::string& type, int size = 2)
        : name(name), type(type), size(size), offset(0) {}

    std::string name;
    std::string type;  // "INT", "BYTE", "STRING", etc.
    int size;          // Size in bytes
    int offset;        // Offset in struct
};

// Struct definition
class StructDefinition {
public:
    StructDefinition() = default;
    StructDefinition(const std::string& name) : name(name), totalSize(0) {}

    void addField(const StructField& field);
    int getFieldOffset(const std::string& fieldName) const;
    const StructField* getField(const std::string& fieldName) const;
    int getTotalSize() const { return totalSize; }
    const std::vector<StructField>& getFields() const { return fields; }

    std::string name;

private:
    std::vector<StructField> fields;
    int totalSize;
};

// Procedure parameter
class ProcedureParameter {
public:
    ProcedureParameter(const std::string& name, const std::string& type, bool byRef = false)
        : name(name), type(type), byRef(byRef) {}

    std::string name;
    std::string type;
    bool byRef;  // By reference parameter
};

// Procedure definition
class ProcedureDefinition {
public:
    ProcedureDefinition() = default;
    ProcedureDefinition(const std::string& name, const std::string& returnType = "VOID")
        : name(name), returnType(returnType), startLine(0), endLine(0) {}

    void addParameter(const ProcedureParameter& param);
    void addLocalVariable(const std::string& name, const std::string& type);

    std::string name;
    std::string returnType;
    std::vector<ProcedureParameter> parameters;
    std::map<std::string, std::string> localVariables;
    int startLine;
    int endLine;
};

// Struct instance variable
class StructInstance {
public:
    StructInstance(const std::string& name, const StructDefinition* structDef)
        : name(name), structDef(structDef) {
        if (structDef) {
            data.resize(structDef->getTotalSize(), 0);
        }
    }

    void setFieldValue(const std::string& fieldName, int value);
    int getFieldValue(const std::string& fieldName) const;

    std::string name;
    const StructDefinition* structDef;
    std::vector<uint8_t> data;
};

// Language feature translator
class BasicLanguageTranslator {
public:
    BasicLanguageTranslator() = default;

    // Register struct definition
    void registerStruct(const StructDefinition& structDef);

    // Register procedure definition
    void registerProcedure(const ProcedureDefinition& procDef);

    // Translate struct literal to BASIC DATA statement
    std::string translateStructLiteral(
        const std::string& structName,
        const std::map<std::string, int>& fieldValues
    );

    // Translate procedure call to GOSUB
    std::string translateProcedureCall(
        const std::string& procName,
        const std::vector<std::string>& arguments
    );

    // Translate struct field access
    std::string translateFieldAccess(
        const std::string& instanceName,
        const std::string& fieldName
    );

    // Generate procedure definition in BASIC
    std::string generateProcedureDefinition(
        const ProcedureDefinition& procDef,
        const std::string& body
    );

    // Validate struct field access
    bool validateFieldAccess(const std::string& structName, const std::string& fieldName) const;

    // Get struct definition
    const StructDefinition* getStruct(const std::string& name) const;

    // Get procedure definition
    const ProcedureDefinition* getProcedure(const std::string& name) const;

private:
    std::map<std::string, StructDefinition> structs;
    std::map<std::string, ProcedureDefinition> procedures;

    int nextDataAddress;
    std::map<std::string, int> dataAddressMap;  // Map instance name to data address
};

// Struct and procedure parser
class StructProcedureParser {
public:
    StructProcedureParser() = default;

    // Parse STRUCT...ENDSTRUCT definition
    StructDefinition parseStructDefinition(const std::vector<std::string>& lines, int& lineIndex);

    // Parse PROC...ENDPROC definition
    ProcedureDefinition parseProcedureDefinition(const std::vector<std::string>& lines, int& lineIndex);

    // Check if line starts a struct definition
    static bool isStructStart(const std::string& line);

    // Check if line starts a procedure definition
    static bool isProcedureStart(const std::string& line);

    // Check if line ends struct definition
    static bool isStructEnd(const std::string& line);

    // Check if line ends procedure definition
    static bool isProcedureEnd(const std::string& line);

private:
    StructField parseFieldDefinition(const std::string& line);
    ProcedureParameter parseParameterDefinition(const std::string& line);
};

#endif  // BASICSTRUCTURES_HPP
