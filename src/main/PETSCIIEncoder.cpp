#include "PETSCIIEncoder.hpp"
#include <cctype>

PETSCIIEncoder::PETSCIIEncoder() {
    initializeColorCodes();
    initializeControlCodes();
}

void PETSCIIEncoder::initializeColorCodes() {
    colorCodes = {
        {"black", 0x90},
        {"white", 0x05},
        {"red", 0x1C},
        {"cyan", 0x9F},
        {"magenta", 0x9C},
        {"purple", 0x9C},
        {"green", 0x1E},
        {"blue", 0x1F},
        {"yellow", 0x9E},
    };
}

void PETSCIIEncoder::initializeControlCodes() {
    controlCodes = {
        {"home", 0x13},
        {"clear", 0x93},
        {"esc", 0x27},
        {"back", 0x08},
        {"delete", 0x14},
        {"del", 0x14},
        {"insert", 0x94},
        {"ins", 0x94},
        {"reverse", 0x12},
        {"rev", 0x12},
        {"under", 0x82},
        {"underline", 0x82},
        {"off", 0x81},
        {"bell", 0x07},
        {"up", 0x91},
        {"down", 0x0A},
        {"left", 0x9D},
        {"right", 0x1D},
        {"tab", 0x09},
        {"crsr-up", 0x91},
        {"crsr-down", 0x0A},
        {"crsr-left", 0x9D},
        {"crsr-right", 0x1D},
        {"f1", 0x85},
        {"f2", 0x89},
        {"f3", 0x86},
        {"f4", 0x8A},
        {"f5", 0x87},
        {"f6", 0x8B},
        {"f7", 0x88},
        {"f8", 0x8C},
        {"shift-f1", 0x85},
        {"shift-f2", 0x89},
        {"shift-f3", 0x86},
        {"shift-f4", 0x8A},
        {"shift-f5", 0x87},
        {"shift-f6", 0x8B},
        {"shift-f7", 0x88},
        {"shift-f8", 0x8C},
    };
}

std::vector<uint8_t> PETSCIIEncoder::encode(const std::string& text) {
    std::vector<uint8_t> result;
    size_t i = 0;

    while (i < text.length()) {
        if (text[i] == '{') {
            size_t end = text.find('}', i);
            if (end != std::string::npos) {
                std::string escape = text.substr(i + 1, end - i - 1);

                if (getColorCode(escape) != 0) {
                    result.push_back(getColorCode(escape));
                } else if (getControlCode(escape) != 0) {
                    result.push_back(getControlCode(escape));
                } else {
                    result.push_back(charToPetscii('{'));
                    for (char c : escape) {
                        result.push_back(charToPetscii(c));
                    }
                    result.push_back(charToPetscii('}'));
                }
                i = end + 1;
            } else {
                result.push_back(charToPetscii(text[i]));
                i++;
            }
        } else {
            result.push_back(charToPetscii(text[i]));
            i++;
        }
    }

    return result;
}

std::string PETSCIIEncoder::decode(const std::vector<uint8_t>& bytes) {
    std::string result;
    for (uint8_t b : bytes) {
        result += petsciiToChar(b);
    }
    return result;
}

uint8_t PETSCIIEncoder::charToPetscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<uint8_t>(c);
    } else if (c >= 'a' && c <= 'z') {
        return static_cast<uint8_t>(c - 32);
    } else if (c >= '0' && c <= '9') {
        return static_cast<uint8_t>(c);
    } else if (c == ' ') {
        return 0x20;
    } else if (c == '!') {
        return 0x21;
    } else if (c == '"') {
        return 0x22;
    } else if (c == '#') {
        return 0x23;
    } else if (c == '$') {
        return 0x24;
    } else if (c == '%') {
        return 0x25;
    } else if (c == '&') {
        return 0x26;
    } else if (c == '\'') {
        return 0x27;
    } else if (c == '(') {
        return 0x28;
    } else if (c == ')') {
        return 0x29;
    } else if (c == '*') {
        return 0x2A;
    } else if (c == '+') {
        return 0x2B;
    } else if (c == ',') {
        return 0x2C;
    } else if (c == '-') {
        return 0x2D;
    } else if (c == '.') {
        return 0x2E;
    } else if (c == '/') {
        return 0x2F;
    } else if (c == ':') {
        return 0x3A;
    } else if (c == ';') {
        return 0x3B;
    } else if (c == '<') {
        return 0x3C;
    } else if (c == '=') {
        return 0x3D;
    } else if (c == '>') {
        return 0x3E;
    } else if (c == '?') {
        return 0x3F;
    } else if (c == '@') {
        return 0x00;
    } else if (c == '[') {
        return 0x5B;
    } else if (c == '\\') {
        return 0x5C;
    } else if (c == ']') {
        return 0x5D;
    } else if (c == '^') {
        return 0x5E;
    } else if (c == '_') {
        return 0x5F;
    }

    return 0x20;
}

char PETSCIIEncoder::petsciiToChar(uint8_t b) {
    if ((b >= 0x41 && b <= 0x5A) || (b >= 0x61 && b <= 0x7A)) {
        return static_cast<char>(b);
    } else if (b >= 0x30 && b <= 0x39) {
        return static_cast<char>(b);
    } else if (b == 0x20) {
        return ' ';
    } else if (b >= 0x21 && b <= 0x2F) {
        return static_cast<char>(b);
    } else if (b >= 0x3A && b <= 0x40) {
        return static_cast<char>(b);
    } else if (b >= 0x5B && b <= 0x5F) {
        return static_cast<char>(b);
    }

    return '?';
}

uint8_t PETSCIIEncoder::getColorCode(const std::string& colorName) const {
    auto it = colorCodes.find(colorName);
    if (it != colorCodes.end()) {
        return it->second;
    }
    return 0;
}

uint8_t PETSCIIEncoder::getControlCode(const std::string& controlName) const {
    auto it = controlCodes.find(controlName);
    if (it != controlCodes.end()) {
        return it->second;
    }
    return 0;
}
