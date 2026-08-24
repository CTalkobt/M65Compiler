#pragma once

#include "DebugInfo.hpp"
#include <string>
#include <vector>
#include <memory>

// Builds DWARF 4 debug information
class DebugInfoBuilder {
public:
    DebugInfoBuilder();

    // Create a new compilation unit
    dwarf::DIE* createCompileUnit(const std::string& filename,
                                   const std::string& directory,
                                   const std::string& producer = "cc45");

    // Create a subprogram (function) DIE
    dwarf::DIE* createSubprogram(dwarf::DIE* parent,
                                  const std::string& name,
                                  uint64_t lowPC,
                                  uint64_t highPC);

    // Create a variable DIE
    dwarf::DIE* createVariable(dwarf::DIE* parent,
                               const std::string& name,
                               const std::string& type,
                               uint32_t location_offset = 0);

    // Create a formal parameter DIE
    dwarf::DIE* createFormalParameter(dwarf::DIE* parent,
                                      const std::string& name,
                                      const std::string& type);

    // Create a base type DIE (int, char, etc.)
    dwarf::DIE* createBaseType(dwarf::DIE* parent,
                               const std::string& name,
                               uint8_t byte_size,
                               uint8_t encoding);

    // Create a pointer type DIE
    dwarf::DIE* createPointerType(dwarf::DIE* parent,
                                  dwarf::DIE* pointee_type,
                                  uint8_t size = 2);

    // Create an array type DIE
    dwarf::DIE* createArrayType(dwarf::DIE* parent,
                                dwarf::DIE* element_type,
                                uint32_t count);

    // Create a struct/union type DIE
    dwarf::DIE* createStructType(dwarf::DIE* parent,
                                 const std::string& name,
                                 uint32_t byte_size,
                                 bool is_struct = true);

    // Create a typedef DIE
    dwarf::DIE* createTypedef(dwarf::DIE* parent,
                              const std::string& name,
                              dwarf::DIE* base_type);

    // Add file to line number program
    uint32_t addFile(const std::string& filename,
                    const std::string& directory = "");

    // Get compile unit DIE
    dwarf::DIE* getCompileUnit() const { return compileUnit_; }

    // Get root DIE (for traversal)
    dwarf::DIE* getRootDIE() const { return root_.get(); }

    // Get string pool
    dwarf::StringPool& getStringPool() { return stringPool_; }
    const dwarf::StringPool& getStringPool() const { return stringPool_; }

    // Get abbreviation table
    dwarf::AbbreviationTable& getAbbreviationTable() { return abbrevTable_; }
    const dwarf::AbbreviationTable& getAbbreviationTable() const { return abbrevTable_; }

    // Get file entries for .debug_line
    const std::vector<dwarf::FileEntry>& getFileEntries() const { return files_; }

    // Set producer string
    void setProducer(const std::string& producer) { producer_ = producer; }

    // Set compilation directory
    void setCompilationDirectory(const std::string& dir) { compDir_ = dir; }

    // Set language
    void setLanguage(uint32_t language) { language_ = language; }

private:
    // Helper to create common DIE attributes
    void addCommonDIEAttributes(dwarf::DIE* die, const std::string& name);
    void addLocationAttribute(dwarf::DIE* die, uint32_t offset);
    void addTypeAttribute(dwarf::DIE* die, dwarf::DIE* type);
    void addAddressAttribute(dwarf::DIE* die, uint64_t address);

    // Get or create standard abbreviations
    uint8_t getOrCreateAbbreviation(dwarf::Tag tag, bool hasChildren,
                                    const std::vector<std::pair<dwarf::Attribute, dwarf::Form>>& attrs);

    std::unique_ptr<dwarf::DIE> root_;
    dwarf::DIE* compileUnit_ = nullptr;
    dwarf::StringPool stringPool_;
    dwarf::AbbreviationTable abbrevTable_;

    std::string producer_ = "cc45";
    std::string compDir_;
    uint32_t language_ = 4;  // DW_LANG_C

    std::vector<dwarf::FileEntry> files_;
    std::map<std::string, uint32_t> fileMap_;  // filename -> index
};
