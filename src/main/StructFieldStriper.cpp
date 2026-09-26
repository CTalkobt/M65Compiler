#include "StructFieldStriper.hpp"
#include "IR.hpp"
#include "TypeInfo.hpp"
#include <algorithm>
#include <iostream>

// Validate that all fields support striping
bool FieldStripedMetadata::validate() const {
    if (fields.empty()) {
        return false;
    }

    // Check all fields are supported
    for (const auto& field : fields) {
        if (!field.isSupported) {
            return false;
        }
        if (field.size <= 0) {
            return false;
        }
    }

    // Check totalStructSize matches sum of fields
    int expected = 0;
    for (const auto& field : fields) {
        expected += field.size;
    }
    if (totalStructSize != expected) {
        return false;
    }

    return true;
}

const StructField* FieldStripedMetadata::getField(const std::string& name) const {
    auto it = std::find_if(fields.begin(), fields.end(),
        [&name](const StructField& f) { return f.name == name; });
    return it != fields.end() ? &(*it) : nullptr;
}

int FieldStripedMetadata::calculateTotalSize(int height, int width) const {
    int total = 0;
    for (const auto& field : fields) {
        total += height * width * field.size;
    }
    return total;
}

std::unique_ptr<FieldStripedMetadata> StructFieldStriper::analyzeStruct(
    const ir::Module& mod,
    const std::string& structName
) {
    // Check cache first
    auto it = analyzedStructs.find(structName);
    if (it != analyzedStructs.end()) {
        return it->second ? std::make_unique<FieldStripedMetadata>(*it->second) : nullptr;
    }

    // Extract struct layout
    auto fields = extractStructFields(mod, structName);
    if (fields.empty()) {
        analyzedStructs[structName] = nullptr;
        return nullptr;
    }

    // Create metadata
    auto metadata = std::make_unique<FieldStripedMetadata>();
    metadata->structName = structName;
    metadata->fields = fields;

    // Calculate total struct size and field offsets
    metadata->totalStructSize = 0;
    metadata->fieldRegionOffset.clear();

    for (const auto& field : fields) {
        metadata->fieldRegionOffset.push_back(metadata->totalStructSize);
        metadata->totalStructSize += field.size;
    }

    // Validate
    if (!metadata->validate()) {
        analyzedStructs[structName] = nullptr;
        return nullptr;
    }

    // Cache and return
    auto result = std::make_unique<FieldStripedMetadata>(*metadata);
    analyzedStructs[structName] = std::move(metadata);
    return result;
}

bool StructFieldStriper::isFieldStripingSupported(
    const ir::Module& mod,
    const std::string& structName,
    std::string& reasonIfNot
) {
    auto fields = extractStructFields(mod, structName);

    if (fields.empty()) {
        reasonIfNot = "struct not found or has no fields";
        return false;
    }

    // Check each field
    for (const auto& field : fields) {
        if (field.size <= 0) {
            reasonIfNot = "field '" + field.name + "' has unknown size";
            return false;
        }

        if (!isFieldTypeSuitable(field.typeName)) {
            reasonIfNot = "field '" + field.name + "' type '" + field.typeName +
                         "' is not supported for field striping (arrays, pointers, nested structs not allowed)";
            return false;
        }

        if (!field.isSupported) {
            reasonIfNot = "field '" + field.name + "' is not suitable for field striping";
            return false;
        }
    }

    return true;
}

std::vector<StructField> StructFieldStriper::getStructLayout(
    const ir::Module& mod,
    const std::string& structName
) {
    return extractStructFields(mod, structName);
}

std::vector<StructField> StructFieldStriper::extractStructFields(
    const ir::Module& mod,
    const std::string& structName
) {
    std::vector<StructField> result;

    // This is a framework function for extracting struct fields from IR
    // In practice, field extraction is performed directly in CodeGenerator
    // during semantic analysis where struct type information is available.
    // This function remains for potential future use with cross-module analysis.
    //
    // The actual extraction happens in:
    // - CodeGenerator::visit(VariableDeclaration&) at lines 1978+
    // - Where structs map provides member info directly

    return result;
}

bool StructFieldStriper::isFieldTypeSuitable(const std::string& typeName) {
    // Simple types that are suitable for field striping
    static const std::vector<std::string> supportedTypes = {
        "char", "unsigned char", "int8_t", "uint8_t",
        "short", "unsigned short", "int16_t", "uint16_t",
        "int", "unsigned int", "int32_t", "uint32_t",
        "long", "unsigned long",
        "float"
    };

    return std::find(supportedTypes.begin(), supportedTypes.end(), typeName)
        != supportedTypes.end();
}
