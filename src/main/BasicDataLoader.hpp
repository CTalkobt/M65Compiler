#ifndef BASICDATALOADER_HPP
#define BASICDATALOADER_HPP

// basic45 --data-loader (#370): a machine-code program as a BASIC program
// that POKEs it from DATA lines and SYSes it (a type-in listing)

#include <cstdint>
#include <string>
#include <vector>

struct DataLoaderOptions {
    bool isPrg = true;          // the input starts with its load address
    int32_t loadAt = -1;        // --load-at: where the code goes (-1: the .prg's address)
    int start = 10;             // the first line number
    int increment = 10;         // between line numbers
    int perLine = 8;            // bytes per DATA line
    bool hex = false;           // DATA items in hex (read as strings, DEC())
    bool checksum = false;      // a checksum per DATA line, checked while loading
    bool sys = true;            // SYS the code after loading it
    std::string name;           // for the REM line
};

struct DataLoaderResult {
    bool ok = false;
    std::string error;
    std::string source;         // the loader: a numbered basic45 program
    uint16_t address = 0;       // where the code goes
    uint32_t count = 0;         // its bytes
};

DataLoaderResult makeDataLoader(const std::vector<uint8_t>& input, const DataLoaderOptions& o);

#endif  // BASICDATALOADER_HPP
