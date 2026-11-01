// ca45 as a library (#117): assemble 45GS02 source held in memory into bytes.
//
// Link with libca45.a (built by `make libca45`, installed by `make install` as
// $(PREFIX)/lib/libca45.a with this header as $(PREFIX)/include/ca45/ca45.hpp).
// The library is the assembler ca45 itself uses: the same instructions,
// addressing modes, expressions and directives. Nothing is read from or written
// to files and nothing is printed: errors come back as strings.
//
// This header uses only the C++17 standard library; the assembler's own
// classes stay internal.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ca45 {

struct AssembleOptions {
    // the address the code is assembled at (a `.org` before the source)
    uint32_t origin = 0;
    // ca45's -O level. 0 (the default) assembles each instruction as
    // written; higher levels let ca45's optimizer change and remove code.
    int optimizationLevel = 0;
    // symbols the source may use (ca45 -D NAME=VALUE), e.g. a debugger's labels
    std::map<std::string, uint32_t> symbols;
};

struct AssembleResult {
    std::vector<uint8_t> bytes;        // the code, the first byte at `origin`
    std::vector<std::string> errors;   // empty on success
    bool ok() const { return errors.empty(); }
};

// Assemble a source text (any number of lines, directives too). The bytes are
// those of a flat binary from the lowest address the source writes.
AssembleResult assemble(const std::string& source, const AssembleOptions& options = {});

// Assemble one line (an instruction, optionally after a label; a directive
// such as `.byte 1,2` works too) at `currentAddr`, which branches are relative
// to. The line may start in column 1 (`LDA #$03`): it is indented for the
// assembler. Returns the number of bytes written to `buf`, or -1 on an error
// (the line does not assemble, or its bytes do not fit in `bufsz`); `error`,
// when given, receives the message.
int assembleLine(const std::string& line, uint8_t* buf, int bufsz,
                 uint32_t currentAddr = 0, std::string* error = nullptr);

// The toolchain version the library comes from (e.g. "1.1.0.dev").
const char* version();

}  // namespace ca45
