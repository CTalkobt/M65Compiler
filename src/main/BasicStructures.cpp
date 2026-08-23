#include "BasicStructures.hpp"
#include <sstream>
#include <algorithm>
#include <regex>

// ==================== StructField ====================

// ==================== StructDefinition ====================

void StructDefinition::addField(const StructField& field) {
    StructField newField = field;
    newField.offset = totalSize;
    totalSize += field.size;
    fields.push_back(newField);
}

int StructDefinition::getFieldOffset(const std::string& fieldName) const {
    for (const auto& field : fields) {
        if (field.name == fieldName) {
            return field.offset;
        }
    }
    return -1;
}

const StructField* StructDefinition::getField(const std::string& fieldName) const {
    for (const auto& field : fields) {
        if (field.name == fieldName) {
            return &field;
        }
    }
    return nullptr;
}

// ==================== ProcedureDefinition ====================

void ProcedureDefinition::addParameter(const ProcedureParameter& param) {
    parameters.push_back(param);
}

void ProcedureDefinition::addLocalVariable(const std::string& name, const std::string& type) {
    localVariables[name] = type;
}

// ==================== StructInstance ====================

void StructInstance::setFieldValue(const std::string& fieldName, int value) {
    if (!structDef) return;

    int offset = structDef->getFieldOffset(fieldName);
    if (offset < 0 || offset >= (int)data.size()) return;

    const StructField* field = structDef->getField(fieldName);
    if (!field) return;

    // Store value (little-endian for multi-byte values)
    if (field->size == 1) {
        data[offset] = value & 0xFF;
    } else if (field->size == 2) {
        data[offset] = value & 0xFF;
        if (offset + 1 < (int)data.size()) {
            data[offset + 1] = (value >> 8) & 0xFF;
        }
    } else if (field->size == 4) {
        data[offset] = value & 0xFF;
        if (offset + 1 < (int)data.size()) data[offset + 1] = (value >> 8) & 0xFF;
        if (offset + 2 < (int)data.size()) data[offset + 2] = (value >> 16) & 0xFF;
        if (offset + 3 < (int)data.size()) data[offset + 3] = (value >> 24) & 0xFF;
    }
}

int StructInstance::getFieldValue(const std::string& fieldName) const {
    if (!structDef) return 0;

    int offset = structDef->getFieldOffset(fieldName);
    if (offset < 0 || offset >= (int)data.size()) return 0;

    const StructField* field = structDef->getField(fieldName);
    if (!field) return 0;

    // Retrieve value (little-endian)
    int value = 0;
    if (field->size == 1) {
        value = data[offset];
    } else if (field->size == 2) {
        value = data[offset];
        if (offset + 1 < (int)data.size()) {
            value |= (data[offset + 1] << 8);
        }
    } else if (field->size == 4) {
        value = data[offset];
        if (offset + 1 < (int)data.size()) value |= (data[offset + 1] << 8);
        if (offset + 2 < (int)data.size()) value |= (data[offset + 2] << 16);
        if (offset + 3 < (int)data.size()) value |= (data[offset + 3] << 24);
    }
    return value;
}

// ==================== BasicLanguageTranslator ====================

void BasicLanguageTranslator::registerStruct(const StructDefinition& structDef) {
    structs[structDef.name] = structDef;
}

void BasicLanguageTranslator::registerProcedure(const ProcedureDefinition& procDef) {
    procedures[procDef.name] = procDef;
}

std::string BasicLanguageTranslator::translateStructLiteral(
    const std::string& structName,
    const std::map<std::string, int>& fieldValues
) {
    auto it = structs.find(structName);
    if (it == structs.end()) {
        return "";  // Struct not found
    }

    std::ostringstream result;
    const StructDefinition& structDef = it->second;

    // Generate DATA statements for struct initialization
    result << "REM Initialize struct " << structName << "\n";
    for (const auto& field : structDef.getFields()) {
        auto valueIt = fieldValues.find(field.name);
        int value = (valueIt != fieldValues.end()) ? valueIt->second : 0;
        result << "DATA " << value << "\n";
    }

    return result.str();
}

std::string BasicLanguageTranslator::translateProcedureCall(
    const std::string& procName,
    const std::vector<std::string>& arguments
) {
    auto it = procedures.find(procName);
    if (it == procedures.end()) {
        return "";  // Procedure not found
    }

    std::ostringstream result;
    const ProcedureDefinition& procDef = it->second;

    // Set up parameter passing (via global variables or memory)
    for (size_t i = 0; i < arguments.size() && i < procDef.parameters.size(); i++) {
        std::string paramVar = "__param_" + std::to_string(i);
        result << paramVar << " = " << arguments[i] << "\n";
    }

    // Call the procedure
    result << "GOSUB " << procName << "\n";

    // Retrieve return value if needed
    if (procDef.returnType != "VOID") {
        result << "__return = " << procName << "_result\n";
    }

    return result.str();
}

std::string BasicLanguageTranslator::translateFieldAccess(
    const std::string& instanceName,
    const std::string&
) {
    // Check if instance struct is known
    auto instanceIt = dataAddressMap.find(instanceName);
    if (instanceIt == dataAddressMap.end()) {
        return "";  // Instance not found
    }

    // Generate memory address calculation
    std::ostringstream result;
    result << "__base_addr + " << instanceIt->second;
    return result.str();
}

std::string BasicLanguageTranslator::generateProcedureDefinition(
    const ProcedureDefinition& procDef,
    const std::string& body
) {
    std::ostringstream result;

    result << procDef.name << ":\n";
    result << "REM Procedure: " << procDef.name << "\n";

    // Document parameters
    if (!procDef.parameters.empty()) {
        result << "REM Parameters:\n";
        for (const auto& param : procDef.parameters) {
            result << "REM   " << param.name << " (" << param.type;
            if (param.byRef) result << " by ref";
            result << ")\n";
        }
    }

    // Document local variables
    if (!procDef.localVariables.empty()) {
        result << "REM Local variables:\n";
        for (const auto& [name, type] : procDef.localVariables) {
            result << "REM   " << name << " (" << type << ")\n";
        }
    }

    result << body;
    result << "RETURN\n";

    return result.str();
}

bool BasicLanguageTranslator::validateFieldAccess(
    const std::string& structName,
    const std::string& fieldName
) const {
    auto it = structs.find(structName);
    if (it == structs.end()) {
        return false;
    }

    return it->second.getField(fieldName) != nullptr;
}

const StructDefinition* BasicLanguageTranslator::getStruct(const std::string& name) const {
    auto it = structs.find(name);
    if (it != structs.end()) {
        return &it->second;
    }
    return nullptr;
}

const ProcedureDefinition* BasicLanguageTranslator::getProcedure(const std::string& name) const {
    auto it = procedures.find(name);
    if (it != procedures.end()) {
        return &it->second;
    }
    return nullptr;
}

// ==================== StructProcedureParser ====================

StructDefinition StructProcedureParser::parseStructDefinition(
    const std::vector<std::string>& lines,
    int& lineIndex
) {
    std::string structName;
    std::istringstream iss(lines[lineIndex]);
    std::string keyword;
    iss >> keyword >> structName;  // STRUCT name

    StructDefinition structDef(structName);

    lineIndex++;
    while (lineIndex < (int)lines.size()) {
        const auto& line = lines[lineIndex];

        if (isStructEnd(line)) {
            break;
        }

        if (!line.empty() && line[0] != 'R') {  // Skip REM
            StructField field = parseFieldDefinition(line);
            if (!field.name.empty()) {
                structDef.addField(field);
            }
        }

        lineIndex++;
    }

    return structDef;
}

ProcedureDefinition StructProcedureParser::parseProcedureDefinition(
    const std::vector<std::string>& lines,
    int& lineIndex
) {
    std::string procName, returnType = "VOID";
    std::istringstream iss(lines[lineIndex]);
    std::string keyword;
    iss >> keyword >> procName;  // PROC name [RETURN type]

    // Check for return type
    std::string token;
    while (iss >> token) {
        if (token == "RETURN") {
            iss >> returnType;
            break;
        }
    }

    ProcedureDefinition procDef(procName, returnType);
    procDef.startLine = lineIndex;

    lineIndex++;
    while (lineIndex < (int)lines.size()) {
        const auto& line = lines[lineIndex];

        if (isProcedureEnd(line)) {
            procDef.endLine = lineIndex;
            break;
        }

        // Parse parameters and local variables
        if (line.find("PARAM") != std::string::npos) {
            ProcedureParameter param = parseParameterDefinition(line);
            procDef.addParameter(param);
        } else if (line.find("LOCAL") != std::string::npos) {
            std::istringstream lineIss(line);
            std::string localKeyword, varName, varType;
            lineIss >> localKeyword >> varName >> varType;  // LOCAL name type
            if (!varName.empty()) {
                procDef.addLocalVariable(varName, varType);
            }
        }

        lineIndex++;
    }

    return procDef;
}

bool StructProcedureParser::isStructStart(const std::string& line) {
    return line.find("STRUCT") == 0;
}

bool StructProcedureParser::isProcedureStart(const std::string& line) {
    return line.find("PROC") == 0;
}

bool StructProcedureParser::isStructEnd(const std::string& line) {
    return line.find("ENDSTRUCT") != std::string::npos;
}

bool StructProcedureParser::isProcedureEnd(const std::string& line) {
    return line.find("ENDPROC") != std::string::npos;
}

StructField StructProcedureParser::parseFieldDefinition(const std::string& line) {
    std::istringstream iss(line);
    std::string name, type, sizeStr;
    int size = 2;

    iss >> name >> type >> sizeStr;

    if (!sizeStr.empty()) {
        try {
            size = std::stoi(sizeStr);
        } catch (...) {
            size = 2;
        }
    }

    return StructField(name, type, size);
}

ProcedureParameter StructProcedureParser::parseParameterDefinition(const std::string& line) {
    std::istringstream iss(line);
    std::string paramKeyword, name, type;
    bool byRef = false;

    iss >> paramKeyword >> name >> type;  // PARAM name type [REF]

    std::string refToken;
    while (iss >> refToken) {
        if (refToken == "REF") {
            byRef = true;
            break;
        }
    }

    return ProcedureParameter(name, type, byRef);
}
