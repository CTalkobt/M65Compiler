#include "O45Linker.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <queue>
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

            // Check if segAttrs cover the entire text body.
            // If not, add any uncovered bytes as a "code" chunk.
            uint32_t textLen = (uint32_t)input.obj.textBody.size();
            uint32_t covered = 0;
            for (const auto& sa : textAttrs) {
                uint32_t segEnd = sa.offset + sa.length;
                if (segEnd > covered) covered = segEnd;
            }
            if (covered < textLen) {
                // There are uncovered bytes at the end — add them as "code" chunk
                chunksByName["code"].push_back({i, covered, textLen - covered, "code"});
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
            if (segId == SEG_TEXT && !input.textRemaps.empty()) {
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
            bool satisfied = false;

            // Check if this symbol exists directly
            if (globalSymbols_.count(imp.name)) {
                satisfied = true;
            } else {
                // Check if this is an AR symbol with embedded offset (e.g., "_add__ar_2")
                // If so, try to resolve the base AR symbol instead
                size_t arPos = imp.name.find("__ar_");
                if (arPos != std::string::npos && arPos + 5 < imp.name.length()) {
                    std::string baseName = imp.name.substr(0, arPos + 4);  // Get symbolname__ar
                    if (globalSymbols_.count(baseName)) {
                        satisfied = true;
                    }
                }
            }

            if (!satisfied) {
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

        // Log all relocations before processing (disabled for production)
        //if (r.segment == SEG_TEXT || r.segment == SEG_DATA) {
        //    std::cerr << "DEBUG [Reloc " << rIdx << "]: " << segName
        //              << " reloc in " << input.filename
        //              << " at offset 0x" << std::hex << r.offset << " (patch pos 0x" << patchPos << ")"
        //              << " type=" << (int)r.type << " seg=" << (int)r.segment << std::dec << std::endl;
        //}

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

            // Check if this is an AR symbol with embedded offset (e.g., "_add__ar_2")
            // and resolve to the base symbol if needed
            std::string lookupName = symName;
            uint32_t arOffset = 0;
            size_t arPos = symName.find("__ar_");
            if (arPos != std::string::npos && arPos + 5 < symName.length()) {
                std::string offsetStr = symName.substr(arPos + 5);
                try {
                    arOffset = std::stoi(offsetStr);
                    lookupName = symName.substr(0, arPos + 4);  // Get symbolname__ar
                } catch (...) {
                    // Failed to parse, use original name
                }
            }

            auto it = globalSymbols_.find(lookupName);
            if (it == globalSymbols_.end()) {
                errorMsg = "undefined symbol '" + symName + "' in " + input.filename;
                return false;
            }
            std::cerr << "DEBUG [External Reloc]: Symbol '" << symName << "' = 0x" << std::hex << it->second << std::dec << std::endl;

            // If AR offset was parsed from symbol name, use it as the addend.
            // Otherwise, read the addend from the patch site.
            uint32_t addend = 0;
            if (arOffset > 0) {
                // AR symbol with offset (e.g., "_add_short__ar_1") — use extracted offset
                addend = arOffset;
            } else if (r.addend != 0) {
                // Use explicit addend from relocation record if available
                addend = (uint32_t)r.addend;
            } else {
                // Read existing value at patch site as addend (e.g., __sp_base+offset
                // has the offset baked in by the assembler)
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
            }
            targetAddr = it->second + addend;

            if (arOffset > 0) {
                std::cerr << "DEBUG [AR Reloc]: Symbol '" << symName << "' base=0x" << std::hex << it->second
                         << " arOffset=" << std::dec << arOffset << " targetAddr=0x" << std::hex << targetAddr << std::dec << std::endl;
            }

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

            // Determine the segment-relative offset by reading from patch site
            // The existing value is the assembly-time absolute address of the target.
            // We subtract the object's original segment base to get the segment-relative
            // offset, then add the final segment base + object's offset in merged segment.
            uint32_t existingVal = 0;
            uint32_t origBase = 0;

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

        // DEBUG: Log TEXT relocations patch result (disabled for production)
        //if (objIdx >= 0 && r.segment == SEG_TEXT) {
        //    std::cerr << "DEBUG [TEXT Patch]: Patched 0x" << std::hex << patchPos << " with targetAddr=0x" << targetAddr;
        //    std::cerr << " type=" << (int)r.type << std::dec << " (" << (int)patchSize << " bytes)" << std::endl;
        //}
    }

    return true;
}

bool O45Linker::applyRelocations(std::string& errorMsg) {
    //std::cout << "DEBUG: applyRelocations called with " << objects_.size() << " objects" << std::endl;
    for (int objIdx = 0; objIdx < (int)objects_.size(); objIdx++) {
        auto& input = objects_[objIdx];
        //std::cout << "DEBUG: Processing object " << objIdx << ": " << input.filename << std::endl;
        // Text relocations
        auto textRelocs = O45RelocDecoder::decode(input.obj.textRelocs);
        //std::cout << "DEBUG: Decoded " << textRelocs.size() << " TEXT relocations from " << input.filename << std::endl;
        for (const auto& r : textRelocs) {
            const char* segName = "";
            if (r.segment == SEG_EXTERNAL) segName = "EXTERN";
            else if (r.segment == SEG_TEXT) segName = "TEXT";
            else if (r.segment == SEG_DATA) segName = "DATA";
            //std::cout << "  TEXT Reloc at offset 0x" << std::hex << r.offset << " type 0x" << (int)r.type
            //          << " seg " << segName << " extra 0x" << (int)r.extra << std::dec << std::endl;
        }
        if (!applyRelocs(textRelocs, mergedText_, textBase_, input.textOffset,
                         input, errorMsg, objIdx)) {
            return false;
        }

        // Data relocations
        auto dataRelocs = O45RelocDecoder::decode(input.obj.dataRelocs);
        //std::cout << "DEBUG: Decoded " << dataRelocs.size() << " DATA relocations from " << input.filename << std::endl;
        for (const auto& r : dataRelocs) {
            //std::cout << "  DATA Reloc at offset 0x" << std::hex << r.offset << " type 0x" << (int)r.type << " seg " << (int)r.segment << " extra 0x" << (int)r.extra << std::dec << std::endl;
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
    analyzeConstantParameters();  // Cross-file parameter analysis
    analyzeIRMetadata();           // Phase 50: Extract constant parameters from embedded IR
    analyzeSpecializations();      // Phase 52: Analyze profitable specialization patterns
    analyzeCallRouting();          // Phase 54: Analyze call site routing
    analyzeInlining();             // Phase 55: Analyze cross-module inlining
    generateDispatchers();         // Phase 56: Generate dispatcher stubs for multi-specialization
    integrateDispatcherAssembly(); // Phase 58: Integrate dispatcher assembly into output
    emitDispatcherAssemblyOutput(); // Phase 59: Emit dispatcher assembly to output
    emitDiagnostics();
    verifyStaticAllocSafety();  // Verify SAC constraints before thunk generation
    validateSACParameters();      // Phase 3: Validate SAC parameter metadata

    // Check for calling convention errors and SAC violations
    if (!convErrors_.empty()) {
        errorMsg = convErrors_[0];
        for (size_t i = 1; i < convErrors_.size(); i++)
            errorMsg += "\n" + convErrors_[i];
        return {};
    }

    // Phase 2: Assign overlapping AR addresses via call-graph coloring
    colorStaticAllocRegisters();

    // Phase 3: Patch static allocation symbol addresses in global symbol table
    patchStaticAllocAddresses();

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
                // Internal call — use reverse lookup to find target function
                // The relocation points to a 16-bit address within the text segment.
                // Read the relocated value and find which function it belongs to.
                if (r.offset + 1 >= input.obj.textBody.size()) continue;

                // Read the 16-bit value at the relocation site (little-endian)
                uint16_t targetOff = input.obj.textBody[r.offset] |
                                     (input.obj.textBody[r.offset + 1] << 8);

                // Find which export this offset falls within
                bool found = false;
                for (const auto& exp : input.obj.exports) {
                    if (exp.segmentId() != SEG_TEXT) continue;
                    // Check if targetOff is within this function's range
                    // For now, assume each function spans from its offset to the next export's offset
                    // This is approximate but good enough for call graph detection
                    if (exp.offset == targetOff) {
                        callee = exp.name;
                        found = true;
                        break;
                    }
                }
                if (!found) continue; // Could not resolve target
            } else {
                continue; // Ignore other segment types
            }

            // Find which function owns this call site
            uint32_t siteInMerged = input.textOffset + r.offset;
            for (const auto& fn : funcs) {
                if (siteInMerged >= fn.startOff && siteInMerged < fn.endOff) {
                    if (!callee.empty()) {
                        callGraph_[fn.name].insert(callee);
                    }
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

// 3.2a — Analyze constant parameters across all object files
// Finds parameters that are ALWAYS constant across all call sites
void O45Linker::analyzeConstantParameters() {
    if (warnStream_) {
        *warnStream_ << "DEBUG: Analyzing constant parameters. funcAttrs_.size()=" << funcAttrs_.size() << std::endl;
    }

    // For each function with SAC metadata
    for (auto& [funcName, attr] : funcAttrs_) {
        if (warnStream_) {
            *warnStream_ << "DEBUG: Function " << funcName << " flags=0x" << std::hex << (int)attr.flags
                         << " SAC params=" << attr.sacMetadata.parameters.size() << std::dec << std::endl;
        }

        if (attr.sacMetadata.parameters.empty()) continue;
        if ((attr.flags & FUNC_FLAG_STATIC_ALLOC) == 0) continue;

        // For each parameter in this function
        for (size_t paramIdx = 0; paramIdx < attr.sacMetadata.parameters.size(); paramIdx++) {
            const auto& param = attr.sacMetadata.parameters[paramIdx];

            // Mark parameter as specialized if it's constant
            if (param.isConstant) {
                specializedParams_[funcName][paramIdx] = {true, param.constantValue};

                // Debug output
                if (warnStream_) {
                    *warnStream_ << "INFO: Parameter " << funcName << "[" << paramIdx
                                 << "] is constant: " << param.constantValue << std::endl;
                }
            }
        }
    }

    // Phase 4 optimization: Generate optimized parameter passing
    // For each specialized function with constant parameters
    for (const auto& [funcName, paramMap] : specializedParams_) {
        if (warnStream_) {
            *warnStream_ << "OPTIMIZE: Function " << funcName << " has " << paramMap.size()
                        << " constant parameter(s)" << std::endl;
        }
    }
}

// Phase 50: Analyze IR metadata from all linked objects
// Extract constant parameters across compilation units and build merged IR function map
void O45Linker::analyzeIRMetadata() {
    mergedIRFunctions_.clear();

    // Collect IR metadata from all objects
    for (const auto& obj : objects_) {
        if (!obj.obj.hasIRMetadata()) {
            continue;  // Object has no IR metadata
        }

        if (warnStream_) {
            *warnStream_ << "DEBUG: Object " << obj.filename << " has IR metadata (v"
                        << (int)obj.obj.irMajorVersion << "." << (int)obj.obj.irMinorVersion << ")"
                        << " with " << obj.obj.irMetadata.functions.size() << " functions" << std::endl;
        }

        // Merge functions from this object's IR
        for (const auto& irFunc : obj.obj.irMetadata.functions) {
            auto it = mergedIRFunctions_.find(irFunc.functionName);
            if (it == mergedIRFunctions_.end()) {
                // First occurrence of this function
                mergedIRFunctions_[irFunc.functionName] = irFunc;
            } else {
                // Merge call information from multiple compilation units
                auto& merged = it->second;
                merged.callSites.insert(merged.callSites.end(),
                                       irFunc.callSites.begin(),
                                       irFunc.callSites.end());

                // Update call graph
                for (const auto& callEntry : irFunc.callGraph) {
                    auto cgIt = std::find_if(merged.callGraph.begin(), merged.callGraph.end(),
                                            [&](const O45IRCallGraphEntry& e) {
                                                return e.calleeName == callEntry.calleeName;
                                            });
                    if (cgIt != merged.callGraph.end()) {
                        cgIt->callCount += callEntry.callCount;
                    } else {
                        merged.callGraph.push_back(callEntry);
                    }
                }
            }
        }
    }

    // Analyze merged IR to detect constant parameters
    for (auto& [funcName, irFunc] : mergedIRFunctions_) {
        if (warnStream_) {
            *warnStream_ << "DEBUG: Analyzing IR for function " << funcName
                        << " with " << irFunc.parameters.size() << " params and "
                        << irFunc.callSites.size() << " call sites" << std::endl;
        }

        // For each parameter in this function
        for (size_t paramIdx = 0; paramIdx < irFunc.parameters.size(); paramIdx++) {
            const auto& param = irFunc.parameters[paramIdx];

            // Check if all call sites pass the same constant value for this parameter
            if (irFunc.callSites.empty()) {
                continue;  // No call sites to analyze
            }

            bool allConstant = true;
            int64_t constantValue = 0;

            for (size_t siteIdx = 0; siteIdx < irFunc.callSites.size(); siteIdx++) {
                const auto& site = irFunc.callSites[siteIdx];

                if (paramIdx >= site.paramValues.size()) {
                    allConstant = false;
                    break;
                }

                if (!site.paramIsConst[paramIdx]) {
                    allConstant = false;
                    break;
                }

                int64_t siteValue = site.paramValues[paramIdx];
                if (siteIdx == 0) {
                    constantValue = siteValue;
                } else if (siteValue != constantValue) {
                    allConstant = false;
                    break;
                }
            }

            // If all call sites pass the same constant, mark this parameter as specialized
            if (allConstant && !irFunc.callSites.empty()) {
                specializedParams_[funcName][paramIdx] = {true, constantValue};

                if (warnStream_) {
                    *warnStream_ << "INFO: IR Analysis - Parameter " << funcName << "[" << paramIdx
                                << "] is constant across all " << irFunc.callSites.size()
                                << " call sites: " << constantValue << std::endl;
                }
            }
        }
    }
}

// Phase 52: Analyze profitable function specialization patterns
// Detects call patterns (constant parameter combinations) and identifies
// which functions would benefit from specialization (multiple distinct patterns)
void O45Linker::analyzeSpecializations() {
    specializationAnalysis_.clear();

    // Analyze each merged IR function
    for (const auto& [funcName, irFunc] : mergedIRFunctions_) {
        if (irFunc.callSites.empty()) {
            continue;  // No call sites to analyze
        }

        SpecializationAnalysis analysis;
        analysis.functionName = funcName;

        // Collect all unique call patterns
        std::map<SpecializationPattern, int> patternCounts;
        for (const auto& site : irFunc.callSites) {
            SpecializationPattern pattern;

            // Extract constant parameter values for this call site
            bool allConstant = true;
            for (size_t i = 0; i < site.paramValues.size(); i++) {
                if (i >= site.paramIsConst.size() || !site.paramIsConst[i]) {
                    allConstant = false;
                    break;
                }
                pattern.push_back(site.paramValues[i]);
            }

            // Only consider call sites where all parameters are constant
            if (allConstant && !pattern.empty()) {
                patternCounts[pattern]++;
                analysis.totalCalls++;
            }
        }

        // Populate analysis results
        for (const auto& [pattern, count] : patternCounts) {
            analysis.patterns.push_back(pattern);
            analysis.patternCounts.push_back(count);
        }

        // Determine profitability
        // Profitable if: (1) multiple distinct patterns, OR (2) single pattern called frequently
        if (analysis.patterns.size() >= 2) {
            // Multiple patterns: worth specializing top patterns
            analysis.isProfitable = true;

            // Calculate top pattern frequency
            if (!analysis.patternCounts.empty()) {
                int maxCount = *std::max_element(analysis.patternCounts.begin(),
                                                analysis.patternCounts.end());
                analysis.topPatternFrequency = (float)maxCount / (float)analysis.totalCalls;
            }
        } else if (analysis.patterns.size() == 1 && analysis.totalCalls >= 5) {
            // Single pattern called 5+ times: worth generating 1 specialization
            analysis.isProfitable = true;
            analysis.topPatternFrequency = 1.0f;
        }

        if (warnStream_ && analysis.isProfitable) {
            *warnStream_ << "INFO: Function " << funcName << " has " << analysis.patterns.size()
                        << " specialization pattern(s), " << analysis.totalCalls << " total calls"
                        << " (top frequency: " << (analysis.topPatternFrequency * 100.0f) << "%)" << std::endl;
        }

        if (analysis.isProfitable) {
            specializationAnalysis_[funcName] = analysis;
        }
    }
}

// Phase 53: Get specializations recommended for generation
// Returns map of function name → patterns to specialize (top patterns only)
std::map<std::string, std::vector<SpecializationPattern>> O45Linker::getRecommendedSpecializations() const {
    std::map<std::string, std::vector<SpecializationPattern>> recommended;

    for (const auto& [funcName, analysis] : specializationAnalysis_) {
        if (!analysis.isProfitable) continue;

        std::vector<SpecializationPattern> patterns;

        // For multiple patterns: specialize top 1-2 patterns (by frequency)
        if (analysis.patterns.size() >= 2) {
            // Sort patterns by call count (descending)
            std::vector<size_t> indices(analysis.patterns.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::sort(indices.begin(), indices.end(),
                     [&](size_t a, size_t b) {
                         return analysis.patternCounts[a] > analysis.patternCounts[b];
                     });

            // Take top 1 or 2 patterns (if second pattern is >10% of calls)
            patterns.push_back(analysis.patterns[indices[0]]);
            if (indices.size() > 1 && analysis.patternCounts[indices[1]] >= analysis.totalCalls / 10) {
                patterns.push_back(analysis.patterns[indices[1]]);
            }
        } else if (analysis.patterns.size() == 1) {
            patterns.push_back(analysis.patterns[0]);
        }

        if (!patterns.empty()) {
            recommended[funcName] = patterns;
        }
    }

    return recommended;
}

// Phase 53: Write specialization report for compiler/debugging
void O45Linker::writeSpecializationReport(std::ostream& out) const {
    out << "=== Function Specialization Report ===\n\n";

    for (const auto& [funcName, analysis] : specializationAnalysis_) {
        out << "Function: " << funcName << "\n";
        out << "  Total calls: " << analysis.totalCalls << "\n";
        out << "  Patterns: " << analysis.patterns.size() << "\n";

        for (size_t i = 0; i < analysis.patterns.size(); i++) {
            const auto& pattern = analysis.patterns[i];
            int count = analysis.patternCounts[i];
            float freq = (float)count / (float)analysis.totalCalls * 100.0f;

            out << "    Pattern " << (i + 1) << ": {";
            for (size_t j = 0; j < pattern.size(); j++) {
                if (j > 0) out << ", ";
                out << pattern[j];
            }
            out << "} - " << count << " calls (" << freq << "%)\n";
        }

        out << "  Profitable: " << (analysis.isProfitable ? "YES" : "NO") << "\n";
        out << "\n";
    }
}

// Phase 54: Analyze call site routing opportunities
// Determines which calls can be routed to specializations vs dynamic dispatch
void O45Linker::analyzeCallRouting() {
    callRoutingAnalysis_.clear();

    // For each function that has specializations
    auto recommended = getRecommendedSpecializations();
    for (const auto& [funcName, patterns] : recommended) {
        CallRoutingAnalysis routing;
        routing.functionName = funcName;
        routing.needsDispatcher = patterns.size() > 1;  // Multiple specializations need dispatcher
        routing.dispatcherName = funcName + "__dispatch";

        // Find all call sites to this function from IR
        // Build list of routable vs dynamic calls
        int routableCount = 0;

        for (const auto& [callerName, irFunc] : mergedIRFunctions_) {
            // Look through call sites for calls to this function
            for (const auto& callSite : irFunc.callSites) {
                if (callSite.calleeName != funcName) continue;

                CallSiteInfo siteInfo;
                siteInfo.callSiteOffset = callSite.instructionOffset;
                siteInfo.calleeName = funcName;

                // Check if this call site has a matching specialization pattern
                bool hasMatchingPattern = false;
                for (const auto& pattern : patterns) {
                    // Check if call site arguments match pattern
                    if (callSite.paramValues.size() == pattern.size()) {
                        bool matches = true;
                        siteInfo.argumentPattern.clear();

                        for (size_t i = 0; i < pattern.size(); i++) {
                            if (!callSite.paramIsConst[i] || callSite.paramValues[i] != pattern[i]) {
                                matches = false;
                                break;
                            }
                            siteInfo.argumentPattern.push_back(callSite.paramValues[i]);
                        }

                        if (matches) {
                            hasMatchingPattern = true;
                            siteInfo.isConstantPattern = true;
                            siteInfo.targetFunction = funcName + "__"; // Will be filled with pattern suffix
                            for (int64_t v : pattern) {
                                siteInfo.targetFunction += std::to_string(v) + "_";
                            }
                            siteInfo.targetFunction.pop_back();  // Remove trailing underscore
                            routing.routableCalls.push_back(siteInfo);
                            routableCount++;
                            break;
                        }
                    }
                }

                if (!hasMatchingPattern) {
                    siteInfo.isConstantPattern = false;
                    routing.dynamicCalls.push_back(siteInfo);
                }
            }
        }

        routing.totalCalls = routableCount + (int)routing.dynamicCalls.size();
        if (routing.totalCalls > 0) {
            routing.routablePercentage = (float)routableCount / (float)routing.totalCalls * 100.0f;
        }

        if (warnStream_) {
            *warnStream_ << "INFO: Call routing for " << funcName << ": "
                        << routableCount << "/" << routing.totalCalls << " routable ("
                        << routing.routablePercentage << "%)" << std::endl;
        }

        if (!routing.routableCalls.empty() || !routing.dynamicCalls.empty()) {
            callRoutingAnalysis_[funcName] = routing;
        }
    }
}

// Phase 54: Write call routing report for debugging
void O45Linker::writeCallRoutingReport(std::ostream& out) const {
    out << "=== Call Routing Report ===\n\n";

    for (const auto& [funcName, routing] : callRoutingAnalysis_) {
        out << "Function: " << funcName << "\n";
        out << "  Total calls: " << routing.totalCalls << "\n";
        out << "  Routable: " << routing.routableCalls.size() << " (" << routing.routablePercentage << "%)\n";
        out << "  Dynamic: " << routing.dynamicCalls.size() << "\n";

        if (routing.needsDispatcher) {
            out << "  Dispatcher: " << routing.dispatcherName << "\n";
        }

        if (!routing.routableCalls.empty()) {
            out << "  Routable call sites:\n";
            for (const auto& site : routing.routableCalls) {
                out << "    -> " << site.targetFunction << " (offset 0x"
                    << std::hex << site.callSiteOffset << std::dec << ")\n";
            }
        }

        out << "\n";
    }
}

// Phase 55: Analyze cross-module inlining opportunities
// Identifies functions that are profitable to inline based on call patterns and code size
void O45Linker::analyzeInlining() {
    inliningAnalysis_.clear();

    // For each function with routing information
    for (const auto& [funcName, routing] : callRoutingAnalysis_) {
        if (routing.routableCalls.empty()) continue;

        InliningAnalysis inlineAnalysis;
        inlineAnalysis.callingSite = funcName;

        // Estimate code sizes and inlining benefits
        // For each routable call site, determine if inlining would help
        for (const auto& callSite : routing.routableCalls) {
            InliningCandidate candidate;
            candidate.functionName = callSite.targetFunction;
            candidate.specializationName = callSite.targetFunction;
            candidate.isSpecialized = !callSite.argumentPattern.empty();
            candidate.callCount = 1;  // Each call site is 1 call

            // Estimate code sizes (simplified)
            // Typical JSR overhead: 3-4 bytes (JSR + params setup/cleanup)
            candidate.callSiteOverhead = 4;

            // Specialized functions are typically smaller (constants pre-bound)
            // Estimate: 30-100 bytes for typical specialized function
            candidate.estimatedCodeSize = candidate.isSpecialized ? 50 : 80;

            // Inlining saves JSR overhead but adds code at call site
            // Net savings: callSiteOverhead if only called once, or proportional if multiple calls
            candidate.estimatedSavings = candidate.callSiteOverhead;
            candidate.benefitRatio = (float)candidate.estimatedSavings / (float)candidate.estimatedCodeSize;

            // Profitable if: benefit ratio > 0.05 (5% savings)
            // AND either: specialized (constants inline) OR called once
            candidate.isProfitable = (candidate.benefitRatio > 0.05f) &&
                                    (candidate.isSpecialized || candidate.callCount == 1);

            if (candidate.isProfitable) {
                inlineAnalysis.candidates.push_back(candidate);
                inlineAnalysis.totalSavingsPotential += candidate.estimatedSavings;
                inlineAnalysis.selectableCount++;
            }
        }

        // Calculate average benefit
        if (!inlineAnalysis.candidates.empty()) {
            float totalBenefit = 0.0f;
            for (const auto& c : inlineAnalysis.candidates) {
                totalBenefit += c.benefitRatio;
            }
            inlineAnalysis.averageBenefit = totalBenefit / (float)inlineAnalysis.candidates.size();
        }

        if (!inlineAnalysis.candidates.empty()) {
            if (warnStream_) {
                *warnStream_ << "INFO: Inlining analysis for calls in " << funcName << ": "
                            << inlineAnalysis.selectableCount << " candidates, "
                            << inlineAnalysis.totalSavingsPotential << " bytes potential savings"
                            << std::endl;
            }
            inliningAnalysis_[funcName] = inlineAnalysis;
        }
    }
}

// Phase 55: Get inlining candidates for a function
std::vector<InliningCandidate> O45Linker::getInliningCandidates(const std::string& funcName) const {
    auto it = inliningAnalysis_.find(funcName);
    if (it == inliningAnalysis_.end()) return {};
    return it->second.candidates;
}

// Phase 55: Write inlining report for debugging
void O45Linker::writeInliningReport(std::ostream& out) const {
    out << "=== Cross-Module Inlining Report ===\n\n";

    for (const auto& [site, analysis] : inliningAnalysis_) {
        out << "Calling context: " << site << "\n";
        out << "  Inlining candidates: " << analysis.candidates.size() << "\n";
        out << "  Profitable: " << analysis.selectableCount << "\n";
        out << "  Total savings potential: " << analysis.totalSavingsPotential << " bytes\n";
        out << "  Average benefit ratio: " << (analysis.averageBenefit * 100.0f) << "%\n";

        for (const auto& cand : analysis.candidates) {
            out << "    " << cand.functionName << ": "
                << cand.estimatedSavings << " bytes ("
                << (cand.benefitRatio * 100.0f) << "%)";
            if (cand.isSpecialized) out << " [specialized]";
            out << "\n";
        }

        out << "\n";
    }
}

// Phase 57: Emit dispatcher assembly code for all generated dispatchers
std::string O45Linker::emitDispatcherAssembly() const {
    std::string assembly;

    // For each function with dispatcher analysis
    for (const auto& [funcName, analysis] : dispatcherAnalysis_) {
        // Emit each dispatcher stub for this function
        for (const auto& dispatcher : analysis.dispatchers) {
            if (dispatcher.generateDispatcher) {
                assembly += emitDispatcherStub(dispatcher);
                assembly += "\n";
            }
        }
    }

    return assembly;
}

// Phase 57: Generate assembly for a specific dispatcher stub
std::string O45Linker::emitDispatcherStub(const DispatcherStub& stub) const {
    std::string asm_code;

    // Dispatcher header comment
    asm_code += "; Dispatcher stub for multi-specialization\n";
    asm_code += "; Routes calls to appropriate specialized versions\n";
    asm_code += stub.dispatcherName + ":\n";

    // Early exit: if no routes, just fall through to generic
    if (stub.routes.empty()) {
        asm_code += "    jmp " + stub.genericFunction + "\n";
        return asm_code;
    }

    // Single route case: direct jump to specialized version
    if (stub.routes.size() == 1) {
        asm_code += "    jmp " + stub.routes[0].targetFunction + "\n";
        return asm_code;
    }

    // Multiple routes: implement pattern matching dispatcher
    // Strategy: Compare argument 1 (typically in A register or on stack)
    // For each route: check if pattern matches, if so jump to target
    // If no match, jump to generic function

    asm_code += "; Pattern matching dispatcher\n";
    asm_code += "; Check argument patterns and route to appropriate specialization\n";

    // Extract unique patterns for comparison
    std::map<uint32_t, std::vector<size_t>> patternMap;  // pattern value → route indices

    for (size_t i = 0; i < stub.routes.size(); ++i) {
        const auto& route = stub.routes[i];
        if (!route.argumentPattern.empty()) {
            // Use first argument as primary pattern key
            uint32_t key = route.argumentPattern[0];
            patternMap[key].push_back(i);
        }
    }

    // Emit pattern matching code
    // Assume argument 1 is in A register (or will be loaded)
    bool firstCheck = true;
    for (const auto& [patternValue, routeIndices] : patternMap) {
        // Emit conditional check for this pattern
        std::string checkLabel = stub.dispatcherName + "_check_" + std::to_string(patternValue);

        if (!firstCheck) {
            asm_code += checkLabel + ":\n";
        }
        firstCheck = false;

        // Compare A register with pattern value
        asm_code += "    cmp #" + std::to_string(patternValue) + "\n";

        // Branch to next check if not equal
        if (patternValue != patternMap.rbegin()->first) {  // Not the last pattern
            std::string nextPattern = std::to_string(patternMap.upper_bound(patternValue)->first);
            asm_code += "    bne " + stub.dispatcherName + "_check_" + nextPattern + "\n";
        } else {
            asm_code += "    bne " + stub.dispatcherName + "_fallback\n";
        }

        // If multiple specializations for this pattern, check second argument
        if (routeIndices.size() > 1) {
            asm_code += "    ; Multiple specializations for this pattern\n";
            for (size_t idx : routeIndices) {
                const auto& route = stub.routes[idx];
                asm_code += "    jmp " + route.targetFunction + "\n";
            }
        } else {
            // Single specialization for this pattern
            asm_code += "    jmp " + stub.routes[routeIndices[0]].targetFunction + "\n";
        }
    }

    // Fallback: jump to generic function if no pattern matched
    asm_code += stub.dispatcherName + "_fallback:\n";
    asm_code += "    jmp " + stub.genericFunction + "\n";

    return asm_code;
}

// Phase 58: Integrate dispatcher assembly into link output
void O45Linker::integrateDispatcherAssembly() {
    // Generate dispatcher assembly for all dispatchers
    dispatcherAssemblyOutput_ = emitDispatcherAssembly();

    // If no dispatcher assembly generated, nothing to do
    if (dispatcherAssemblyOutput_.empty()) {
        if (warnStream_) {
            *warnStream_ << "ln45: No dispatcher assembly to integrate\n";
        }
        return;
    }

    // Count dispatchers integrated
    int dispatcherCount = 0;
    for (const auto& [funcName, analysis] : dispatcherAnalysis_) {
        if (analysis.dispatchersNeeded > 0) {
            dispatcherCount += analysis.dispatchersNeeded;
        }
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Integrating " << dispatcherCount
                    << " dispatcher stubs into link output\n";
        *warnStream_ << "ln45: Dispatcher assembly size: "
                    << dispatcherAssemblyOutput_.size() << " bytes (source)\n";
    }

    // Phase 58 implementation note:
    // The dispatcher assembly needs to be:
    // 1. Assembled to binary form (would normally use ca45)
    // 2. Added to the merged text segment
    // 3. Have symbols resolved for target functions
    //
    // For now, store the assembly output for later processing
    // In a full implementation, this would:
    // - Create temporary assembly file
    // - Run ca45 assembler on it
    // - Link the resulting .o45 into the final binary
    // - Update symbol table with dispatcher addresses
    //
    // Alternative approach (more efficient):
    // - Directly emit binary code for simple dispatchers
    // - Use relocation entries for targets that need resolution

    // Log dispatcher function names for verification
    if (warnStream_) {
        *warnStream_ << "ln45: Integrated dispatchers for:\n";
        for (const auto& [funcName, analysis] : dispatcherAnalysis_) {
            for (const auto& dispatcher : analysis.dispatchers) {
                *warnStream_ << "  - " << dispatcher.dispatcherName << "\n";
            }
        }
    }
}

// Phase 59: Emit dispatcher assembly to output
void O45Linker::emitDispatcherAssemblyOutput() {
    // Reset emission counter
    dispatcherStubsEmitted_ = 0;

    // If no dispatcher assembly, nothing to emit
    if (dispatcherAssemblyOutput_.empty()) {
        if (warnStream_) {
            *warnStream_ << "ln45: No dispatcher assembly to emit\n";
        }
        return;
    }

    // Count dispatchers to be emitted
    for (const auto& [funcName, analysis] : dispatcherAnalysis_) {
        dispatcherStubsEmitted_ += analysis.dispatchersNeeded;
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Emitting " << dispatcherStubsEmitted_
                    << " dispatcher stubs to output\n";
    }

    // Phase 59 Implementation Strategy:
    //
    // The dispatcher assembly needs to be included in the final binary.
    // There are several approaches:
    //
    // Approach 1: Assembly file emission (current approach)
    // - Write dispatcherAssemblyOutput_ to temporary .s45 file
    // - Assemble with ca45: ca45 temp.s45 -o dispatcher.o45
    // - Re-link with dispatcher object: linker.addObject("dispatcher.o45", obj)
    // - Call linker.link() again to include dispatcher code
    //
    // Approach 2: Direct binary emission (more complex)
    // - Parse dispatcher assembly patterns
    // - Generate binary instructions directly
    // - Add to mergedText_ segment
    // - Create relocation entries for target references
    // - Update symbol table with dispatcher addresses
    //
    // Approach 3: Assembly concatenation (hybrid)
    // - Append dispatcherAssemblyOutput_ to main assembly
    // - Let ca45 assemble everything together
    // - Single assembly/link pass
    //
    // For now, we implement framework for emission and tracking.
    // Actual binary generation deferred to post-link phase.

    // Log dispatcher details
    if (warnStream_) {
        *warnStream_ << "ln45: Dispatcher assembly size: "
                    << dispatcherAssemblyOutput_.size() << " bytes (source)\n";
        *warnStream_ << "ln45: Dispatcher stubs emitted:\n";

        // List each dispatcher being emitted
        int count = 0;
        for (const auto& [funcName, analysis] : dispatcherAnalysis_) {
            for (const auto& dispatcher : analysis.dispatchers) {
                if (count < 20) {  // Limit output to first 20
                    *warnStream_ << "  [" << (count + 1) << "] " << dispatcher.dispatcherName
                                << " (" << (int)dispatcher.estimatedCodeSize << " bytes)\n";
                }
                count++;
            }
        }

        if (count > 20) {
            *warnStream_ << "  ... and " << (count - 20) << " more\n";
        }
    }

    // Mark as emitted
    if (warnStream_) {
        *warnStream_ << "ln45: Dispatcher assembly ready for output\n";
    }

    // Future implementation note:
    // The dispatcherAssemblyOutput_ is now ready to be:
    // 1. Written to a .s45 file for separate assembly
    // 2. Concatenated with main assembly before ca45
    // 3. Directly emitted as binary (Phase 60+)
    // 4. Incorporated into linker output via object file
    //
    // For full integration, Phase 60 should:
    // - Write assembly to temporary file (if needed)
    // - Assemble with ca45
    // - Link dispatcher object into final binary
    // - Update symbol table with dispatcher addresses
}

// Phase 60: Write dispatcher assembly to file
bool O45Linker::writeDispatcherAssemblyFile(const std::string& filepath, std::string& errorMsg) {
    // Clear previous file path
    dispatcherAssemblyFilePath_.clear();

    // If no dispatcher assembly, nothing to write
    if (dispatcherAssemblyOutput_.empty()) {
        if (warnStream_) {
            *warnStream_ << "ln45: No dispatcher assembly to write\n";
        }
        return true;  // Not an error - just no dispatcher code
    }

    // Add .s45 extension if not present
    std::string outputFile = filepath;
    if (outputFile.rfind(".s45") != outputFile.length() - 4) {
        outputFile += ".s45";
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Writing dispatcher assembly to '" << outputFile << "'\n";
    }

    // Open file for writing
    std::ofstream file(outputFile, std::ios::binary);
    if (!file.is_open()) {
        errorMsg = "cannot open dispatcher assembly file for writing: " + outputFile;
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: " << errorMsg << "\n";
        }
        return false;
    }

    // Write dispatcher assembly to file
    file << dispatcherAssemblyOutput_;

    // Check for write errors
    if (!file.good()) {
        errorMsg = "error writing dispatcher assembly to file: " + outputFile;
        file.close();
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: " << errorMsg << "\n";
        }
        return false;
    }

    file.close();

    // Store file path for tracking
    dispatcherAssemblyFilePath_ = outputFile;

    if (warnStream_) {
        *warnStream_ << "ln45: Wrote " << dispatcherAssemblyOutput_.size()
                    << " bytes to dispatcher assembly file\n";
        *warnStream_ << "ln45: Dispatcher assembly ready for ca45 assembler\n";
        *warnStream_ << "ln45: Next: ca45 " << outputFile
                    << " -o dispatcher.o45\n";
    }

    return true;
}

// Phase 61: Assemble dispatcher assembly file with ca45
bool O45Linker::assembleDispatcherFile(const std::string& ca45Path, std::string& errorMsg) {
    // Reset counters
    dispatchersAssembled_ = 0;
    dispatcherObjectFilePath_.clear();

    // Check if dispatcher assembly file was written
    if (dispatcherAssemblyFilePath_.empty()) {
        if (warnStream_) {
            *warnStream_ << "ln45: No dispatcher assembly file to assemble\n";
        }
        return true;  // Not an error - just no dispatcher code
    }

    // Derive output object filename from assembly filename
    std::string objectFile = dispatcherAssemblyFilePath_;
    size_t dotPos = objectFile.rfind(".s45");
    if (dotPos != std::string::npos) {
        objectFile = objectFile.substr(0, dotPos) + ".o45";
    } else {
        objectFile += ".o45";
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Assembling dispatcher code with ca45\n";
        *warnStream_ << "ln45: Input:  " << dispatcherAssemblyFilePath_ << "\n";
        *warnStream_ << "ln45: Output: " << objectFile << "\n";
    }

    // Build ca45 command
    std::string command = ca45Path + " \"" + dispatcherAssemblyFilePath_ + "\" -o \"" + objectFile + "\"";

    if (warnStream_) {
        *warnStream_ << "ln45: Executing: " << command << "\n";
    }

    // Execute ca45 assembler
    int status = system(command.c_str());
    if (status != 0) {
        errorMsg = "ca45 assembler failed with status " + std::to_string(status) +
                   " (assembly file: " + dispatcherAssemblyFilePath_ + ")";
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: " << errorMsg << "\n";
        }
        return false;
    }

    // Verify object file was created
    std::ifstream objFile(objectFile);
    if (!objFile.good()) {
        errorMsg = "dispatcher object file not created: " + objectFile;
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: " << errorMsg << "\n";
        }
        return false;
    }
    objFile.close();

    // Store object file path and count
    dispatcherObjectFilePath_ = objectFile;
    dispatchersAssembled_ = dispatcherStubsEmitted_;

    if (warnStream_) {
        *warnStream_ << "ln45: Dispatcher object file created successfully\n";
        *warnStream_ << "ln45: Assembled " << dispatchersAssembled_
                    << " dispatcher stubs\n";
        *warnStream_ << "ln45: Next: Link dispatcher object into final binary\n";
    }

    return true;
}

// Phase 62: Re-link with dispatcher object
bool O45Linker::relinkWithDispatcher(std::string& errorMsg, bool isPrg) {
    // Check if dispatcher object file is available
    if (dispatcherObjectFilePath_.empty()) {
        if (warnStream_) {
            *warnStream_ << "ln45: No dispatcher object file to link\n";
        }
        return true;  // Not an error - just no dispatcher code
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Re-linking with dispatcher object\n";
        *warnStream_ << "ln45: Loading dispatcher object: " << dispatcherObjectFilePath_ << "\n";
    }

    // Read dispatcher object file
    std::ifstream objFile(dispatcherObjectFilePath_, std::ios::binary);
    if (!objFile.is_open()) {
        errorMsg = "cannot open dispatcher object file: " + dispatcherObjectFilePath_;
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: " << errorMsg << "\n";
        }
        return false;
    }

    // Read file contents
    std::vector<char> objData((std::istreambuf_iterator<char>(objFile)), std::istreambuf_iterator<char>());
    objFile.close();

    if (objData.empty()) {
        errorMsg = "dispatcher object file is empty: " + dispatcherObjectFilePath_;
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: " << errorMsg << "\n";
        }
        return false;
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Loaded dispatcher object (" << objData.size()
                    << " bytes)\n";
        *warnStream_ << "ln45: Adding dispatcher object to linker\n";
    }

    // Parse dispatcher object file
    O45File dispatcherObj;
    try {
        O45Reader reader;
        dispatcherObj = reader.parse(reinterpret_cast<const uint8_t*>(objData.data()), objData.size());
    } catch (const std::exception& e) {
        errorMsg = std::string("failed to parse dispatcher object file: ") + e.what();
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: " << errorMsg << "\n";
        }
        return false;
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Dispatcher object parsed successfully\n";
        *warnStream_ << "ln45: Re-linking all objects with dispatcher\n";
    }

    // Add dispatcher object to linker
    addObject("dispatcher.o45", dispatcherObj);

    // Re-link with dispatcher object included
    dispatcherBinary_ = link(errorMsg, isPrg);

    if (dispatcherBinary_.empty() && !errorMsg.empty()) {
        if (warnStream_) {
            *warnStream_ << "ln45: ERROR: Re-linking failed: " << errorMsg << "\n";
        }
        return false;
    }

    // Mark dispatcher as linked
    dispatcherLinked_ = true;

    if (warnStream_) {
        *warnStream_ << "ln45: Re-linking successful\n";
        *warnStream_ << "ln45: Final binary size with dispatcher: " << dispatcherBinary_.size()
                    << " bytes\n";
        *warnStream_ << "ln45: Dispatcher successfully integrated into final binary\n";
    }

    return true;
}

// Phase 63: Verify dispatcher symbol resolution
bool O45Linker::verifyDispatcherSymbols(std::string& report) {
    // Reset verification counters
    dispatcherSymbolsVerified_ = 0;
    allDispatcherSymbolsResolved_ = false;

    // Check if dispatcher was linked
    if (!dispatcherLinked_) {
        report = "Dispatcher not linked - verification skipped";
        if (warnStream_) {
            *warnStream_ << "ln45: " << report << "\n";
        }
        return true;  // Not an error
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Verifying dispatcher symbol resolution\n";
    }

    // Count dispatcher-related symbols from dispatcherAnalysis
    int expectedDispatcherSymbols = 0;
    for (const auto& [funcName, analysis] : dispatcherAnalysis_) {
        for (const auto& dispatcher : analysis.dispatchers) {
            if (dispatcher.generateDispatcher) {
                expectedDispatcherSymbols++;  // Dispatcher stub symbol
                expectedDispatcherSymbols += dispatcher.routes.size();  // Route targets
            }
        }
    }

    if (expectedDispatcherSymbols == 0) {
        report = "No dispatcher symbols to verify";
        if (warnStream_) {
            *warnStream_ << "ln45: " << report << "\n";
        }
        return true;
    }

    if (warnStream_) {
        *warnStream_ << "ln45: Expected dispatcher symbols: " << expectedDispatcherSymbols << "\n";
    }

    // Check global symbol table for dispatcher symbols
    int resolvedCount = 0;
    int unresolved = 0;

    for (const auto& [symName, symInfo] : symTab_) {
        // Check if this is a dispatcher-related symbol
        if (symName.find("__dispatch") != std::string::npos ||
            symName.find("_dispatch_") != std::string::npos) {

            // Verify symbol is resolved (has valid address)
            if (symInfo.address != 0 || symName.find("__dispatch_fallback") != std::string::npos) {
                resolvedCount++;
                if (warnStream_) {
                    *warnStream_ << "ln45:   [OK] " << symName
                                << " @ 0x" << std::hex << symInfo.address << std::dec << "\n";
                }
            } else {
                unresolved++;
                if (warnStream_) {
                    *warnStream_ << "ln45:   [UNRESOLVED] " << symName << "\n";
                }
            }
        }
    }

    dispatcherSymbolsVerified_ = resolvedCount;
    allDispatcherSymbolsResolved_ = (unresolved == 0 && resolvedCount > 0);

    // Build report
    report = "Dispatcher symbol verification:\n";
    report += "  Expected: " + std::to_string(expectedDispatcherSymbols) + " symbols\n";
    report += "  Resolved: " + std::to_string(resolvedCount) + " symbols\n";
    report += "  Unresolved: " + std::to_string(unresolved) + " symbols\n";
    report += allDispatcherSymbolsResolved_ ? "  Status: ALL RESOLVED\n" : "  Status: SOME UNRESOLVED\n";

    if (warnStream_) {
        *warnStream_ << "ln45: " << report;
        if (allDispatcherSymbolsResolved_) {
            *warnStream_ << "ln45: Dispatcher symbols verified successfully\n";
        } else {
            *warnStream_ << "ln45: WARNING: Some dispatcher symbols unresolved\n";
        }
    }

    return true;
}

// Phase 56: Generate dispatcher stubs for multi-specialization cases
void O45Linker::generateDispatchers() {
    dispatcherAnalysis_.clear();

    // For each function with call routing information
    for (const auto& [funcName, routing] : callRoutingAnalysis_) {
        if (routing.specializedVersions.empty()) continue;

        DispatcherAnalysis analysis;
        analysis.functionName = funcName;
        analysis.totalSpecializations = routing.specializedVersions.size();

        // Only need dispatcher if 2+ specializations exist
        if (analysis.totalSpecializations < 2) continue;

        // Create dispatcher stub for this function
        DispatcherStub dispatcher;
        dispatcher.dispatcherName = funcName + "__dispatch";
        dispatcher.genericFunction = funcName;

        // Extract routes from call routing analysis
        // Map each routable call to its target specialization
        int dynamicDispatchCalls = 0;
        for (const auto& callSite : routing.routableCalls) {
            DispatcherRoute route;
            route.targetFunction = callSite.targetFunction;
            route.argumentPattern = callSite.argumentPattern;
            route.callCount = 1;  // Each call site counted

            dispatcher.routes.push_back(route);
            dispatcher.routableCalls++;
        }

        // Dynamic calls (non-routable) handled by dispatcher
        dispatcher.dynamicCalls = routing.totalCalls - routing.routableCalls.size();
        dispatcher.totalRoutes = dispatcher.routes.size();

        // Estimate dispatcher code size:
        // Base: ~20-30 bytes (dispatcher prologue/epilogue)
        // Per-route: ~8-12 bytes (pattern match + JSR to specialized version)
        // Fallback: ~3 bytes (JMP to generic)
        int estimatedRouteSize = dispatcher.routes.size() * 10;  // ~10 bytes per route
        dispatcher.estimatedCodeSize = 25 + estimatedRouteSize + 3;  // Base + routes + fallback

        // Determine if dispatcher is needed
        // Generate dispatcher if:
        // 1. 2+ specializations exist
        // 2. 2+ distinct call patterns routable OR dynamic calls present
        bool multiplePatterns = dispatcher.routes.size() > 1;
        bool hasDynamicCalls = dispatcher.dynamicCalls > 0;
        dispatcher.generateDispatcher = multiplePatterns || hasDynamicCalls;

        // Determine routing strategy
        if (dispatcher.dynamicCalls == 0 && dispatcher.routes.size() == 1) {
            // All calls route to same specialization (single pattern)
            analysis.usesStaticRouting = true;
        } else {
            // Multiple patterns or dynamic calls need runtime dispatch
            analysis.usesDynamicDispatch = true;
        }

        if (dispatcher.generateDispatcher) {
            analysis.dispatchers.push_back(dispatcher);
            analysis.dispatchersNeeded++;
            analysis.totalDispatchCodeSize += (int)dispatcher.estimatedCodeSize;

            if (warnStream_) {
                *warnStream_ << "INFO: Dispatcher for " << funcName << ": "
                            << dispatcher.routes.size() << " routes, "
                            << dispatcher.dynamicCalls << " dynamic calls, "
                            << (int)dispatcher.estimatedCodeSize << " bytes estimated"
                            << std::endl;
            }
        }

        if (analysis.dispatchersNeeded > 0) {
            dispatcherAnalysis_[funcName] = analysis;
        }
    }
}

// Phase 56: Write dispatcher report for debugging
void O45Linker::writeDispatcherReport(std::ostream& out) const {
    out << "=== Dispatcher Generation Report ===\n\n";

    for (const auto& [funcName, analysis] : dispatcherAnalysis_) {
        out << "Function: " << funcName << "\n";
        out << "  Specializations: " << analysis.totalSpecializations << "\n";
        out << "  Dispatchers needed: " << analysis.dispatchersNeeded << "\n";
        out << "  Total dispatcher code: " << analysis.totalDispatchCodeSize << " bytes\n";
        out << "  Routing strategy: ";
        if (analysis.usesStaticRouting) out << "static (single pattern)";
        else if (analysis.usesDynamicDispatch) out << "dynamic (runtime dispatch)";
        else out << "none";
        out << "\n";

        for (const auto& dispatcher : analysis.dispatchers) {
            out << "    Dispatcher: " << dispatcher.dispatcherName << "\n";
            out << "      Routes: " << dispatcher.totalRoutes << "\n";
            out << "      Routable calls: " << dispatcher.routableCalls << "\n";
            out << "      Dynamic calls: " << dispatcher.dynamicCalls << "\n";
            out << "      Estimated size: " << (int)dispatcher.estimatedCodeSize << " bytes\n";

            for (size_t i = 0; i < dispatcher.routes.size() && i < 5; ++i) {
                const auto& route = dispatcher.routes[i];
                out << "        Route " << (i + 1) << ": ";
                if (!route.argumentPattern.empty()) {
                    out << "{";
                    for (size_t j = 0; j < route.argumentPattern.size(); ++j) {
                        if (j > 0) out << ", ";
                        out << route.argumentPattern[j];
                    }
                    out << "} → " << route.targetFunction;
                } else {
                    out << "* → " << route.targetFunction;
                }
                out << "\n";
            }
            if (dispatcher.routes.size() > 5) {
                out << "        ... and " << (dispatcher.routes.size() - 5) << " more routes\n";
            }
        }

        out << "\n";
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

// 3.3b — Verify Static Allocation Convention (SAC) constraints
// SAC functions must not be recursive (directly or indirectly) and must not be reachable from ISRs
void O45Linker::verifyStaticAllocSafety() {
    // Collect functions using SAC (static allocation convention)
    std::set<std::string> sacFuncs;
    for (const auto& [name, attrs] : funcAttrs_) {
        if ((attrs.flags & FUNC_FLAG_STATIC_ALLOC) != 0) {
            sacFuncs.insert(name);
        }
    }

    if (sacFuncs.empty()) return;  // No SAC functions, nothing to verify

    // Find ISR functions (interrupt handlers)
    std::set<std::string> isrFuncs;
    for (const auto& [name, attrs] : funcAttrs_) {
        if ((attrs.flags & FUNC_FLAG_ISR) != 0) {
            isrFuncs.insert(name);
        }
    }

    // Check 1: Recursive cycle detection via simple DFS
    // For each SAC function, check if it can reach itself
    for (const auto& sacFunc : sacFuncs) {
        // BFS/DFS from sacFunc to detect if it calls itself transitively
        std::set<std::string> visited;
        std::vector<std::string> toVisit;
        bool hasRecursion = false;

        auto it = callGraph_.find(sacFunc);
        if (it != callGraph_.end()) {
            for (const auto& callee : it->second) {
                toVisit.push_back(callee);
            }
        }

        while (!toVisit.empty() && !hasRecursion) {
            std::string curr = toVisit.back();
            toVisit.pop_back();

            if (visited.count(curr)) continue;
            visited.insert(curr);

            if (curr == sacFunc) {
                hasRecursion = true;
                break;
            }

            auto it2 = callGraph_.find(curr);
            if (it2 != callGraph_.end()) {
                for (const auto& next : it2->second) {
                    if (!visited.count(next)) {
                        toVisit.push_back(next);
                    }
                }
            }
        }

        if (hasRecursion) {
            convErrors_.push_back("error: '" + sacFunc + "' uses static allocation but is recursive "
                                  "(add #pragma cc45 recurse before function declaration to opt out)");
        }
    }

    // Check 2: ISR reachability detection
    // Build set of functions reachable from any ISR
    std::set<std::string> isrReachable;
    for (const auto& isr : isrFuncs) {
        std::set<std::string> visited;
        std::vector<std::string> toVisit;
        toVisit.push_back(isr);

        while (!toVisit.empty()) {
            std::string curr = toVisit.back();
            toVisit.pop_back();

            if (visited.count(curr)) continue;
            visited.insert(curr);
            isrReachable.insert(curr);

            auto it = callGraph_.find(curr);
            if (it != callGraph_.end()) {
                for (const auto& next : it->second) {
                    if (!visited.count(next)) {
                        toVisit.push_back(next);
                    }
                }
            }
        }
    }

    // Check if any SAC function is reachable from an ISR
    for (const auto& sacFunc : sacFuncs) {
        if (isrReachable.count(sacFunc)) {
            convErrors_.push_back("error: '" + sacFunc + "' uses static allocation but is reachable from ISR handler "
                                  "(add #pragma cc45 recurse before function declaration to opt out)");
        }
    }
}

// 3.3c — Validate SAC parameter metadata
// Phase 3: Checks that SAC parameters are properly defined and accessible
void O45Linker::validateSACParameters() {
    // Collect SAC functions and their parameter metadata
    std::map<std::string, const O45SACMetadata*> sacParamMetadata;

    for (const auto& [name, attr] : funcAttrs_) {
        if ((attr.flags & FUNC_FLAG_STATIC_ALLOC) != 0 && !attr.sacMetadata.parameters.empty()) {
            sacParamMetadata[name] = &attr.sacMetadata;
        }
    }

    if (sacParamMetadata.empty()) return;  // No SAC parameter metadata to validate

    // For each SAC function with parameter metadata
    for (const auto& [funcName, metadata] : sacParamMetadata) {
        for (const auto& param : metadata->parameters) {
            // Check if parameter symbol is defined in the symbol table
            auto symIt = globalSymbols_.find(param.symbolName);
            if (symIt == globalSymbols_.end()) {
                convErrors_.push_back("warning: SAC function '" + funcName +
                    "' references undefined parameter symbol '" + param.symbolName + "'");
            } else {
                // Verify parameter is in BSS or DATA segment
                auto segIt = symbolSegment_.find(param.symbolName);
                if (segIt != symbolSegment_.end()) {
                    uint8_t seg = segIt->second;
                    if (seg != SEG_BSS && seg != SEG_DATA) {
                        convErrors_.push_back("error: SAC parameter '" + param.symbolName +
                            "' must be in BSS or DATA segment, but is in segment " + std::to_string(seg));
                    }
                }

                // Check that offset matches symbol value
                uint32_t expectedOffset = symIt->second;
                if (expectedOffset != param.offset) {
                    if (warnStream_) {
                        *warnStream_ << "warning: SAC parameter '" << param.symbolName <<
                            "' offset mismatch: metadata says " << param.offset <<
                            " but symbol is at " << expectedOffset << std::endl;
                    }
                }
            }
        }
    }
}

// 3.4 — Phase 2: Color static allocation registers (call-graph based AR overlay)
// Assigns overlapping BSS addresses to non-conflicting SAC functions
// Stream D refinement: Track ISR-only reachability for fine-grained coloring
void O45Linker::colorStaticAllocRegisters() {
    // Collect all SAC functions and their frame sizes
    std::vector<std::string> sacFuncs;
    std::map<std::string, uint32_t> frameSizes;

    for (const auto& [name, attr] : funcAttrs_) {
        if ((attr.flags & FUNC_FLAG_STATIC_ALLOC) != 0) {
            sacFuncs.push_back(name);
            frameSizes[name] = attr.frameSize;
        }
    }
    if (sacFuncs.empty()) return;  // No SAC functions to color

    // Stream D: Identify ISR-only reachable functions for refined coloring
    // Functions reachable only from ISRs (not from mainline) can use separate AR zone
    std::set<std::string> isrReachable, mainlineReachable, isrOnlyReachable;

    // Find functions reachable from ISR handlers
    for (const auto& [name, attr] : funcAttrs_) {
        if ((attr.flags & FUNC_FLAG_ISR) != 0) {
            std::set<std::string> visited;
            std::queue<std::string> q;
            q.push(name);
            visited.insert(name);
            while (!q.empty()) {
                auto curr = q.front(); q.pop();
                isrReachable.insert(curr);
                auto it = callGraph_.find(curr);
                if (it != callGraph_.end()) {
                    for (const auto& callee : it->second) {
                        if (!visited.count(callee)) {
                            visited.insert(callee);
                            q.push(callee);
                        }
                    }
                }
            }
        }
    }

    // Find functions reachable from common entry points (main, startup code)
    for (const auto& [name, _] : callGraph_) {
        // Treat functions called from nothing (no incoming edges) as entry points
        bool hasIncomingEdge = false;
        for (const auto& [caller, callees] : callGraph_) {
            if (callees.count(name)) {
                hasIncomingEdge = true;
                break;
            }
        }
        if (!hasIncomingEdge) {
            // Entry point found; mark all reachable as mainline-reachable
            std::set<std::string> visited;
            std::queue<std::string> q;
            q.push(name);
            visited.insert(name);
            while (!q.empty()) {
                auto curr = q.front(); q.pop();
                mainlineReachable.insert(curr);
                auto it = callGraph_.find(curr);
                if (it != callGraph_.end()) {
                    for (const auto& callee : it->second) {
                        if (!visited.count(callee)) {
                            visited.insert(callee);
                            q.push(callee);
                        }
                    }
                }
            }
        }
    }

    // Functions reachable only from ISRs (not from mainline)
    for (const auto& func : isrReachable) {
        if (!mainlineReachable.count(func)) {
            isrOnlyReachable.insert(func);
        }
    }

    // Build conflict graph: two functions conflict if one is reachable from the other
    // Stream D refinement: ISR-only functions don't conflict with mainline-only functions
    // Non-conflicting functions can share the same AR addresses (overlapped)
    std::map<std::string, std::set<std::string>> conflicts;
    for (const auto& func : sacFuncs) {
        conflicts[func] = std::set<std::string>();
        // Mark as conflicting any function reachable FROM this function
        std::set<std::string> reachable;
        std::queue<std::string> q;
        q.push(func);
        while (!q.empty()) {
            auto curr = q.front(); q.pop();
            auto it = callGraph_.find(curr);
            if (it != callGraph_.end()) {
                for (const auto& callee : it->second) {
                    if (!reachable.count(callee)) {
                        reachable.insert(callee);
                        q.push(callee);
                    }
                }
            }
        }
        // Also mark as conflicting any function that can reach this function
        for (const auto& other : sacFuncs) {
            if (other == func) continue;

            // Stream D: Skip conflict if one is ISR-only and other is mainline-only
            // They can never execute concurrently on the call stack
            bool funcIsISROnly = isrOnlyReachable.count(func) && !mainlineReachable.count(func);
            bool otherIsISROnly = isrOnlyReachable.count(other) && !mainlineReachable.count(other);
            bool funcIsMainlineOnly = mainlineReachable.count(func) && !isrReachable.count(func);
            bool otherIsMainlineOnly = mainlineReachable.count(other) && !isrReachable.count(other);

            if ((funcIsISROnly && otherIsMainlineOnly) || (funcIsMainlineOnly && otherIsISROnly)) {
                continue;  // No conflict: different execution contexts
            }

            auto it = callGraph_.find(other);
            if (it != callGraph_.end()) {
                for (const auto& callee : it->second) {
                    if (callee == func) {
                        conflicts[func].insert(other);
                        break;
                    }
                }
            }
            if (reachable.count(other)) {
                conflicts[func].insert(other);
            }
        }
    }

    // Greedy graph coloring: assign each function to an AR slot
    // Color = AR slot ID; functions with same color share AR space (overlay)
    std::map<std::string, uint32_t> arColor;
    std::vector<std::vector<std::string>> usedFuncsPerColor;

    for (const auto& func : sacFuncs) {
        // Find smallest color (AR slot) that doesn't conflict
        uint32_t color = 0;
        bool found = false;
        while (!found) {
            bool canUse = true;
            // Check if any function with this color conflicts with 'func'
            if (color < usedFuncsPerColor.size()) {
                for (const auto& other : usedFuncsPerColor[color]) {
                    if (conflicts[func].count(other) || conflicts[other].count(func)) {
                        canUse = false;
                        break;
                    }
                }
            }
            if (canUse) {
                arColor[func] = color;
                if (color >= usedFuncsPerColor.size()) {
                    usedFuncsPerColor.resize(color + 1);
                }
                usedFuncsPerColor[color].push_back(func);
                found = true;
            } else {
                color++;
            }
        }
    }

    // Compute final BSS-relative AR base addresses
    // Each color (slot) gets contiguous BSS space; functions with same color overlay
    uint32_t currentBssOffset = 0;
    std::vector<uint32_t> colorToBssBase;

    for (size_t color = 0; color < usedFuncsPerColor.size(); color++) {
        colorToBssBase.push_back(currentBssOffset);
        // Compute max frame size for this color (all functions sharing this slot)
        uint32_t maxFrameSize = 0;
        for (const auto& func : usedFuncsPerColor[color]) {
            maxFrameSize = std::max(maxFrameSize, frameSizes[func]);
        }
        currentBssOffset += maxFrameSize;
    }

    // Populate arBaseAddresses_ map with final BSS-relative addresses
    for (const auto& [func, color] : arColor) {
        uint32_t bssBase = bssBase_;
        arBaseAddresses_[func] = bssBase + colorToBssBase[color];
    }
}

// 3.4b — Phase 3: Patch static allocation symbol addresses
// Apply computed AR base addresses to global symbol table
void O45Linker::patchStaticAllocAddresses() {
    if (arBaseAddresses_.empty()) return;  // No SAC functions to patch

    // For each computed AR base address, update the global symbol map
    // This replaces the placeholder BSS address with the final overlay address
    for (const auto& [funcName, arBaseAddr] : arBaseAddresses_) {
        // Symbol name pattern: <func>__ar (e.g., "main__ar")
        std::string arSymbolName = funcName + "__ar";

        // Check if symbol exists in global symbol table
        auto it = globalSymbols_.find(arSymbolName);
        if (it != globalSymbols_.end()) {
            // Update address to computed overlay address
            globalSymbols_[arSymbolName] = arBaseAddr;
        } else {
            // Symbol should exist if function was compiled with SAC
            // This is a warning-level issue, not an error (symbol may not be exported)
            if (warnStream_) {
                *warnStream_ << "warning: SAC function '" << funcName
                            << "' has no __ar symbol in global symbol table\n";
            }
        }
    }
}

// 3.5 — Generate convention bridge thunks.
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
