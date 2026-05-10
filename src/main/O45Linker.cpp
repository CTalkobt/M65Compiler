#include "O45Linker.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

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

        // External and internal references have a 4-byte symbol index
        // (O45Reader always widens 16-bit .o65 indices to 4 bytes)
        if (r.segment == SEG_EXTERNAL || r.segment == SEG_TEXT) {
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
    symbolWeak_.clear();

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
            uint32_t finalAddr = base + objOffset + exp.offset;

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
            // Read existing value at patch site as addend (e.g., __sp_base+offset
            // has the offset baked in by the assembler)
            uint32_t addend = 0;
            if (r.type == R_HIGH) {
                // For R_HIGH relocations, reconstruct 16-bit addend from:
                // - High byte at patch site
                // - Low byte stored in extra field (see issue #36)
                uint8_t hi = body[patchPos];
                uint8_t lo = r.extra;
                addend = (hi << 8) | lo;
            } else {
                int pSize = o45RelocPatchSize((uint8_t)r.type);
                for (int i = 0; i < pSize && (patchPos + i) < body.size(); i++) {
                    addend |= ((uint32_t)body[patchPos + i]) << (i * 8);
                }
            }
            targetAddr = it->second + addend;
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

            targetAddr = segBase + segObjOff + (existingVal - origBase);
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

    // Phase 3: function attribute integration
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
// Only ZP→stack calls are errors (stack callers can generate ZP setup for fastcall).
void O45Linker::emitDiagnostics() {
    convErrors_.clear();

    for (const auto& [caller, callees] : callGraph_) {
        auto callerIt = funcAttrs_.find(caller);
        if (callerIt == funcAttrs_.end()) continue;

        // Only enforce if caller is ZP convention
        if (!(callerIt->second.flags & FUNC_FLAG_ZP_CONV)) continue;

        for (const auto& callee : callees) {
            auto calleeIt = funcAttrs_.find(callee);
            if (calleeIt == funcAttrs_.end()) continue;

            // Error if callee is stack convention (ZP_CONV bit clear)
            if (!(calleeIt->second.flags & FUNC_FLAG_ZP_CONV)) {
                convErrors_.push_back(
                    "calling convention mismatch: '" + caller + "' (zp_call)"
                    + " calls '" + callee + "' (stack_call)");
            }
        }
    }
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
}
