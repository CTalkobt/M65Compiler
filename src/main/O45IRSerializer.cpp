#include "O45IRSerializer.hpp"
#include <cstring>

// Helper: Check if we can read 'needed' bytes from 'offset'
bool O45IRSerializer::checkBounds(const std::vector<uint8_t>& data, size_t offset, size_t needed) {
    return offset + needed <= data.size();
}

// Serialization helpers
void O45IRSerializer::writeU16(std::vector<uint8_t>& out, uint16_t val) {
    out.push_back((uint8_t)(val & 0xFF));
    out.push_back((uint8_t)(val >> 8));
}

void O45IRSerializer::writeU32(std::vector<uint8_t>& out, uint32_t val) {
    out.push_back((uint8_t)(val & 0xFF));
    out.push_back((uint8_t)((val >> 8) & 0xFF));
    out.push_back((uint8_t)((val >> 16) & 0xFF));
    out.push_back((uint8_t)(val >> 24));
}

void O45IRSerializer::writeI64(std::vector<uint8_t>& out, int64_t val) {
    uint64_t uval = (uint64_t)val;
    out.push_back((uint8_t)(uval & 0xFF));
    out.push_back((uint8_t)((uval >> 8) & 0xFF));
    out.push_back((uint8_t)((uval >> 16) & 0xFF));
    out.push_back((uint8_t)((uval >> 24) & 0xFF));
    out.push_back((uint8_t)((uval >> 32) & 0xFF));
    out.push_back((uint8_t)((uval >> 40) & 0xFF));
    out.push_back((uint8_t)((uval >> 48) & 0xFF));
    out.push_back((uint8_t)(uval >> 56));
}

void O45IRSerializer::writeString(std::vector<uint8_t>& out, const std::string& str) {
    out.push_back((uint8_t)str.length());
    for (char c : str) {
        out.push_back((uint8_t)c);
    }
}

// Deserialization helpers
uint16_t O45IRSerializer::readU16(const std::vector<uint8_t>& data, size_t& offset) {
    uint16_t val = data[offset] | ((uint16_t)data[offset + 1] << 8);
    offset += 2;
    return val;
}

uint32_t O45IRSerializer::readU32(const std::vector<uint8_t>& data, size_t& offset) {
    uint32_t val = data[offset] |
                   ((uint32_t)data[offset + 1] << 8) |
                   ((uint32_t)data[offset + 2] << 16) |
                   ((uint32_t)data[offset + 3] << 24);
    offset += 4;
    return val;
}

int64_t O45IRSerializer::readI64(const std::vector<uint8_t>& data, size_t& offset) {
    uint64_t uval = (uint64_t)data[offset] |
                    ((uint64_t)data[offset + 1] << 8) |
                    ((uint64_t)data[offset + 2] << 16) |
                    ((uint64_t)data[offset + 3] << 24) |
                    ((uint64_t)data[offset + 4] << 32) |
                    ((uint64_t)data[offset + 5] << 40) |
                    ((uint64_t)data[offset + 6] << 48) |
                    ((uint64_t)data[offset + 7] << 56);
    offset += 8;
    return (int64_t)uval;
}

std::string O45IRSerializer::readString(const std::vector<uint8_t>& data, size_t& offset) {
    uint8_t len = data[offset++];
    std::string str((const char*)&data[offset], len);
    offset += len;
    return str;
}

// Serialize IR metadata to binary format
std::vector<uint8_t> O45IRSerializer::serialize(const O45IRMetadata& ir) {
    std::vector<uint8_t> out;

    // Header: version
    out.push_back(ir.majorVersion);
    out.push_back(ir.minorVersion);

    // Function count
    writeU16(out, ir.functions.size());

    // Each function
    for (const auto& func : ir.functions) {
        // Function name
        writeString(out, func.functionName);

        // Signature hash
        writeU32(out, func.signatureHash);

        // Parameter count and info
        out.push_back((uint8_t)func.parameters.size());
        for (const auto& param : func.parameters) {
            out.push_back((uint8_t)param.type);
            out.push_back(param.flags);
            if (param.flags & O45_IR_PARAM_IS_CONST) {
                writeI64(out, param.constValue);
            }
            writeString(out, param.name);
        }

        // Call site count and info
        writeU16(out, func.callSites.size());
        for (const auto& site : func.callSites) {
            writeU32(out, site.instructionOffset);
            writeString(out, site.calleeName);
            out.push_back((uint8_t)site.paramValues.size());
            for (size_t i = 0; i < site.paramValues.size(); i++) {
                out.push_back(site.paramIsConst[i]);
                writeI64(out, site.paramValues[i]);
            }
        }

        // Call graph entry count and info
        writeU16(out, func.callGraph.size());
        for (const auto& entry : func.callGraph) {
            writeString(out, entry.calleeName);
            writeU16(out, entry.callCount);
            out.push_back(entry.allCallsConstant ? 1 : 0);
        }
    }

    return out;
}

// Deserialize binary IR data to IR metadata structure
bool O45IRSerializer::deserialize(const std::vector<uint8_t>& data, O45IRMetadata& out, std::string& errorMsg) {
    if (data.empty()) {
        errorMsg = "empty IR data";
        return false;
    }

    size_t offset = 0;

    // Read version
    if (!checkBounds(data, offset, 2)) {
        errorMsg = "truncated IR version";
        return false;
    }
    out.majorVersion = data[offset++];
    out.minorVersion = data[offset++];

    // Check compatibility
    if (!isVersionCompatible(out.majorVersion, out.minorVersion)) {
        errorMsg = "incompatible IR version " + std::to_string(out.majorVersion) +
                  "." + std::to_string(out.minorVersion);
        return false;
    }

    // Read function count
    if (!checkBounds(data, offset, 2)) {
        errorMsg = "truncated function count";
        return false;
    }
    uint16_t funcCount = readU16(data, offset);

    // Read each function
    for (int f = 0; f < funcCount; f++) {
        O45IRFunction func;

        // Function name
        if (!checkBounds(data, offset, 1)) {
            errorMsg = "truncated function name length";
            return false;
        }
        func.functionName = readString(data, offset);

        // Signature hash
        if (!checkBounds(data, offset, 4)) {
            errorMsg = "truncated function signature hash";
            return false;
        }
        func.signatureHash = readU32(data, offset);

        // Parameters
        if (!checkBounds(data, offset, 1)) {
            errorMsg = "truncated parameter count";
            return false;
        }
        uint8_t paramCount = data[offset++];
        for (int p = 0; p < paramCount; p++) {
            if (!checkBounds(data, offset, 2)) {
                errorMsg = "truncated parameter type/flags";
                return false;
            }
            O45IRParam param;
            param.type = (O45IRType)data[offset++];
            param.flags = data[offset++];

            if (param.flags & O45_IR_PARAM_IS_CONST) {
                if (!checkBounds(data, offset, 8)) {
                    errorMsg = "truncated constant parameter value";
                    return false;
                }
                param.constValue = readI64(data, offset);
            }

            if (!checkBounds(data, offset, 1)) {
                errorMsg = "truncated parameter name";
                return false;
            }
            param.name = readString(data, offset);

            func.parameters.push_back(param);
        }

        // Call sites
        if (!checkBounds(data, offset, 2)) {
            errorMsg = "truncated call site count";
            return false;
        }
        uint16_t callSiteCount = readU16(data, offset);
        for (int c = 0; c < callSiteCount; c++) {
            if (!checkBounds(data, offset, 4)) {
                errorMsg = "truncated call site offset";
                return false;
            }
            O45IRCallSite site;
            site.instructionOffset = readU32(data, offset);

            if (!checkBounds(data, offset, 1)) {
                errorMsg = "truncated call site callee name";
                return false;
            }
            site.calleeName = readString(data, offset);

            if (!checkBounds(data, offset, 1)) {
                errorMsg = "truncated call site parameter count";
                return false;
            }
            uint8_t siteParamCount = data[offset++];
            for (int sp = 0; sp < siteParamCount; sp++) {
                if (!checkBounds(data, offset, 1)) {
                    errorMsg = "truncated call site param is_const flag";
                    return false;
                }
                uint8_t isConst = data[offset++];
                site.paramIsConst.push_back(isConst);

                if (!checkBounds(data, offset, 8)) {
                    errorMsg = "truncated call site parameter value";
                    return false;
                }
                site.paramValues.push_back(readI64(data, offset));
            }

            func.callSites.push_back(site);
        }

        // Call graph
        if (!checkBounds(data, offset, 2)) {
            errorMsg = "truncated call graph entry count";
            return false;
        }
        uint16_t graphCount = readU16(data, offset);
        for (int g = 0; g < graphCount; g++) {
            if (!checkBounds(data, offset, 1)) {
                errorMsg = "truncated call graph entry name";
                return false;
            }
            O45IRCallGraphEntry entry;
            entry.calleeName = readString(data, offset);

            if (!checkBounds(data, offset, 2)) {
                errorMsg = "truncated call count";
                return false;
            }
            entry.callCount = readU16(data, offset);

            if (!checkBounds(data, offset, 1)) {
                errorMsg = "truncated call graph all_const flag";
                return false;
            }
            entry.allCallsConstant = (data[offset++] != 0);

            func.callGraph.push_back(entry);
        }

        out.functions.push_back(func);
    }

    return true;
}

bool O45IRSerializer::isVersionCompatible(uint8_t majorVersion, uint8_t minorVersion) {
    // Major version must match exactly
    return majorVersion == O45_IR_VERSION_MAJOR;
}
