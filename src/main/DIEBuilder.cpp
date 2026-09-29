#include "DebugInfoBuilder.hpp"
#include <algorithm>

// Implementation of helper methods for DIE generation
// (Primary implementations are in DebugInfoBuilder.cpp)
// This file serves as a container for additional DIE builder utilities

// Helper: Create a struct member
dwarf::DIE* DebugInfoBuilder::createStructMember(dwarf::DIE* parent,
                                                  const std::string& name,
                                                  dwarf::DIE* type,
                                                  uint32_t byteOffset) {
    if (!parent) return nullptr;

    dwarf::DIE* member = parent->addChild(dwarf::Tag::MEMBER);

    member->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));

    if (type) {
        member->addAttribute(dwarf::Attribute::TYPE,
            dwarf::AttributeValue(dwarf::Form::REF4, 0));  // Will be fixed up
    }

    member->addAttribute(dwarf::Attribute::BYTE_SIZE,
        dwarf::AttributeValue(dwarf::Form::DATA4, (uint64_t)byteOffset));

    return member;
}

// Helper: Create an enumeration type
dwarf::DIE* DebugInfoBuilder::createEnumType(dwarf::DIE* parent,
                                             const std::string& name,
                                             uint8_t byteSize) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* enum_type = parent->addChild(dwarf::Tag::ENUMERATION_TYPE);

    enum_type->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));
    enum_type->addAttribute(dwarf::Attribute::BYTE_SIZE,
        dwarf::AttributeValue(dwarf::Form::DATA1, (uint64_t)byteSize));

    return enum_type;
}

// Helper: Create an enumerator
dwarf::DIE* DebugInfoBuilder::createEnumerator(dwarf::DIE* parent,
                                               const std::string& name,
                                               int64_t value) {
    if (!parent) return nullptr;

    dwarf::DIE* enumerator = parent->addChild(dwarf::Tag::ENUMERATOR);

    enumerator->addAttribute(dwarf::Attribute::NAME,
        dwarf::AttributeValue(dwarf::Form::STRP, name));
    enumerator->addAttribute(dwarf::Attribute::CONST_VALUE,
        dwarf::AttributeValue(dwarf::Form::SDATA, (uint64_t)value));

    return enumerator;
}

// Helper: Create a subrange type (for arrays)
dwarf::DIE* DebugInfoBuilder::createSubrangeType(dwarf::DIE* parent,
                                                 uint32_t count) {
    if (!parent) return nullptr;

    dwarf::DIE* subrange = parent->addChild(dwarf::Tag::SUBRANGE_TYPE);

    subrange->addAttribute(dwarf::Attribute::BYTE_SIZE,
        dwarf::AttributeValue(dwarf::Form::DATA4, (uint64_t)count));

    return subrange;
}

// Helper: Create a lexical block
dwarf::DIE* DebugInfoBuilder::createLexicalBlock(dwarf::DIE* parent,
                                                 uint64_t lowPC,
                                                 uint64_t highPC) {
    if (!parent) parent = compileUnit_;

    dwarf::DIE* block = parent->addChild(dwarf::Tag::LEXICAL_BLOCK);

    block->addAttribute(dwarf::Attribute::LOW_PC,
        dwarf::AttributeValue(dwarf::Form::ADDR, lowPC));
    block->addAttribute(dwarf::Attribute::HIGH_PC,
        dwarf::AttributeValue(dwarf::Form::ADDR, highPC));

    return block;
}
