#include "ArcImage.hpp"
#include <cstring>
#include <algorithm>

// ============================================================================
// ARC format implementation
// Based on cbmconvert's unarc.c by Chris Smeets and Marko Mäkelä (GPL).
// This is a clean-room reimplementation using the documented format.
// ============================================================================

// --- BitReader ---

uint8_t ArcImage::BitReader::getByte() {
    if (pos >= size) return 0;
    return data[pos++];
}

uint16_t ArcImage::BitReader::getWord() {
    uint16_t lo = getByte();
    uint16_t hi = getByte();
    return lo | (hi << 8);
}

unsigned ArcImage::BitReader::getBit() {
    unsigned result = buf >>= 1;
    if (result == 1)
        return 1 & (buf = getByte() | 0x0100U);
    else
        return 1 & result;
}

// --- Format detection ---

bool ArcImage::isArcFile(const std::vector<uint8_t>& data) {
    if (data.size() < 12) return false;
    uint8_t ver = data[0];
    if (ver == 1 || ver == 2) {
        uint8_t mode = data[1];
        if (mode <= 5) {
            uint8_t ft = data[9];
            return ft == 'P' || ft == 'S' || ft == 'U' || ft == 'R';
        }
    }
    return false;
}

// --- SDA header detection ---

size_t ArcImage::getStartPos() const {
    if (image_.size() < 12) return 0;
    if (image_[0] == 2) return 0; // v2 archive starts directly

    // Check for SDA: BASIC stub with SYS token
    if (image_.size() > 10 && image_[0] == 1) {
        // Try to find SYS token (0x9E) in first 10 bytes
        for (size_t i = 4; i < std::min((size_t)10, image_.size()); i++) {
            if (image_[i] == 0x9E) {
                // It's an SDA — skip the header blocks
                // Line number at offset 4-5 gives header block count
                uint16_t linenum = image_[4] | (image_[5] << 8);
                return (linenum - 6) * 254;
            }
        }
        return 0; // v1 archive
    }
    return 0;
}

// --- Parse all archive entries ---

std::vector<ArcImage::ArcEntry> ArcImage::parseEntries() const {
    std::vector<ArcEntry> entries;
    size_t pos = getStartPos();

    while (pos + 11 < image_.size()) {
        ArcEntry e;
        e.version = image_[pos++];
        if (e.version == 0 || e.version > 2) break; // end of archive

        e.mode = image_[pos++];
        if (e.mode > 5) break;

        e.checksum = image_[pos] | (image_[pos + 1] << 8); pos += 2;
        e.origSize = image_[pos] | (image_[pos + 1] << 8) | (image_[pos + 2] << 16); pos += 3;
        e.blocks = image_[pos] | (image_[pos + 1] << 8); pos += 2;
        e.fileType = image_[pos++];
        e.fnLen = image_[pos++];

        if (e.fnLen > 16) break;
        if (pos + e.fnLen > image_.size()) break;

        std::memcpy(e.filename, image_.data() + pos, e.fnLen);
        e.filename[e.fnLen] = 0;
        pos += e.fnLen;

        if (e.version > 1) {
            if (pos + 3 > image_.size()) break;
            e.recLen = image_[pos++];
            e.date = image_[pos] | (image_[pos + 1] << 8); pos += 2;
        }

        // RLE control byte follows header for packed modes
        unsigned ctrlByte = 254;
        if (e.mode == 1) {
            if (pos >= image_.size()) break;
            ctrlByte = image_[pos++];
        }

        // Huffman code table follows for modes 2 and 4
        // (handled during decompression)

        e.dataOffset = pos;
        entries.push_back(e);

        // Skip compressed data to find next entry
        pos += e.blocks * 254;
    }
    return entries;
}

// --- Huffman tree ---

bool ArcImage::HuffmanTree::build(BitReader& reader) {
    count = 255;
    for (int w = 0; w < 256; w++) {
        values[w] = (uint8_t)w;
        lengths[w] = 0;
        unsigned long mask = 1;
        for (int i = 0; i < 5; i++) {
            if (reader.getBit())
                lengths[w] |= (uint8_t)mask;
            mask <<= 1;
        }
        if (lengths[w] > 24) return false;

        codes[w] = 0;
        if (lengths[w]) {
            unsigned long m = 1;
            for (unsigned i = 0; i < lengths[w]; i++) {
                if (reader.getBit())
                    codes[w] |= m;
                m <<= 1;
            }
        } else {
            count--;
        }
    }
    // Shell sort by code length (descending)
    for (size_t gap = 128; gap > 0; gap >>= 1) {
        for (size_t i = gap; i < 256; i++) {
            for (size_t j = i; j >= gap && lengths[j - gap] < lengths[j]; j -= gap) {
                std::swap(codes[j], codes[j - gap]);
                std::swap(values[j], values[j - gap]);
                std::swap(lengths[j], lengths[j - gap]);
            }
        }
    }
    return true;
}

int ArcImage::HuffmanTree::decode(BitReader& reader) const {
    unsigned long hcode = 0;
    unsigned long mask = 1;
    int size = 1;
    int now = count;

    do {
        if (reader.getBit())
            hcode |= mask;
        while (now >= 0 && lengths[now] == size) {
            if (codes[now] == hcode)
                return values[now];
            now--;
        }
        if (now < 0) return -1;
        size++;
        mask <<= 1;
    } while (size < 24);

    return -1; // error
}

// --- RLE unpacking ---

std::vector<uint8_t> ArcImage::unpackRLE(const std::vector<uint8_t>& input,
                                          unsigned ctrl, uint8_t version) {
    std::vector<uint8_t> output;
    output.reserve(input.size());
    size_t i = 0;
    while (i < input.size()) {
        uint8_t c = input[i++];
        if (c == ctrl && i + 1 < input.size()) {
            int count = input[i++];
            uint8_t val = input[i++];
            if (count == 0)
                count = (version == 1) ? 255 : 256;
            for (int j = 0; j < count; j++)
                output.push_back(val);
        } else {
            output.push_back(c);
        }
    }
    return output;
}

// --- Huffman decompression (modes 2, 4) ---

std::vector<uint8_t> ArcImage::decompressHuffman(BitReader& reader, size_t origSize) {
    HuffmanTree tree;
    if (!tree.build(reader)) return {};

    std::vector<uint8_t> output;
    output.reserve(origSize);
    while (output.size() < origSize && !reader.eof()) {
        int c = tree.decode(reader);
        if (c < 0) break;
        output.push_back((uint8_t)c);
    }
    return output;
}

// --- LZW decompression (modes 3, 5) ---

std::vector<uint8_t> ArcImage::decompressLZW(BitReader& reader, size_t origSize, uint8_t mode) {
    struct LZEntry { uint16_t prefix; uint8_t ext; };
    LZEntry table[4096];
    uint8_t stack[512];

    int cdlen = 9;          // start with 9-bit codes
    int wtcl = 256;         // bump code size at this count
    int wttcl = 254;        // first time: 254 due to 2 reserved codes
    unsigned ncodes = 258;  // codes 0-255 = literals, 256 = EOF, 257 = reserved

    std::vector<uint8_t> output;
    output.reserve(origSize);

    // Read first code
    unsigned code = 0;
    for (int i = 0; i < cdlen; i++)
        code = (code << 1) | reader.getBit();

    if (code == 256) return output; // empty file

    uint8_t finchar = (uint8_t)code;
    output.push_back(finchar);
    unsigned oldcode = code;

    auto getCode = [&]() -> unsigned {
        unsigned c = 0;
        for (int i = 0; i < cdlen; i++)
            c = (c << 1) | reader.getBit();

        // Mode 5: checksum and size follow EOF code
        if (c == 256 && mode == 5) {
            // Read trailing checksum (16 bits), size (24 bits), padding (16 bits)
            for (int i = 0; i < 16 + 24 + 16; i++) reader.getBit();
        }

        if (cdlen < 12) {
            if (!(--wttcl)) {
                wtcl <<= 1;
                cdlen++;
                wttcl = wtcl;
            }
        }
        return c;
    };

    while (output.size() < origSize && !reader.eof()) {
        unsigned incode = getCode();
        if (incode == 256) break; // EOF

        unsigned lzsp = 0;
        uint8_t kay;

        if (incode >= ncodes) {
            // Undefined code — special case
            kay = finchar;
            if (lzsp < sizeof(stack)) stack[lzsp++] = kay;
            code = oldcode;
        } else {
            code = incode;
        }

        // Decompose string
        while (code > 255) {
            if (lzsp < sizeof(stack)) stack[lzsp++] = table[code].ext;
            code = table[code].prefix;
        }
        finchar = kay = (uint8_t)code;
        output.push_back(kay);

        // Emit stack in reverse
        while (lzsp > 0)
            output.push_back(stack[--lzsp]);

        // Add new table entry
        if (ncodes < 4096) {
            table[ncodes].prefix = oldcode;
            table[ncodes].ext = kay;
            ncodes++;
        }
        oldcode = incode;
    }
    return output;
}

// --- Decompress a single entry ---

std::vector<uint8_t> ArcImage::decompressEntry(const ArcEntry& entry) const {
    BitReader reader(image_.data(), image_.size(), entry.dataOffset);

    unsigned ctrl = 254;
    // For mode 1, ctrl byte is already consumed during parseEntries
    // and is stored just before dataOffset. Read it back.
    if (entry.mode == 1 && entry.dataOffset > 0) {
        ctrl = image_[entry.dataOffset - 1];
    }

    switch (entry.mode) {
    case 0: { // Stored
        size_t size = entry.origSize;
        if (entry.dataOffset + size > image_.size())
            size = image_.size() - entry.dataOffset;
        return std::vector<uint8_t>(image_.begin() + entry.dataOffset,
                                    image_.begin() + entry.dataOffset + size);
    }

    case 1: { // Packed (RLE only)
        size_t compSize = entry.blocks * 254;
        if (entry.dataOffset + compSize > image_.size())
            compSize = image_.size() - entry.dataOffset;
        std::vector<uint8_t> raw(image_.begin() + entry.dataOffset,
                                 image_.begin() + entry.dataOffset + compSize);
        return unpackRLE(raw, ctrl, entry.version);
    }

    case 2: { // Squeezed (Huffman)
        return decompressHuffman(reader, entry.origSize);
    }

    case 3: // Crunched (LZW)
    case 5: // Crunched one-pass
        return decompressLZW(reader, entry.origSize, entry.mode);

    case 4: { // Squeezed + packed (Huffman + RLE)
        auto huffed = decompressHuffman(reader, entry.origSize * 2); // overallocate
        return unpackRLE(huffed, ctrl, entry.version);
    }

    default:
        return {};
    }
}

// ============================================================================
// Public interface
// ============================================================================

void ArcImage::format(const std::string&, const std::string&) {
    image_.clear();
    // Empty archive: just a terminator (version 0)
    image_.push_back(0);
}

std::vector<FileInfo> ArcImage::listFiles() const {
    std::vector<FileInfo> files;
    for (auto& e : parseEntries()) {
        FileInfo fi;
        fi.name = petsciiToAscii(std::string(e.filename, e.fnLen));
        switch (e.fileType) {
            case 'P': fi.type = CbmFileType::PRG; break;
            case 'S': fi.type = CbmFileType::SEQ; break;
            case 'U': fi.type = CbmFileType::USR; break;
            case 'R': fi.type = CbmFileType::REL; break;
            default:  fi.type = CbmFileType::PRG; break;
        }
        fi.sizeInSectors = e.blocks;
        fi.sizeInBytes = e.origSize;
        fi.closed = true;
        files.push_back(fi);
    }
    return files;
}

bool ArcImage::fileExists(const std::string& name) const {
    std::string petName = asciiToPetscii(name);
    for (auto& e : parseEntries()) {
        std::string eName(e.filename, e.fnLen);
        if (eName == petName) return true;
    }
    return false;
}

std::vector<uint8_t> ArcImage::readFile(const std::string& name) const {
    std::string petName = asciiToPetscii(name);
    for (auto& e : parseEntries()) {
        std::string eName(e.filename, e.fnLen);
        if (eName == petName)
            return decompressEntry(e);
    }
    return {};
}

bool ArcImage::addFile(const std::string& name, CbmFileType type,
                       const std::vector<uint8_t>& data) {
    if (fileExists(name)) return false;

    // Remove terminator byte if present
    if (!image_.empty() && image_.back() == 0)
        image_.pop_back();

    // Build v2 header in stored mode (no compression)
    std::string petName = asciiToPetscii(name);
    if (petName.size() > 16) petName.resize(16);

    int blocks = ((int)data.size() + 253) / 254;
    if (blocks == 0) blocks = 1;

    // Version
    image_.push_back(2);
    // Mode: stored
    image_.push_back(0);
    // Checksum (simple sum for v2 with XOR)
    uint16_t crc = 0;
    uint8_t crc2 = 0;
    for (uint8_t b : data) crc += (b ^ (++crc2));
    image_.push_back(crc & 0xFF);
    image_.push_back((crc >> 8) & 0xFF);
    // Original size (3 bytes)
    image_.push_back(data.size() & 0xFF);
    image_.push_back((data.size() >> 8) & 0xFF);
    image_.push_back((data.size() >> 16) & 0xFF);
    // Block count
    image_.push_back(blocks & 0xFF);
    image_.push_back((blocks >> 8) & 0xFF);
    // File type
    char ft = 'P';
    switch (type) {
        case CbmFileType::SEQ: ft = 'S'; break;
        case CbmFileType::PRG: ft = 'P'; break;
        case CbmFileType::USR: ft = 'U'; break;
        case CbmFileType::REL: ft = 'R'; break;
        default: ft = 'P'; break;
    }
    image_.push_back((uint8_t)ft);
    // Filename length + name
    image_.push_back((uint8_t)petName.size());
    image_.insert(image_.end(), petName.begin(), petName.end());
    // V2 extras: record length + date
    image_.push_back(0); // record length
    image_.push_back(0); image_.push_back(0); // date

    // File data (block-aligned)
    image_.insert(image_.end(), data.begin(), data.end());
    int pad = blocks * 254 - (int)data.size();
    for (int i = 0; i < pad; i++) image_.push_back(0);

    // Terminator
    image_.push_back(0);
    return true;
}

bool ArcImage::removeFile(const std::string& name) {
    // ARC removal requires rebuilding the archive
    auto entries = parseEntries();
    std::string petName = asciiToPetscii(name);
    int targetIdx = -1;
    for (int i = 0; i < (int)entries.size(); i++) {
        if (std::string(entries[i].filename, entries[i].fnLen) == petName) {
            targetIdx = i;
            break;
        }
    }
    if (targetIdx < 0) return false;

    // Rebuild: decompress all, remove target, re-add all in stored mode
    std::vector<uint8_t> newImage;
    for (int i = 0; i < (int)entries.size(); i++) {
        if (i == targetIdx) continue;
        auto& e = entries[i];
        // Copy the raw entry header + data from original
        // For simplicity, decompress and re-store
        auto data = decompressEntry(e);
        // Build stored entry
        ArcImage temp;
        temp.format();
        CbmFileType ft = CbmFileType::PRG;
        switch (e.fileType) {
            case 'S': ft = CbmFileType::SEQ; break;
            case 'U': ft = CbmFileType::USR; break;
            case 'R': ft = CbmFileType::REL; break;
            default: break;
        }
        temp.addFile(std::string(e.filename, e.fnLen), ft, data);
        // Append (strip the terminator of temp)
        if (!temp.image_.empty() && temp.image_.back() == 0)
            temp.image_.pop_back();
        newImage.insert(newImage.end(), temp.image_.begin(), temp.image_.end());
    }
    newImage.push_back(0); // terminator
    image_ = std::move(newImage);
    return true;
}
