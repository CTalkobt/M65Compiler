#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "AssemblerToken.hpp"

// Markers inside STRING_LITERAL token values. Source strings never contain
// these control bytes raw; escapes are encoded with them by lexString.
namespace asmstr {
constexpr char PREFIX  = '\x01';  // at start, followed by 's' or 'p' (s"..." / p"...")
constexpr char EXACT   = '\x02';  // next char is an exact byte (\xHH, octal, \t, \", \\ ...)
constexpr char NEWLINE = '\x03';  // \n: $0D in .text/.screencode, $0A in .ascii
constexpr char RETURN  = '\x04';  // \r: $0A in .text/.screencode, $0D in .ascii

// Number of bytes a string token value assembles to
inline int byteLength(const std::string& v) {
    size_t i = 0;
    if (v.size() >= 2 && v[0] == PREFIX) i = 2;
    int n = 0;
    for (; i < v.size(); ++i, ++n) if (v[i] == EXACT) ++i;
    return n;
}
inline uint8_t toPetscii(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    if (c >= 'A' && c <= 'Z') return c + 128;   // $C1-$DA, as BASIC's ASC("A")
    return (uint8_t)c;
}

inline uint8_t toScreencode(char c) {
    uint8_t u = (uint8_t)c;
    if (u == 0x40) return 0x00;             // @ → 0
    if (u >= 0x41 && u <= 0x5A) return u - 0x40;  // A-Z → 1-26
    if (u >= 0x5B && u <= 0x5F) return u - 0x40;  // [\]^_ → 27-31
    if (u >= 0x61 && u <= 0x7A) return u - 0x60;  // a-z → 1-26
    return u;                               // space, digits, punctuation unchanged
}

// The bytes .text (PETSCII), .screencode or .ascii (`directive`) assemble a
// string token value to (its s"..." / p"..." prefix and escapes applied;
// inline: cc45's IRBuilder uses it too, for the bytes of a C literal)
inline std::vector<uint8_t> encode(const std::string& v, const std::string& directive) {
    std::vector<uint8_t> out;
    std::string str = v;
    // Encoding prefix s"..." / p"..." arrives from the lexer as "\x01s..." / "\x01p..."
    char prefix = 0;
    if (str.size() >= 2 && str[0] == PREFIX) {
        prefix = str[1];
        str = str.substr(2);
    }
    bool screen = prefix == 's' || prefix == 'S' || (prefix == 0 && directive == "screencode");
    bool ascii = prefix == 0 && directive == "ascii";
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == EXACT && i + 1 < str.size()) out.push_back((uint8_t)str[++i]);
        else if (c == NEWLINE) out.push_back(ascii ? 0x0A : 0x0D);
        else if (c == RETURN) out.push_back(ascii ? 0x0D : 0x0A);
        else if (screen) out.push_back(toScreencode(c));
        else if (ascii) out.push_back((uint8_t)c);
        else out.push_back(toPetscii(c));
    }
    return out;
}
}  // namespace asmstr

class AssemblerLexer {
public:
    AssemblerLexer(const std::string& source);
    std::vector<AssemblerToken> tokenize();
    // the files tokens come from (AssemblerToken::file): [0] "" (the main
    // source), then each #line "file" in turn
    const std::vector<std::string>& files() const { return files_; }

private:
    std::vector<std::string> files_{""};
    int curFile_ = 0;
    std::string source;
    size_t pos;
    int line;
    int column;

    char peek() const;
    char get();
    void skipWhitespaceAndComments();
    AssemblerToken nextToken();
    
    AssemblerToken lexIdentifierOrInstruction();
    AssemblerToken lexNumber();
    AssemblerToken lexString();
    AssemblerToken lexChar();
};
