#include "O45Linker.hpp"
#include <algorithm>

// =============================================================================
// O45RelocDecoder — inverse of O45RelocEncoder
// =============================================================================

std::vector<O45Reloc> O45RelocDecoder::decode(const std::vector<uint8_t>& raw) {
    std::vector<O45Reloc> out;
    size_t pos = 0;
    uint32_t offset = 0;

    while (pos < raw.size()) {
        uint8_t delta = raw[pos++];
        if (delta == O45_RELOC_END) break;

        // Handle escape sequences
        while (delta == O45_RELOC_ESCAPE) {
            offset += O45_RELOC_SKIP;
            if (pos >= raw.size()) return out;
            delta = raw[pos++];
        }
        offset += delta;

        if (pos >= raw.size()) break;
        uint8_t typeSeg = raw[pos++];

        O45Reloc r;
        r.offset = offset;
        r.type = (O45RelocType)(typeSeg & O45_RTYPE_MASK);
        r.segment = (O45Segment)(typeSeg & O45_RSEG_MASK);

        // R_HIGH has an extra byte
        if (r.type == R_HIGH) {
            if (pos >= raw.size()) break;
            r.extra = raw[pos++];
        }

        // External references have a 4-byte symbol index
        // (O45Reader always widens 16-bit .o65 indices to 4 bytes)
        if (r.segment == SEG_EXTERNAL) {
            if (pos + 4 > raw.size()) break;
            r.symbolIndex = raw[pos] | (raw[pos+1] << 8) | (raw[pos+2] << 16) | (raw[pos+3] << 24);
            pos += 4;
        }

        out.push_back(r);
    }

    return out;
}

// =============================================================================
// O45Linker
// =============================================================================

void O45Linker::addObject(const std::string& filename, const O45File& obj) {
    objects_.push_back({filename, obj, 0, 0, 0, 0});
}

void O45Linker::addLibrary(const std::string& filename, const Ar45Archive& lib) {
    libraries_.push_back({filename, lib});
}

// Selectively pull library members that resolve undefined symbols.
// Iterates until no more symbols can be resolved (handles chains where
// pulling member A introduces a new import satisfied by member B).
bool O45Linker::resolveLibraries(std::string& errorMsg) {
    // Collect all defined symbols from current objects
    std::set<std::string> defined;
    for (const auto& input : objects_) {
        for (const auto& exp : input.obj.exports) {
            defined.insert(exp.name);
        }
    }

    // Collect all undefined symbols from current objects
    std::set<std::string> undefined;
    for (const auto& input : objects_) {
        for (const auto& imp : input.obj.imports) {
            if (!defined.count(imp.name)) undefined.insert(imp.name);
        }
    }

    // Track which library members have been pulled in
    std::set<std::string> pulledMembers; // "libname:membername"

    bool changed = true;
    while (changed && !undefined.empty()) {
        changed = false;
        for (const auto& libEntry : libraries_) {
            for (const auto& sym : libEntry.lib.symbols) {
                if (!undefined.count(sym.name)) continue;

                std::string key = libEntry.filename + ":" + libEntry.lib.members[sym.memberIndex].name;
                if (pulledMembers.count(key)) continue;

                // Pull this member
                const auto& member = libEntry.lib.members[sym.memberIndex];
                O45File obj;
                std::string err;
                if (!O45Reader::read(member.data, obj, err)) {
                    errorMsg = "error reading " + member.name + " from " +
                               libEntry.filename + ": " + err;
                    return false;
                }

                objects_.push_back({libEntry.filename + "(" + member.name + ")", obj, 0, 0, 0, 0});
                pulledMembers.insert(key);
                changed = true;

                // Update defined/undefined sets
                for (const auto& exp : obj.exports) {
                    defined.insert(exp.name);
                    undefined.erase(exp.name);
                }
                for (const auto& imp : obj.imports) {
                    if (!defined.count(imp.name)) undefined.insert(imp.name);
                }
            }
        }
    }

    (void)errorMsg;
    return true;
}

uint32_t O45Linker::segmentBase(O45Segment seg) const {
    switch (seg) {
        case SEG_TEXT: return textBase_;
        case SEG_DATA: return dataBase_;
        case SEG_BSS:  return bssBase_;
        case SEG_ZP:   return zpBase_;
        default:       return 0;
    }
}

// --- Step 1: Layout segments ---
// Concatenate each object's segment bodies and assign per-object offsets.

bool O45Linker::layoutSegments(std::string& errorMsg) {
    mergedText_.clear();
    mergedData_.clear();
    mergedBssLen_ = 0;
    mergedZpLen_ = 0;

    for (auto& input : objects_) {
        // Text
        input.textOffset = (uint32_t)mergedText_.size();
        mergedText_.insert(mergedText_.end(),
                           input.obj.textBody.begin(), input.obj.textBody.end());

        // Data
        input.dataOffset = (uint32_t)mergedData_.size();
        mergedData_.insert(mergedData_.end(),
                           input.obj.dataBody.begin(), input.obj.dataBody.end());

        // BSS
        input.bssOffset = mergedBssLen_;
        mergedBssLen_ += input.obj.blen;

        // ZP
        input.zpOffset = mergedZpLen_;
        mergedZpLen_ += input.obj.zlen;
    }

    // Auto-place data right after text if not explicitly set
    if (!dataBaseSet_) {
        dataBase_ = textBase_ + (uint32_t)mergedText_.size();
    }
    if (!bssBaseSet_) {
        bssBase_ = dataBase_ + (uint32_t)mergedData_.size();
    }
    if (!zpBaseSet_) {
        zpBase_ = 0x02; // default ZP start
    }

    (void)errorMsg;
    return true;
}

// --- Step 2: Resolve symbols ---
// Build a global symbol map from all exports. Check imports.

bool O45Linker::resolveSymbols(std::string& errorMsg) {
    globalSymbols_.clear();
    symbolSource_.clear();

    // Collect all exports
    for (const auto& input : objects_) {
        for (const auto& exp : input.obj.exports) {
            // Compute final absolute address
            uint32_t base = 0;
            uint32_t objOffset = 0;
            switch ((O45Segment)exp.segment) {
                case SEG_TEXT: base = textBase_; objOffset = input.textOffset; break;
                case SEG_DATA: base = dataBase_; objOffset = input.dataOffset; break;
                case SEG_BSS:  base = bssBase_;  objOffset = input.bssOffset;  break;
                case SEG_ZP:   base = zpBase_;   objOffset = input.zpOffset;   break;
                default:       base = 0; objOffset = 0; break;
            }
            uint32_t finalAddr = base + objOffset + exp.offset;

            // Check for duplicates
            if (globalSymbols_.count(exp.name)) {
                errorMsg = "duplicate symbol '" + exp.name + "' (defined in " +
                           symbolSource_[exp.name] + " and " + input.filename + ")";
                return false;
            }

            globalSymbols_[exp.name] = finalAddr;
            symbolSource_[exp.name] = input.filename;
        }
    }

    // Check that all imports are satisfied
    for (const auto& input : objects_) {
        for (const auto& imp : input.obj.imports) {
            if (!globalSymbols_.count(imp.name)) {
                errorMsg = "undefined symbol '" + imp.name + "' (referenced in " +
                           input.filename + ")";
                return false;
            }
        }
    }

    return true;
}

// --- Step 3: Apply relocations ---
// Decode each object's reloc tables and patch the merged segment bodies.

bool O45Linker::applyRelocs(const std::vector<O45Reloc>& relocs,
                             std::vector<uint8_t>& body,
                             uint32_t /*bodyBase*/,
                             uint32_t objOffset,
                             const InputObject& input,
                             std::string& errorMsg) {
    for (const auto& r : relocs) {
        // The relocation offset is relative to the object's segment start.
        // In the merged body, add the object's offset within the merged segment.
        uint32_t patchPos = objOffset + r.offset;

        if (patchPos >= body.size()) {
            errorMsg = "relocation offset " + std::to_string(patchPos) +
                       " out of range in " + input.filename;
            return false;
        }

        // Determine the target address
        uint32_t targetAddr = 0;

        if (r.segment == SEG_EXTERNAL) {
            // Look up the import name
            if (r.symbolIndex >= input.obj.imports.size()) {
                errorMsg = "invalid symbol index " + std::to_string(r.symbolIndex) +
                           " in " + input.filename;
                return false;
            }
            const std::string& symName = input.obj.imports[r.symbolIndex].name;
            auto it = globalSymbols_.find(symName);
            if (it == globalSymbols_.end()) {
                errorMsg = "undefined symbol '" + symName + "' in " + input.filename;
                return false;
            }
            targetAddr = it->second;
        } else {
            // Internal relocation — read the current value at the patch site
            // and add the segment base + object offset for that segment.
            uint32_t segBase = segmentBase(r.segment);
            uint32_t segObjOff = 0;
            switch (r.segment) {
                case SEG_TEXT: segObjOff = input.textOffset; break;
                case SEG_DATA: segObjOff = input.dataOffset; break;
                case SEG_BSS:  segObjOff = input.bssOffset;  break;
                case SEG_ZP:   segObjOff = input.zpOffset;   break;
                default: break;
            }

            // Read the existing value at the patch site
            uint32_t existingVal = 0;
            int patchSize = o45RelocPatchSize((uint8_t)r.type);
            for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                existingVal |= ((uint32_t)body[patchPos + i]) << (i * 8);
            }

            targetAddr = segBase + segObjOff + existingVal;
        }

        // Patch the bytes at the relocation site
        int patchSize = o45RelocPatchSize((uint8_t)r.type);
        if (patchPos + patchSize > body.size()) {
            errorMsg = "relocation patch overflows segment in " + input.filename;
            return false;
        }

        switch (r.type) {
            case R_WORD:
                body[patchPos]     = (uint8_t)(targetAddr & 0xFF);
                body[patchPos + 1] = (uint8_t)((targetAddr >> 8) & 0xFF);
                break;
            case R_LOW:
                body[patchPos] = (uint8_t)(targetAddr & 0xFF);
                break;
            case R_HIGH:
                body[patchPos] = (uint8_t)((targetAddr >> 8) & 0xFF);
                break;
            case R_LINEAR24:
                body[patchPos]     = (uint8_t)(targetAddr & 0xFF);
                body[patchPos + 1] = (uint8_t)((targetAddr >> 8) & 0xFF);
                body[patchPos + 2] = (uint8_t)((targetAddr >> 16) & 0xFF);
                break;
            case R_LINEAR32:
                body[patchPos]     = (uint8_t)(targetAddr & 0xFF);
                body[patchPos + 1] = (uint8_t)((targetAddr >> 8) & 0xFF);
                body[patchPos + 2] = (uint8_t)((targetAddr >> 16) & 0xFF);
                body[patchPos + 3] = (uint8_t)((targetAddr >> 24) & 0xFF);
                break;
            case R_SEGADR:
                body[patchPos]     = (uint8_t)(targetAddr & 0xFF);
                body[patchPos + 1] = (uint8_t)((targetAddr >> 8) & 0xFF);
                body[patchPos + 2] = (uint8_t)((targetAddr >> 16) & 0xFF);
                break;
            default:
                errorMsg = "unknown relocation type in " + input.filename;
                return false;
        }
    }

    return true;
}

bool O45Linker::applyRelocations(std::string& errorMsg) {
    for (auto& input : objects_) {
        // Text relocations
        auto textRelocs = O45RelocDecoder::decode(input.obj.textRelocs);
        if (!applyRelocs(textRelocs, mergedText_, textBase_, input.textOffset,
                         input, errorMsg)) {
            return false;
        }

        // Data relocations
        auto dataRelocs = O45RelocDecoder::decode(input.obj.dataRelocs);
        if (!applyRelocs(dataRelocs, mergedData_, dataBase_, input.dataOffset,
                         input, errorMsg)) {
            return false;
        }
    }

    return true;
}

// --- Main link entry point ---

std::vector<uint8_t> O45Linker::link(std::string& errorMsg, bool isPrg) {
    if (objects_.empty()) {
        errorMsg = "no input objects";
        return {};
    }

    // Selectively pull library members to resolve undefined symbols
    if (!resolveLibraries(errorMsg)) return {};

    if (!layoutSegments(errorMsg)) return {};
    if (!resolveSymbols(errorMsg)) return {};
    if (!applyRelocations(errorMsg)) return {};

    // Build the output binary: text + data (BSS is not emitted)
    std::vector<uint8_t> binary;

    if (isPrg) {
        // 2-byte load address header (little-endian)
        binary.push_back((uint8_t)(textBase_ & 0xFF));
        binary.push_back((uint8_t)((textBase_ >> 8) & 0xFF));
    }

    binary.insert(binary.end(), mergedText_.begin(), mergedText_.end());

    // If data follows text contiguously, append it directly.
    // If there's a gap, fill with zeros.
    uint32_t textEnd = textBase_ + (uint32_t)mergedText_.size();
    if (!mergedData_.empty()) {
        if (dataBase_ > textEnd) {
            // Fill gap between text and data
            binary.insert(binary.end(), dataBase_ - textEnd, 0x00);
        }
        binary.insert(binary.end(), mergedData_.begin(), mergedData_.end());
    }

    return binary;
}
