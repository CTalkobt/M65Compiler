#pragma once
#include <vector>
#include <string>
#include <set>
#include <map>
#include <cstdint>
#include "O45Types.hpp"
#include "O45Reader.hpp"
#include "O45Writer.hpp"
#include "O45Archive.hpp"

// Decodes a raw relocation byte stream (as stored in O45File::textRelocs/dataRelocs)
// back into a list of high-level O45Reloc entries.
class O45RelocDecoder {
public:
    static std::vector<O45Reloc> decode(const std::vector<uint8_t>& raw);
};

// Linker for .o45 relocatable object files.
//
// Usage:
//   O45Linker linker;
//   linker.addObject("main.o45", mainObj);
//   linker.addObject("math.o45", mathObj);
//   linker.setTextBase(0x2000);
//   std::string err;
//   auto binary = linker.link(err);
//
class O45Linker {
public:
    // Add an object file to the link. Order matters for segment layout.
    void addObject(const std::string& filename, const O45File& obj);

    // Add a library archive. Members are selectively pulled in to resolve
    // undefined symbols. May be called multiple times; resolution iterates
    // until no new symbols are satisfied.
    void addLibrary(const std::string& filename, const Ar45Archive& lib);

    // Set the base addresses for merged segments.
    // If not set, text starts at 0x0000.
    void setTextBase(uint32_t addr) { textBase_ = addr; }
    void setDataBase(uint32_t addr) { dataBase_ = addr; dataBaseSet_ = true; }
    void setBssBase(uint32_t addr)  { bssBase_ = addr; bssBaseSet_ = true; }
    void setZpBase(uint32_t addr)   { zpBase_ = addr; zpBaseSet_ = true; }

    // Link all objects and return the final binary.
    // Returns empty vector on error (with errorMsg set).
    // If isPrg is true, prepends a 2-byte load address header.
    std::vector<uint8_t> link(std::string& errorMsg, bool isPrg = false);

    // After a successful link, retrieve the global symbol map (name -> final address).
    const std::map<std::string, uint32_t>& getSymbolMap() const { return globalSymbols_; }

private:
    struct InputObject {
        std::string filename;
        O45File obj;
        // Per-object offsets within the merged segments
        uint32_t textOffset = 0;
        uint32_t dataOffset = 0;
        uint32_t bssOffset = 0;
        uint32_t zpOffset = 0;
    };

    std::vector<InputObject> objects_;

    struct LibraryEntry {
        std::string filename;
        Ar45Archive lib;
    };
    std::vector<LibraryEntry> libraries_;

    uint32_t textBase_ = 0x0000;
    uint32_t dataBase_ = 0;  bool dataBaseSet_ = false;
    uint32_t bssBase_ = 0;   bool bssBaseSet_ = false;
    uint32_t zpBase_ = 0;    bool zpBaseSet_ = false;

    // Merged segment bodies
    std::vector<uint8_t> mergedText_;
    std::vector<uint8_t> mergedData_;
    uint32_t mergedBssLen_ = 0;
    uint32_t mergedZpLen_ = 0;

    // Global symbol table: name -> final absolute address
    std::map<std::string, uint32_t> globalSymbols_;
    // Track which file defined each symbol (for error messages)
    std::map<std::string, std::string> symbolSource_;

    bool resolveLibraries(std::string& errorMsg);
    bool layoutSegments(std::string& errorMsg);
    bool resolveSymbols(std::string& errorMsg);
    bool applyRelocations(std::string& errorMsg);

    // Get the final base address for a segment ID
    uint32_t segmentBase(O45Segment seg) const;

    // Apply relocations from one object's reloc table to a merged segment body
    bool applyRelocs(const std::vector<O45Reloc>& relocs,
                     std::vector<uint8_t>& body,
                     uint32_t bodyBase,
                     uint32_t objOffset,
                     const InputObject& input,
                     std::string& errorMsg);
};
