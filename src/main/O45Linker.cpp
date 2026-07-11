#include "O45Linker.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <cstring>

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
    objects_.push_back({filename, obj, 0, 0, 0, 0, {}});
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

                objects_.push_back({libEntry.filename + "(" + member.name + ")", obj, 0, 0, 0, 0, {}});
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

    // --- Text segment: sub-segment-aware layout ---
    // Priority order for named text sub-segments. "init" comes first.
    static const std::vector<std::string> textSubSegOrder = {"init", "code"};

    // Collect sub-segment chunks from all objects, grouped by sub-segment name.
    struct TextChunk {
        size_t objectIdx;
        uint32_t srcOffset;     // offset within object's textBody
        uint32_t srcLen;        // byte length
        std::string subSegName; // "init", "code", etc.
    };
    std::map<std::string, std::vector<TextChunk>> chunksByName;
    std::vector<TextChunk> defaultChunks; // objects without segAttrs

    for (size_t i = 0; i < objects_.size(); i++) {
        auto& input = objects_[i];
        if (input.obj.textBody.empty()) continue;

        // Check for OPT_SEGATTR records covering the text segment
        std::vector<O45File::SegAttr> textAttrs;
        for (const auto& sa : input.obj.segAttrs) {
            if (sa.segId == SEG_TEXT) textAttrs.push_back(sa);
        }

        if (textAttrs.empty()) {
            // No sub-segments — entire text body is one chunk ("code")
            defaultChunks.push_back({i, 0, (uint32_t)input.obj.textBody.size(), "code"});
        } else {
            // Sort by offset within text body
            std::sort(textAttrs.begin(), textAttrs.end(),
                      [](const O45File::SegAttr& a, const O45File::SegAttr& b) {
                          return a.offset < b.offset;
                      });
            for (const auto& sa : textAttrs) {
                chunksByName[sa.name].push_back({i, sa.offset, sa.length, sa.name});
            }
        }
    }

    // Add default chunks (no segAttrs) to "code" group
    for (auto& dc : defaultChunks) {
        chunksByName["code"].push_back(dc);
    }

    // Build merged text in priority order: known names first, then any remaining
    std::vector<std::string> orderedNames;
    for (const auto& name : textSubSegOrder) {
        if (chunksByName.count(name)) orderedNames.push_back(name);
    }
    for (const auto& [name, chunks] : chunksByName) {
        bool found = false;
        for (const auto& n : orderedNames) if (n == name) { found = true; break; }
        if (!found) orderedNames.push_back(name);
    }

    // Concatenate chunks in order, building remaps for each object
    for (const auto& name : orderedNames) {
        for (const auto& chunk : chunksByName[name]) {
            auto& input = objects_[chunk.objectIdx];
            uint32_t destOffset = (uint32_t)mergedText_.size();

            // Copy bytes from object's textBody
            auto begin = input.obj.textBody.begin() + chunk.srcOffset;
            auto end = begin + chunk.srcLen;
            mergedText_.insert(mergedText_.end(), begin, end);

            // Record remap
            input.textRemaps.push_back({chunk.srcOffset, chunk.srcLen, destOffset, chunk.subSegName});
        }
    }

    // For objects with no sub-segments and no text body processed above,
    // set simple textOffset (already handled via defaultChunks → "code")
    for (auto& input : objects_) {
        if (input.textRemaps.empty() && !input.obj.textBody.empty()) {
            // Shouldn't happen — all objects should have been processed above
            input.textOffset = (uint32_t)mergedText_.size();
            mergedText_.insert(mergedText_.end(),
                               input.obj.textBody.begin(), input.obj.textBody.end());
        } else if (input.textRemaps.size() == 1 && input.textRemaps[0].srcOffset == 0) {
            // Single chunk — set textOffset for backward compatibility
            input.textOffset = input.textRemaps[0].destOffset;
        } else if (!input.textRemaps.empty()) {
            // Multiple chunks — textOffset points to the first chunk
            input.textOffset = input.textRemaps[0].destOffset;
        }
    }

    // --- Data, BSS, ZP: simple concatenation (unchanged) ---
    for (auto& input : objects_) {
        input.dataOffset = (uint32_t)mergedData_.size();
        mergedData_.insert(mergedData_.end(),
                           input.obj.dataBody.begin(), input.obj.dataBody.end());

        input.bssOffset = mergedBssLen_;
        mergedBssLen_ += input.obj.blen;

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
    symbolWeak_.clear();
    symbolSegment_.clear();

    // Collect all exports, handling weak-vs-strong resolution
    for (const auto& input : objects_) {
        for (const auto& exp : input.obj.exports) {
            bool isWeak = exp.isWeak();
            uint8_t segId = exp.segmentId();

            // Compute final absolute address
            uint32_t base = 0;
            uint32_t objOffset = 0;
            switch ((O45Segment)segId) {
                case SEG_TEXT: base = textBase_; objOffset = input.textOffset; break;
                case SEG_DATA: base = dataBase_; objOffset = input.dataOffset; break;
                case SEG_BSS:  base = bssBase_;  objOffset = input.bssOffset;  break;
                case SEG_ZP:   base = zpBase_;   objOffset = input.zpOffset;   break;
                default:       base = 0; objOffset = 0; break;
            }
            uint32_t finalAddr;
            if (segId == SEG_TEXT && !input.textRemaps.empty() && input.textRemaps.size() > 1) {
                // Symbol offset is within the object's original text body;
                // remap to position in the merged text body.
                finalAddr = base + input.remapTextOffset(exp.offset);
            } else {
                finalAddr = base + objOffset + exp.offset;
            }

            if (globalSymbols_.count(exp.name)) {
                bool existingWeak = symbolWeak_[exp.name];
                if (!existingWeak && !isWeak) {
                    // Two strong definitions = error
                    errorMsg = "duplicate symbol '" + exp.name + "' (defined in " +
                               symbolSource_[exp.name] + " and " + input.filename + ")";
                    return false;
                }
                if (existingWeak && !isWeak) {
                    // Strong overrides weak
                    globalSymbols_[exp.name] = finalAddr;
                    symbolSource_[exp.name] = input.filename;
                    symbolWeak_[exp.name] = false;
                }
                // else: existing is strong, or both weak — keep existing
                continue;
            }

            globalSymbols_[exp.name] = finalAddr;
            symbolSource_[exp.name] = input.filename;
            symbolWeak_[exp.name] = isWeak;
            symbolSegment_[exp.name] = segId;
        }
    }

    // Detect cross-segment address collisions (different symbols from different
    // segments placed at the same address — indicates an assembler segment bug)
    {
        // Build address -> (name, segId) map
        std::map<uint32_t, std::pair<std::string, uint8_t>> addrMap;
        for (const auto& [name, addr] : globalSymbols_) {
            auto segIt = symbolSegment_.find(name);
            uint8_t seg = (segIt != symbolSegment_.end()) ? segIt->second : 0;
            auto it = addrMap.find(addr);
            if (it != addrMap.end() && it->second.second != seg) {
                if (warnStream_) {
                    *warnStream_ << "warning: symbols '" << it->second.first
                                 << "' and '" << name << "' overlap at $"
                                 << std::hex << std::uppercase << std::setfill('0')
                                 << std::setw(4) << addr << std::dec
                                 << " (different segments)" << std::endl;
                }
            }
            if (it == addrMap.end()) {
                addrMap[addr] = {name, seg};
            }
        }
    }

    // Inject linker-defined BSS boundary symbols.
    // These are used by _init_bss in the CRT to zero BSS at startup.
    if (!globalSymbols_.count("__bss_start")) {
        globalSymbols_["__bss_start"] = bssBase_;
        symbolSource_["__bss_start"] = "<linker>";
        symbolWeak_["__bss_start"] = false;
    }
    if (!globalSymbols_.count("__bss_end")) {
        globalSymbols_["__bss_end"] = bssBase_ + mergedBssLen_;
        symbolSource_["__bss_end"] = "<linker>";
        symbolWeak_["__bss_end"] = false;
    }

    // Check that all imports are satisfied
    std::set<std::string> importedSymbols;
    for (const auto& input : objects_) {
        for (const auto& imp : input.obj.imports) {
            if (!globalSymbols_.count(imp.name)) {
                errorMsg = "undefined symbol '" + imp.name + "' (referenced in " +
                           input.filename + ")";
                return false;
            }
            importedSymbols.insert(imp.name);
        }
    }

    // Check for unused global symbols
    if (warnStream_) {
        for (const auto& [name, addr] : globalSymbols_) {
            (void)addr;
            if (name != "_main" &&
                !(name.size() >= 2 && name[0] == '_' && name[1] == '_') &&
                importedSymbols.find(name) == importedSymbols.end() &&
                symbolSource_[name] != "<linker>") {
                *warnStream_ << "ln45: warning: unused global symbol '" << name << "' (defined in " << symbolSource_[name] << ")" << std::endl;
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
                             std::string& errorMsg,
                             int objIdx) {
    for (int rIdx = 0; rIdx < (int)relocs.size(); rIdx++) {
        const auto& r = relocs[rIdx];

        // Log TEXT relocations too
        const char* segName = "";
        if (objIdx >= 0 && r.segment == SEG_TEXT) segName = "TEXT";
        else if (r.segment == SEG_DATA) segName = "DATA";
        else if (r.segment == SEG_EXTERNAL) segName = "EXTERNAL";

        // The relocation offset is relative to the object's segment start.
        // If the object has text sub-segment remaps (init/code reordering)
        // and we're patching text, use remapTextOffset for correct positioning.
        uint32_t patchPos;
        if (!input.textRemaps.empty() && input.textRemaps.size() > 1
            && objOffset == input.textOffset) {
            patchPos = input.remapTextOffset(r.offset);
        } else {
            patchPos = objOffset + r.offset;
        }

        // Log all relocations before processing
        if (r.segment == SEG_TEXT || r.segment == SEG_DATA) {
            std::cerr << "DEBUG [Reloc " << rIdx << "]: " << segName
                      << " reloc in " << input.filename
                      << " at offset 0x" << std::hex << r.offset << " (patch pos 0x" << patchPos << ")"
                      << " type=" << (int)r.type << " seg=" << (int)r.segment << std::dec << std::endl;
        }

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
            std::cerr << "DEBUG [External Reloc]: Symbol '" << symName << "' = 0x" << std::hex << it->second << std::dec << std::endl;
            // Read existing value at patch site as addend (e.g., __sp_base+offset
            // has the offset baked in by the assembler)
            uint32_t addend = 0;
            if (r.type == R_HIGH) {
                // For R_HIGH relocations, reconstruct 16-bit addend from:
                // - High byte at patch site
                // - Low byte stored in extra field (see issue #36)
                uint8_t hi = body[patchPos];
                uint8_t lo = r.extra;
                uint16_t w = (hi << 8) | lo;
                addend = (uint32_t)(int32_t)(int16_t)w; // sign-extend 16-bit addend
            } else {
                int pSize = o45RelocPatchSize((uint8_t)r.type);
                for (int i = 0; i < pSize && (patchPos + i) < body.size(); i++) {
                    addend |= ((uint32_t)body[patchPos + i]) << (i * 8);
                }
                // Sign-extend addend if it's smaller than 32-bit
                if (pSize == 1) addend = (uint32_t)(int32_t)(int8_t)(uint8_t)addend;
                else if (pSize == 2) addend = (uint32_t)(int32_t)(int16_t)(uint16_t)addend;
                else if (pSize == 3) {
                    if (addend & 0x800000) addend |= 0xFF000000;
                }
            }
            targetAddr = it->second + addend;

            // Check for thunk override (convention bridge)
            if (objIdx >= 0) {
                auto overIt = callSiteOverrides_.find({objIdx, rIdx});
                if (overIt != callSiteOverrides_.end()) {
                    targetAddr = overIt->second;
                }
            }
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

            // Read the existing value at the patch site and compute target address.
            // The existing value is the assembly-time absolute address of the target.
            // We subtract the object's original segment base to get the segment-relative
            // offset, then add the final segment base + object's offset in merged segment.
            uint32_t existingVal = 0;
            if (r.type == R_HIGH) {
                // extra = original low byte, patch site = high byte
                uint8_t hi = body[patchPos];
                uint8_t lo = r.extra;
                existingVal = (hi << 8) | lo;
            } else {
                int patchSize = o45RelocPatchSize((uint8_t)r.type);
                for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                    existingVal |= ((uint32_t)body[patchPos + i]) << (i * 8);
                }
            }

            // Subtract the object's original base for the target segment to get
            // the segment-relative offset
            uint32_t origBase = 0;
            switch (r.segment) {
                case SEG_TEXT: origBase = input.obj.tbase; break;
                case SEG_DATA: origBase = input.obj.dbase; break;
                case SEG_BSS:  origBase = input.obj.bbase; break;
                case SEG_ZP:   origBase = input.obj.zbase; break;
                default: break;
            }

            uint32_t segRelOff = existingVal - origBase;
            if (r.segment == SEG_TEXT && !input.textRemaps.empty() && input.textRemaps.size() > 1) {
                targetAddr = segBase + input.remapTextOffset(segRelOff);
            } else {
                targetAddr = segBase + segObjOff + segRelOff;
            }

            // DEBUG: Log ALL DATA relocations
            if (r.segment == SEG_DATA) {
                // Log all DATA relocations with detailed information
                std::cerr << "\n=== DEBUG: DATA relocation at offset 0x" << std::hex << patchPos << std::dec << " ===" << std::endl;
                std::cerr << "  RelType: " << (int)r.type << " (32=R_LOW, 64=R_HIGH, 128=R_WORD)" << std::endl;
                std::cerr << "  Extra: 0x" << std::hex << (int)r.extra << std::dec << std::endl;
                std::cerr << "  PatchPos: 0x" << std::hex << patchPos << std::dec << std::endl;
                std::cerr << "  ExistingVal: 0x" << std::hex << existingVal << std::dec << std::endl;
                std::cerr << "  SegBase (dataBase_): 0x" << std::hex << segBase << std::dec << std::endl;
                std::cerr << "  SegObjOff (dataOffset): 0x" << std::hex << segObjOff << std::dec << std::endl;
                std::cerr << "  OrigBase (dbase): 0x" << std::hex << origBase << std::dec << std::endl;
                std::cerr << "  SegRelOff (existingVal - origBase): 0x" << std::hex << segRelOff << std::dec << std::endl;
                std::cerr << "  TargetAddr (segBase + segObjOff + segRelOff): 0x" << std::hex << targetAddr << std::dec << std::endl;

                // Show context: what's before this patch in the body (likely code)
                std::cerr << "  Context (bytes -2 to +3 around patch): ";
                for (int i = -2; i <= 3; i++) {
                    int pos = (int)patchPos + i;
                    if (pos >= 0 && pos < (int)body.size()) {
                        if (i == 0) std::cerr << "[";
                        std::cerr << std::hex << (int)body[pos] << " ";
                        if (i == o45RelocPatchSize((uint8_t)r.type) - 1) std::cerr << "]";
                    }
                }
                std::cerr << std::dec << std::endl;

                std::cerr << "  Bytes before patch: 0x" << std::hex;
                int patchSize = o45RelocPatchSize((uint8_t)r.type);
                for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                    std::cerr << (int)body[patchPos + i] << " ";
                }
                std::cerr << std::dec << std::endl;
            }
        }

        // Patch the bytes at the relocation site
        int patchSize = o45RelocPatchSize((uint8_t)r.type);
        if (patchPos + patchSize > body.size()) {
            errorMsg = "relocation patch overflows segment in " + input.filename;
            return false;
        }

        // DEBUG: Log bytes being patched for DATA relocations
        bool shouldLogPatch = false;
        if (r.segment == SEG_DATA) {
            shouldLogPatch = true;
            std::cerr << "DEBUG [DATA Patch]: at 0x" << std::hex << patchPos << " type=" << (int)r.type << " targetAddr=0x" << targetAddr << std::dec << "\n";
            std::cerr << "  Bytes BEFORE patch: 0x" << std::hex;
            for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                std::cerr << std::setw(2) << std::setfill('0') << (int)body[patchPos + i] << " ";
            }
            std::cerr << std::dec << std::endl;
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

        // DEBUG: Complete logging for DATA relocations
        if (shouldLogPatch) {
            for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                std::cerr << (int)body[patchPos + i] << " ";
            }
            std::cerr << std::dec << std::endl;
            std::cerr << "==================================================\n" << std::endl;
        }

        // DEBUG: Log TEXT relocations patch result
        if (objIdx >= 0 && r.segment == SEG_TEXT) {
            std::cerr << "DEBUG [TEXT Patch]: Patched 0x" << std::hex << patchPos << " with targetAddr=0x" << targetAddr;
            std::cerr << " type=" << (int)r.type << std::dec << " (" << (int)patchSize << " bytes)" << std::endl;
        }
    }

    return true;
}

bool O45Linker::applyRelocations(std::string& errorMsg) {
    std::cout << "DEBUG: applyRelocations called with " << objects_.size() << " objects" << std::endl;
    for (int objIdx = 0; objIdx < (int)objects_.size(); objIdx++) {
        auto& input = objects_[objIdx];
        std::cout << "DEBUG: Processing object " << objIdx << ": " << input.filename << std::endl;
        // Text relocations
        auto textRelocs = O45RelocDecoder::decode(input.obj.textRelocs);
        std::cout << "DEBUG: Decoded " << textRelocs.size() << " TEXT relocations from " << input.filename << std::endl;
        for (const auto& r : textRelocs) {
            const char* segName = "";
            if (r.segment == SEG_EXTERNAL) segName = "EXTERN";
            else if (r.segment == SEG_TEXT) segName = "TEXT";
            else if (r.segment == SEG_DATA) segName = "DATA";
            std::cout << "  TEXT Reloc at offset 0x" << std::hex << r.offset << " type 0x" << (int)r.type
                      << " seg " << segName << " extra 0x" << (int)r.extra << std::dec << std::endl;
        }
        if (!applyRelocs(textRelocs, mergedText_, textBase_, input.textOffset,
                         input, errorMsg, objIdx)) {
            return false;
        }

        // Data relocations
        auto dataRelocs = O45RelocDecoder::decode(input.obj.dataRelocs);
        std::cout << "DEBUG: Decoded " << dataRelocs.size() << " DATA relocations from " << input.filename << std::endl;
        for (const auto& r : dataRelocs) {
            std::cout << "  DATA Reloc at offset 0x" << std::hex << r.offset << " type 0x" << (int)r.type << " seg " << (int)r.segment << " extra 0x" << (int)r.extra << std::dec << std::endl;
        }
        if (!applyRelocs(dataRelocs, mergedData_, dataBase_, input.dataOffset,
                         input, errorMsg, -1)) {
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

    // Phase 3: function attribute integration and debug info
    mergeLineMaps();
    buildFuncAttrs();
    buildCallGraph();
    computeTransitiveClobbers();
    emitDiagnostics();

    // Check for calling convention errors
    if (!convErrors_.empty()) {
        errorMsg = convErrors_[0];
        for (size_t i = 1; i < convErrors_.size(); i++)
            errorMsg += "\n" + convErrors_[i];
        return {};
    }

    // Generate thunks for convention mismatches (appends to mergedText_)
    generateThunks();

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

// =============================================================================
// Phase 3: Function attribute integration
// =============================================================================

// 3.1 — Extract function attributes from all loaded objects into a global map.
void O45Linker::buildFuncAttrs() {
    funcAttrs_.clear();
    for (const auto& input : objects_) {
        for (const auto& exp : input.obj.exports) {
            if (exp.hasFuncAttr) {
                // Use the first (or strong) definition's attributes
                if (!funcAttrs_.count(exp.name)) {
                    funcAttrs_[exp.name] = exp.funcAttr;
                } else if (!symbolWeak_.count(exp.name) || !symbolWeak_[exp.name]) {
                    // Strong definition overrides weak's attributes
                    funcAttrs_[exp.name] = exp.funcAttr;
                }
            }
        }
    }
}

// 3.2 — Build a call graph by scanning JSR relocation entries.
// A JSR instruction is 3 bytes: opcode ($20) + 16-bit address.
// We identify call sites as R_WORD relocations to SEG_EXTERNAL where the
// byte preceding the relocation target is the JSR opcode ($20).
// We also need to determine which function "owns" each call site.
void O45Linker::buildCallGraph() {
    callGraph_.clear();

    for (const auto& input : objects_) {
        // Build a map of text-segment offset -> function name for this object.
        // Exports in text segment, sorted by offset, define function boundaries.
        struct FuncRange {
            std::string name;
            uint32_t startOff; // offset within merged text
            uint32_t endOff;   // exclusive
        };
        std::vector<FuncRange> funcs;
        for (const auto& exp : input.obj.exports) {
            if (exp.segmentId() == SEG_TEXT) {
                funcs.push_back({exp.name, input.textOffset + exp.offset, 0});
            }
        }
        // Sort by start offset
        std::sort(funcs.begin(), funcs.end(),
                  [](const FuncRange& a, const FuncRange& b) { return a.startOff < b.startOff; });
        // Set end offsets (each function ends where the next begins, or at object's text end)
        uint32_t objTextEnd = input.textOffset + input.obj.tlen;
        for (size_t i = 0; i < funcs.size(); i++) {
            funcs[i].endOff = (i + 1 < funcs.size()) ? funcs[i + 1].startOff : objTextEnd;
        }

        // Decode text relocations and identify JSR call sites
        auto textRelocs = O45RelocDecoder::decode(input.obj.textRelocs);
        for (const auto& r : textRelocs) {
            if (r.type != R_WORD) continue;

            // The relocation offset is relative to the object's text body.
            // Check if the byte before the relocation target is JSR ($20).
            if (r.offset == 0) continue; // can't look back
            if (r.offset - 1 >= input.obj.textBody.size()) continue;
            uint8_t opcode = input.obj.textBody[r.offset - 1];
            if (opcode != 0x20) continue; // not JSR

            // Resolve the callee name based on relocation segment type
            std::string callee;
            if (r.segment == SEG_EXTERNAL) {
                // External call — look up in imports
                if (r.symbolIndex >= input.obj.imports.size()) continue;
                callee = input.obj.imports[r.symbolIndex].name;
            } else if (r.segment == SEG_TEXT) {
                // Internal call — look up in exports to get the function name
                // The symbol index for TEXT relocations points to an export within this object
                if (r.symbolIndex >= input.obj.exports.size()) continue;
                const auto& exp = input.obj.exports[r.symbolIndex];
                if (exp.segmentId() != SEG_TEXT) continue; // Skip non-text symbols
                callee = exp.name;
            } else {
                continue; // Ignore other segment types
            }

            // Find which function owns this call site
            uint32_t siteInMerged = input.textOffset + r.offset;
            for (const auto& fn : funcs) {
                if (siteInMerged >= fn.startOff && siteInMerged < fn.endOff) {
                    callGraph_[fn.name].insert(callee);
                    break;
                }
            }
        }
    }
}

// 3.2 — Compute transitive clobber sets through call chains.
// For each function, its effective clobber is the union of its own clobbers
// plus all callees' transitive clobbers.
void O45Linker::computeTransitiveClobbers() {
    transitiveClobbers_.clear();

    // Seed with direct attributes
    for (const auto& [name, attr] : funcAttrs_) {
        transitiveClobbers_[name] = attr;
    }

    // Iterate until stable (handles cycles via convergence)
    bool changed = true;
    int iterations = 0;
    const int maxIterations = 100; // safety limit
    while (changed && iterations++ < maxIterations) {
        changed = false;
        for (const auto& [caller, callees] : callGraph_) {
            auto& callerAttr = transitiveClobbers_[caller];
            for (const auto& callee : callees) {
                auto it = transitiveClobbers_.find(callee);
                if (it == transitiveClobbers_.end()) continue;
                const auto& calleeAttr = it->second;

                // Merge: union of clobber sets
                uint8_t newRegClob = callerAttr.regClobbers | calleeAttr.regClobbers;
                uint8_t newFlagClob = callerAttr.flagClobbers | calleeAttr.flagClobbers;
                uint32_t newZpClob = callerAttr.zpClobbers | calleeAttr.zpClobbers;

                if (newRegClob != callerAttr.regClobbers ||
                    newFlagClob != callerAttr.flagClobbers ||
                    newZpClob != callerAttr.zpClobbers) {
                    callerAttr.regClobbers = newRegClob;
                    callerAttr.flagClobbers = newFlagClob;
                    callerAttr.zpClobbers = newZpClob;
                    changed = true;
                }
            }
        }
    }
}

// 3.3 — Emit diagnostics: enforce calling convention compatibility.
// Detect all mismatches (both directions). In THUNK_ERROR mode, report them
// as errors. In THUNK_AUTO/THUNK_WARN mode, record them for thunk generation.
void O45Linker::emitDiagnostics() {
    convErrors_.clear();

    for (const auto& [caller, callees] : callGraph_) {
        auto callerIt = funcAttrs_.find(caller);
        if (callerIt == funcAttrs_.end()) continue;
        bool callerIsZp = (callerIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;

        for (const auto& callee : callees) {
            auto calleeIt = funcAttrs_.find(callee);
            if (calleeIt == funcAttrs_.end()) continue;
            bool calleeIsZp = (calleeIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;

            if (callerIsZp == calleeIsZp) continue; // same convention, fine

            std::string direction = callerIsZp ? "zp_call -> stack_call" : "stack_call -> zp_call";
            std::string msg = "calling convention mismatch: '" + caller + "' calls '" + callee + "' (" + direction + ")";

            if (thunkMode_ == THUNK_ERROR) {
                convErrors_.push_back(msg);
            } else if (warnStream_) {
                *warnStream_ << "ln45: warning: " << msg << " (generating thunk)" << std::endl;
            }
        }
    }
}

// 3.4 — Generate convention bridge thunks.
// Appended to mergedText_, symbol addresses updated for relocation patching.
void O45Linker::generateThunks() {
    if (thunkMode_ == THUNK_ERROR) return;

    // Collect unique (caller_conv, callee) pairs that need thunks
    struct ThunkNeed {
        std::string callee;
        bool callerIsZp;   // true = ZP caller → stack callee (z2s), false = stack caller → ZP callee (s2z)
    };
    std::map<std::string, ThunkNeed> needed; // key: thunk symbol name

    for (const auto& [caller, callees] : callGraph_) {
        auto callerIt = funcAttrs_.find(caller);
        if (callerIt == funcAttrs_.end()) continue;
        bool callerIsZp = (callerIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;

        for (const auto& callee : callees) {
            auto calleeIt = funcAttrs_.find(callee);
            if (calleeIt == funcAttrs_.end()) continue;
            bool calleeIsZp = (calleeIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;
            if (callerIsZp == calleeIsZp) continue;

            std::string prefix = callerIsZp ? "__thunk_z2s_" : "__thunk_s2z_";
            std::string thunkName = prefix + callee;
            if (!needed.count(thunkName)) {
                needed[thunkName] = {callee, callerIsZp};
            }
        }
    }

    if (needed.empty()) return;

    // ZP param base for zpcall convention (matches CodeGenerator/IRCodeGen: zpBase = zpBase_ + 8)
    uint8_t zpParamBase = (uint8_t)(zpBase_ + 8);

    // __sp_base symbol (needed for stack-relative access in thunks)
    uint32_t spBase = 0x0101; // default
    auto spIt = globalSymbols_.find("__sp_base");
    if (spIt != globalSymbols_.end()) spBase = spIt->second;

    for (auto& [thunkName, need] : needed) {
        auto calleeIt = funcAttrs_.find(need.callee);
        uint8_t paramSize = (calleeIt != funcAttrs_.end()) ? calleeIt->second.paramSize : 0;
        auto calleeAddrIt = globalSymbols_.find(need.callee);
        if (calleeAddrIt == globalSymbols_.end()) continue;
        uint16_t calleeAddr = (uint16_t)calleeAddrIt->second;

        uint32_t thunkAddr = textBase_ + (uint32_t)mergedText_.size();
        thunkAddresses_[thunkName] = thunkAddr;
        globalSymbols_[thunkName] = thunkAddr;

        if (!need.callerIsZp) {
            // Stack→ZP thunk (s2z): copy stack args to ZP param slots, then JMP callee
            // Stack layout at entry: [ret_lo, ret_hi, param0, param1, ...]
            // TSX; LDA spBase+2+i,X; STA $03+i  for each param byte
            for (int i = 0; i < paramSize; i++) {
                mergedText_.push_back(0xBA);                       // TSX
                mergedText_.push_back(0xBD);                       // LDA abs,X
                uint16_t srcAddr = (uint16_t)(spBase + 2 + i);
                mergedText_.push_back((uint8_t)(srcAddr & 0xFF));
                mergedText_.push_back((uint8_t)(srcAddr >> 8));
                mergedText_.push_back(0x85);                       // STA zp
                mergedText_.push_back((uint8_t)(zpParamBase + i));
            }
            // JMP callee (tail-call; callee's RTS returns directly to original caller)
            mergedText_.push_back(0x4C);                           // JMP abs
            mergedText_.push_back((uint8_t)(calleeAddr & 0xFF));
            mergedText_.push_back((uint8_t)(calleeAddr >> 8));
        } else {
            // ZP→Stack thunk (z2s): push ZP params onto stack, JSR callee, clean up, RTS
            // Push params in reverse order (highest address first) so they appear
            // in the correct order on the stack for the callee
            for (int i = paramSize - 1; i >= 0; i--) {
                mergedText_.push_back(0xA5);                       // LDA zp
                mergedText_.push_back((uint8_t)(zpParamBase + i));
                mergedText_.push_back(0x48);                       // PHA
            }
            // JSR callee
            mergedText_.push_back(0x20);                           // JSR abs
            mergedText_.push_back((uint8_t)(calleeAddr & 0xFF));
            mergedText_.push_back((uint8_t)(calleeAddr >> 8));
            // Clean up stack: remove paramSize bytes
            // TSX; TXA; CLC; ADC #paramSize; TAX; TXS
            if (paramSize > 0) {
                mergedText_.push_back(0xBA);                       // TSX
                mergedText_.push_back(0x8A);                       // TXA
                mergedText_.push_back(0x18);                       // CLC
                mergedText_.push_back(0x69);                       // ADC #imm
                mergedText_.push_back((uint8_t)paramSize);
                mergedText_.push_back(0xAA);                       // TAX
                mergedText_.push_back(0x9A);                       // TXS
            }
            // RTS (return to ZP caller)
            mergedText_.push_back(0x60);                           // RTS
        }
    }

    // Now build call-site overrides: for each object's JSR relocation that targets
    // a mismatched callee, record the thunk address to use instead.
    for (int objIdx = 0; objIdx < (int)objects_.size(); objIdx++) {
        const auto& input = objects_[objIdx];
        auto textRelocs = O45RelocDecoder::decode(input.obj.textRelocs);

        // Build function ranges for this object (same logic as buildCallGraph)
        struct FuncRange { std::string name; uint32_t startOff, endOff; };
        std::vector<FuncRange> funcs;
        for (const auto& exp : input.obj.exports) {
            if (exp.segmentId() == SEG_TEXT)
                funcs.push_back({exp.name, input.textOffset + exp.offset, 0});
        }
        std::sort(funcs.begin(), funcs.end(),
                  [](const FuncRange& a, const FuncRange& b) { return a.startOff < b.startOff; });
        uint32_t objTextEnd = input.textOffset + input.obj.tlen;
        for (size_t i = 0; i < funcs.size(); i++)
            funcs[i].endOff = (i + 1 < funcs.size()) ? funcs[i + 1].startOff : objTextEnd;

        for (int rIdx = 0; rIdx < (int)textRelocs.size(); rIdx++) {
            const auto& r = textRelocs[rIdx];
            if (r.type != R_WORD) continue;
            if (r.offset == 0 || r.offset - 1 >= input.obj.textBody.size()) continue;
            if (input.obj.textBody[r.offset - 1] != 0x20) continue; // not JSR

            // Get callee name
            std::string callee;
            if (r.segment == SEG_EXTERNAL) {
                if (r.symbolIndex >= input.obj.imports.size()) continue;
                callee = input.obj.imports[r.symbolIndex].name;
            } else if (r.segment == SEG_TEXT) {
                if (r.symbolIndex >= input.obj.exports.size()) continue;
                callee = input.obj.exports[r.symbolIndex].name;
            } else continue;

            // Find caller
            uint32_t siteInMerged = input.textOffset + r.offset;
            std::string caller;
            for (const auto& fn : funcs) {
                if (siteInMerged >= fn.startOff && siteInMerged < fn.endOff) {
                    caller = fn.name;
                    break;
                }
            }
            if (caller.empty()) continue;

            auto callerIt = funcAttrs_.find(caller);
            auto calleeIt = funcAttrs_.find(callee);
            if (callerIt == funcAttrs_.end() || calleeIt == funcAttrs_.end()) continue;

            bool callerIsZp = (callerIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;
            bool calleeIsZp = (calleeIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;
            if (callerIsZp == calleeIsZp) continue;

            std::string prefix = callerIsZp ? "__thunk_z2s_" : "__thunk_s2z_";
            std::string thunkName = prefix + callee;
            auto thunkIt = thunkAddresses_.find(thunkName);
            if (thunkIt != thunkAddresses_.end()) {
                callSiteOverrides_[{objIdx, rIdx}] = thunkIt->second;
            }
        }
    }
}

// =============================================================================
// Debug line map
// =============================================================================

void O45Linker::mergeLineMaps() {
    mergedLineMap_.clear();
    for (const auto& input : objects_) {
        if (input.obj.lineInfos.empty()) continue;
        for (const auto& li : input.obj.lineInfos) {
            std::string file;
            if (li.fileIndex < input.obj.lineFiles.size())
                file = input.obj.lineFiles[li.fileIndex];
            uint32_t addr = textBase_ + input.remapTextOffset(li.textOffset);
            mergedLineMap_.push_back({addr, file, (int)li.line});
        }
    }
    // Sort by address
    std::sort(mergedLineMap_.begin(), mergedLineMap_.end(),
              [](const MergedLineEntry& a, const MergedLineEntry& b) { return a.address < b.address; });
}

void O45Linker::writeLineMap(std::ostream& out) const {
    if (mergedLineMap_.empty()) return;
    out << "[\n";
    std::string prevFile;
    int prevLine = -1;
    bool first = true;
    for (size_t i = 0; i < mergedLineMap_.size(); i++) {
        const auto& e = mergedLineMap_[i];
        if (e.file == prevFile && e.line == prevLine) continue;
        if (!first) out << ",\n";
        out << "  {\"addr\":" << e.address
            << ",\"file\":\"" << e.file
            << "\",\"line\":" << e.line << "}";
        prevFile = e.file;
        prevLine = e.line;
        first = false;
    }
    out << "\n]\n";
}

// =============================================================================
// Map file output
// =============================================================================

void O45Linker::writeMap(std::ostream& out) const {
    // --- Memory layout ---
    out << "Linker Map\n";
    out << "==========\n\n";

    out << "Memory Layout\n";
    out << "-------------\n";
    auto printSeg = [&](const char* name, uint32_t base, uint32_t len) {
        if (len == 0) return;
        char buf[80];
        snprintf(buf, sizeof(buf), "  %-6s $%04X - $%04X  (%u bytes)\n",
                 name, base, base + len - 1, len);
        out << buf;
    };
    printSeg("TEXT", textBase_, (uint32_t)mergedText_.size());
    printSeg("DATA", dataBase_, (uint32_t)mergedData_.size());
    printSeg("BSS", bssBase_, mergedBssLen_);
    printSeg("ZP", zpBase_, mergedZpLen_);
    out << "\n";

    // --- Per-object contributions ---
    out << "Object Files\n";
    out << "------------\n";
    for (const auto& input : objects_) {
        out << "  " << input.filename << "\n";
        char buf[80];
        if (input.obj.tlen > 0) {
            uint32_t start = textBase_ + input.textOffset;
            snprintf(buf, sizeof(buf), "    TEXT  $%04X - $%04X  (%u bytes)\n",
                     start, start + input.obj.tlen - 1, input.obj.tlen);
            out << buf;
        }
        if (input.obj.dlen > 0) {
            uint32_t start = dataBase_ + input.dataOffset;
            snprintf(buf, sizeof(buf), "    DATA  $%04X - $%04X  (%u bytes)\n",
                     start, start + input.obj.dlen - 1, input.obj.dlen);
            out << buf;
        }
        if (input.obj.blen > 0) {
            uint32_t start = bssBase_ + input.bssOffset;
            snprintf(buf, sizeof(buf), "    BSS   $%04X - $%04X  (%u bytes)\n",
                     start, start + input.obj.blen - 1, input.obj.blen);
            out << buf;
        }
        if (input.obj.zlen > 0) {
            uint32_t start = zpBase_ + input.zpOffset;
            snprintf(buf, sizeof(buf), "    ZP    $%04X - $%04X  (%u bytes)\n",
                     start, start + input.obj.zlen - 1, input.obj.zlen);
            out << buf;
        }
    }
    out << "\n";

    // --- Symbols sorted by address ---
    out << "Symbols (by address)\n";
    out << "--------------------\n";

    // Build address-sorted list
    std::vector<std::pair<uint32_t, std::string>> sorted;
    sorted.reserve(globalSymbols_.size());
    for (const auto& [name, addr] : globalSymbols_) {
        sorted.push_back({addr, name});
    }
    std::sort(sorted.begin(), sorted.end());

    for (const auto& [addr, name] : sorted) {
        char buf[120];
        auto srcIt = symbolSource_.find(name);
        auto weakIt = symbolWeak_.find(name);
        const char* weakTag = (weakIt != symbolWeak_.end() && weakIt->second) ? " [weak]" : "";
        snprintf(buf, sizeof(buf), "  $%04X  %-30s %s%s",
                 addr, name.c_str(),
                 srcIt != symbolSource_.end() ? srcIt->second.c_str() : "",
                 weakTag);
        out << buf;

        // Append function attributes if present
        auto attrIt = funcAttrs_.find(name);
        if (attrIt != funcAttrs_.end()) {
            const auto& fa = attrIt->second;
            out << "  [";
            // ZP mask formatter
            auto fmtZp = [](uint32_t mask) -> std::string {
                if (mask == 0) return "-";
                int first = -1, last = -1;
                for (int i = 0; i < 32; i++) {
                    if (mask & (1u << i)) {
                        if (first < 0) first = i;
                        last = i;
                    }
                }
                char b[16];
                // ZP param slots start at $03 (zeroPageStart+1, default $02+1)
                if (first == last)
                    snprintf(b, sizeof(b), "%02X", 0x03 + first);
                else
                    snprintf(b, sizeof(b), "%02X-%02X", 0x03 + first, 0x03 + last);
                return b;
            };
            out << "uses:" << fmtZp(fa.zpUses);
            out << " clob:" << fmtZp(fa.zpClobbers);
            out << " rel:" << fmtZp(fa.zpRelease);
            std::string regs;
            if (fa.regClobbers & 0x01) regs += 'A';
            if (fa.regClobbers & 0x02) regs += 'X';
            if (fa.regClobbers & 0x04) regs += 'Y';
            if (fa.regClobbers & 0x08) regs += 'Z';
            out << " regs:" << (regs.empty() ? "-" : regs);
            out << "]";
        }

        // Append transitive clobbers if different from direct
        auto transIt = transitiveClobbers_.find(name);
        if (transIt != transitiveClobbers_.end() && attrIt != funcAttrs_.end()) {
            const auto& tc = transIt->second;
            const auto& fa = attrIt->second;
            if (tc.zpClobbers != fa.zpClobbers || tc.regClobbers != fa.regClobbers) {
                auto fmtZp = [](uint32_t mask) -> std::string {
                    if (mask == 0) return "-";
                    int first = -1, last = -1;
                    for (int i = 0; i < 32; i++) {
                        if (mask & (1u << i)) {
                            if (first < 0) first = i;
                            last = i;
                        }
                    }
                    char b[16];
                    if (first == last)
                        snprintf(b, sizeof(b), "%02X", 0x03 + first);
                    else
                        snprintf(b, sizeof(b), "%02X-%02X", 0x03 + first, 0x03 + last);
                    return b;
                };
                out << " (trans: clob:" << fmtZp(tc.zpClobbers);
                std::string regs;
                if (tc.regClobbers & 0x01) regs += 'A';
                if (tc.regClobbers & 0x02) regs += 'X';
                if (tc.regClobbers & 0x04) regs += 'Y';
                if (tc.regClobbers & 0x08) regs += 'Z';
                out << " regs:" << (regs.empty() ? "-" : regs) << ")";
            }
        }

        out << "\n";
    }

    // Call graph section
    if (!callGraph_.empty()) {
        out << "\nCall Graph\n";
        out << "----------\n";
        for (const auto& [caller, callees] : callGraph_) {
            out << "  " << caller << " ->";
            for (const auto& callee : callees) {
                out << " " << callee;
            }
            out << "\n";
        }
    }

    // Source line map section — one entry per unique file:line (first address only)
    if (!mergedLineMap_.empty()) {
        out << "\nSource Lines\n";
        out << "------------\n";
        std::set<std::pair<std::string, int>> seen;
        for (const auto& entry : mergedLineMap_) {
            auto key = std::make_pair(entry.file, entry.line);
            if (seen.insert(key).second) {
                char buf[256];
                snprintf(buf, sizeof(buf), "  $%04X  %s:%d\n",
                         entry.address, entry.file.c_str(), entry.line);
                out << buf;
            }
        }
    }
}
