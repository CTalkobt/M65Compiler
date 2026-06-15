#include "LnxImage.hpp"
#include <cstring>
#include <algorithm>
#include <sstream>

// ============================================================================
// LNX format detection
// ============================================================================

bool LnxImage::isLnxFile(const std::vector<uint8_t>& data) {
    // Look for "LYNX" string in the first 128 bytes (within BASIC stub)
    if (data.size() < 100) return false;
    for (size_t i = 0; i + 3 < data.size() && i < 128; i++) {
        if (data[i] == 'L' && data[i+1] == 'Y' && data[i+2] == 'N' && data[i+3] == 'X')
            return true;
    }
    return false;
}

// ============================================================================
// ASCII field readers
// ============================================================================

std::string LnxImage::readCRString(size_t& pos) const {
    std::string result;
    while (pos < image_.size() && image_[pos] != 0x0D) {
        result += (char)image_[pos++];
    }
    if (pos < image_.size()) pos++; // skip CR
    return result;
}

int LnxImage::readAsciiNumber(size_t& pos) const {
    std::string s = readCRString(pos);
    // Trim spaces
    size_t start = s.find_first_not_of(' ');
    if (start == std::string::npos) return 0;
    return std::atoi(s.c_str() + start);
}

// ============================================================================
// Header parsing
// ============================================================================

bool LnxImage::parseHeader(size_t& dirStart, int& fileCount) const {
    // Find the LYNX signature and skip the BASIC stub
    // The stub is typically ~94 bytes, followed by the signature line
    size_t pos = 0;

    // Find the signature line containing "LYNX"
    // Scan for a CR that's followed by what looks like the directory block count
    // The structure after the BASIC stub is:
    //   <dir_blocks> <space(s)> <signature containing LYNX> <CR>
    //   <space(s)> <file_count> <space(s)> <CR>

    // Strategy: find "LYNX" then backtrack to find the start of that line
    size_t lynxPos = std::string::npos;
    for (size_t i = 0; i + 3 < image_.size() && i < 128; i++) {
        if (image_[i] == 'L' && image_[i+1] == 'Y' && image_[i+2] == 'N' && image_[i+3] == 'X') {
            lynxPos = i;
            break;
        }
    }
    if (lynxPos == std::string::npos) return false;

    // Backtrack to find the start of this line (after previous CR or start of data)
    pos = lynxPos;
    while (pos > 0 && image_[pos - 1] != 0x0D && image_[pos - 1] != 0x00)
        pos--;

    // Read directory block count (first number on this line, before the signature text)
    // Format: "<dir_blocks>  *LYNX XV  BY WILL CORLEY<CR>"
    // Or just: "<dir_blocks> <signature><CR>"
    std::string sigLine = readCRString(pos);

    // Extract the leading number from the signature line
    int dirBlocks = 0;
    size_t numEnd = sigLine.find_first_not_of("0123456789 ");
    if (numEnd > 0) {
        std::string numStr = sigLine.substr(0, numEnd);
        size_t ns = numStr.find_first_not_of(' ');
        if (ns != std::string::npos)
            dirBlocks = std::atoi(numStr.c_str() + ns);
    }
    if (dirBlocks <= 0) dirBlocks = 1;

    // Read file count (next line)
    fileCount = readAsciiNumber(pos);
    if (fileCount <= 0 || fileCount > 1000) return false;

    dirStart = pos;
    return true;
}

// ============================================================================
// Parse all directory entries
// ============================================================================

std::vector<LnxImage::LnxEntry> LnxImage::parseEntries() const {
    std::vector<LnxEntry> entries;
    size_t dirStart;
    int fileCount;
    if (!parseHeader(dirStart, fileCount)) return entries;

    size_t pos = dirStart;

    // Parse directory entries
    for (int i = 0; i < fileCount && pos < image_.size(); i++) {
        LnxEntry e = {};

        // Filename: 16 bytes PETSCII, followed by CR
        if (pos + 16 >= image_.size()) break;
        std::memcpy(e.filename, image_.data() + pos, 16);
        e.filename[16] = 0;
        pos += 16;
        if (pos < image_.size() && image_[pos] == 0x0D) pos++; // skip CR after filename

        // Block count
        e.blockCount = readAsciiNumber(pos);

        // File type (P/S/U/R)
        std::string typeStr = readCRString(pos);
        size_t ts = typeStr.find_first_not_of(' ');
        e.fileType = (ts != std::string::npos) ? typeStr[ts] : 'P';

        // LSU or record size
        int val = readAsciiNumber(pos);

        if (e.fileType == 'R') {
            // For REL files: this was record size, next is LSU
            e.recSize = val;
            e.lsu = readAsciiNumber(pos);
        } else {
            e.lsu = val;
            e.recSize = 0;
        }

        entries.push_back(e);
    }

    // Calculate data offsets
    // Directory is block-aligned, so find the start of data
    // Data starts after the directory blocks
    // We need to figure out how many 254-byte blocks the header+directory occupies

    // Find the first CR after all directory entries — that's the end of the directory
    // Then round up to next 254-byte block boundary
    size_t dirEndPos = pos;
    // The header (BASIC stub + signature + file count + dir entries) is in the first N blocks
    // Data starts at the next 254-byte boundary
    size_t dataStart = ((dirEndPos + 253) / 254) * 254;

    size_t dataPos = dataStart;
    for (auto& e : entries) {
        e.dataOffset = dataPos;
        dataPos += e.blockCount * 254;
    }

    return entries;
}

// ============================================================================
// Public interface
// ============================================================================

void LnxImage::format(const std::string&, const std::string&) {
    // Create empty LNX with 0 files
    image_ = buildHeader(0);
    // Pad to 254-byte block boundary
    while (image_.size() % 254 != 0)
        image_.push_back(0);
}

std::vector<uint8_t> LnxImage::buildHeader(int fileCount) {
    std::vector<uint8_t> hdr;

    // Minimal BASIC stub: just enough to hold the "LYNX" marker
    // SYS stub: $01 $08 $0A $08 $E4 $07 $9E $32 $30 $36 $31 $00 $00 $00
    // Then "USE LYNX TO DISSOLVE THIS FILE" message area
    // For simplicity, write a minimal stub:
    uint8_t stub[] = {
        0x01, 0x08,             // load address $0801
        0x1E, 0x08,             // pointer to next BASIC line
        0x0A, 0x00,             // line number 10
        0x99,                   // PRINT token
        0x22,                   // "
    };
    hdr.insert(hdr.end(), stub, stub + sizeof(stub));

    // Print message
    const char* msg = "USE LYNX TO DISSOLVE THIS FILE";
    for (const char* p = msg; *p; p++)
        hdr.push_back((uint8_t)*p);
    hdr.push_back(0x22); // closing "
    hdr.push_back(0x00); // end of BASIC line
    hdr.push_back(0x00); hdr.push_back(0x00); // end of BASIC program

    // Ensure "LYNX" appears at a recognizable position
    // Signature line: "<dir_blocks>  *LYNX  BY CC45<CR>"
    std::string sig = "1  *LYNX  BY CC45\r";
    hdr.insert(hdr.end(), sig.begin(), sig.end());

    // File count line
    std::string countStr = " " + std::to_string(fileCount) + " \r";
    hdr.insert(hdr.end(), countStr.begin(), countStr.end());

    return hdr;
}

std::vector<FileInfo> LnxImage::listFiles() const {
    std::vector<FileInfo> files;
    for (auto& e : parseEntries()) {
        FileInfo fi;
        fi.name = petsciiToAscii(std::string(e.filename, 16));
        switch (e.fileType) {
            case 'P': fi.type = CbmFileType::PRG; break;
            case 'S': fi.type = CbmFileType::SEQ; break;
            case 'U': fi.type = CbmFileType::USR; break;
            case 'R': fi.type = CbmFileType::REL; break;
            default:  fi.type = CbmFileType::PRG; break;
        }
        fi.sizeInSectors = e.blockCount;
        fi.sizeInBytes = (e.blockCount > 0) ?
            (e.blockCount - 1) * 254 + e.lsu : 0;
        fi.closed = true;
        files.push_back(fi);
    }
    return files;
}

bool LnxImage::fileExists(const std::string& name) const {
    char petName[16];
    padPetsciiName(petName, name);
    for (auto& e : parseEntries()) {
        if (std::memcmp(e.filename, petName, 16) == 0)
            return true;
    }
    return false;
}

std::vector<uint8_t> LnxImage::readFile(const std::string& name) const {
    char petName[16];
    padPetsciiName(petName, name);
    for (auto& e : parseEntries()) {
        if (std::memcmp(e.filename, petName, 16) != 0) continue;

        int byteSize = (e.blockCount > 0) ?
            (e.blockCount - 1) * 254 + e.lsu : 0;
        if (e.dataOffset + byteSize > image_.size()) return {};
        return std::vector<uint8_t>(image_.begin() + e.dataOffset,
                                    image_.begin() + e.dataOffset + byteSize);
    }
    return {};
}

bool LnxImage::addFile(const std::string& name, CbmFileType type,
                       const std::vector<uint8_t>& data) {
    if (fileExists(name)) return false;

    // Parse existing entries to rebuild
    auto entries = parseEntries();
    int newCount = (int)entries.size() + 1;

    // Build new header
    auto newImage = buildHeader(newCount);

    // Write existing directory entries
    for (auto& e : entries) {
        newImage.insert(newImage.end(), (uint8_t*)e.filename, (uint8_t*)e.filename + 16);
        newImage.push_back(0x0D);
        std::string bs = " " + std::to_string(e.blockCount) + " \r";
        newImage.insert(newImage.end(), bs.begin(), bs.end());
        std::string ts(1, e.fileType);
        ts += "\r";
        newImage.insert(newImage.end(), ts.begin(), ts.end());
        if (e.fileType == 'R') {
            std::string rs = " " + std::to_string(e.recSize) + " \r";
            newImage.insert(newImage.end(), rs.begin(), rs.end());
        }
        std::string ls = " " + std::to_string(e.lsu) + " \r";
        newImage.insert(newImage.end(), ls.begin(), ls.end());
    }

    // Write new entry
    int blocks = ((int)data.size() + 253) / 254;
    if (blocks == 0) blocks = 1;
    int lsu = (int)data.size() % 254;
    if (lsu == 0 && !data.empty()) lsu = 254;
    if (data.empty()) lsu = 0;

    char petName[16];
    padPetsciiName(petName, name);
    newImage.insert(newImage.end(), (uint8_t*)petName, (uint8_t*)petName + 16);
    newImage.push_back(0x0D);
    std::string bs = " " + std::to_string(blocks) + " \r";
    newImage.insert(newImage.end(), bs.begin(), bs.end());
    char ft = 'P';
    switch (type) {
        case CbmFileType::SEQ: ft = 'S'; break;
        case CbmFileType::USR: ft = 'U'; break;
        case CbmFileType::REL: ft = 'R'; break;
        default: ft = 'P'; break;
    }
    newImage.push_back((uint8_t)ft);
    newImage.push_back(0x0D);
    std::string ls = " " + std::to_string(lsu) + " \r";
    newImage.insert(newImage.end(), ls.begin(), ls.end());

    // Pad directory to 254-byte block boundary
    while (newImage.size() % 254 != 0)
        newImage.push_back(0);

    // Copy existing file data
    for (auto& e : entries) {
        int fsize = e.blockCount * 254;
        if (e.dataOffset + fsize <= image_.size()) {
            newImage.insert(newImage.end(),
                           image_.begin() + e.dataOffset,
                           image_.begin() + e.dataOffset + fsize);
        }
    }

    // Append new file data (block-aligned)
    newImage.insert(newImage.end(), data.begin(), data.end());
    int pad = blocks * 254 - (int)data.size();
    for (int i = 0; i < pad; i++) newImage.push_back(0);

    image_ = std::move(newImage);
    return true;
}

bool LnxImage::removeFile(const std::string& name) {
    char petName[16];
    padPetsciiName(petName, name);
    auto entries = parseEntries();
    int targetIdx = -1;
    for (int i = 0; i < (int)entries.size(); i++) {
        if (std::memcmp(entries[i].filename, petName, 16) == 0) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx < 0) return false;

    int newCount = (int)entries.size() - 1;
    auto newImage = buildHeader(newCount);

    // Write directory entries (skip target)
    for (int i = 0; i < (int)entries.size(); i++) {
        if (i == targetIdx) continue;
        auto& e = entries[i];
        newImage.insert(newImage.end(), (uint8_t*)e.filename, (uint8_t*)e.filename + 16);
        newImage.push_back(0x0D);
        std::string bs = " " + std::to_string(e.blockCount) + " \r";
        newImage.insert(newImage.end(), bs.begin(), bs.end());
        std::string ts(1, e.fileType); ts += "\r";
        newImage.insert(newImage.end(), ts.begin(), ts.end());
        if (e.fileType == 'R') {
            std::string rs = " " + std::to_string(e.recSize) + " \r";
            newImage.insert(newImage.end(), rs.begin(), rs.end());
        }
        std::string ls = " " + std::to_string(e.lsu) + " \r";
        newImage.insert(newImage.end(), ls.begin(), ls.end());
    }

    while (newImage.size() % 254 != 0)
        newImage.push_back(0);

    // Copy file data (skip target)
    for (int i = 0; i < (int)entries.size(); i++) {
        if (i == targetIdx) continue;
        auto& e = entries[i];
        int fsize = e.blockCount * 254;
        if (e.dataOffset + fsize <= image_.size()) {
            newImage.insert(newImage.end(),
                           image_.begin() + e.dataOffset,
                           image_.begin() + e.dataOffset + fsize);
        }
    }

    image_ = std::move(newImage);
    return true;
}
