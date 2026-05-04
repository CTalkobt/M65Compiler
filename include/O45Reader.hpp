#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "O45Types.hpp"

// Parsed representation of a .o45 file.
struct O45File {
    // Header
    uint16_t mode = 0;
    uint8_t cpuId = 0;

    uint32_t tbase = 0, tlen = 0;
    uint32_t dbase = 0, dlen = 0;
    uint32_t bbase = 0, blen = 0;
    uint32_t zbase = 0, zlen = 0;

    // Options
    struct Option {
        uint8_t type;
        std::vector<uint8_t> data;
    };
    std::vector<Option> options;

    // Segment bodies
    std::vector<uint8_t> textBody;
    std::vector<uint8_t> dataBody;

    // Raw relocation tables (pre-decoded stream, without the $00 terminator)
    std::vector<uint8_t> textRelocs;
    std::vector<uint8_t> dataRelocs;

    // Symbol tables
    struct Import {
        std::string name;
    };
    struct Export {
        std::string name;
        uint8_t segment;    // low 7 bits = segment ID, high bit = weak flag
        uint32_t offset;
        bool hasFuncAttr = false;
        O45FuncAttr funcAttr;
        bool isWeak() const { return (segment & O45_EXPORT_WEAK) != 0; }
        uint8_t segmentId() const { return segment & O45_EXPORT_SEG_MASK; }
    };
    std::vector<Import> imports;
    std::vector<Export> exports;
};

// Reads and parses a .o45 or .o65 file from a byte vector.
// Supports both 16-bit (.o65, SIZE32 clear) and 32-bit (.o45, SIZE32 set) formats.
// Returns true on success, false on parse error (with errorMsg set).
class O45Reader {
public:
    static bool read(const std::vector<uint8_t>& data, O45File& out, std::string& errorMsg);
};
