#include "BasicCharacterMap.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

BasicCharacterMap::BasicCharacterMap() {
    asciiToPETSCII.resize(256);
    petsciiToScreencode.resize(256);
    screencodeToASCII.resize(256);
    initializeConversionTables();
    initializeCharacterNames();
}

void BasicCharacterMap::initializeConversionTables() {
    // ASCII to PETSCII conversion (basic mapping)
    for (int i = 0; i < 256; i++) {
        asciiToPETSCII[i] = i;  // Direct mapping for most
    }

    // Handle special characters
    asciiToPETSCII['['] = 0x5B;
    asciiToPETSCII[']'] = 0x5D;
    asciiToPETSCII['`'] = 0x27;  // Grave to quote
    asciiToPETSCII['{'] = 0x7B;
    asciiToPETSCII['}'] = 0x7D;

    // PETSCII to Screencode conversion
    for (int i = 0; i < 256; i++) {
        petsciiToScreencode[i] = i;  // Direct for most
    }

    // Handle uppercase/lowercase conversion (PETSCII is uppercase by default)
    for (int i = 0x61; i <= 0x7A; i++) {  // lowercase a-z
        petsciiToScreencode[i] = i - 0x20;  // Convert to uppercase
    }

    // Build reverse table
    for (int i = 0; i < 256; i++) {
        screencodeToASCII[petsciiToScreencode[i]] = i;
    }
}

void BasicCharacterMap::initializeCharacterNames() {
    // Control characters
    charNames[0x00] = "NULL";
    charNames[0x0D] = "CARRIAGE_RETURN";
    charNames[0x0A] = "LINE_FEED";
    charNames[0x09] = "TAB";
    charNames[0x08] = "BACKSPACE";

    // Space and printable ASCII
    charNames[0x20] = "SPACE";
    charNames[0x21] = "EXCLAMATION";
    charNames[0x22] = "QUOTE";
    charNames[0x23] = "HASH";
    charNames[0x24] = "DOLLAR";
    charNames[0x25] = "PERCENT";
    charNames[0x26] = "AMPERSAND";
    charNames[0x27] = "APOSTROPHE";
    charNames[0x28] = "LEFT_PAREN";
    charNames[0x29] = "RIGHT_PAREN";
    charNames[0x2A] = "ASTERISK";
    charNames[0x2B] = "PLUS";
    charNames[0x2C] = "COMMA";
    charNames[0x2D] = "MINUS";
    charNames[0x2E] = "PERIOD";
    charNames[0x2F] = "SLASH";

    // Digits
    for (int i = 0x30; i <= 0x39; i++) {
        charNames[i] = "DIGIT_" + std::to_string(i - 0x30);
    }

    // Special
    charNames[0x3A] = "COLON";
    charNames[0x3B] = "SEMICOLON";
    charNames[0x3C] = "LESS_THAN";
    charNames[0x3D] = "EQUALS";
    charNames[0x3E] = "GREATER_THAN";
    charNames[0x3F] = "QUESTION";
    charNames[0x40] = "AT";

    // Graphics characters (0xA0-0xFF in PETSCII)
    for (int i = 0xA0; i <= 0xFF; i++) {
        charNames[i] = "GRAPHICS_" + std::to_string(i - 0xA0);
    }
}

uint8_t BasicCharacterMap::convert(uint8_t character, ConversionMode mode) {
    switch (mode) {
        case ConversionMode::ASCII_TO_PETSCII:
            return convertByte(character, asciiToPETSCII);
        case ConversionMode::PETSCII_TO_ASCII:
            // Simple reverse lookup
            for (int i = 0; i < 256; i++) {
                if (asciiToPETSCII[i] == character) {
                    return i;
                }
            }
            return character;
        case ConversionMode::ASCII_TO_SCREENCODE:
            return convertByte(
                convertByte(character, asciiToPETSCII),
                petsciiToScreencode
            );
        case ConversionMode::SCREENCODE_TO_PETSCII:
            return screencodeToASCII[character];
        case ConversionMode::PETSCII_TO_SCREENCODE:
            return convertByte(character, petsciiToScreencode);
    }
    return character;
}

std::string BasicCharacterMap::convertString(const std::string& input, ConversionMode mode) {
    std::string result;
    for (uint8_t c : input) {
        result += (char)convert(c, mode);
    }
    return result;
}

bool BasicCharacterMap::loadCustomCharset(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    customCharset.clear();
    uint8_t code = 0;

    while (file.read(reinterpret_cast<char*>(&code), 1)) {
        Character ch;
        ch.code = code;
        ch.bitmap.resize(8);

        if (!file.read(reinterpret_cast<char*>(ch.bitmap.data()), 8)) {
            return false;
        }

        ch.isPrintable = true;
        ch.isGraphics = (code >= 0xA0);
        customCharset[code] = ch;
    }

    return true;
}

void BasicCharacterMap::defineCharacter(uint8_t code, const std::vector<uint8_t>& bitmap) {
    if (bitmap.size() != 8) {
        return;  // Must be 8 bytes
    }

    Character ch;
    ch.code = code;
    ch.bitmap = bitmap;
    ch.isPrintable = (code >= 0x20 && code < 0x7F) || code >= 0xA0;
    ch.isGraphics = (code >= 0xA0);
    ch.name = getCharacterName(code);

    customCharset[code] = ch;
}

std::vector<uint8_t> BasicCharacterMap::getCharacterBitmap(uint8_t code) {
    auto it = customCharset.find(code);
    if (it != customCharset.end()) {
        return it->second.bitmap;
    }
    // Return default (blank) bitmap
    return std::vector<uint8_t>(8, 0);
}

bool BasicCharacterMap::isPrintable(uint8_t character, CharacterSet charset) {
    if (charset == CharacterSet::PETSCII || charset == CharacterSet::ASCII) {
        return (character >= 0x20 && character < 0x7F) || character >= 0xA0;
    }
    return true;
}

std::string BasicCharacterMap::getCharacterName(uint8_t code, CharacterSet) {
    auto it = charNames.find(code);
    if (it != charNames.end()) {
        return it->second;
    }
    return "CHAR_" + std::to_string(code);
}

std::vector<uint8_t> BasicCharacterMap::generateCharacterROM() {
    std::vector<uint8_t> rom;
    rom.reserve(256 * 8);  // 256 characters × 8 bytes each

    for (int i = 0; i < 256; i++) {
        auto bitmap = getCharacterBitmap(i);
        rom.insert(rom.end(), bitmap.begin(), bitmap.end());
    }

    return rom;
}

BasicCharacterMap::CharsetStats BasicCharacterMap::getStats(CharacterSet charset) {
    CharsetStats stats = {0, 0, 0, 0};
    stats.totalCharacters = 256;

    for (int i = 0; i < 256; i++) {
        if (isPrintable(i, charset)) {
            stats.printableCharacters++;
        } else if (i < 0x20) {
            stats.controlCharacters++;
        } else if (i >= 0xA0) {
            stats.graphicsCharacters++;
        }
    }

    return stats;
}

uint8_t BasicCharacterMap::convertByte(uint8_t input, const std::vector<uint8_t>& table) {
    if (input < table.size()) {
        return table[input];
    }
    return input;
}

// ==================== CharacterSetEditor ====================

void CharacterSetEditor::editCharacter(uint8_t code, const std::vector<bool>& pixelGrid) {
    if (pixelGrid.size() != 64) {  // 8x8 pixels
        return;
    }

    auto bytes = convertPixelsToBytes(pixelGrid);
    characters[code] = bytes;
}

std::string CharacterSetEditor::previewCharacter(uint8_t code) {
    auto it = characters.find(code);
    if (it == characters.end()) {
        return "";
    }

    auto pixels = convertBytesToPixels(it->second);
    std::ostringstream preview;

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            int pixelIdx = row * 8 + col;
            preview << (pixels[pixelIdx] ? "█" : " ");
        }
        preview << "\n";
    }

    return preview.str();
}

std::string CharacterSetEditor::generateCharacterDefinition(uint8_t startCode, uint8_t endCode) {
    std::ostringstream def;
    def << "REM Character Set Definition\n";

    for (uint8_t code = startCode; code <= endCode; code++) {
        auto it = characters.find(code);
        if (it != characters.end()) {
            def << "REM Character " << (int)code << "\n";
            def << "DATA ";
            for (int i = 0; i < 8; i++) {
                def << (int)it->second[i];
                if (i < 7) def << ",";
            }
            def << "\n";
        }
    }

    return def.str();
}

bool CharacterSetEditor::saveCharacterSet(const std::string& filename, uint8_t startCode, uint8_t endCode) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    for (uint8_t code = startCode; code <= endCode; code++) {
        file.write(reinterpret_cast<const char*>(&code), 1);

        auto it = characters.find(code);
        if (it != characters.end()) {
            file.write(reinterpret_cast<const char*>(it->second.data()), 8);
        } else {
            // Write empty character
            uint8_t empty[8] = {0};
            file.write(reinterpret_cast<const char*>(empty), 8);
        }
    }

    return file.good();
}

bool CharacterSetEditor::loadCharacterSet(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }

    characters.clear();
    uint8_t code;

    while (file.read(reinterpret_cast<char*>(&code), 1)) {
        std::vector<uint8_t> bitmap(8);
        if (!file.read(reinterpret_cast<char*>(bitmap.data()), 8)) {
            return false;
        }
        characters[code] = bitmap;
    }

    return true;
}

std::vector<uint8_t> CharacterSetEditor::convertPixelsToBytes(const std::vector<bool>& pixels) {
    std::vector<uint8_t> bytes(8, 0);

    for (int row = 0; row < 8; row++) {
        uint8_t byte = 0;
        for (int col = 0; col < 8; col++) {
            size_t pixelIdx = row * 8 + col;
            if (pixelIdx < pixels.size() && pixels[pixelIdx]) {
                byte |= (1 << (7 - col));  // MSB first
            }
        }
        bytes[row] = byte;
    }

    return bytes;
}

std::vector<bool> CharacterSetEditor::convertBytesToPixels(const std::vector<uint8_t>& bytes) {
    std::vector<bool> pixels(64, false);

    for (size_t row = 0; row < 8 && row < bytes.size(); row++) {
        uint8_t byte = bytes[row];
        for (int col = 0; col < 8; col++) {
            size_t pixelIdx = row * 8 + col;
            pixels[pixelIdx] = (byte & (1 << (7 - col))) != 0;
        }
    }

    return pixels;
}
