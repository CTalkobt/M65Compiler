#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

class PETSCIIEncoder {
public:
    PETSCIIEncoder();

    std::vector<uint8_t> encode(const std::string& text);
    // A string in an encoding: 'u' PETSCII with letters folded to $41-$5A
    // (basic45's default), 'p' PETSCII as ca45/cc45 map it (a-z $41-$5A,
    // A-Z $61-$7A: mixed case), 's' screen codes (ca45's s".."), 'a' ASCII.
    // {name} escapes give the same bytes in every encoding
    std::vector<uint8_t> encodeAs(const std::string& text, char encoding);
    std::string decode(const std::vector<uint8_t>& bytes);
    uint8_t getColorCode(const std::string& colorName) const;
    uint8_t getControlCode(const std::string& controlName) const;
    // The bytes of a {name} escape: a colour or control code, a MEGA65 ESC
    // sequence ({clear-eos}, {esc-x}, ...: $1B + key) or {$xx}. False if
    // the name is not an escape
    bool escapeBytes(const std::string& name, std::vector<uint8_t>& out) const;
    // MEGA65 ESC sequences: name -> the key after ESC
    const std::unordered_map<std::string, char>& getEscSequences() const { return escSequences; }

private:
    std::unordered_map<std::string, uint8_t> colorCodes;
    std::unordered_map<std::string, uint8_t> controlCodes;
    std::unordered_map<std::string, char> escSequences;

    void initializeColorCodes();
    void initializeControlCodes();
    uint8_t charToPetscii(char c);
    char petsciiToChar(uint8_t b);
};
