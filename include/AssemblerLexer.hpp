#pragma once
#include <vector>
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
}  // namespace asmstr

class AssemblerLexer {
public:
    AssemblerLexer(const std::string& source);
    std::vector<AssemblerToken> tokenize();

private:
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
