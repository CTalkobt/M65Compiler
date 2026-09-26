#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

class PETSCIIEncoder {
public:
    PETSCIIEncoder();

    std::vector<uint8_t> encode(const std::string& text);
    std::string decode(const std::vector<uint8_t>& bytes);
    uint8_t getColorCode(const std::string& colorName) const;
    uint8_t getControlCode(const std::string& controlName) const;

private:
    std::unordered_map<std::string, uint8_t> colorCodes;
    std::unordered_map<std::string, uint8_t> controlCodes;

    void initializeColorCodes();
    void initializeControlCodes();
    uint8_t charToPetscii(char c);
    char petsciiToChar(uint8_t b);
};
