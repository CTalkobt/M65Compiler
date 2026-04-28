#pragma once
#include <vector>
#include <string>
#include <map>
#include <cstdint>

// =============================================================================
// .lib Archive Format for .o45 Object Files
//
// Layout:
//   magic         4 bytes   "a45\0"
//   version       1 byte    $01
//   member_count  4 bytes   LE
//   symbol_count  4 bytes   LE
//   [symbol index: symbol_count entries]
//     name        NUL-terminated
//     member_idx  4 bytes LE  (index into member list)
//   [members: member_count entries]
//     name_len    2 bytes LE
//     name        name_len bytes (original filename)
//     data_len    4 bytes LE
//     data        data_len bytes (.o45 file content)
// =============================================================================

constexpr uint8_t  AR45_MAGIC[4] = { 'a', '4', '5', 0x00 };
constexpr uint8_t  AR45_VERSION  = 0x01;

struct Ar45Member {
    std::string name;             // member name (filename)
    std::vector<uint8_t> data;    // raw .o45 file content
};

struct Ar45SymbolEntry {
    std::string name;             // exported symbol name
    uint32_t memberIndex;         // index of the member that defines it
};

struct Ar45Archive {
    std::vector<Ar45Member> members;
    std::vector<Ar45SymbolEntry> symbols;  // global symbol index
};

// Read/write .lib archive files.
class Ar45Writer {
public:
    // Build an archive from a list of members.
    // Automatically scans each member's .o45 exports to build the symbol index.
    // Returns the archive as a byte vector.
    static std::vector<uint8_t> write(const Ar45Archive& archive);
};

class Ar45Reader {
public:
    // Parse a .lib archive from a byte vector.
    static bool read(const std::vector<uint8_t>& data, Ar45Archive& out, std::string& errorMsg);
};
