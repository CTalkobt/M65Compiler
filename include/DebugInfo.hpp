#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdint>
#include <variant>

// DWARF 4 Debug Information structures
namespace dwarf {

// DWARF Tag constants
enum class Tag : uint8_t {
    ARRAY_TYPE = 0x04,
    CLASS_TYPE = 0x05,
    ENTRY_POINT = 0x03,
    ENUMERATION_TYPE = 0x07,
    FORMAL_PARAMETER = 0x05,
    IMPORTED_DECLARATION = 0x08,
    LABEL = 0x0A,
    LEXICAL_BLOCK = 0x0B,
    MEMBER = 0x0D,
    POINTER_TYPE = 0x0F,
    REFERENCE_TYPE = 0x10,
    COMPILE_UNIT = 0x11,
    STRING_TYPE = 0x12,
    STRUCTURE_TYPE = 0x13,
    SUBROUTINE_TYPE = 0x15,
    TYPEDEF = 0x16,
    UNION_TYPE = 0x17,
    UNSPECIFIED_PARAMETERS = 0x18,
    VARIANT = 0x19,
    COMMON_BLOCK = 0x1A,
    COMMON_INCLUSION = 0x1B,
    INHERITANCE = 0x1C,
    INLINED_SUBROUTINE = 0x1D,
    MODULE = 0x1E,
    PTR_TO_MEMBER_TYPE = 0x1F,
    SET_TYPE = 0x20,
    SUBRANGE_TYPE = 0x21,
    WITH_STMT = 0x22,
    ACCESS_DECLARATION = 0x23,
    BASE_TYPE = 0x24,
    CATCH_BLOCK = 0x25,
    CONST_TYPE = 0x26,
    CONSTANT = 0x27,
    ENUMERATOR = 0x28,
    FILE_TYPE = 0x29,
    FRIEND = 0x2A,
    NAMELIST = 0x2B,
    NAMELIST_ITEM = 0x2C,
    PACKED_TYPE = 0x2D,
    SUBPROGRAM = 0x2E,
    TEMPLATE_TYPE_PARAMETER = 0x2F,
    TEMPLATE_VALUE_PARAMETER = 0x30,
    THROWN_TYPE = 0x31,
    TRY_BLOCK = 0x32,
    VARIANT_PART = 0x33,
    VARIABLE = 0x34,
    VOLATILE_TYPE = 0x35,
};

// DWARF Attribute constants
enum class Attribute : uint16_t {
    SIBLING = 0x01,
    LOCATION = 0x02,
    NAME = 0x03,
    ORDERING = 0x09,
    BYTE_SIZE = 0x0B,
    BIT_OFFSET = 0x0C,
    BIT_SIZE = 0x0D,
    STMT_LIST = 0x10,
    LOW_PC = 0x11,
    HIGH_PC = 0x12,
    LANGUAGE = 0x13,
    DISCR = 0x15,
    DISCR_VALUE = 0x16,
    VISIBILITY = 0x17,
    IMPORT = 0x18,
    STRING_LENGTH = 0x19,
    COMMON_REFERENCE = 0x1A,
    COMP_DIR = 0x1B,
    CONST_VALUE = 0x1C,
    CONTAINING_TYPE = 0x1D,
    DEFAULT_VALUE = 0x1E,
    FRIENDS = 0x1F,
    IDENTIFIER_CASE = 0x20,
    MACRO_INFO = 0x21,
    NAMELIST_ITEM = 0x22,
    PRIORITY = 0x23,
    SEGMENT = 0x24,
    SPECIFICATION = 0x25,
    STATIC_LINK = 0x26,
    TYPE = 0x27,
    USE_LOCATION = 0x28,
    VARIABLE_PARAMETER = 0x29,
    VIRTUALITY = 0x2A,
    VTABLE_ELEM_LOCATION = 0x2B,
    ALLOCATED = 0x4E,
    ASSOCIATED = 0x4F,
    DATA_LOCATION = 0x50,
    STRIDE = 0x51,
    ENTRY_PC = 0x52,
    USE_UTF8 = 0x53,
    RANGES = 0x55,
    CALL_COLUMN = 0x57,
    CALL_FILE = 0x58,
    CALL_LINE = 0x59,
    DESCRIPTION = 0x5A,
};

// DWARF Form constants
enum class Form : uint8_t {
    ADDR = 0x01,
    DATA2 = 0x05,
    DATA4 = 0x06,
    DATA8 = 0x07,
    STRING = 0x08,
    BLOCK = 0x09,
    BLOCK1 = 0x0A,
    DATA1 = 0x0B,
    FLAG = 0x0C,
    SDATA = 0x0D,
    STRP = 0x0E,
    UDATA = 0x0F,
    REF_ADDR = 0x10,
    REF1 = 0x11,
    REF2 = 0x12,
    REF4 = 0x13,
    REF8 = 0x14,
    REF_UDATA = 0x15,
    INDIRECT = 0x16,
    SEC_OFFSET = 0x17,
    EXPRLOC = 0x18,
    FLAG_PRESENT = 0x19,
    REF_BLOCK = 0x20,
};

// Attribute value (can hold different types)
struct AttributeValue {
    using Value = std::variant<uint64_t, std::string, std::vector<uint8_t>>;
    Form form;
    Value value;

    AttributeValue() : form(Form::DATA1), value(0UL) {}
    AttributeValue(Form f, uint64_t v) : form(f), value(v) {}
    AttributeValue(Form f, const std::string& v) : form(f), value(v) {}
    AttributeValue(Form f, const std::vector<uint8_t>& v) : form(f), value(v) {}
};

// Debug Information Entry (DIE)
struct DIE {
    Tag tag;
    uint64_t offset = 0;  // Offset in .debug_info section
    std::vector<std::pair<Attribute, AttributeValue>> attributes;
    std::vector<std::unique_ptr<DIE>> children;
    DIE* parent = nullptr;

    DIE(Tag t) : tag(t) {}

    // Add attribute to this DIE
    void addAttribute(Attribute attr, const AttributeValue& value) {
        attributes.emplace_back(attr, value);
    }

    // Add child DIE
    DIE* addChild(Tag tag) {
        auto child = std::make_unique<DIE>(tag);
        child->parent = this;
        auto* ptr = child.get();
        children.push_back(std::move(child));
        return ptr;
    }
};

// String pool for .debug_str section
class StringPool {
public:
    // Add string and return its offset
    uint32_t addString(const std::string& str) {
        auto it = stringMap_.find(str);
        if (it != stringMap_.end()) {
            return it->second;
        }

        uint32_t offset = currentOffset_;
        stringMap_[str] = offset;
        strings_.push_back(str);
        currentOffset_ += str.length() + 1;  // +1 for null terminator
        return offset;
    }

    // Get string offset
    uint32_t getOffset(const std::string& str) const {
        auto it = stringMap_.find(str);
        return (it != stringMap_.end()) ? it->second : 0;
    }

    // Get all strings for emission
    const std::vector<std::string>& getStrings() const { return strings_; }

    // Get total size of string pool
    uint32_t getTotalSize() const { return currentOffset_; }

private:
    std::map<std::string, uint32_t> stringMap_;
    std::vector<std::string> strings_;
    uint32_t currentOffset_ = 0;
};

// Abbreviation table entry
struct Abbreviation {
    uint8_t code = 0;
    Tag tag = Tag::ARRAY_TYPE;
    bool hasChildren = false;
    std::vector<std::pair<Attribute, Form>> attributes;

    Abbreviation() = default;
    Abbreviation(Tag t, bool children = false) : tag(t), hasChildren(children) {}

    void addAttribute(Attribute attr, Form form) {
        attributes.emplace_back(attr, form);
    }
};

// Abbreviation table
class AbbreviationTable {
public:
    // Register an abbreviation
    uint8_t registerAbbreviation(const Abbreviation& abbr) {
        uint8_t code = nextCode_++;
        abbreviations_[code] = abbr;
        abbreviations_[code].code = code;
        return code;
    }

    // Get abbreviation by code
    const Abbreviation* getAbbreviation(uint8_t code) const {
        auto it = abbreviations_.find(code);
        return (it != abbreviations_.end()) ? &it->second : nullptr;
    }

    // Get all abbreviations
    const std::map<uint8_t, Abbreviation>& getAbbreviations() const {
        return abbreviations_;
    }

private:
    std::map<uint8_t, Abbreviation> abbreviations_;
    uint8_t nextCode_ = 1;  // Start at 1; 0 is null abbreviation
};

// Source file information
struct FileEntry {
    std::string filename;
    std::string directory;
    uint32_t mod_time = 0;
    uint32_t file_size = 0;
};

// Line number program state
struct LineState {
    uint64_t address = 0;
    uint32_t file = 1;  // File index (1-based)
    uint32_t line = 1;
    uint32_t column = 0;
    bool is_stmt = true;
    bool basic_block = false;
    bool end_sequence = false;

    LineState() = default;

    void reset() {
        address = 0;
        file = 1;
        line = 1;
        column = 0;
        is_stmt = true;
        basic_block = false;
        end_sequence = false;
    }
};

}  // namespace dwarf
