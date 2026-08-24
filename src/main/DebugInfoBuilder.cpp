#include "DebugInfoBuilder.hpp"

DebugInfoBuilder::DebugInfoBuilder() {
    // Create root DIE (will be filled with compile unit)
    root_ = std::make_unique<dwarf::DIE>(dwarf::Tag::COMPILE_UNIT);

    // Pre-register common abbreviations for better compression
    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> compileUnitAttrs = {
        {dwarf::Attribute::SPECIFICATION, dwarf::Form::STRP},
        {dwarf::Attribute::LANGUAGE, dwarf::Form::DATA1},
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::COMP_DIR, dwarf::Form::STRP},
    };
    getOrCreateAbbreviation(dwarf::Tag::COMPILE_UNIT, true, compileUnitAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> subprogramAttrs = {
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::LOW_PC, dwarf::Form::ADDR},
        {dwarf::Attribute::HIGH_PC, dwarf::Form::ADDR},
        {dwarf::Attribute::LANGUAGE, dwarf::Form::DATA1},
    };
    getOrCreateAbbreviation(dwarf::Tag::SUBPROGRAM, true, subprogramAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> variableAttrs = {
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::TYPE, dwarf::Form::REF4},
        {dwarf::Attribute::LOCATION, dwarf::Form::SEC_OFFSET},
    };
    getOrCreateAbbreviation(dwarf::Tag::VARIABLE, false, variableAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> parameterAttrs = {
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::TYPE, dwarf::Form::REF4},
        {dwarf::Attribute::LOCATION, dwarf::Form::SEC_OFFSET},
    };
    getOrCreateAbbreviation(dwarf::Tag::FORMAL_PARAMETER, false, parameterAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> baseTypeAttrs = {
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::BYTE_SIZE, dwarf::Form::DATA1},
    };
    getOrCreateAbbreviation(dwarf::Tag::BASE_TYPE, false, baseTypeAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> pointerTypeAttrs = {
        {dwarf::Attribute::BYTE_SIZE, dwarf::Form::DATA1},
        {dwarf::Attribute::TYPE, dwarf::Form::REF4},
    };
    getOrCreateAbbreviation(dwarf::Tag::POINTER_TYPE, false, pointerTypeAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> arrayTypeAttrs = {
        {dwarf::Attribute::TYPE, dwarf::Form::REF4},
    };
    getOrCreateAbbreviation(dwarf::Tag::ARRAY_TYPE, true, arrayTypeAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> structTypeAttrs = {
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::BYTE_SIZE, dwarf::Form::DATA4},
    };
    getOrCreateAbbreviation(dwarf::Tag::STRUCTURE_TYPE, true, structTypeAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> unionTypeAttrs = {
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::BYTE_SIZE, dwarf::Form::DATA4},
    };
    getOrCreateAbbreviation(dwarf::Tag::UNION_TYPE, true, unionTypeAttrs);

    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> typedefAttrs = {
        {dwarf::Attribute::NAME, dwarf::Form::STRP},
        {dwarf::Attribute::TYPE, dwarf::Form::REF4},
    };
    getOrCreateAbbreviation(dwarf::Tag::TYPEDEF, false, typedefAttrs);
}

dwarf::DIE* DebugInfoBuilder::createCompileUnit(const std::string& filename,
                                                const std::string& directory,
                                                const std::string& producer) {
    compileUnit_ = root_.get();
    setProducer(producer);
    setCompilationDirectory(directory);

    // Add required attributes (using SPECIFICATION for producer string temporarily)
    compileUnit_->addAttribute(dwarf::Attribute::SPECIFICATION,
        dwarf::AttributeValue(dwarf::Form::STRP, producer));
    compileUnit_->addAttribute(dwarf::Attribute::LANGUAGE,
        dwarf::AttributeValue(dwarf::Form::DATA1, (uint64_t)language_));
    compileUnit_->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, filename));
    compileUnit_->addAttribute(dwarf::Attribute::COMP_DIR,
        dwarf::AttributeValue(dwarf::Form::STRP, directory));

    // Add file entry
    addFile(filename, directory);

    return compileUnit_;
}

dwarf::DIE* DebugInfoBuilder::createSubprogram(dwarf::DIE* parent,
                                               const std::string& name,
                                               uint64_t lowPC,
                                               uint64_t highPC) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* subprog = parent->addChild(dwarf::Tag::SUBPROGRAM);

    // Add attributes
    subprog->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));
    subprog->addAttribute(dwarf::Attribute::LOW_PC,
        dwarf::AttributeValue(dwarf::Form::ADDR, lowPC));
    subprog->addAttribute(dwarf::Attribute::HIGH_PC,
        dwarf::AttributeValue(dwarf::Form::ADDR, highPC));
    subprog->addAttribute(dwarf::Attribute::LANGUAGE,
        dwarf::AttributeValue(dwarf::Form::DATA1, (uint64_t)language_));

    return subprog;
}

dwarf::DIE* DebugInfoBuilder::createVariable(dwarf::DIE* parent,
                                             const std::string& name,
                                             const std::string& type,
                                             uint32_t location_offset) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* var = parent->addChild(dwarf::Tag::VARIABLE);

    var->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));

    if (location_offset > 0) {
        var->addAttribute(dwarf::Attribute::LOCATION,
            dwarf::AttributeValue(dwarf::Form::SEC_OFFSET, (uint64_t)location_offset));
    }

    return var;
}

dwarf::DIE* DebugInfoBuilder::createFormalParameter(dwarf::DIE* parent,
                                                    const std::string& name,
                                                    const std::string& type) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* param = parent->addChild(dwarf::Tag::FORMAL_PARAMETER);

    param->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));

    return param;
}

dwarf::DIE* DebugInfoBuilder::createBaseType(dwarf::DIE* parent,
                                             const std::string& name,
                                             uint8_t byte_size,
                                             uint8_t encoding) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* type = parent->addChild(dwarf::Tag::BASE_TYPE);

    type->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));
    type->addAttribute(dwarf::Attribute::BYTE_SIZE,
        dwarf::AttributeValue(dwarf::Form::DATA1, (uint64_t)byte_size));

    return type;
}

dwarf::DIE* DebugInfoBuilder::createPointerType(dwarf::DIE* parent,
                                                dwarf::DIE* pointee_type,
                                                uint8_t size) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* ptr_type = parent->addChild(dwarf::Tag::POINTER_TYPE);

    ptr_type->addAttribute(dwarf::Attribute::BYTE_SIZE,
        dwarf::AttributeValue(dwarf::Form::DATA1, (uint64_t)size));

    if (pointee_type) {
        ptr_type->addAttribute(dwarf::Attribute::TYPE,
            dwarf::AttributeValue(dwarf::Form::REF4, 0));  // Will be fixed up
    }

    return ptr_type;
}

dwarf::DIE* DebugInfoBuilder::createArrayType(dwarf::DIE* parent,
                                              dwarf::DIE* element_type,
                                              uint32_t count) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* array = parent->addChild(dwarf::Tag::ARRAY_TYPE);

    if (element_type) {
        array->addAttribute(dwarf::Attribute::TYPE,
            dwarf::AttributeValue(dwarf::Form::REF4, 0));  // Will be fixed up
    }

    // Add subrange DIE for array bounds
    dwarf::DIE* subrange = array->addChild(dwarf::Tag::SUBRANGE_TYPE);
    subrange->addAttribute(dwarf::Attribute::BYTE_SIZE,
        dwarf::AttributeValue(dwarf::Form::DATA4, (uint64_t)count));

    return array;
}

dwarf::DIE* DebugInfoBuilder::createStructType(dwarf::DIE* parent,
                                               const std::string& name,
                                               uint32_t byte_size,
                                               bool is_struct) {
    if (!parent) parent = compileUnit_;

    dwarf::Tag tag = is_struct ? dwarf::Tag::STRUCTURE_TYPE : dwarf::Tag::UNION_TYPE;
    dwarf::DIE* struct_type = parent->addChild(tag);

    struct_type->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));
    struct_type->addAttribute(dwarf::Attribute::BYTE_SIZE,
        dwarf::AttributeValue(dwarf::Form::DATA4, (uint64_t)byte_size));

    return struct_type;
}

dwarf::DIE* DebugInfoBuilder::createTypedef(dwarf::DIE* parent,
                                            const std::string& name,
                                            dwarf::DIE* base_type) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* typedef_die = parent->addChild(dwarf::Tag::TYPEDEF);

    typedef_die->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));

    if (base_type) {
        typedef_die->addAttribute(dwarf::Attribute::TYPE,
            dwarf::AttributeValue(dwarf::Form::REF4, 0));  // Will be fixed up
    }

    return typedef_die;
}

uint32_t DebugInfoBuilder::addFile(const std::string& filename,
                                   const std::string& directory) {
    auto it = fileMap_.find(filename);
    if (it != fileMap_.end()) {
        return it->second;
    }

    uint32_t index = files_.size() + 1;  // DWARF uses 1-based file indices
    fileMap_[filename] = index;

    dwarf::FileEntry entry;
    entry.filename = filename;
    entry.directory = directory;
    files_.push_back(entry);

    return index;
}

void DebugInfoBuilder::addCommonDIEAttributes(dwarf::DIE* die, const std::string& name) {
    if (!name.empty()) {
        die->addAttribute(dwarf::Attribute::NAME,
            dwarf::AttributeValue(dwarf::Form::STRP, name));
    }
}

void DebugInfoBuilder::addLocationAttribute(dwarf::DIE* die, uint32_t offset) {
    if (offset > 0) {
        die->addAttribute(dwarf::Attribute::LOCATION,
            dwarf::AttributeValue(dwarf::Form::SEC_OFFSET, (uint64_t)offset));
    }
}

void DebugInfoBuilder::addTypeAttribute(dwarf::DIE* die, dwarf::DIE* type) {
    if (type) {
        die->addAttribute(dwarf::Attribute::TYPE,
            dwarf::AttributeValue(dwarf::Form::REF4, 0));  // Will be fixed up
    }
}

void DebugInfoBuilder::addAddressAttribute(dwarf::DIE* die, uint64_t address) {
    die->addAttribute(dwarf::Attribute::LOW_PC,
        dwarf::AttributeValue(dwarf::Form::ADDR, address));
}

uint8_t DebugInfoBuilder::getOrCreateAbbreviation(
    dwarf::Tag tag,
    bool hasChildren,
    const std::vector<std::pair<dwarf::Attribute, dwarf::Form>>& attrs) {

    // Convert to vector for comparison
    std::vector<std::pair<dwarf::Attribute, dwarf::Form>> attrVec(attrs);

    // Check if we already have this pattern
    for (const auto& [code, abbr] : abbrevTable_.getAbbreviations()) {
        if (abbr.tag == tag && abbr.hasChildren == hasChildren &&
            abbr.attributes.size() == attrVec.size()) {
            bool matches = true;
            for (size_t i = 0; i < attrVec.size(); i++) {
                if (abbr.attributes[i] != attrVec[i]) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return code;
            }
        }
    }

    // Create new abbreviation
    dwarf::Abbreviation abbr(tag, hasChildren);
    for (const auto& [attr, form] : attrVec) {
        abbr.addAttribute(attr, form);
    }

    return abbrevTable_.registerAbbreviation(abbr);
}
