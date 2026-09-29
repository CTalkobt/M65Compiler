#pragma once

// Phase 95: Field-level striping within struct elements
// Analyzes struct layout and prepares field-striped array metadata

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace ir {
    struct Function;
    struct Module;
}

// Information about a single field in a struct
struct StructField {
    std::string name;           // Field name (e.g., "r", "g", "b")
    int offset = 0;             // Byte offset within struct
    int size = 0;               // Field size in bytes (1, 2, 4, etc.)
    std::string typeName;       // Type name (e.g., "unsigned char", "int")
    bool isSupported = true;    // Can be field-striped (no arrays/pointers/nested structs)
};

// Metadata for field-striped struct arrays
struct FieldStripedMetadata {
    std::string structName;     // Name of the struct type
    std::vector<StructField> fields;  // Field layout
    int totalStructSize = 0;    // Sum of all field sizes
    bool isFieldStriped = true; // Will use field-level striping

    // Per-field offsets in the field-striped layout
    // fieldRegionOffset[i] = byte offset where field i's data starts
    std::vector<int> fieldRegionOffset;

    // Validate that all fields support striping
    bool validate() const;

    // Get field by name
    const StructField* getField(const std::string& name) const;

    // Get total memory needed for field-striped layout
    int calculateTotalSize(int height, int width) const;
};

// Analyzer for struct field layouts and field-striping compatibility
class StructFieldStriper {
public:
    StructFieldStriper() = default;

    // Analyze a struct type from the module's type system
    // Returns null if struct not found or not suitable for field striping
    std::unique_ptr<FieldStripedMetadata> analyzeStruct(
        const ir::Module& mod,
        const std::string& structName
    );

    // Check if a struct is suitable for field-level striping
    // Logs detailed reasons if not suitable
    bool isFieldStripingSupported(
        const ir::Module& mod,
        const std::string& structName,
        std::string& reasonIfNot
    );

    // Get the layout of a struct as a vector of fields
    // Returns empty vector if struct not found
    std::vector<StructField> getStructLayout(
        const ir::Module& mod,
        const std::string& structName
    );

private:
    // Helper: Extract field information from struct type
    std::vector<StructField> extractStructFields(
        const ir::Module& mod,
        const std::string& structName
    );

    // Helper: Check if a field type is supported for striping
    bool isFieldTypeSuitable(const std::string& typeName);

    // Cache of analyzed structs to avoid reanalysis
    std::map<std::string, std::unique_ptr<FieldStripedMetadata>> analyzedStructs;
};
