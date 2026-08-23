#ifndef BASICCHARACTERMAP_HPP
#define BASICCHARACTERMAP_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>

// Character set mapping and conversion utilities for MEGA65 BASIC
class BasicCharacterMap {
public:
    enum class CharacterSet {
        ASCII,          // Standard ASCII
        PETSCII,        // Commodore PETSCII (default)
        SCREENCODE,     // MEGA65 screen codes
        CUSTOM          // User-defined character set
    };

    enum class ConversionMode {
        ASCII_TO_PETSCII,
        PETSCII_TO_ASCII,
        ASCII_TO_SCREENCODE,
        SCREENCODE_TO_PETSCII,
        PETSCII_TO_SCREENCODE
    };

    BasicCharacterMap();

    // Convert single character
    uint8_t convert(uint8_t character, ConversionMode mode);

    // Convert string
    std::string convertString(const std::string& input, ConversionMode mode);

    // Load custom character set from file
    bool loadCustomCharset(const std::string& filename);

    // Define custom character
    void defineCharacter(uint8_t code, const std::vector<uint8_t>& bitmap);

    // Get character bitmap (8x8 or 8x16)
    std::vector<uint8_t> getCharacterBitmap(uint8_t code);

    // Check if character is printable
    bool isPrintable(uint8_t character, CharacterSet charset = CharacterSet::PETSCII);

    // Get character description
    std::string getCharacterName(uint8_t code, CharacterSet charset = CharacterSet::PETSCII);

    // Generate character ROM data
    std::vector<uint8_t> generateCharacterROM();

    // Character statistics
    struct CharsetStats {
        int totalCharacters;
        int printableCharacters;
        int controlCharacters;
        int graphicsCharacters;
    };

    CharsetStats getStats(CharacterSet charset);

private:
    struct Character {
        uint8_t code;
        std::string name;
        std::vector<uint8_t> bitmap;  // 8 bytes for 8x8 bitmap
        bool isPrintable;
        bool isGraphics;
    };

    std::unordered_map<uint8_t, Character> customCharset;

    // Conversion tables
    std::vector<uint8_t> asciiToPETSCII;
    std::vector<uint8_t> petsciiToScreencode;
    std::vector<uint8_t> screencodeToASCII;

    // Built-in character names
    std::unordered_map<uint8_t, std::string> charNames;

    void initializeConversionTables();
    void initializeCharacterNames();
    uint8_t convertByte(uint8_t input, const std::vector<uint8_t>& table);
};

// Character set editor for creating custom character sets
class CharacterSetEditor {
public:
    CharacterSetEditor() = default;

    // Edit character bitmap (8x8 grid)
    void editCharacter(uint8_t code, const std::vector<bool>& pixelGrid);

    // Display character preview (ASCII art)
    std::string previewCharacter(uint8_t code);

    // Generate character definition code
    std::string generateCharacterDefinition(uint8_t startCode, uint8_t endCode);

    // Save character set
    bool saveCharacterSet(const std::string& filename, uint8_t startCode, uint8_t endCode);

    // Load character set
    bool loadCharacterSet(const std::string& filename);

private:
    std::unordered_map<uint8_t, std::vector<uint8_t>> characters;

    std::vector<uint8_t> convertPixelsToBytes(const std::vector<bool>& pixels);
    std::vector<bool> convertBytesToPixels(const std::vector<uint8_t>& bytes);
};

#endif  // BASICCHARACTERMAP_HPP
