#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "O45Types.hpp"

// IR Serialization and Deserialization for .o45 Extended Format
// Handles conversion between O45IRMetadata and binary representation

class O45IRSerializer {
public:
    // Serialize IR metadata to binary format
    // Returns vector of bytes suitable for embedding in .o45 file
    static std::vector<uint8_t> serialize(const O45IRMetadata& ir);

    // Deserialize binary IR data to IR metadata structure
    // Returns true on success, false if format is invalid or version incompatible
    static bool deserialize(const std::vector<uint8_t>& data, O45IRMetadata& out, std::string& errorMsg);

    // Helper to check compatibility between IR versions
    static bool isVersionCompatible(uint8_t majorVersion, uint8_t minorVersion);

private:
    // Helper methods for serialization
    static void writeU16(std::vector<uint8_t>& out, uint16_t val);
    static void writeU32(std::vector<uint8_t>& out, uint32_t val);
    static void writeString(std::vector<uint8_t>& out, const std::string& str);
    static void writeI64(std::vector<uint8_t>& out, int64_t val);

    // Helper methods for deserialization
    static uint16_t readU16(const std::vector<uint8_t>& data, size_t& offset);
    static uint32_t readU32(const std::vector<uint8_t>& data, size_t& offset);
    static std::string readString(const std::vector<uint8_t>& data, size_t& offset);
    static int64_t readI64(const std::vector<uint8_t>& data, size_t& offset);

    static bool checkBounds(const std::vector<uint8_t>& data, size_t offset, size_t needed);
};
