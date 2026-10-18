#include "O45Linker.hpp"
#include "O45OverlayFile.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <numeric>
#include <queue>
#include <sstream>
#include <cstring>
#include <map>
#include "LinkerLog.hpp"
#include <iostream>
#include <streambuf>

// ln45's verbosity (-v, -vv); see LinkerLog.hpp
int g_ln45Verbosity = 0;
namespace {
struct NullBuf : std::streambuf { int overflow(int c) override { return c; } };
NullBuf g_nullBuf;
std::ostream g_nullStream(&g_nullBuf);
}
std::ostream& ln45info() { return g_ln45Verbosity >= 1 ? std::cerr : g_nullStream; }
std::ostream& ln45debug() { return g_ln45Verbosity >= 2 ? std::cerr : g_nullStream; }

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

        // R_HIGH and R_LOWX have an extra byte
        if (o45RelocHasExtra(r.type)) {
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
    for (const auto& n : forcedUndefined_) undefined.insert(n);   // (the overlay runtime)

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

// Dead code removal before layout. resolveLibraries pulls a member for every
// undefined name, also names only a member pulled earlier needs, and keeps a
// member whose weak definitions all lost to strong ones; a member's unused
// functions come along too. ln45 keeps what is reachable from the roots --
// the command-line objects (whole), the startup code (an "init"
// sub-segment) and KEEP sections -- and drops the rest.
//
// The units: each section of a sectioned object (OPT_SECTIONS), and per
// object its residual (the bytes no section covers, live whenever anything
// of the object is). An unsectioned object is just its residual. A live
// unit's relocations make their targets live: an import through the
// definition the link uses (the first strong one, else the first weak one;
// X__ar / X__ar_N reach X), an internal relocation through its section
// reference (OPT_SECTREF) or the section its stored value lies in.
//
// Objects with nothing live are dropped; a sectioned object with dead
// sections is rewritten without them (compactObject), so the later stages
// see ordinary objects.
namespace {
struct UnitMap {
    // sections of one segment, sorted by offset: [offset, end) -> section index
    std::map<uint8_t, std::vector<std::pair<std::pair<uint32_t, uint32_t>, int>>> bySeg;
    int residual = 0;   // the residual's unit index (= number of sections)
    int unitAt(uint8_t seg, uint32_t off) const {
        auto it = bySeg.find(seg);
        if (it == bySeg.end()) return residual;
        for (const auto& [range, idx] : it->second)
            if (off >= range.first && off < range.second) return idx;
        return residual;
    }
};
}

static bool compactable(const O45File& o) {
    if (o.sections.empty()) return false;
    int textParts = 0;
    for (const auto& sa : o.segAttrs) if (sa.segId == SEG_TEXT) textParts++;
    return textParts <= 1;
}

static UnitMap unitMapOf(const O45File& o) {
    UnitMap m;
    if (!compactable(o)) return m;   // residual only (unit 0)
    for (size_t i = 0; i < o.sections.size(); i++) {
        const auto& s = o.sections[i];
        m.bySeg[s.segId].push_back({{s.offset, s.offset + s.length}, (int)i});
    }
    for (auto& [seg, v] : m.bySeg) std::sort(v.begin(), v.end());
    m.residual = (int)o.sections.size();
    return m;
}

// An internal relocation's stored value as the linker reads it (applyRelocs)
static uint32_t storedValue(const O45Reloc& r, const std::vector<uint8_t>& body) {
    if (r.offset >= body.size()) return 0;
    if (r.type == R_HIGH) return (uint32_t)(body[r.offset] << 8) | r.extra;
    if (r.type == R_LOWX) return ((uint32_t)r.extra << 8) | body[r.offset];
    uint32_t v = 0;
    int n = o45RelocPatchSize((uint8_t)r.type);
    for (int i = 0; i < n && r.offset + i < body.size(); i++) v |= (uint32_t)body[r.offset + i] << (8 * i);
    return v;
}

static uint32_t segOrigBase(const O45File& o, uint8_t seg) {
    switch (seg) {
        case SEG_TEXT: return o.tbase;
        case SEG_DATA: return o.dbase;
        case SEG_BSS:  return o.bbase;
        case SEG_ZP:   return o.zbase;
        default:       return 0;
    }
}

void O45Linker::gcSections(size_t numRoots) {
    size_t n = objects_.size();
    std::vector<UnitMap> maps(n);
    std::vector<std::vector<O45Reloc>> textR(n), dataR(n);
    std::vector<std::map<std::pair<uint8_t, uint32_t>, int>> refUnit(n);   // (kind, key) -> unit
    std::vector<std::vector<std::pair<int, uint32_t>>> importRefs(n);     // .ref: (unit, import index)
    std::vector<std::set<uint32_t>> refImports(n);                          // imports with a .ref
    for (size_t i = 0; i < n; i++) {
        const auto& o = objects_[i].obj;
        maps[i] = unitMapOf(o);
        textR[i] = O45RelocDecoder::decode(o.textRelocs);
        dataR[i] = O45RelocDecoder::decode(o.dataRelocs);
        if (compactable(o))
            for (const auto& r : o.sectRefs) {
                int u = r.section == O45_SECT_RESIDUAL ? maps[i].residual : r.section;
                if (r.kind == O45_SECTREF_IMPORT) {
                    importRefs[i].push_back({u, r.key});
                    refImports[i].insert(r.key);
                } else {
                    refUnit[i][{r.kind, r.key}] = u;
                }
            }
    }
    // the definition each name binds to
    std::map<std::string, std::pair<size_t, size_t>> strongDef, weakDef;   // name -> (object, export)
    for (size_t i = 0; i < n; i++)
        for (size_t e = 0; e < objects_[i].obj.exports.size(); e++) {
            const auto& exp = objects_[i].obj.exports[e];
            (exp.isWeak() ? weakDef : strongDef).emplace(exp.name, std::make_pair(i, e));
        }
    auto definer = [&](const std::string& name, std::pair<size_t, size_t>& out) {
        auto it = strongDef.find(name);
        if (it == strongDef.end()) it = weakDef.find(name);
        if (it == weakDef.end()) return false;
        out = it->second;
        return true;
    };
    auto exportUnit = [&](size_t i, size_t e) {
        auto r = refUnit[i].find({O45_SECTREF_EXPORT, (uint32_t)e});
        if (r != refUnit[i].end()) return r->second;
        const auto& exp = objects_[i].obj.exports[e];
        uint8_t seg = exp.segmentId();
        if (seg < SEG_TEXT || seg > SEG_ZP) return maps[i].residual;
        return maps[i].unitAt(seg, exp.offset);
    };
    // the unit an internal relocation (in the text or data table) points into
    auto internalTarget = [&](size_t i, bool inText, const O45Reloc& r) {
        auto ref = refUnit[i].find({(uint8_t)(inText ? O45_SECTREF_TEXT_RELOC : O45_SECTREF_DATA_RELOC), r.offset});
        if (ref != refUnit[i].end()) return ref->second;
        const auto& o = objects_[i].obj;
        uint32_t v = storedValue(r, inText ? o.textBody : o.dataBody);
        return maps[i].unitAt((uint8_t)r.segment, v - segOrigBase(o, (uint8_t)r.segment));
    };

    std::vector<std::vector<bool>> live(n);
    for (size_t i = 0; i < n; i++) live[i].assign(maps[i].residual + 1, false);
    std::vector<std::pair<size_t, int>> work;
    auto mark = [&](size_t i, int u) {
        if (live[i][u]) return;
        live[i][u] = true;
        work.push_back({i, u});
        int res = maps[i].residual;
        if (!live[i][res]) { live[i][res] = true; work.push_back({i, res}); }   // the residual comes along
    };
    // --gc-inputs: the input objects are not roots themselves -- when there
    // is startup code to reach them from (else they stay whole)
    if (gcInputs_) {
        bool hasInit = false;
        for (size_t i = 0; i < n; i++)
            for (const auto& sa : objects_[i].obj.segAttrs) if (sa.name == "init") hasInit = true;
        if (hasInit) numRoots = 0;
    }
    for (const auto& name : keepSymbols_) {   // -u
        std::pair<size_t, size_t> d;
        if (definer(name, d)) mark(d.first, exportUnit(d.first, d.second));
    }
    for (size_t i = 0; i < n; i++) {
        bool root = i < numRoots;
        for (const auto& sa : objects_[i].obj.segAttrs) if (sa.name == "init") root = true;
        if (root) { for (int u = 0; u <= maps[i].residual; u++) mark(i, u); continue; }
        if (compactable(objects_[i].obj))
            for (size_t s = 0; s < objects_[i].obj.sections.size(); s++)
                if (objects_[i].obj.sections[s].keep) mark(i, (int)s);
    }
    while (!work.empty()) {
        auto [i, u] = work.back();
        work.pop_back();
        const auto& o = objects_[i].obj;
        for (int t = 0; t < 2; t++) {
            bool inText = t == 0;
            for (const auto& r : inText ? textR[i] : dataR[i]) {
                if (maps[i].unitAt(inText ? SEG_TEXT : SEG_DATA, r.offset) != u) continue;
                if (r.segment == SEG_EXTERNAL) {
                    if (r.symbolIndex >= o.imports.size()) continue;
                    std::string name = o.imports[r.symbolIndex].name;
                    std::pair<size_t, size_t> d;
                    bool found = definer(name, d);
                    if (!found) {
                        size_t ar = name.rfind("__ar");
                        if (ar != std::string::npos) found = definer(name.substr(0, ar), d);
                    }
                    if (found) mark(d.first, exportUnit(d.first, d.second));
                } else {
                    mark(i, internalTarget(i, inText, r));
                }
            }
        }
        // .ref: the unit needs an import's definition (no relocation)
        for (const auto& [ru, k] : importRefs[i]) {
            if (ru != u || k >= o.imports.size()) continue;
            std::pair<size_t, size_t> d;
            if (definer(o.imports[k].name, d)) mark(d.first, exportUnit(d.first, d.second));
        }
        // an import no relocation uses (a .extern for its side effect) still pulls its definer
        if (u == maps[i].residual)
            for (size_t k = 0; k < o.imports.size(); k++) {
                const auto& imp = o.imports[k];
                if (refImports[i].count((uint32_t)k)) continue;   // tied to a section by .ref
                std::pair<size_t, size_t> d;
                bool found = definer(imp.name, d);
                if (!found) {
                    size_t ar = imp.name.rfind("__ar");
                    if (ar != std::string::npos) found = definer(imp.name.substr(0, ar), d);
                }
                bool used = false;
                for (const auto* tab : {&textR[i], &dataR[i]})
                    for (const auto& r : *tab) if (r.segment == SEG_EXTERNAL && r.symbolIndex < o.imports.size() &&
                                                   o.imports[r.symbolIndex].name == imp.name) { used = true; break; }
                if (found && !used) mark(d.first, exportUnit(d.first, d.second));
            }
    }

    std::vector<InputObject> kept;
    for (size_t i = 0; i < n; i++) {
        if (!live[i][maps[i].residual]) { discardedObjects_.push_back(objects_[i].filename); continue; }
        if (compactable(objects_[i].obj)) {
            std::vector<bool> keepSec(live[i].begin(), live[i].end() - 1);
            uint32_t before = objects_[i].obj.tlen + objects_[i].obj.dlen;
            size_t dropped = std::count(keepSec.begin(), keepSec.end(), false);
            if (dropped) {
                std::string err;
                O45File compacted;
                if (!compactObject(objects_[i].obj, keepSec, textR[i], dataR[i], refUnit[i], importRefs[i], compacted, err)) {
                    gcErrors_.push_back(objects_[i].filename + ": " + err);
                } else {
                    discardedSections_.push_back({objects_[i].filename, dropped,
                        before - (compacted.tlen + compacted.dlen)});
                    for (size_t s = 0; s < keepSec.size(); s++)
                        if (!keepSec[s]) discardedSectionNames_.push_back(objects_[i].filename + ": " +
                            objects_[i].obj.sections[s].name + " (" + std::to_string(objects_[i].obj.sections[s].length) + " bytes)");
                    objects_[i].obj = std::move(compacted);
                }
            }
        }
        kept.push_back(std::move(objects_[i]));
    }
    objects_ = std::move(kept);
}

// Rewrite a sectioned object without its dead sections: segment bodies and
// lengths shrink (a kept section keeps its offset modulo its alignment),
// relocation sites, exports, line info and the text sub-segment move, and the
// stored values of internal relocations follow their targets
bool O45Linker::compactObject(const O45File& o, const std::vector<bool>& keepSec,
                              const std::vector<O45Reloc>& textRel, const std::vector<O45Reloc>& dataRel,
                              const std::map<std::pair<uint8_t, uint32_t>, int>& refUnit,
                              const std::vector<std::pair<int, uint32_t>>& importRefs,
                              O45File& out, std::string& err) {
    UnitMap m = unitMapOf(o);
    int residual = m.residual;
    // pieces per segment: [oldStart, oldEnd) -> newStart, unit
    struct Piece { uint32_t oldStart, oldEnd, newStart; int unit; };
    std::map<uint8_t, std::vector<Piece>> pieces;
    std::map<uint8_t, uint32_t> newLen;
    auto segLen = [&](uint8_t seg) -> uint32_t {
        return seg == SEG_TEXT ? o.tlen : seg == SEG_DATA ? o.dlen : seg == SEG_BSS ? o.blen : o.zlen;
    };
    for (uint8_t seg : {(uint8_t)SEG_TEXT, (uint8_t)SEG_DATA, (uint8_t)SEG_BSS, (uint8_t)SEG_ZP}) {
        uint32_t len = segLen(seg), pos = 0, np = 0;
        auto& v = pieces[seg];
        const auto& secs = m.bySeg[seg];
        size_t k = 0;
        while (pos < len) {
            if (k < secs.size() && secs[k].first.first == pos) {
                int s = secs[k].second;
                uint32_t end = secs[k].first.second;
                if (keepSec[s]) {
                    uint32_t a = 1u << o.sections[s].alignLog2;
                    while (np % a != pos % a) np++;   // the same offset modulo its alignment
                    v.push_back({pos, end, np, s});
                    np += end - pos;
                }
                pos = end;
                k++;
            } else {
                uint32_t end = k < secs.size() ? secs[k].first.first : len;
                v.push_back({pos, end, np, residual});
                np += end - pos;
                pos = end;
            }
        }
        newLen[seg] = np;
    }
    // new offset of an old offset in a unit (an end offset maps to the piece it ends)
    auto mapOff = [&](uint8_t seg, uint32_t off, int unit, uint32_t& outOff) {
        const Piece* best = nullptr;
        for (const auto& p : pieces[seg]) {
            if (p.unit != unit) continue;
            if (off >= p.oldStart && off < p.oldEnd) { best = &p; break; }
            if (off == p.oldEnd && !best) best = &p;
        }
        if (!best) {   // an empty residual (or an offset outside every piece)
            if (unit == residual) { outOff = off; return true; }
            return false;
        }
        outOff = best->newStart + (off - best->oldStart);
        return true;
    };
    auto siteNew = [&](uint8_t seg, uint32_t off, uint32_t& outOff) -> bool {
        int u = m.unitAt(seg, off);
        if (u != residual && !keepSec[u]) return false;   // dead
        return mapOff(seg, off, u, outOff);
    };

    out = o;
    out.sections.clear();
    out.sectRefs.clear();
    // bodies
    for (uint8_t seg : {(uint8_t)SEG_TEXT, (uint8_t)SEG_DATA}) {
        const auto& src = seg == SEG_TEXT ? o.textBody : o.dataBody;
        std::vector<uint8_t> body(newLen[seg], 0);
        for (const auto& p : pieces[seg])
            for (uint32_t b = p.oldStart; b < p.oldEnd && b < src.size(); b++) body[p.newStart + (b - p.oldStart)] = src[b];
        (seg == SEG_TEXT ? out.textBody : out.dataBody) = std::move(body);
    }
    out.tlen = newLen[SEG_TEXT];
    out.dlen = newLen[SEG_DATA];
    out.blen = newLen[SEG_BSS];
    out.zlen = newLen[SEG_ZP];
    // imports: those only dropped code used go (an import no relocation used
    // at all stays: it may be there for its definition's sake); renumber
    std::vector<bool> usedBefore(o.imports.size(), false), usedAfter(o.imports.size(), false);
    for (int t = 0; t < 2; t++)
        for (const auto& r : t == 0 ? textRel : dataRel) {
            if (r.segment != SEG_EXTERNAL || r.symbolIndex >= o.imports.size()) continue;
            usedBefore[r.symbolIndex] = true;
            uint32_t ns;
            if (siteNew(t == 0 ? SEG_TEXT : SEG_DATA, r.offset, ns)) usedAfter[r.symbolIndex] = true;
        }
    for (const auto& [u, k] : importRefs) {   // .ref uses
        if (k >= o.imports.size()) continue;
        usedBefore[k] = true;
        if (u == residual || keepSec[u]) usedAfter[k] = true;
    }
    std::vector<uint32_t> newImportIndex(o.imports.size(), 0);
    out.imports.clear();
    for (size_t k = 0; k < o.imports.size(); k++) {
        if (usedBefore[k] && !usedAfter[k]) continue;
        newImportIndex[k] = (uint32_t)out.imports.size();
        out.imports.push_back(o.imports[k]);
    }

    // relocations: drop dead sites, move live ones, retarget internal values
    for (int t = 0; t < 2; t++) {
        bool inText = t == 0;
        uint8_t siteSeg = inText ? SEG_TEXT : SEG_DATA;
        const auto& srcBody = inText ? o.textBody : o.dataBody;
        auto& body = inText ? out.textBody : out.dataBody;
        std::vector<O45Reloc> kept;
        for (O45Reloc r : inText ? textRel : dataRel) {
            uint32_t ns;
            if (!siteNew(siteSeg, r.offset, ns)) continue;
            uint32_t oldSite = r.offset;
            r.offset = ns;
            if (r.segment == SEG_EXTERNAL && r.symbolIndex < newImportIndex.size())
                r.symbolIndex = newImportIndex[r.symbolIndex];
            if (r.segment != SEG_EXTERNAL) {
                auto ref = refUnit.find({(uint8_t)(inText ? O45_SECTREF_TEXT_RELOC : O45_SECTREF_DATA_RELOC), oldSite});
                O45Reloc probe = r;
                probe.offset = oldSite;
                uint32_t v = storedValue(probe, srcBody);
                uint32_t base = segOrigBase(o, (uint8_t)r.segment);
                uint32_t rel = v - base;
                int unit = ref != refUnit.end() ? ref->second : m.unitAt((uint8_t)r.segment, rel);
                if (unit != residual && !keepSec[unit]) {
                    err = "a kept relocation (offset " + std::to_string(oldSite) + ") points into dropped section '" +
                          o.sections[unit].name + "'";
                    return false;
                }
                // the target section's (or residual piece's) move
                int64_t delta = 0;
                if (r.type == R_LOW && ref != refUnit.end()) {
                    // only the low byte is stored: the section's start moves the value
                    uint32_t start = unit == residual ? 0 : o.sections[unit].offset, ns2;
                    if (!mapOff((uint8_t)r.segment, start, unit, ns2)) { err = "unmapped R_LOW target"; return false; }
                    delta = (int64_t)ns2 - start;
                } else {
                    uint32_t nrel;
                    // a value outside the target unit (one past the end, a negative addend):
                    // move it with the unit's first piece
                    if (!mapOff((uint8_t)r.segment, rel, unit, nrel)) {
                        uint32_t start = unit == residual ? 0 : o.sections[unit].offset;
                        if (!mapOff((uint8_t)r.segment, start, unit, nrel)) { err = "unmapped relocation target"; return false; }
                        nrel = nrel + (rel - start);
                    }
                    delta = (int64_t)nrel - rel;
                }
                uint32_t nv = (uint32_t)((int64_t)v + delta);
                if (r.type == R_HIGH) {
                    body[r.offset] = (uint8_t)(nv >> 8);
                    r.extra = (uint8_t)nv;
                } else if (r.type == R_LOWX) {
                    body[r.offset] = (uint8_t)nv;
                    r.extra = (uint8_t)(nv >> 8);
                } else {
                    int sz = o45RelocPatchSize((uint8_t)r.type);
                    for (int b = 0; b < sz && r.offset + b < body.size(); b++) body[r.offset + b] = (uint8_t)(nv >> (8 * b));
                }
            }
            kept.push_back(r);
        }
        std::sort(kept.begin(), kept.end(), [](const O45Reloc& a, const O45Reloc& b) { return a.offset < b.offset; });
        (inText ? out.textRelocs : out.dataRelocs) = O45RelocEncoder::encode(kept);
    }
    // exports
    out.exports.clear();
    for (size_t e = 0; e < o.exports.size(); e++) {
        auto exp = o.exports[e];
        uint8_t seg = exp.segmentId();
        if (seg >= SEG_TEXT && seg <= SEG_ZP) {
            auto ref = refUnit.find({O45_SECTREF_EXPORT, (uint32_t)e});
            int unit = ref != refUnit.end() ? ref->second : m.unitAt(seg, exp.offset);
            if (unit != residual && !keepSec[unit]) continue;
            uint32_t no;
            if (!mapOff(seg, exp.offset, unit, no)) continue;
            exp.offset = no;
        }
        out.exports.push_back(exp);
    }
    // line info and the text sub-segment
    out.lineInfos.clear();
    for (auto li : o.lineInfos) {
        uint32_t no;
        if (!siteNew(SEG_TEXT, li.textOffset, no)) continue;
        li.textOffset = no;
        out.lineInfos.push_back(li);
    }
    for (auto& sa : out.segAttrs)
        if (sa.segId == SEG_TEXT) { sa.offset = 0; sa.length = out.tlen; }
    return true;
}

// Largest alignment an object asks for in a segment (cc45.align.data / .bss)
static uint32_t objAlign(const O45File& o, const char* marker) {
    uint32_t a = 1;
    for (const auto& exp : o.exports)
        if (exp.name == marker && exp.offset > a) a = exp.offset;
    return a;
}

// BSS: each object's part in order (on its cc45.align.bss boundary), less
// the SAC blocks computeSacOverlay moved, then the SAC overlay area
void O45Linker::layoutBss() {
    mergedBssLen_ = 0;
    bssLenBeforeZpPage_ = -1;
    for (int idx = 0; idx < (int)objects_.size(); idx++) {
        auto& input = objects_[idx];
        uint32_t ba = objAlign(input.obj, "cc45.align.bss");
        mergedBssLen_ = (mergedBssLen_ + ba - 1) / ba * ba;
        input.bssOffset = mergedBssLen_;
        input.bssRemaps.clear();
        auto it = sacMoves_.find(idx);
        if (it == sacMoves_.end()) {
            input.bssKeptLen = input.obj.blen;
            mergedBssLen_ += input.obj.blen;
            continue;
        }
        auto moves = it->second;
        std::sort(moves.begin(), moves.end(),
                  [](const SacMove& a, const SacMove& b) { return a.srcOffset < b.srcOffset; });
        uint32_t cursor = 0, dest = input.bssOffset;
        for (const auto& m : moves) {
            if (m.srcOffset > cursor) {
                input.bssRemaps.push_back({cursor, m.srcOffset - cursor, dest, false});
                dest += m.srcOffset - cursor;
            }
            cursor = m.srcOffset + m.len;
        }
        if (input.obj.blen > cursor) {
            input.bssRemaps.push_back({cursor, input.obj.blen - cursor, dest, false});
            dest += input.obj.blen - cursor;
        }
        input.bssKeptLen = dest - input.bssOffset;
        mergedBssLen_ = dest;
    }
    sacOverlayOffset_ = mergedBssLen_;
    mergedBssLen_ += sacOverlayLen_;
    for (auto& [idx, moves] : sacMoves_)
        for (const auto& m : moves)
            objects_[idx].bssRemaps.push_back(
                {m.srcOffset, m.len, sacOverlayOffset_ + m.overlayOffset, true});
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
    for (auto& t : ovlText_) t.clear();
    mergedData_.clear();
    mergedBssLen_ = 0;
    bssLenBeforeZpPage_ = -1;
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

    // __far code: each object's "far" chunk goes to an automatic mapped
    // overlay (far1, far2, ...) of 8 KB (16 KB when one chunk needs more).
    // The packing follows the calls: a chunk is one file's far code (it may
    // use relative BSRs inside: never split), and chunks that call each
    // other share an overlay when they fit, so those calls need no change
    // of the map.
    //   1. weights: JSR/JMP sites from one chunk to a far function of
    //      another count 4, other references (an address taken) 1
    //   2. clusters: from the heaviest pair down, two clusters are merged
    //      when they fit in one window together
    //   3. overlays: the clusters, largest first, each into the overlay it
    //      has the most calls with that has room for it (none: the first
    //      with room; no room: a new overlay)
    ovlNames_.resize(ovlUserCount_);
    std::map<size_t, int> farOvl;   // object -> its far chunk's overlay
    if (chunksByName.count("far")) {
        const auto& fc = chunksByName["far"];
        size_t n = fc.size();
        uint32_t cap = 0x2000;
        for (const auto& c : fc) {
            if (c.srcLen > 0x4000) {
                errorMsg = "__far code of " + objects_[c.objectIdx].filename + " (" + std::to_string(c.srcLen) +
                           " bytes) is larger than the 16 KB window";
                return false;
            }
            if (c.srcLen > cap) cap = 0x4000;
        }
        // which chunk defines each far symbol
        std::map<std::string, size_t> farDef;
        for (size_t k = 0; k < n; k++)
            for (const auto& exp : objects_[fc[k].objectIdx].obj.exports)
                if (exp.segmentId() == SEG_TEXT && exp.offset >= fc[k].srcOffset &&
                    exp.offset < fc[k].srcOffset + fc[k].srcLen)
                    farDef.emplace(exp.name, k);
        // 1. weights between chunks
        std::map<std::pair<size_t, size_t>, uint32_t> weight;
        for (size_t k = 0; k < n; k++) {
            const auto& in = objects_[fc[k].objectIdx];
            const auto& body = in.obj.textBody;
            for (const auto& r : O45RelocDecoder::decode(in.obj.textRelocs)) {
                if (r.offset < fc[k].srcOffset || r.offset >= fc[k].srcOffset + fc[k].srcLen) continue;
                if (r.segment != SEG_EXTERNAL || r.symbolIndex >= in.obj.imports.size()) continue;
                auto d = farDef.find(in.obj.imports[r.symbolIndex].name);
                if (d == farDef.end() || d->second == k) continue;
                bool call = r.type == R_WORD && r.offset > 0 && r.offset - 1 < body.size() &&
                            (body[r.offset - 1] == 0x20 || body[r.offset - 1] == 0x4C);
                weight[{std::min(k, d->second), std::max(k, d->second)}] += call ? 4 : 1;
            }
        }
        // 2. clusters (union-find), heaviest pairs first
        std::vector<size_t> root(n);
        std::vector<uint32_t> size(n);
        for (size_t k = 0; k < n; k++) { root[k] = k; size[k] = fc[k].srcLen; }
        auto find = [&](size_t x) { while (root[x] != x) x = root[x] = root[root[x]]; return x; };
        std::vector<std::pair<uint32_t, std::pair<size_t, size_t>>> edges;
        for (const auto& [pr, w] : weight) edges.push_back({w, pr});
        std::stable_sort(edges.begin(), edges.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        for (const auto& [w, pr] : edges) {
            size_t a = find(pr.first), b = find(pr.second);
            if (a == b || size[a] + size[b] > cap) continue;
            if (b < a) std::swap(a, b);
            root[b] = a;
            size[a] += size[b];
        }
        // 3. clusters into overlays
        std::vector<size_t> clusters;
        for (size_t k = 0; k < n; k++) if (find(k) == k) clusters.push_back(k);
        std::stable_sort(clusters.begin(), clusters.end(), [&](size_t a, size_t b) { return size[a] > size[b]; });
        std::vector<uint32_t> used;
        std::vector<int> groupOf(n, -1);   // cluster root -> overlay
        for (size_t c : clusters) {
            std::vector<uint32_t> link(used.size(), 0);
            for (const auto& [pr, w] : weight) {
                size_t a = find(pr.first), b = find(pr.second);
                if (a == c && groupOf[b] >= 0) link[groupOf[b]] += w;
                else if (b == c && groupOf[a] >= 0) link[groupOf[a]] += w;
            }
            int best = -1;
            for (size_t g = 0; g < used.size(); g++)
                if (used[g] + size[c] <= cap && (best < 0 || link[g] > link[best])) best = (int)g;
            if (best < 0) {
                best = (int)used.size();
                used.push_back(0);
                ovlNames_.push_back("far" + std::to_string(best + 1));
            }
            used[best] += size[c];
            groupOf[c] = best;
        }
        for (size_t k = 0; k < n; k++)
            farOvl[fc[k].objectIdx] = (int)(ovlUserCount_ + groupOf[find(k)] + 1);
    }
    ovlText_.resize(ovlNames_.size());

    // Concatenate chunks in order, building remaps for each object. An
    // object with aligned functions (cc45.align.text) keeps its code at the
    // same address modulo that alignment as its offset in the object
    auto textAlignOf = [](const O45File& o) -> uint32_t {
        uint32_t a = 1;
        for (const auto& exp : o.exports)
            if (exp.name == "cc45.align.text" && exp.offset > a) a = exp.offset;
        return a;
    };
    for (const auto& name : orderedNames) {
        for (const auto& chunk : chunksByName[name]) {
            auto& input = objects_[chunk.objectIdx];
            // (a code overlay's: in its own text, at the region's address,
            // which is page-aligned; __far code: in its automatic overlay)
            int chunkOvl = chunk.subSegName == "far" ? farOvl[chunk.objectIdx] : input.ovl;
            auto& text = chunkOvl ? ovlText_[chunkOvl - 1] : mergedText_;
            uint32_t tbase = chunkOvl ? 0 : textBase_;
            uint32_t ta = textAlignOf(input.obj);
            if (ta > 1)
                while ((tbase + (uint32_t)text.size() - chunk.srcOffset) % ta) text.push_back(0);
            uint32_t destOffset = (uint32_t)text.size();

            // Copy bytes from object's textBody
            auto begin = input.obj.textBody.begin() + chunk.srcOffset;
            auto end = begin + chunk.srcLen;
            text.insert(text.end(), begin, end);

            // Record remap
            input.textRemaps.push_back({chunk.srcOffset, chunk.srcLen, destOffset, chunk.subSegName, chunkOvl});
        }
    }

    // For objects with no sub-segments and no text body processed above,
    // set simple textOffset (already handled via defaultChunks → "code")
    for (auto& input : objects_) {
        if (input.textRemaps.empty() && !input.obj.textBody.empty()) {
            // Shouldn't happen — all objects should have been processed above
            auto& text = input.ovl ? ovlText_[input.ovl - 1] : mergedText_;
            input.textOffset = (uint32_t)text.size();
            text.insert(text.end(), input.obj.textBody.begin(), input.obj.textBody.end());
        } else if (input.textRemaps.size() == 1 && input.textRemaps[0].srcOffset == 0) {
            // Single chunk — set textOffset for backward compatibility
            input.textOffset = input.textRemaps[0].destOffset;
        } else if (!input.textRemaps.empty()) {
            // Multiple chunks — textOffset points to the first chunk
            input.textOffset = input.textRemaps[0].destOffset;
        }
    }

    // --- Data, BSS, ZP: concatenation. An object whose data/BSS holds aligned
    // variables (_Alignas, __attribute__((aligned))) exports cc45.align.data /
    // cc45.align.bss: its part starts on that boundary, and the segment bases
    // are aligned to the largest such value.
    dataAlign_ = bssAlign_ = 1;
    for (auto& input : objects_) {
        uint32_t da = objAlign(input.obj, "cc45.align.data");
        uint32_t ba = objAlign(input.obj, "cc45.align.bss");
        dataAlign_ = std::max(dataAlign_, da);
        bssAlign_ = std::max(bssAlign_, ba);
        while (mergedData_.size() % da) mergedData_.push_back(0);
        input.dataOffset = (uint32_t)mergedData_.size();
        mergedData_.insert(mergedData_.end(),
                           input.obj.dataBody.begin(), input.obj.dataBody.end());

        input.zpOffset = mergedZpLen_;
        mergedZpLen_ += input.obj.zlen;
    }
    layoutBss();

    // Auto-place data right after text if not explicitly set
    auto alignUp = [](uint32_t v, uint32_t a) { return (v + a - 1) / a * a; };
    if (!dataBaseSet_) {
        dataBase_ = alignUp(textBase_ + (uint32_t)mergedText_.size(), dataAlign_);
    }
    if (!bssBaseSet_) {
        // after the data, or after the text when -d put the data below it
        bssBase_ = alignUp(std::max(dataBase_ + (uint32_t)mergedData_.size(),
                                    textBase_ + (uint32_t)mergedText_.size()), bssAlign_);
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
    symOvl_.clear();
    realAddr_.clear();

    // Collect all exports, handling weak-vs-strong resolution
    for (const auto& input : objects_) {
        for (const auto& exp : input.obj.exports) {
            bool isWeak = exp.isWeak();
            uint8_t segId = exp.segmentId();

            // Compute final absolute address
            uint32_t base = 0;
            uint32_t objOffset = 0;
            switch ((O45Segment)segId) {
                case SEG_TEXT: base = textBaseOf(input); objOffset = input.textOffset; break;
                case SEG_DATA: base = dataBase_; objOffset = input.dataOffset; break;
                case SEG_BSS:  base = bssBase_;  objOffset = input.bssOffset;  break;
                case SEG_ZP:   base = zpBase_;   objOffset = input.zpOffset;   break;
                default:       base = 0; objOffset = 0; break;
            }
            uint32_t finalAddr;
            if (segId == SEG_TEXT && !input.textRemaps.empty()) {
                // Symbol offset is within the object's original text body;
                // remap to position in the merged text body (or its overlay's)
                finalAddr = textAddrOf(input, exp.offset);
            } else if (segId == SEG_BSS) {
                finalAddr = base + input.remapBssOffset(exp.offset);
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
            if (segId == SEG_TEXT) { symOvl_[exp.name] = input.ovlAt(exp.offset); realAddr_[exp.name] = finalAddr; }
        }
    }
    // (a strong definition replacing a weak one: its object decides)
    for (const auto& input : objects_)
        for (const auto& exp : input.obj.exports)
            if (exp.segmentId() == SEG_TEXT && symbolSource_[exp.name] == input.filename) {
                symOvl_[exp.name] = input.ovlAt(exp.offset);
                realAddr_[exp.name] = globalSymbols_[exp.name];
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

    // __zp_page: a page-aligned 256-byte block at the end of BSS. The CRT moves
    // the base page register there (TAB) so the program's zero page never
    // overlaps the ROM's. Only allocated when some object references it.
    if (!globalSymbols_.count("__zp_page")) {
        bool wanted = false;
        for (const auto& input : objects_)
            for (const auto& imp : input.obj.imports)
                if (imp.name == "__zp_page") wanted = true;
        if (wanted) {
            if (bssLenBeforeZpPage_ < 0) bssLenBeforeZpPage_ = (int64_t)mergedBssLen_;
            uint32_t page = (bssBase_ + (uint32_t)bssLenBeforeZpPage_ + 0xFF) & ~0xFFu;
            // __interrupt handlers run on their own base page (__zp_page_isr),
            // the page after __zp_page, so they never clobber the zero page
            // (parameters, temporaries, frame pointer) of the code they interrupt
            bool hasIsr = false;
            for (const auto& input : objects_)
                for (const auto& exp : input.obj.exports)
                    if (exp.hasFuncAttr && (exp.funcAttr.flags & FUNC_FLAG_ISR)) hasIsr = true;
            mergedBssLen_ = page + (hasIsr ? 0x200 : 0x100) - bssBase_;
            globalSymbols_["__zp_page"] = page;
            symbolSource_["__zp_page"] = "<linker>";
            symbolWeak_["__zp_page"] = false;
            if (!globalSymbols_.count("__zp_page_isr")) {
                globalSymbols_["__zp_page_isr"] = hasIsr ? page + 0x100 : page;
                symbolSource_["__zp_page_isr"] = "<linker>";
                symbolWeak_["__zp_page_isr"] = false;
            }
        }
    }

    // <f>__zar: function f's private base-page block (cc45 .zar), placed by
    // computeZarOverlay in $80-$E9 of the program's base page (__zp_page is
    // page-aligned: the low byte of the address is the base-page address)
    {
        uint32_t page = globalSymbols_.count("__zp_page") ? globalSymbols_["__zp_page"] : 0;
        for (const auto& input : objects_)
            for (const auto& imp : input.obj.imports) {
                const std::string& n = imp.name;
                if (n.size() <= 5 || n.compare(n.size() - 5, 5, "__zar") != 0 || globalSymbols_.count(n)) continue;
                auto it = zarOffset_.find(n.substr(0, n.size() - 5));
                globalSymbols_[n] = page + (it != zarOffset_.end() ? it->second : ZAR_LO);
                symbolSource_[n] = "<linker>";
                symbolWeak_[n] = false;
            }
    }

    // __zp variables: __zp_page + their base-page address (placeZpVars)
    {
        uint32_t page = globalSymbols_.count("__zp_page") ? globalSymbols_["__zp_page"] : 0;
        for (const auto& [sym, at] : zpVarAddr_) {
            if (globalSymbols_.count(sym) && symbolSource_[sym] != "<linker>") continue;
            globalSymbols_[sym] = page + at;
            symbolSource_[sym] = "<linker>";
            symbolWeak_[sym] = false;
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
    // The stack's share below __stack_top (malloc's heap ends there):
    // --stack-size (default 4096), unless an object defines it (cc45
    // -fstack-size=N in the module defining main)
    if (!globalSymbols_.count("__stack_size")) {
        globalSymbols_["__stack_size"] = stackSize_;
        symbolSource_["__stack_size"] = "<linker>";
        symbolWeak_["__stack_size"] = false;
    }
    // The byte crt0 fills BSS with: --bss-fill (default 0), unless an object
    // defines it (cc45 -fbss-fill=V in the module defining main)
    if (!globalSymbols_.count("__bss_fill")) {
        globalSymbols_["__bss_fill"] = bssFill_;
        symbolSource_["__bss_fill"] = "<linker>";
        symbolWeak_["__bss_fill"] = false;
    }

    // Code overlays: references from outside an overlay go to the stubs
    if (!ovlNames_.empty()) {
        for (const auto& [n, a] : ovlStubs_) globalSymbols_[n] = a;
        auto def = [&](const std::string& n, uint32_t v) {
            globalSymbols_[n] = v; symbolSource_[n] = "<linker>"; symbolWeak_[n] = false;
        };
        def("__ovl_region", ovlBase_);
        def("___ovl_region", ovlBase_);   // (C: __ovl_region, <overlay.h>)
        def("__ovl_region_end", ovlBase_ + ovlSize_);
        def("__ovl_mapped", ovlMap_ ? 1 : 0);
        def("__ovl_table", ovlTableAddr_);
        def("__ovl_file", ovlFileAddr_);
        // the far memory the overlay files take (C: __ovl_store_bank, ...,
        // __ovl_store2_bank, ... for a second bank): far_malloc leaves it alone
        for (size_t r = 0; r < 2; r++) {
            std::string k = r ? "2" : "";
            bool used = r < ovlStores_.size() && r < ovlStoreEnd_.size() && (r == 0 || ovlStoreEnd_[r] > ovlStores_[r].first);
            uint32_t first = used ? ovlStores_[r].first : 0;
            uint32_t end = used ? std::max(ovlStoreEnd_[r], first + 1) : 1;
            def("___ovl_store" + k + "_bank", used ? first >> 16 : 0);
            def("___ovl_store" + k + "_first", first & 0xFFFF);
            def("___ovl_store" + k + "_last", used ? (end - 1) & 0xFFFF : 0);
        }
        if (ovlMap_) {
            // the heap starts above the window ($8000)
            auto hs = globalSymbols_.find("__heap_start");
            if (hs == globalSymbols_.end() || hs->second == 0 || hs->second < 0x8000) def("__heap_start", 0x8000);
        } else {
            // the heap ends below the region
            auto he = globalSymbols_.find("__heap_end");
            if (he == globalSymbols_.end() || he->second == 0 || he->second > ovlBase_) def("__heap_end", ovlBase_);
        }
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
                name.rfind("cc45.", 0) != 0 &&
                !(name.size() > 4 && name.compare(name.size() - 4, 4, "__ar") == 0) &&
                importedSymbols.find(name) == importedSymbols.end() &&
                symbolSource_[name] != "<linker>") {
                // informational (most are library internals and C globals
                // used only in their own object): only with -v
                if (g_ln45Verbosity >= 1 && warnStream_)
                    *warnStream_ << "ln45: warning: unused global symbol '" << name << "' (defined in " << symbolSource_[name] << ")" << std::endl;
            }
        }
    }

    return true;
}

// --- Step 3: Apply relocations ---
// Decode each object's reloc tables and patch the merged segment bodies.

bool O45Linker::applyRelocs(const std::vector<O45Reloc>& relocs,
                             std::vector<uint8_t>& bodyArg,
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
        // Text (objIdx >= 0): its chunk decides where it went -- the merged
        // text or an overlay's (sub-segments, __far code)
        int siteOvl = objIdx >= 0 ? input.ovlAt(r.offset) : 0;
        auto& body = objIdx >= 0 ? (siteOvl ? ovlText_[siteOvl - 1] : mergedText_) : bodyArg;
        uint32_t patchPos = objIdx >= 0 ? input.remapTextOffset(r.offset) : objOffset + r.offset;

        // Log all relocations before processing (disabled for production)
        //if (r.segment == SEG_TEXT || r.segment == SEG_DATA) {
        //    std::cerr << "DEBUG [Reloc " << rIdx << "]: " << segName
        //              << " reloc in " << input.filename
        //              << " at offset 0x" << std::hex << r.offset << " (patch pos 0x" << patchPos << ")"
        //              << " type=" << (int)r.type << " seg=" << (int)r.segment << std::dec << std::endl;
        //}

        if ((uint8_t)r.segment > SEG_ZP) {
            errorMsg = "unknown relocation segment " + std::to_string((int)r.segment) +
                       " in " + input.filename;
            return false;
        }

        if (patchPos >= body.size()) {
            errorMsg = "relocation offset " + std::to_string(patchPos) +
                       " out of range in " + input.filename;
            return false;
        }

        // Determine the target address
        uint32_t targetAddr = 0;
        int targetOvl = 0;   // an address in a code overlay's region: which
        // a JSR/JMP (patched in text): may go straight into its own overlay
        bool isCall = objIdx >= 0 && r.type == R_WORD && patchPos > 0 &&
                      (body[patchPos - 1] == 0x20 || body[patchPos - 1] == 0x4C);

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
            // a function of a code overlay: its stub, but for a call from
            // inside the same overlay (resident then)
            uint32_t symAddr = it->second;
            {
                auto o = symOvl_.find(lookupName);
                if (o != symOvl_.end() && o->second) {
                    bool stubbed = ovlStubs_.count(lookupName) > 0;
                    if (!stubbed || (isCall && siteOvl == o->second)) {
                        symAddr = realAddr_[lookupName];
                        targetOvl = o->second;
                        if (!(isCall && siteOvl == o->second)) {
                            errorMsg = "'" + lookupName + "' (code overlay " + ovlNames_[o->second - 1] +
                                       ") is referenced from " + input.filename + " without a stub";
                            return false;
                        }
                    }
                }
            }
            ln45debug() << "DEBUG [External Reloc]: Symbol '" << symName << "' = 0x" << std::hex << it->second << std::dec << std::endl;

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
                } else if (r.type == R_LOWX) {
                    // low byte at the patch site, high byte in extra
                    uint16_t w = (uint16_t)((r.extra << 8) | body[patchPos]);
                    addend = (uint32_t)(int32_t)(int16_t)w;
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
            targetAddr = symAddr + addend;

            if (arOffset > 0) {
                ln45debug() << "DEBUG [AR Reloc]: Symbol '" << symName << "' base=0x" << std::hex << it->second
                         << " arOffset=" << std::dec << arOffset << " targetAddr=0x" << std::hex << targetAddr << std::dec << std::endl;
            }

            // Check for thunk override (convention bridge)
            if (objIdx >= 0) {
                auto overIt = callSiteOverrides_.find({objIdx, rIdx});
                if (overIt != callSiteOverrides_.end()) {
                    targetAddr = overIt->second;
                    targetOvl = 0;
                }
            }
        } else {
            // Internal relocation — read the current value at the patch site
            // and add the segment base + object offset for that segment.
            uint32_t segBase = r.segment == SEG_TEXT ? textBase_ : segmentBase(r.segment);
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
            } else if (r.type == R_LOWX) {
                // patch site = low byte, extra = original high byte
                existingVal = (r.extra << 8) | body[patchPos];
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

            if (r.segment == SEG_TEXT && !input.textRemaps.empty()) {
                targetAddr = textAddrOf(input, segRelOff);
            } else if (r.segment == SEG_BSS) {
                targetAddr = segBase + input.remapBssOffset(segRelOff);
            } else {
                targetAddr = segBase + segObjOff + segRelOff;
            }
            // into a code overlay's text: a function's address taken (not a
            // JSR/JMP), or called from outside that overlay, is its stub
            int tOvl = r.segment == SEG_TEXT ? input.ovlAt(segRelOff) : 0;
            if (tOvl) {
                targetOvl = tOvl;
                if (!isCall || siteOvl != tOvl) {
                    bool stubbed = false;
                    for (const auto& exp : input.obj.exports)
                        if (exp.segmentId() == SEG_TEXT && exp.offset == segRelOff && ovlStubs_.count(exp.name)) {
                            targetAddr = ovlStubs_[exp.name];
                            targetOvl = 0;
                            stubbed = true;
                            break;
                        }
                    if (!stubbed && isCall) {
                        errorMsg = "a call in " + input.filename + " into code overlay " + ovlNames_[tOvl - 1] +
                                   " has no stub (not at a function's start)";
                        return false;
                    }
                }
            }

            // DEBUG: Log ALL DATA relocations
            if (r.segment == SEG_DATA) {
                // Log all DATA relocations with detailed information
                ln45debug() << "\n=== DEBUG: DATA relocation at offset 0x" << std::hex << patchPos << std::dec << " ===" << std::endl;
                ln45debug() << "  RelType: " << (int)r.type << " (32=R_LOW, 64=R_HIGH, 128=R_WORD)" << std::endl;
                ln45debug() << "  Extra: 0x" << std::hex << (int)r.extra << std::dec << std::endl;
                ln45debug() << "  PatchPos: 0x" << std::hex << patchPos << std::dec << std::endl;
                ln45debug() << "  ExistingVal: 0x" << std::hex << existingVal << std::dec << std::endl;
                ln45debug() << "  SegBase (dataBase_): 0x" << std::hex << segBase << std::dec << std::endl;
                ln45debug() << "  SegObjOff (dataOffset): 0x" << std::hex << segObjOff << std::dec << std::endl;
                ln45debug() << "  OrigBase (dbase): 0x" << std::hex << origBase << std::dec << std::endl;
                ln45debug() << "  SegRelOff (existingVal - origBase): 0x" << std::hex << segRelOff << std::dec << std::endl;
                ln45debug() << "  TargetAddr (segBase + segObjOff + segRelOff): 0x" << std::hex << targetAddr << std::dec << std::endl;

                // Show context: what's before this patch in the body (likely code)
                ln45debug() << "  Context (bytes -2 to +3 around patch): ";
                for (int i = -2; i <= 3; i++) {
                    int pos = (int)patchPos + i;
                    if (pos >= 0 && pos < (int)body.size()) {
                        if (i == 0) ln45debug() << "[";
                        ln45debug() << std::hex << (int)body[pos] << " ";
                        if (i == o45RelocPatchSize((uint8_t)r.type) - 1) ln45debug() << "]";
                    }
                }
                ln45debug() << std::dec << std::endl;

                ln45debug() << "  Bytes before patch: 0x" << std::hex;
                int patchSize = o45RelocPatchSize((uint8_t)r.type);
                for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                    ln45debug() << (int)body[patchPos + i] << " ";
                }
                ln45debug() << std::dec << std::endl;
            }
        }

        // A reference to a function reached through its base-page wrapper
        if (!zarWrapAt_.empty() && r.type != R_LOW) {
            auto w = zarWrapAt_.find(zarKey(targetOvl, targetAddr));
            if (w != zarWrapAt_.end()) targetAddr = w->second;
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
            ln45debug() << "DEBUG [DATA Patch]: at 0x" << std::hex << patchPos << " type=" << (int)r.type << " targetAddr=0x" << targetAddr << std::dec << "\n";
            ln45debug() << "  Bytes BEFORE patch: 0x" << std::hex;
            for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                ln45debug() << std::setw(2) << std::setfill('0') << (int)body[patchPos + i] << " ";
            }
            ln45debug() << std::dec << std::endl;
        }

        switch (r.type) {
            case R_WORD:
                body[patchPos]     = (uint8_t)(targetAddr & 0xFF);
                body[patchPos + 1] = (uint8_t)((targetAddr >> 8) & 0xFF);
                break;
            case R_LOW:
            case R_LOWX:
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
            // Phase 78: SMC immediate relocation patching
            case R_IMM8:
                // Patch 8-bit immediate with low byte of parameter value
                body[patchPos] = (uint8_t)(targetAddr & 0xFF);
                break;
            case R_IMM16:
                // Patch 16-bit immediate with low word of parameter value
                body[patchPos]     = (uint8_t)(targetAddr & 0xFF);
                body[patchPos + 1] = (uint8_t)((targetAddr >> 8) & 0xFF);
                break;
            default:
                errorMsg = "unknown relocation type in " + input.filename;
                return false;
        }

        // DEBUG: Complete logging for DATA relocations
        if (shouldLogPatch) {
            for (int i = 0; i < patchSize && (patchPos + i) < body.size(); i++) {
                ln45debug() << (int)body[patchPos + i] << " ";
            }
            ln45debug() << std::dec << std::endl;
            ln45debug() << "==================================================\n" << std::endl;
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
        if (!applyRelocs(textRelocs, mergedText_, textBase_,
                         input.textOffset, input, errorMsg, objIdx)) {
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

    // Selectively pull library members to resolve undefined symbols, then
    // drop the members only dead code needed
    size_t numCommandLine = objects_.size();
    if (!resolveLibraries(errorMsg)) return {};
    // Code overlays: their runtime (lib/stdlib_asm/ovl.s45), which the stubs
    // generated later call: pulled in and kept
    assignOverlays();
    if (!ovlNames_.empty() || hasFar_) {
        bool have = false;
        for (const auto& input : objects_)
            for (const auto& exp : input.obj.exports)
                if (exp.name == "__ovl_call") have = true;
        if (!have) {
            forcedUndefined_ = {"__ovl_call"};
            if (!resolveLibraries(errorMsg)) return {};
            forcedUndefined_.clear();
        }
        for (const char* k : {"__ovl_call", "__ovl_init", "__ovl_sa", "__ovl_jsave", "__ovl_jrestore"})
            if (std::find(keepSymbols_.begin(), keepSymbols_.end(), k) == keepSymbols_.end()) keepSymbols_.push_back(k);
    }
    if (gcObjectsOn_) {
        gcSections(numCommandLine);
        if (!gcErrors_.empty()) {
            errorMsg = "dropping unused sections: " + gcErrors_[0];
            return {};
        }
    }
    assignOverlays();

    if (!layoutSegments(errorMsg)) return {};
    if (!placeOverlayRegion(errorMsg)) return {};
    if (!placeZpVars(errorMsg)) return {};
    if (!resolveSymbols(errorMsg)) return {};

    // Phase 3: function attribute integration and debug info
    mergeLineMaps();
    buildFuncAttrs();
    buildCallGraph();
    computeTransitiveClobbers();
    emitDiagnostics();
    verifyStaticAllocSafety();  // Verify SAC constraints before thunk generation
    checkZpVarsInIsr();
    validateSACParameters();      // Phase 3: Validate SAC parameter metadata
    validateImmediateRelocations(); // Phase 78.4: Validate immediate relocation symbols

    // Check for calling convention errors and SAC violations
    if (!convErrors_.empty()) {
        errorMsg = convErrors_[0];
        for (size_t i = 1; i < convErrors_.size(); i++)
            errorMsg += "\n" + convErrors_[i];
        return {};
    }

    // SAC storage of functions never active at the same time is shared:
    // the blocks move into the overlay area at the end of BSS
    computeSacOverlay();
    if (!sacMoves_.empty()) {
        layoutBss();
        if (!resolveSymbols(errorMsg)) return {};
    }
    // and their private base-page blocks
    if (!computeZarOverlay(errorMsg)) return {};
    if (!zarOffset_.empty() && !resolveSymbols(errorMsg)) return {};

    // Stubs of the code overlays' functions (appended to mergedText_)
    if (!generateOverlayStubs(errorMsg)) return {};
    // Wrappers of functions whose base-page blocks are shared (appended to mergedText_)
    generateZarWrappers();
    // Generate thunks for convention mismatches (appends to mergedText_)
    generateThunks();
    // Copies of the __zp variables' initial values (appended to mergedText_)
    generateZpInit();

    // Thunks were appended to the text after data/BSS were placed: if they now
    // overlap automatically placed data or BSS, move them up and re-resolve.
    {
        uint32_t textEnd = textBase_ + (uint32_t)mergedText_.size();
        bool moved = false;
        if (!dataBaseSet_ && dataBase_ < textEnd) {
            dataBase_ = (textEnd + dataAlign_ - 1) / dataAlign_ * dataAlign_;
            moved = true;
        }
        uint32_t after = std::max(textEnd, dataBase_ + (uint32_t)mergedData_.size());
        if (!bssBaseSet_ && bssBase_ < after) {
            bssBase_ = (after + bssAlign_ - 1) / bssAlign_ * bssAlign_;
            moved = true;
        }
        if (moved) {
            if (!resolveSymbols(errorMsg)) return {};
            for (const auto& [name, addr] : thunkAddresses_) globalSymbols_[name] = addr;
        }
    }

    patchZpInit();

    // A weak absolute symbol an object's code was assembled with (ca45
    // OPT_WEAKUSE: .weak x / x = value, used there) must keep that value:
    // another definition would leave that object using the old one
    for (const auto& input : objects_)
        for (const auto& opt : input.obj.options) {
            if (opt.type != OPT_WEAKUSE) continue;
            size_t i = 0;
            while (i < opt.data.size()) {
                std::string name;
                while (i < opt.data.size() && opt.data[i]) name += (char)opt.data[i++];
                i++;
                if (i + 4 > opt.data.size()) break;
                uint32_t used = opt.data[i] | (opt.data[i + 1] << 8) | (opt.data[i + 2] << 16) | ((uint32_t)opt.data[i + 3] << 24);
                i += 4;
                auto g = globalSymbols_.find(name);
                if (g == globalSymbols_.end() || g->second == used) continue;
                auto hex = [](uint32_t v) { char b[12]; snprintf(b, sizeof b, "$%X", (unsigned)v); return std::string(b); };
                errorMsg = "'" + name + "' is weak in " + input.filename + ", whose code was assembled with its value there (" +
                           hex(used) + "), but the program defines it as " + hex(g->second) + " (" +
                           (symbolSource_.count(name) ? symbolSource_[name] : std::string("?")) +
                           "): a weak absolute symbol cannot be overridden where it is used; declare it .extern there "
                           "(and define its default elsewhere), or remove the other definition";
                return {};
            }
        }

    if (!applyRelocations(errorMsg)) return {};

    // Code overlays: the program (to the end of BSS and its base page) stays
    // below the region; the images go to the overlay file
    if (!ovlNames_.empty()) {
        uint32_t end = bssBase_ + mergedBssLen_;
        auto zp = globalSymbols_.find("__zp_page");
        if (zp != globalSymbols_.end()) end = std::max(end, zp->second + 0x100);
        end = std::max(end, dataBase_ + (uint32_t)mergedData_.size());
        if (end > ovlBase_) {
            char b[160];
            snprintf(b, sizeof b, "code overlays: the program ends at $%04X, above the overlay region at $%04X "
                     "(%u bytes below the stack's share): move more code into overlays or shrink the stack",
                     (unsigned)end, (unsigned)ovlBase_, (unsigned)ovlSize_);
            errorMsg = b;
            return {};
        }
        buildOverlayFile();
    }

    // Build the output binary: text + data (BSS is not emitted)
    std::vector<uint8_t> binary;

    // The image holds text and data at their addresses, the gap between
    // them filled with zeros: data after the text (the default, or -d above
    // it), or below it (-d under the text base: the image then starts at
    // the data base, imageBase_). Data overlapping the text is an error.
    uint32_t textEnd = textBase_ + (uint32_t)mergedText_.size();
    uint32_t dataEnd = dataBase_ + (uint32_t)mergedData_.size();
    imageBase_ = textBase_;
    auto hex4 = [](uint32_t v) {
        char b[8]; snprintf(b, sizeof b, "$%04X", (unsigned)v); return std::string(b);
    };
    if (!mergedData_.empty() && dataBase_ < textEnd && dataEnd > textBase_) {
        errorMsg = "data segment " + hex4(dataBase_) + "-" + hex4(dataEnd - 1) +
                   " overlaps the text segment " + hex4(textBase_) + "-" + hex4(textEnd - 1);
        return {};
    }
    if (!mergedData_.empty() && dataBase_ < textBase_) {
        imageBase_ = dataBase_;
        binary.insert(binary.end(), mergedData_.begin(), mergedData_.end());
        binary.insert(binary.end(), textBase_ - dataEnd, 0x00);
        binary.insert(binary.end(), mergedText_.begin(), mergedText_.end());
    } else {
        binary.insert(binary.end(), mergedText_.begin(), mergedText_.end());
        if (!mergedData_.empty()) {
            binary.insert(binary.end(), dataBase_ - textEnd, 0x00);
            binary.insert(binary.end(), mergedData_.begin(), mergedData_.end());
        }
    }

    if (isPrg) {
        // 2-byte load address header (little-endian): where the image starts
        binary.insert(binary.begin(), (uint8_t)((imageBase_ >> 8) & 0xFF));
        binary.insert(binary.begin(), (uint8_t)(imageBase_ & 0xFF));
    }

    return binary;
}

// =============================================================================
// Phase 3: Function attribute integration
// =============================================================================

// 3.1 — Extract function attributes from all loaded objects into a global map.
void O45Linker::buildFuncAttrs() {
    // Compilation mode of each object (cc45.mode_zp / cc45.mode_stack marker)
    funcObjMode_.clear();
    funcZpCalls_.clear();
    for (const auto& input : objects_) {
        int mode = -1;
        for (const auto& exp : input.obj.exports) {
            if (exp.name == "cc45.mode_zp") mode = 1;
            else if (exp.name == "cc45.mode_stack") mode = 0;
        }
        if (mode < 0) continue;
        std::set<std::string> zpCalls;   // cc45.zpcall.<callee>: __fastcall__ calls of a stack-mode object
        for (const auto& exp : input.obj.exports)
            if (exp.name.rfind("cc45.zpcall.", 0) == 0) zpCalls.insert(exp.name.substr(12));
        for (const auto& exp : input.obj.exports) {
            if (exp.segmentId() != SEG_TEXT) continue;
            funcObjMode_[exp.name] = mode;
            if (!zpCalls.empty()) funcZpCalls_[exp.name] = zpCalls;
        }
    }

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
            // cc45 exports a function's computed-goto labels (f__lbl_name,
            // for &&name in a static table): they are inside f, not new
            // functions (calls after one belong to f)
            if (exp.name.find("__lbl_") != std::string::npos) continue;
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

            // Find which function owns this call site. A SAC function's
            // X__sac label (its body past the parameter thunk) is the same
            // function as X: SAC frame overlap and thunk decisions need one node
            auto sacBase = [this](std::string n) {
                if (n.size() > 5 && n.compare(n.size() - 5, 5, "__sac") == 0 &&
                    funcAttrs_.count(n.substr(0, n.size() - 5)))
                    n.resize(n.size() - 5);
                return n;
            };
            uint32_t siteInMerged = input.textOffset + r.offset;
            for (const auto& fn : funcs) {
                if (siteInMerged >= fn.startOff && siteInMerged < fn.endOff) {
                    if (!callee.empty()) {
                        callGraph_[sacBase(fn.name)].insert(sacBase(callee));
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

// Convention a call site used, and whether it needs a bridge thunk.
// An object built by cc45 exports cc45.mode_zp or cc45.mode_stack. Its calls to
// functions of an object with the same mode already follow the callee's
// convention. Otherwise a cc45 caller used its module's mode, and a hand-written
// (unmarked) caller is assumed to use its own convention.
int O45Linker::funcMode(const std::string& fn) const {
    auto it = funcObjMode_.find(fn);
    return it == funcObjMode_.end() ? -1 : it->second;
}

bool O45Linker::callNeedsThunk(const std::string& callerName, const std::string& callee, bool& callerIsZp) const {
    // A call inside a SAC function lies past its __sac entry label
    std::string caller = callerName;
    if (!funcAttrs_.count(caller) && caller.size() > 5 && caller.compare(caller.size() - 5, 5, "__sac") == 0)
        caller = caller.substr(0, caller.size() - 5);
    auto callerIt = funcAttrs_.find(caller);
    auto calleeIt = funcAttrs_.find(callee);
    int callerMode = funcMode(caller);
    if (callerMode < 0 && callerIt == funcAttrs_.end()) callerMode = funcMode(callerName);
    // Static functions export no attributes; their object's mode marker decides
    if ((callerIt == funcAttrs_.end() && callerMode < 0) || calleeIt == funcAttrs_.end()) return false;
    bool calleeIsZp = (calleeIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;
    int calleeMode = funcMode(callee);
    // a __fastcall__ call from a stack-mode object (cc45.zpcall.<callee>)
    auto zc = funcZpCalls_.find(caller);
    if (zc == funcZpCalls_.end()) zc = funcZpCalls_.find(callerName);
    if (zc != funcZpCalls_.end() && zc->second.count(callee)) { callerIsZp = true; return !calleeIsZp; }
    if (callerMode < 0) callerIsZp = (callerIt->second.flags & FUNC_FLAG_ZP_CONV) != 0;
    else if (calleeMode == callerMode) callerIsZp = calleeIsZp;
    else callerIsZp = callerMode == 1;
    return callerIsZp != calleeIsZp;
}

// 3.3 — Emit diagnostics: enforce calling convention compatibility.
// Detect all mismatches (both directions). In THUNK_ERROR mode, report them
// as errors. In THUNK_AUTO/THUNK_WARN mode, record them for thunk generation.
void O45Linker::emitDiagnostics() {
    convErrors_.clear();

    for (const auto& [caller, callees] : callGraph_) {
        for (const auto& callee : callees) {
            bool callerIsZp = false;
            if (!callNeedsThunk(caller, callee, callerIsZp)) continue;

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

// Phase 78.4: Validate immediate relocations for SMC parameters
void O45Linker::validateImmediateRelocations() {
    // Scan through all objects and their relocations for R_IMM8/R_IMM16 types
    // Validate that referenced symbols exist and are resolvable

    int immediatRelocCount = 0;
    int validationErrors = 0;

    for (int objIdx = 0; objIdx < (int)objects_.size(); objIdx++) {
        const auto& input = objects_[objIdx];

        // Check text relocations
        auto textRelocs = O45RelocDecoder::decode(input.obj.textRelocs);
        for (const auto& r : textRelocs) {
            if (r.type != R_IMM8 && r.type != R_IMM16) continue;

            immediatRelocCount++;

            // Validate symbol resolution for external immediate relocations
            if (r.segment == SEG_EXTERNAL) {
                if (r.symbolIndex >= input.obj.imports.size()) {
                    convErrors_.push_back("error: invalid symbol index in immediate relocation in " +
                                        input.filename);
                    validationErrors++;
                    continue;
                }

                const std::string& symName = input.obj.imports[r.symbolIndex].name;
                auto symIt = globalSymbols_.find(symName);

                if (symIt == globalSymbols_.end()) {
                    convErrors_.push_back("error: undefined symbol '" + symName +
                                        "' in immediate relocation in " + input.filename);
                    validationErrors++;
                }
            }
        }

        // Check data relocations (less common for immediates, but be thorough)
        auto dataRelocs = O45RelocDecoder::decode(input.obj.dataRelocs);
        for (const auto& r : dataRelocs) {
            if (r.type != R_IMM8 && r.type != R_IMM16) continue;

            immediatRelocCount++;

            if (r.segment == SEG_EXTERNAL) {
                if (r.symbolIndex >= input.obj.imports.size()) {
                    convErrors_.push_back("error: invalid symbol index in immediate relocation in " +
                                        input.filename);
                    validationErrors++;
                    continue;
                }

                const std::string& symName = input.obj.imports[r.symbolIndex].name;
                auto symIt = globalSymbols_.find(symName);

                if (symIt == globalSymbols_.end()) {
                    convErrors_.push_back("error: undefined symbol '" + symName +
                                        "' in immediate relocation in " + input.filename);
                    validationErrors++;
                }
            }
        }
    }

    // Log diagnostics if any immediate relocations were found
    if (immediatRelocCount > 0 && warnStream_) {
        ln45info() << "Phase 78: Found " << immediatRelocCount << " immediate relocations";
        if (validationErrors > 0) {
            ln45info() << " (" << validationErrors << " validation errors)";
        }
        ln45info() << std::endl;
    }
}

void O45Linker::buildCallGraph(std::map<std::string, std::set<std::string>>& graph,
                               std::set<std::string>& addrTaken) {
    auto sacBase = [this](std::string n) {
        if (n.size() > 5 && n.compare(n.size() - 5, 5, "__sac") == 0 &&
            funcAttrs_.count(n.substr(0, n.size() - 5)))
            n.resize(n.size() - 5);
        return n;
    };

    graph = callGraph_;
    addrTaken.clear();

    for (const auto& input : objects_) {
        bool isCc45 = false;
        for (const auto& exp : input.obj.exports)
            if (exp.name == "cc45.mode_zp" || exp.name == "cc45.mode_stack") isCc45 = true;

        // text labels by offset in the object's text body
        std::map<uint32_t, std::vector<std::string>> labels;
        for (const auto& exp : input.obj.exports)
            if (exp.segmentId() == SEG_TEXT) labels[exp.offset].push_back(sacBase(exp.name));

        if (!isCc45 && labels.size() > 1) {
            for (auto it = labels.begin(); std::next(it) != labels.end(); ++it)
                for (const auto& a : it->second)
                    for (const auto& b : std::next(it)->second)
                        graph[a].insert(b);
        }

        auto owners = [&](uint32_t off) -> const std::vector<std::string>* {
            auto it = labels.upper_bound(off);
            if (it == labels.begin()) return nullptr;
            return &std::prev(it)->second;
        };

        // the function names a relocation refers to
        auto targets = [&](const O45Reloc& r, const std::vector<uint8_t>& body,
                           std::vector<std::string>& out) {
            out.clear();
            if (r.segment == SEG_EXTERNAL) {
                if (r.symbolIndex < input.obj.imports.size())
                    out.push_back(sacBase(input.obj.imports[r.symbolIndex].name));
                return;
            }
            if (r.segment != SEG_TEXT) return;
            uint32_t v;
            if (r.type == R_WORD) {
                if (r.offset + 1 >= body.size()) return;
                v = body[r.offset] | (body[r.offset + 1] << 8);
            } else if (r.type == R_HIGH) {
                if (r.offset >= body.size()) return;
                v = (body[r.offset] << 8) | r.extra;
            } else if (r.type == R_LOWX) {
                if (r.offset >= body.size()) return;
                v = (r.extra << 8) | body[r.offset];
            } else {
                return;   // R_LOW: its R_HIGH partner names the target
            }
            auto it = labels.find((v - input.obj.tbase) & 0xFFFF);
            if (it != labels.end()) out = it->second;
        };

        std::vector<std::string> tgt;
        for (const auto& r : O45RelocDecoder::decode(input.obj.textRelocs)) {
            targets(r, input.obj.textBody, tgt);
            if (tgt.empty()) continue;
            uint8_t op = (r.offset > 0 && r.offset - 1 < input.obj.textBody.size())
                         ? input.obj.textBody[r.offset - 1] : 0;
            if (r.type == R_WORD && (op == 0x20 || op == 0x4C)) {   // JSR / JMP
                if (const auto* own = owners(r.offset))
                    for (const auto& o : *own)
                        for (const auto& t : tgt) graph[o].insert(t);
            } else {
                addrTaken.insert(tgt.begin(), tgt.end());
            }
        }
        for (const auto& r : O45RelocDecoder::decode(input.obj.dataRelocs)) {
            targets(r, input.obj.dataBody, tgt);
            addrTaken.insert(tgt.begin(), tgt.end());
        }
    }

    // Calls through pointers: a function that makes them (cc45:
    // cc45.icall.<f>; any label of an object not made by cc45, or made by a
    // cc45 that did not mark them, cc45.icallinfo) can call every function
    // whose address is used
    std::set<std::string> icallers;
    for (const auto& input : objects_) {
        bool marked = false;
        for (const auto& exp : input.obj.exports)
            if (exp.name == "cc45.icallinfo") marked = true;
        for (const auto& exp : input.obj.exports) {
            if (marked && exp.name.rfind("cc45.icall.", 0) == 0) icallers.insert(exp.name.substr(11));
            else if (!marked && exp.segmentId() == SEG_TEXT) icallers.insert(sacBase(exp.name));
        }
    }
    for (const auto& c : icallers)
        for (const auto& t : addrTaken)
            if (c != t) graph[c].insert(t);
}

// Base-page blocks (cc45 .zar, size cc45.zarsize.<f>): a static-allocation
// function keeps variables (pointers) in base-page bytes of its own, which
// no callee touches, so they survive calls. The blocks are placed first-fit
// in $80-$E9 of the program's base page: two functions share bytes when
// neither reaches the other through calls. A function whose address is used
// (callable from anywhere) and everything it reaches share with nothing.
// When a block does not fit, it goes where it shares the fewest bytes with
// blocks of functions active together with it, and the function on the
// inside of each such pair (the callee; both when an address-taken function
// is involved) is reached through a wrapper that saves and restores its
// block around the call (generateZarWrappers): a warning, slower calls.
bool O45Linker::computeZarOverlay(std::string& errorMsg) {
    (void)errorMsg;   // (no longer an error: see generateZarWrappers)
    zarOffset_.clear();
    zarWrapped_.clear();
    zarWrapAt_.clear();
    zarUsed_ = zarTotal_ = 0;
    std::map<std::string, uint32_t> sizes;
    for (const auto& input : objects_)
        for (const auto& exp : input.obj.exports)
            if (exp.name.rfind("cc45.zarsize.", 0) == 0 && exp.offset > 0) {
                auto& sz = sizes[exp.name.substr(13)];
                sz = std::max(sz, (uint32_t)exp.offset);
            }
    zarSize_ = sizes;
    if (sizes.empty()) return true;

    std::map<std::string, std::set<std::string>> graph;
    std::set<std::string> addrTaken;
    buildCallGraph(graph, addrTaken);
    auto reachFrom = [&](const std::string& start) {
        std::set<std::string> seen;
        std::vector<std::string> todo{start};
        while (!todo.empty()) {
            std::string n = todo.back();
            todo.pop_back();
            auto it = graph.find(n);
            if (it == graph.end()) continue;
            for (const auto& c : it->second)
                if (seen.insert(c).second) todo.push_back(c);
        }
        return seen;
    };
    // (calls through pointers are edges of the graph: buildCallGraph)
    std::set<std::string> unsafe;
    std::map<std::string, std::set<std::string>> reach;
    for (const auto& [f, len] : sizes) reach[f] = reachFrom(f);
    auto conflict = [&](const std::string& a, const std::string& b) {
        return unsafe.count(a) || unsafe.count(b) || reach[a].count(b) || reach[b].count(a);
    };

    // Callers before callees (a function then lands just above the blocks of
    // the functions active when it runs, so the area needed is the heaviest
    // call path); largest first among unrelated ones and inside cycles
    std::vector<std::pair<std::string, uint32_t>> blocks(sizes.begin(), sizes.end());
    std::sort(blocks.begin(), blocks.end(), [](const auto& a, const auto& b) {
        return a.second != b.second ? a.second > b.second : a.first < b.first;
    });
    {
        std::vector<std::pair<std::string, uint32_t>> ordered;
        std::set<std::string> done;
        while (ordered.size() < blocks.size()) {
            bool progress = false;
            for (const auto& b : blocks) {
                if (done.count(b.first)) continue;
                bool ready = true;   // every block function that reaches it is placed
                for (const auto& o : blocks)
                    if (o.first != b.first && !done.count(o.first) && reach[o.first].count(b.first) &&
                        !reach[b.first].count(o.first))
                        { ready = false; break; }
                if (!ready) continue;
                ordered.push_back(b);
                done.insert(b.first);
                progress = true;
            }
            if (!progress)   // (a cycle through calls via pointers: take the next as it comes)
                for (const auto& b : blocks)
                    if (!done.count(b.first)) { ordered.push_back(b); done.insert(b.first); break; }
        }
        blocks = ordered;
    }
    const uint32_t area = ZAR_HI - ZAR_LO - zpVarTotal_;   // (the __zp variables are above)
    for (const auto& [f, len] : blocks) {
        std::vector<std::pair<uint32_t, uint32_t>> busy;
        for (const auto& [g, at] : zarOffset_)
            if (conflict(f, g)) busy.push_back({at - ZAR_LO, at - ZAR_LO + sizes[g]});
        std::sort(busy.begin(), busy.end());
        uint32_t at = 0;
        for (const auto& [lo, hi] : busy) {
            if (at + len <= lo) break;
            at = std::max(at, hi);
        }
        if (at + len > area) {
            // fewest bytes shared with conflicting blocks
            uint32_t best = 0, bestShared = UINT32_MAX;
            for (uint32_t pos = 0; pos + len <= area; pos++) {
                uint32_t shared = 0;
                for (const auto& [lo, hi] : busy)
                    shared += std::max<int64_t>(0, (int64_t)std::min(hi, pos + len) - (int64_t)std::max(lo, pos));
                if (shared < bestShared) { bestShared = shared; best = pos; }
            }
            at = best;
            std::string wrapped;
            for (const auto& [g, gat] : zarOffset_) {
                uint32_t glo = gat - ZAR_LO, ghi = glo + sizes[g];
                if (!conflict(f, g) || ghi <= at || glo >= at + len) continue;
                bool fInside = reach[g].count(f) || unsafe.count(f) || unsafe.count(g);
                bool gInside = reach[f].count(g) || unsafe.count(f) || unsafe.count(g);
                if (fInside && zarWrapped_.insert(f).second) wrapped += (wrapped.empty() ? "" : ", ") + f;
                if (gInside && zarWrapped_.insert(g).second) wrapped += (wrapped.empty() ? "" : ", ") + g;
            }
            if (warnStream_)
                *warnStream_ << "ln45: warning: base-page storage: " << f << " needs " << len
                             << " bytes of $80-$E9 that functions active together with it use; calls to "
                             << (wrapped.empty() ? f : wrapped)
                             << " save and restore the bytes they share (compile with cc45 -fno-zp-locals to avoid this)"
                             << std::endl;
        }
        zarOffset_[f] = ZAR_LO + at;
        zarUsed_ = std::max(zarUsed_, at + len);
        zarTotal_ += len;
        ln45debug() << "base-page block: " << f << " at $" << std::hex << (ZAR_LO + at) << std::dec
                    << " len " << len << "\n";
    }
    ln45info() << "base-page blocks: " << blocks.size() << " functions, " << zarTotal_ << " bytes in "
               << zarUsed_ << "\n";
    return true;
}

// __zp variables (cc45 cc45.zpvar.<sym> = size, one definition each): at
// the top of $80-$E9 in name order; the base-page blocks share what is left
bool O45Linker::placeZpVars(std::string& errorMsg) {
    zpVarAddr_.clear();
    zpVarSize_.clear();
    zpVarTotal_ = 0;
    std::map<std::string, std::string> definedIn;
    for (const auto& input : objects_)
        for (const auto& exp : input.obj.exports) {
            if (exp.name.rfind("cc45.zpvar.", 0) != 0) continue;
            std::string sym = exp.name.substr(11);
            if (definedIn.count(sym)) {
                errorMsg = "__zp variable '" + sym + "' is defined in " + definedIn[sym] + " and " + input.filename;
                return false;
            }
            definedIn[sym] = input.filename;
            zpVarSize_[sym] = exp.offset;
            zpVarTotal_ += exp.offset;
        }
    if (zpVarTotal_ == 0) return true;
    if (zpVarTotal_ > ZAR_HI - ZAR_LO) {
        errorMsg = "__zp variables need " + std::to_string(zpVarTotal_) + " bytes; the program's base page has " +
                   std::to_string(ZAR_HI - ZAR_LO) + " ($80-$E9)";
        return false;
    }
    uint32_t at = ZAR_HI - zpVarTotal_;
    for (const auto& [sym, size] : zpVarSize_) {
        zpVarAddr_[sym] = at;
        ln45debug() << "__zp variable: " << sym << " at $" << std::hex << at << std::dec << " len " << size << "\n";
        at += size;
    }
    ln45info() << "__zp variables: " << zpVarAddr_.size() << ", " << zpVarTotal_ << " bytes\n";
    return true;
}

// An __interrupt handler runs with its own base page (__zp_page_isr): a
// base-page operand of a __zp variable (R_LOWX against it) in code a handler
// reaches would use that page's byte. Absolute references are fine.
void O45Linker::checkZpVarsInIsr() {
    if (zpVarAddr_.empty()) return;
    std::set<std::string> isrFuncs;
    for (const auto& [name, attrs] : funcAttrs_)
        if (attrs.flags & FUNC_FLAG_ISR) isrFuncs.insert(name);
    if (isrFuncs.empty()) return;
    std::map<std::string, std::string> reachedBy;   // function -> a handler reaching it
    for (const auto& isr : isrFuncs) {
        std::vector<std::string> todo{isr};
        while (!todo.empty()) {
            std::string f = todo.back();
            todo.pop_back();
            if (!reachedBy.emplace(f, isr).second) continue;
            auto it = callGraph_.find(f);
            if (it != callGraph_.end()) for (const auto& c : it->second) todo.push_back(c);
        }
    }
    std::set<std::string> reported;
    for (const auto& input : objects_) {
        std::vector<std::pair<uint32_t, std::string>> funcs;   // text offset, function
        for (const auto& exp : input.obj.exports)
            if (exp.segmentId() == SEG_TEXT && exp.name.find("__lbl_") == std::string::npos)
                funcs.push_back({exp.offset, exp.name});
        std::sort(funcs.begin(), funcs.end());
        for (const auto& r : O45RelocDecoder::decode(input.obj.textRelocs)) {
            if (r.type != R_LOWX || r.segment != SEG_EXTERNAL || r.symbolIndex >= input.obj.imports.size()) continue;
            const std::string& var = input.obj.imports[r.symbolIndex].name;
            if (!zpVarAddr_.count(var)) continue;
            std::string f;
            for (const auto& [off, name] : funcs) if (off <= r.offset) f = name;
            if (f.size() > 5 && f.compare(f.size() - 5, 5, "__sac") == 0) f.resize(f.size() - 5);
            auto it = reachedBy.find(f);
            if (it == reachedBy.end() || !reported.insert(f + " " + var).second) continue;
            convErrors_.push_back("error: '" + f + "' uses the __zp variable '" + var + "' and is reachable from "
                                  "interrupt handler '" + it->second + "', which runs on its own base page "
                                  "(share data with interrupt handlers in plain globals)");
        }
    }
}

// __zp_init: the initial values of __zp variables (cc45.zpinit.<sym>, in
// data) copied into the base page, one loop per variable:
//   LDX #n-1 / LDA src,X / STA dst,X / DEX / BPL (n <= 106) ... RTS
// The addresses are written by patchZpInit, once data and BSS are final.
void O45Linker::generateZpInit() {
    zpInitAt_ = 0;
    zpInitPatches_.clear();
    std::vector<std::string> inits;
    for (const auto& [sym, size] : zpVarSize_)
        if (size > 0 && globalSymbols_.count("cc45.zpinit." + sym)) inits.push_back(sym);
    if (inits.empty()) return;
    auto& t = mergedText_;
    zpInitAt_ = textBase_ + (uint32_t)t.size();
    for (const auto& sym : inits) {
        t.push_back(0xA2); t.push_back((uint8_t)(zpVarSize_[sym] - 1));   // LDX #n-1
        t.push_back(0xBD); zpInitPatches_.push_back({(uint32_t)t.size(), "cc45.zpinit." + sym});
        t.push_back(0); t.push_back(0);                                   // LDA src,X
        t.push_back(0x9D); zpInitPatches_.push_back({(uint32_t)t.size(), sym});
        t.push_back(0); t.push_back(0);                                   // STA dst,X
        t.push_back(0xCA);                                                // DEX
        t.push_back(0x10); t.push_back(0xF7);                             // BPL (to the LDA)
    }
    t.push_back(0x60);                                                    // RTS
    ln45debug() << "__zp_init at $" << std::hex << zpInitAt_ << std::dec << "\n";
}

void O45Linker::patchZpInit() {
    if (!zpInitAt_) return;
    globalSymbols_["__zp_init"] = zpInitAt_;   // (replaces crt0's weak RTS)
    symbolSource_["__zp_init"] = "<linker>";
    symbolWeak_["__zp_init"] = false;
    for (const auto& [off, sym] : zpInitPatches_) {
        uint32_t v = globalSymbols_.count(sym) ? globalSymbols_[sym] : 0;
        mergedText_[off] = v & 0xFF;
        mergedText_[off + 1] = (v >> 8) & 0xFF;
    }
}

// The wrapper of a function whose base-page block is shared with a function
// active together with it: save the block's bytes, take the caller's return
// address off the stack (stack-convention arguments are then where the
// function expects them), call it, keep A/X/Y/Z (the result), put the
// return address back, restore the bytes. One per entry (<f>, <f>__sac),
// with its save area after its code; a SAC function is not reentrant.
// applyRelocations sends every reference to the entry (calls, addresses
// in code and data) to the wrapper.
void O45Linker::generateZarWrappers() {
    zarWrapAt_.clear();
    for (const auto& f : zarWrapped_) {
        uint32_t base = zarOffset_[f], len = zarSize_[f];
        for (const std::string& entry : {f, f + "__sac"}) {
            auto it = globalSymbols_.find(entry);
            if (it == globalSymbols_.end()) continue;
            uint32_t wrap = textBase_ + (uint32_t)mergedText_.size();
            // (a function of a code overlay: reached through its stub when it
            // has one, its address in the region from inside the overlay)
            int fOvl = symOvl_.count(entry) ? symOvl_[entry] : 0;
            bool stubbed = ovlStubs_.count(entry) > 0;
            // code size: 5L (save) + 8 + 3 + 12 + 8 + 5L (restore) + 12 + 1
            uint32_t save = wrap + 10 * len + 44;
            uint32_t ret = save + len, regs = ret + 2;
            auto& t = mergedText_;
            auto abs = [&](uint8_t op, uint32_t a) { t.push_back(op); t.push_back(a & 0xFF); t.push_back((a >> 8) & 0xFF); };
            for (uint32_t k = 0; k < len; k++) { t.push_back(0xA5); t.push_back((uint8_t)(base + k)); abs(0x8D, save + k); }
            t.push_back(0x68); abs(0x8D, ret);           // PLA; STA ret      (return address, low first)
            t.push_back(0x68); abs(0x8D, ret + 1);       // PLA; STA ret+1
            abs(0x20, it->second);                       // JSR entry
            abs(0x8D, regs); abs(0x8E, regs + 1);        // STA/STX/STY/STZ regs
            abs(0x8C, regs + 2); abs(0x9C, regs + 3);
            abs(0xAD, ret + 1); t.push_back(0x48);       // LDA ret+1; PHA
            abs(0xAD, ret); t.push_back(0x48);           // LDA ret; PHA
            for (uint32_t k = 0; k < len; k++) { abs(0xAD, save + k); t.push_back(0x85); t.push_back((uint8_t)(base + k)); }
            abs(0xAB, regs + 3); abs(0xAC, regs + 2);    // LDZ/LDY/LDX/LDA regs
            abs(0xAE, regs + 1); abs(0xAD, regs);
            t.push_back(0x60);                           // RTS
            if (textBase_ + t.size() != save) {
                ln45debug() << "base-page wrapper size mismatch for " << entry << "\n";
            }
            for (uint32_t k = 0; k < len + 6; k++) t.push_back(0);
            if (fOvl) {
                zarWrapAt_[zarKey(fOvl, realAddr_[entry])] = wrap;
                if (stubbed) zarWrapAt_[zarKey(0, it->second)] = wrap;
            } else {
                zarWrapAt_[zarKey(0, it->second)] = wrap;
            }
            ln45debug() << "base-page wrapper for " << entry << " at $" << std::hex << wrap << std::dec << "\n";
        }
    }
}

// =============================================================================
// Code overlays
// =============================================================================
//
// An object exporting cc45.overlay.<name> (cc45 #pragma cc45 overlay(name),
// -foverlay=name, ca45 .overlay) has its code in overlay <name>: linked to
// run in one region of bank 0 shared by all overlays (__ovl_region, below
// the stack's share; the heap ends there), kept in far memory (from
// --overlay-store, default $50000: bank 1 is BASIC's and the ROM's) and
// copied into the region by DMA when
// a call needs it (lib/stdlib_asm/ovl.s45). Its data and BSS stay resident.
// A reference to an overlay's function from anywhere but a JSR/JMP in the
// same overlay (a call from other code, an address taken, a pointer in
// data) goes to a resident stub, which loads the overlay and calls through
// __ovl_call. The images go to a file of their own (overlayFile()), which
// the program loads at start (__ovl_init).

uint32_t O45Linker::textBaseOf(const InputObject& in) const {
    return in.ovl ? ovlBase_ : textBase_;
}

void O45Linker::assignOverlays() {
    ovlNames_.clear();
    for (auto& input : objects_) {
        input.ovl = 0;
        for (const auto& exp : input.obj.exports) {
            if (exp.name.rfind("cc45.overlay.", 0) != 0) continue;
            std::string name = exp.name.substr(13);
            auto it = std::find(ovlNames_.begin(), ovlNames_.end(), name);
            if (it == ovlNames_.end()) { ovlNames_.push_back(name); it = ovlNames_.end() - 1; }
            input.ovl = (int)(it - ovlNames_.begin()) + 1;
        }
    }
    ovlUserCount_ = ovlNames_.size();
    // __far functions (cc45): a "far" text sub-segment, packed into
    // automatic mapped overlays when the text is laid out
    hasFar_ = false;
    for (const auto& input : objects_)
        for (const auto& sa : input.obj.segAttrs)
            if (sa.segId == SEG_TEXT && sa.name == "far" && sa.length) hasFar_ = true;
    if (hasFar_) ovlMap_ = true;
    ovlText_.assign(ovlNames_.size(), {});
}

uint32_t O45Linker::textAddrOf(const InputObject& in, uint32_t origOff) const {
    return (in.ovlAt(origOff) ? ovlBase_ : textBase_) + in.remapTextOffset(origOff);
}

// The region: the largest overlay, page-aligned, right below the stack's
// share (__stack_size below __stack_top; $C000 with the 8-bit stack)
bool O45Linker::placeOverlayRegion(std::string& errorMsg) {
    if (ovlNames_.empty()) return true;
    ovlSize_ = 0;
    for (const auto& t : ovlText_) ovlSize_ = std::max(ovlSize_, (uint32_t)t.size());
    uint32_t top = 0xBFFF, size = stackSize_;
    bool topStrong = false;
    for (const auto& input : objects_)
        for (const auto& exp : input.obj.exports) {
            if (exp.segmentId() != SEG_ABSOLUTE) continue;
            if (exp.name == "__stack_top" && (!topStrong || !exp.isWeak())) {
                top = exp.offset; topStrong = !exp.isWeak();
            }
            if (exp.name == "__stack_size") size = exp.offset;
        }
    uint32_t end = top ? (top + 1 > size ? top + 1 - size : 0) : 0xC000;
    if (ovlMap_) {
        // whole 8 KB blocks of the map's lower half ending at $8000: $6000
        // or $4000 (the upper half keeps the KERNAL mapped at $E000, and
        // blocks 0-1 hold the zero page, the stack page, the screen and the
        // program's start); the heap goes above it
        uint32_t winSize = (ovlSize_ + 0x1FFF) & ~0x1FFFu;
        if (winSize > 0x4000) {
            errorMsg = "code overlays: mapped overlays run in $4000-$7FFF: the largest (" +
                       std::to_string(ovlSize_) + " bytes) is over 16 KB";
            return false;
        }
        if (end < 0x8000) {
            errorMsg = "code overlays: mapped overlays need RAM to $7FFF below the stack's share";
            return false;
        }
        ovlBase_ = 0x8000 - winSize;
        return true;
    }
    if (end < ovlSize_ + textBase_) {
        errorMsg = "code overlays: the largest overlay (" + std::to_string(ovlSize_) +
                   " bytes) does not fit below the stack";
        return false;
    }
    ovlBase_ = (end - ovlSize_) & ~0xFFu;
    return true;
}

bool O45Linker::generateOverlayStubs(std::string& errorMsg) {
    ovlStubs_.clear();
    if (ovlNames_.empty()) return true;
    for (const auto& input : objects_)
        for (const auto& exp : input.obj.exports)
            if (exp.segmentId() == SEG_TEXT && exp.hasFuncAttr && (exp.funcAttr.flags & FUNC_FLAG_ISR) &&
                input.ovlAt(exp.offset)) {
                errorMsg = "interrupt handler '" + exp.name + "' is in code overlay '" +
                           ovlNames_[input.ovlAt(exp.offset) - 1] + "' (it must stay resident)";
                return false;
            }
    // Nor may an interrupt handler reach an overlay's function, through any
    // chain of calls (calls through pointers included): the overlay runtime
    // keeps its state in single variables (__ovl_sa, __ovl_want, __ovl_tgt,
    // the overlay return stack) and an interrupt could arrive in the middle
    // of a call of the main program
    {
        std::map<std::string, std::set<std::string>> graph;
        std::set<std::string> addrTaken;
        buildCallGraph(graph, addrTaken);
        auto ovlOf = [&](const std::string& n) {
            auto o = symOvl_.find(n);
            return o == symOvl_.end() ? 0 : o->second;
        };
        for (const auto& [isr, attrs] : funcAttrs_) {
            if (!(attrs.flags & FUNC_FLAG_ISR)) continue;
            std::map<std::string, std::string> from;   // breadth first: the shortest chain
            std::vector<std::string> queue = {isr};
            from[isr] = "";
            for (size_t q = 0; q < queue.size(); q++) {
                const std::string cur = queue[q];
                if (cur != isr && ovlOf(cur)) {
                    std::string chain = cur;
                    for (std::string p = from[cur]; !p.empty(); p = from[p]) chain = p + " -> " + chain;
                    errorMsg = "'" + cur + "' (code overlay " + ovlNames_[ovlOf(cur) - 1] +
                               ") is reachable from interrupt handler '" + isr + "' (" + chain +
                               "): the overlay runtime is not reentrant; keep what an interrupt handler calls resident";
                    return false;
                }
                auto g = graph.find(cur);
                if (g == graph.end()) continue;
                for (const auto& next : g->second)
                    if (!from.count(next)) { from[next] = cur; queue.push_back(next); }
            }
        }
        // The address of __noescape data passed to a function that can call
        // into another overlay (cc45 cc45.nesc.<caller>.<callee> = the line;
        // "icall": through a pointer): while that overlay runs, the data is
        // not in the region (Issue #301). A warning: the call may not reach
        // it, or not while the pointer is used.
        if (warnStream_)
            for (const auto& input : objects_)
                for (const auto& exp : input.obj.exports) {
                    if (exp.name.rfind("cc45.nesc.", 0) != 0) continue;
                    std::string rest = exp.name.substr(10);
                    size_t dot = rest.find('.');
                    if (dot == std::string::npos) continue;
                    std::string caller = rest.substr(0, dot), callee = rest.substr(dot + 1);
                    int home = ovlOf(caller);
                    if (!home) continue;
                    std::map<std::string, std::string> from;
                    std::vector<std::string> queue;
                    if (callee == "icall") { for (const auto& t : addrTaken) { from[t] = ""; queue.push_back(t); } }
                    else { from[callee] = ""; queue.push_back(callee); }
                    for (size_t q = 0; q < queue.size(); q++) {
                        const std::string cur = queue[q];
                        int o = ovlOf(cur);
                        if (o && o != home) {
                            std::string chain = cur;
                            for (std::string pr = from[cur]; !pr.empty(); pr = from[pr]) chain = pr + " -> " + chain;
                            *warnStream_ << "ln45: warning: " << input.filename << ", line " << exp.offset << ": '" << caller
                                         << "' (code overlay " << ovlNames_[home - 1] << ") passes the address of "
                                         << "__noescape data to " << (callee == "icall" ? std::string("a call through a pointer")
                                                                                         : "'" + callee + "'")
                                         << ", which can call into code overlay " << ovlNames_[o - 1] << " (" << chain
                                         << "): the data is not in the region while that runs (__escape keeps a literal resident)"
                                         << std::endl;
                            break;
                        }
                        auto g = graph.find(cur);
                        if (g == graph.end()) continue;
                        for (const auto& next : g->second)
                            if (!from.count(next)) { from[next] = cur; queue.push_back(next); }
                    }
                }
    }
    auto isCall = [](const std::vector<uint8_t>& body, uint32_t off, uint8_t type) {
        return type == R_WORD && off > 0 && off - 1 < body.size() &&
               (body[off - 1] == 0x20 || body[off - 1] == 0x4C);   // JSR, JMP
    };
    // the export of an overlay object at an offset of its text
    auto exportAt = [](const InputObject& in, uint32_t off) -> std::string {
        for (const auto& exp : in.obj.exports)
            if (exp.segmentId() == SEG_TEXT && exp.offset == off && exp.name.find("__lbl_") == std::string::npos)
                return exp.name;
        return "";
    };
    // __noescape data kept in an overlay's code (cc45 cc45.noescape.<sym>):
    // no stub; a reference from outside its overlay is an error
    std::set<std::string> noescapeData;
    for (const auto& input : objects_)
        for (const auto& exp : input.obj.exports)
            if (exp.name.rfind("cc45.noescape.", 0) == 0) noescapeData.insert(exp.name.substr(14));
    auto outsideRef = [&](const std::string& n, int ovl, const InputObject& from) {
        errorMsg = "'" + n + "' is __noescape data kept in code overlay " + ovlNames_[ovl - 1] + ", but " +
                   from.filename + " refers to it from outside that overlay (its address must not leave it)";
        return false;
    };
    std::set<std::string> need;
    for (const auto& input : objects_) {
        for (int pass = 0; pass < 2; pass++) {
            bool inText = pass == 0;
            const auto& body = inText ? input.obj.textBody : input.obj.dataBody;
            for (const auto& r : O45RelocDecoder::decode(inText ? input.obj.textRelocs : input.obj.dataRelocs)) {
                if (r.segment == SEG_EXTERNAL) {
                    if (r.symbolIndex >= input.obj.imports.size()) continue;
                    const std::string& n = input.obj.imports[r.symbolIndex].name;
                    auto o = symOvl_.find(n);
                    if (o == symOvl_.end() || o->second == 0) continue;
                    if (noescapeData.count(n)) {
                        if (!inText || input.ovlAt(r.offset) != o->second) return outsideRef(n, o->second, input);
                        continue;
                    }
                    if (!inText || input.ovlAt(r.offset) != o->second || !isCall(body, r.offset, r.type)) need.insert(n);
                } else if (r.segment == SEG_TEXT) {
                    uint32_t off = storedValue(r, body) - segOrigBase(input.obj, SEG_TEXT);
                    int tOvl = input.ovlAt(off);
                    if (!tOvl) continue;
                    if (inText && isCall(body, r.offset, r.type) && input.ovlAt(r.offset) == tOvl) continue;
                    std::string n = exportAt(input, off);
                    if (!n.empty() && noescapeData.count(n)) {
                        if (!inText || input.ovlAt(r.offset) != tOvl) return outsideRef(n, tOvl, input);
                        continue;
                    }
                    if (!n.empty()) need.insert(n);
                }
            }
        }
    }
    auto sym = [&](const char* n) -> uint32_t {
        auto it = globalSymbols_.find(n);
        return it == globalSymbols_.end() ? 0 : it->second;
    };
    uint32_t sa = sym("__ovl_sa"), want = sym("__ovl_want"), tgt = sym("__ovl_tgt"), call = sym("__ovl_call");
    if (!call || !sa) {
        errorMsg = "code overlays: the overlay runtime (__ovl_call, lib/stdlib_asm/ovl.s45) is not linked";
        return false;
    }
    auto& t = mergedText_;
    auto abs = [&](uint8_t op, uint32_t a) { t.push_back(op); t.push_back(a & 0xFF); t.push_back((a >> 8) & 0xFF); };
    for (const auto& n : need) {
        auto ra = realAddr_.find(n);
        if (ra == realAddr_.end()) continue;
        ovlStubs_[n] = textBase_ + (uint32_t)t.size();
        abs(0x8D, sa);                                        // STA __ovl_sa
        t.push_back(0xA9); t.push_back((uint8_t)symOvl_[n]);  // LDA #id
        abs(0x8D, want);                                      // STA __ovl_want
        t.push_back(0xA9); t.push_back(ra->second & 0xFF);    // LDA #<f
        abs(0x8D, tgt);                                       // STA __ovl_tgt
        t.push_back(0xA9); t.push_back((ra->second >> 8) & 0xFF);
        abs(0x8D, tgt + 1);
        abs(0x4C, call);                                      // JMP __ovl_call
        globalSymbols_[n] = ovlStubs_[n];
    }
    // __ovl_table: per overlay its far address (24 bits, then bits 20-27)
    // and length, or (mapped) the MAP registers that map the window to it;
    // __ovl_file: the first file's bank, the number of files, the name
    globalSymbols_["__ovl_table"] = textBase_ + (uint32_t)t.size();
    // each overlay in the first range it fits in (first fit; never across a
    // bank): a file per range up to the last one used
    std::vector<uint32_t> cur;
    for (const auto& r : ovlStores_) cur.push_back(r.first);
    ovlAt_.clear();
    ovlFileOf_.clear();
    uint8_t loMask = 0, hiMask = 0;   // the window's blocks
    for (uint32_t b = ovlBase_ >> 13; ovlMap_ && b < ((ovlBase_ + ovlSize_ + 0x1FFF) >> 13); b++)
        (b < 4 ? loMask : hiMask) |= (uint8_t)(1 << (b & 3));
    for (size_t i = 0; i < ovlText_.size(); i++) {
        const auto& img = ovlText_[i];
        size_t r = 0;
        uint32_t at = 0;
        for (; r < cur.size(); r++) {
            at = ovlMap_ ? (cur[r] + 0xFF) & ~0xFFu : cur[r];   // (MAP offsets are whole pages)
            if (at + img.size() <= (uint64_t)ovlStores_[r].second + 1) break;
        }
        if (r == cur.size()) {
            uint32_t room = 0;
            for (size_t k = 0; k < cur.size(); k++) room += ovlStores_[k].second + 1 - cur[k];
            errorMsg = "code overlays: overlay " + ovlNames_[i] + " (" + std::to_string(img.size()) +
                       " bytes) does not fit in the overlay store (" + std::to_string(room) + " bytes left in " +
                       std::to_string(cur.size()) + (cur.size() == 1 ? " bank" : " banks") + ")";
            if (cur.size() == 1 && (ovlStores_[0].first >> 16) < 5) {
                char b[160];
                snprintf(b, sizeof(b), "; the next bank as a second range: --overlay-store=0x%X-0x%X,0x%X0000-0x%XFFFF",
                         ovlStores_[0].first, ovlStores_[0].second, (ovlStores_[0].first >> 16) + 1, (ovlStores_[0].first >> 16) + 1);
                errorMsg += b;
            }
            return false;
        }
        cur[r] = at + (uint32_t)img.size();
        ovlAt_.push_back(at);
        ovlFileOf_.push_back((int)r);
        if (ovlMap_) {
            uint32_t off = at - ovlBase_;   // CPU address + offset = where it is kept
            // (the lower half of MAP; the upper half stays the ROM's)
            t.push_back((off >> 8) & 0xFF); t.push_back(((off >> 16) & 0x0F) | (loMask << 4));
            t.push_back(0); t.push_back(0); t.push_back(0); t.push_back(0);
        } else {
            t.push_back(at & 0xFF); t.push_back((at >> 8) & 0xFF); t.push_back((at >> 16) & 0xFF);
            t.push_back((at >> 20) & 0xFF);
            t.push_back(img.size() & 0xFF); t.push_back((img.size() >> 8) & 0xFF);
        }
    }
    // (the description is a file of its own, never loaded: only the code
    // counts)
    ovlStoreEnd_ = cur;
    size_t nfiles = 1;
    for (int r : ovlFileOf_) nfiles = std::max(nfiles, (size_t)r + 1);
    std::string name = ovlFileNameOf(0);
    if (nfiles > 1 && (name.back() < '0' || name.back() + (int)nfiles - 1 > '9')) {
        errorMsg = "code overlays: " + std::to_string(nfiles) + " overlay files: the name ('" + name +
                   "', --overlay-name) must end in a digit the others count up from (" + name.substr(0, name.size() - 1) + "1)";
        return false;
    }
    // __ovl_file: the first file's bank, the number of files, the name
    globalSymbols_["__ovl_file"] = textBase_ + (uint32_t)t.size();
    t.push_back((ovlStores_[0].first >> 16) & 0xFF); t.push_back((uint8_t)nfiles);
    t.push_back((uint8_t)name.size());
    for (char c : name) t.push_back((uint8_t)c);
    for (const auto* s2 : {"__ovl_table", "__ovl_file"}) { symbolSource_[s2] = "<linker>"; symbolWeak_[s2] = false; }
    ovlTableAddr_ = globalSymbols_["__ovl_table"];
    ovlFileAddr_ = globalSymbols_["__ovl_file"];
    return true;
}

void O45Linker::buildOverlayFile() {
    ovlFiles_.clear();
    ovlDesc_.clear();
    if (ovlNames_.empty()) return;
    // a file per range up to the last one used: its start's low 16 bits,
    // then its overlays (in address order: first fit appends)
    size_t nfiles = 1;
    for (int r : ovlFileOf_) nfiles = std::max(nfiles, (size_t)r + 1);
    ovlFiles_.resize(nfiles);
    for (size_t r = 0; r < nfiles; r++) {
        ovlFiles_[r].push_back(ovlStores_[r].first & 0xFF);
        ovlFiles_[r].push_back((ovlStores_[r].first >> 8) & 0xFF);
    }
    for (size_t i = 0; i < ovlText_.size(); i++) {
        auto& f = ovlFiles_[ovlFileOf_[i]];
        uint32_t start = ovlStores_[ovlFileOf_[i]].first;
        // (mapped overlays start on a page: zeros between them)
        while (start + f.size() - 2 < ovlAt_[i]) f.push_back(0);
        f.insert(f.end(), ovlText_[i].begin(), ovlText_[i].end());
    }
    // the description, for nm45 and objdump45: the overlays and their
    // symbols at the addresses they run at
    O45OverlayFile d;
    d.region = ovlBase_;
    d.mapped = ovlMap_;
    for (size_t i = 0; i < ovlText_.size(); i++)
        d.overlays.push_back({ovlNames_[i], ovlAt_[i], (uint32_t)ovlText_[i].size(), ovlFileOf_[i] + 1,
                              2 + (ovlAt_[i] - ovlStores_[ovlFileOf_[i]].first)});
    for (const auto& [name, k] : symOvl_) {
        if (!k || name.rfind("cc45.", 0) == 0) continue;
        auto a = realAddr_.find(name);
        if (a != realAddr_.end()) d.symbols.push_back({k, a->second, name});
    }
    std::sort(d.symbols.begin(), d.symbols.end(), [](const auto& a, const auto& b) {
        return a.overlay != b.overlay ? a.overlay < b.overlay : a.address != b.address ? a.address < b.address : a.name < b.name;
    });
    ovlDesc_ = d.describe();
}

// SAC overlay. A static-allocation (SAC) function keeps its parameters and
// locals in a BSS block (cc45: <f>__ar, size cc45.sacsize.<f>). Two SAC
// functions can share storage when neither can be active while the other
// is: neither reaches the other through calls. The blocks of such
// functions are placed first-fit in one overlay area at the end of BSS.
//
// Calls are JSR and JMP references to a function. Any other reference
// (an address loaded or stored in data) makes the function callable
// through a pointer: from every function that calls through pointers
// (buildCallGraph adds those edges). In objects not made by cc45 (assembly), exported labels are
// not function boundaries: a label's code can fall through to or branch to
// another, so all labels of such an object are treated as one function.
void O45Linker::computeSacOverlay() {
    sacMoves_.clear();
    sacOverlayLen_ = 0;
    sacOverlaySaved_ = 0;
    if (!sacOverlayOn_) return;

    bool anySac = false;
    for (const auto& [name, attr] : funcAttrs_)
        if (attr.flags & FUNC_FLAG_STATIC_ALLOC) anySac = true;
    if (!anySac) return;

    std::map<std::string, std::set<std::string>> graph;
    std::set<std::string> addrTaken;
    buildCallGraph(graph, addrTaken);

    auto reachFrom = [&](const std::string& start) {
        std::set<std::string> seen;
        std::vector<std::string> todo{start};
        while (!todo.empty()) {
            std::string n = todo.back();
            todo.pop_back();
            auto it = graph.find(n);
            if (it == graph.end()) continue;
            for (const auto& c : it->second)
                if (seen.insert(c).second) todo.push_back(c);
        }
        return seen;
    };

    // (calls through pointers are edges of the graph: buildCallGraph)
    std::set<std::string> unsafe;

    // the blocks that can move: the definition the link uses, in an object
    // without aligned BSS
    struct Block { std::string func; int obj; uint32_t off, len; };
    std::vector<Block> blocks;
    for (int idx = 0; idx < (int)objects_.size(); idx++) {
        const auto& input = objects_[idx];
        if (objAlign(input.obj, "cc45.align.bss") > 1) continue;
        // a plain R_LOW into BSS has only the low byte of its target: the
        // linker cannot tell which variable it is (older objects; ca45
        // writes R_LOWX), so nothing of this object's BSS moves
        bool lowOnly = false;
        for (const auto* tbl : {&input.obj.textRelocs, &input.obj.dataRelocs})
            for (const auto& r : O45RelocDecoder::decode(*tbl))
                if (r.type == R_LOW && r.segment == SEG_BSS) lowOnly = true;
        if (lowOnly) continue;
        std::map<std::string, uint32_t> sizes;
        for (const auto& exp : input.obj.exports)
            if (exp.name.rfind("cc45.sacsize.", 0) == 0) sizes[exp.name.substr(13)] = exp.offset;
        for (const auto& exp : input.obj.exports) {
            if (exp.segmentId() != SEG_BSS) continue;
            if (exp.name.size() <= 4 || exp.name.compare(exp.name.size() - 4, 4, "__ar") != 0) continue;
            std::string fn = exp.name.substr(0, exp.name.size() - 4);
            auto sz = sizes.find(fn);
            if (sz == sizes.end() || sz->second == 0) continue;
            auto fa = funcAttrs_.find(fn);
            if (fa == funcAttrs_.end() || !(fa->second.flags & FUNC_FLAG_STATIC_ALLOC)) continue;
            if (unsafe.count(fn)) continue;
            if (symbolSource_[fn] != input.filename) continue;
            // this object's definition is the one in use: resident, at its
            // place in the merged text; or in a code overlay (its address is
            // in the overlay region, later its stub), from this object (as
            // symbolSource_ says). Its block is resident BSS either way, and
            // calls through the stubs are calls in the graph (Issue #302)
            bool defined = false;
            for (const auto& e2 : input.obj.exports)
                if (e2.name == fn && e2.segmentId() == SEG_TEXT &&
                    (globalSymbols_[fn] == textBase_ + input.remapTextOffset(e2.offset) || input.ovlAt(e2.offset)))
                    defined = true;
            if (!defined) continue;
            if (exp.offset + sz->second > input.obj.blen) continue;
            blocks.push_back({fn, idx, exp.offset, sz->second});
        }
    }
    for (const auto& f : addrTaken) ln45debug() << "SAC overlay: address taken " << f << "\n";
    for (const auto& f : unsafe) ln45debug() << "SAC overlay: unsafe " << f << "\n";
    for (const auto& b : blocks) ln45debug() << "SAC overlay: block " << b.func << " " << b.len << "\n";
    if (blocks.size() < 2) return;

    std::map<std::string, std::set<std::string>> reach;
    for (const auto& b : blocks) reach[b.func] = reachFrom(b.func);
    auto conflict = [&](const std::string& a, const std::string& b) {
        return reach[a].count(b) || reach[b].count(a);
    };

    // first fit, largest blocks first
    std::sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) {
        return a.len != b.len ? a.len > b.len : a.func < b.func;
    });
    struct Placed { const Block* b; uint32_t at; };
    std::vector<Placed> placed;
    uint32_t total = 0;
    for (const auto& b : blocks) {
        std::vector<std::pair<uint32_t, uint32_t>> busy;
        for (const auto& p : placed)
            if (conflict(b.func, p.b->func)) busy.push_back({p.at, p.at + p.b->len});
        std::sort(busy.begin(), busy.end());
        uint32_t at = 0;
        for (const auto& [lo, hi] : busy) {
            if (at + b.len <= lo) break;
            at = std::max(at, hi);
        }
        placed.push_back({&b, at});
        sacOverlayLen_ = std::max(sacOverlayLen_, at + b.len);
        total += b.len;
    }
    if (sacOverlayLen_ >= total) { sacOverlayLen_ = 0; return; }   // nothing shared

    sacOverlaySaved_ = total - sacOverlayLen_;
    for (const auto& p : placed) {
        sacMoves_[p.b->obj].push_back({p.b->off, p.b->len, p.at, p.b->func});
        ln45debug() << "SAC overlay: place " << p.b->func << " at " << p.at << " len " << p.b->len << "\n";
    }
    ln45info() << "SAC overlay: " << placed.size() << " functions, " << total
               << " bytes of storage in " << sacOverlayLen_ << " (" << sacOverlaySaved_
               << " saved)\n";
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
        for (const auto& callee : callees) {
            bool callerIsZp = false;
            if (!callNeedsThunk(caller, callee, callerIsZp)) continue;

            std::string prefix = callerIsZp ? "__thunk_z2s_" : "__thunk_s2z_";
            std::string thunkName = prefix + callee;
            if (!needed.count(thunkName)) {
                needed[thunkName] = {callee, callerIsZp};
            }
        }
    }

    if (needed.empty()) return;

    // ZP convention parameter block ($10-$2F, IRCodeGen: zeroPageStart $08 + 8)
    const uint8_t zpParamBase = 0x10;

    for (auto& [thunkName, need] : needed) {
        auto calleeIt = funcAttrs_.find(need.callee);
        uint8_t paramSize = (calleeIt != funcAttrs_.end()) ? calleeIt->second.paramSize : 0;
        auto calleeAddrIt = globalSymbols_.find(need.callee);
        if (calleeAddrIt == globalSymbols_.end()) continue;
        uint16_t calleeAddr = (uint16_t)calleeAddrIt->second;
        int calleeOvl = symOvl_.count(need.callee) && !ovlStubs_.count(need.callee) ? symOvl_[need.callee] : 0;
        auto wrapIt = zarWrapAt_.find(zarKey(calleeOvl, calleeAddr));   // (its base-page wrapper)
        if (wrapIt != zarWrapAt_.end()) calleeAddr = (uint16_t)wrapIt->second;

        uint32_t thunkAddr = textBase_ + (uint32_t)mergedText_.size();
        thunkAddresses_[thunkName] = thunkAddr;
        globalSymbols_[thunkName] = thunkAddr;

        if (!need.callerIsZp) {
            // Stack→ZP thunk (s2z): copy the stack arguments into the ZP parameter
            // block, then JMP to the callee (its RTS returns to the original caller).
            // The stack is reached through a pointer (TSX/TSY), not page-1 abs,X,
            // so it works with a 16-bit stack. Stack slots and ZP slots have the
            // same layout (at least 2 bytes per argument).
            int copy = paramSize;
            if (calleeIt != funcAttrs_.end()) {
                // Hand-written ZP routines declare their parameters with .zp_uses
                // (bit 0 = $09): copy up to the highest parameter byte used
                uint32_t uses = calleeIt->second.zpUses;
                for (int bit = 31; bit >= 0; bit--) {
                    int addr = 0x09 + bit;
                    if ((uses & (1u << bit)) && addr >= zpParamBase && addr < zpParamBase + 32) {
                        copy = std::max(copy, addr - zpParamBase + 1);
                        break;
                    }
                }
            }
            mergedText_.push_back(0xBA);                           // TSX
            mergedText_.push_back(0x0B);                           // TSY
            mergedText_.push_back(0x86); mergedText_.push_back(0x08);  // STX $08
            mergedText_.push_back(0x84); mergedText_.push_back(0x09);  // STY $09
            for (int i = 0; i < copy; i++) {
                mergedText_.push_back(0xA0);                       // LDY #(3+i): past the return address
                mergedText_.push_back((uint8_t)(3 + i));
                mergedText_.push_back(0xB1); mergedText_.push_back(0x08);  // LDA ($08),Y
                mergedText_.push_back(0x85);                       // STA zp
                mergedText_.push_back((uint8_t)(zpParamBase + i));
            }
            mergedText_.push_back(0x4C);                           // JMP callee
            mergedText_.push_back((uint8_t)(calleeAddr & 0xFF));
            mergedText_.push_back((uint8_t)(calleeAddr >> 8));
        } else {
            // ZP→Stack thunk (z2s): push the ZP parameters (highest first), JSR the
            // callee, drop them with PLA (correct for a 16-bit stack; X/Y/Z of the
            // result are untouched, A is saved), RTS.
            for (int i = paramSize - 1; i >= 0; i--) {
                mergedText_.push_back(0xA5);                       // LDA zp
                mergedText_.push_back((uint8_t)(zpParamBase + i));
                mergedText_.push_back(0x48);                       // PHA
            }
            mergedText_.push_back(0x20);                           // JSR callee
            mergedText_.push_back((uint8_t)(calleeAddr & 0xFF));
            mergedText_.push_back((uint8_t)(calleeAddr >> 8));
            if (paramSize > 0) {
                mergedText_.push_back(0x85); mergedText_.push_back(0x0E);  // STA $0E
                for (int i = 0; i < paramSize; i++) mergedText_.push_back(0x68);  // PLA
                mergedText_.push_back(0xA5); mergedText_.push_back(0x0E);  // LDA $0E
            }
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

            bool callerIsZp = false;
            if (!callNeedsThunk(caller, callee, callerIsZp)) continue;

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
            uint32_t addr = textAddrOf(input, li.textOffset);
            mergedLineMap_.push_back({addr, file, (int)li.line, input.ovlAt(li.textOffset)});
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
            << "\",\"line\":" << e.line;
        if (e.ovl > 0 && e.ovl <= (int)ovlNames_.size()) out << ",\"overlay\":\"" << ovlNames_[e.ovl - 1] << "\"";
        out << "}";
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
    if (sacOverlayLen_ > 0) {
        char buf[100];
        snprintf(buf, sizeof(buf), "  (SAC overlay $%04X - $%04X, %u bytes; %u saved by sharing)\n",
                 bssBase_ + sacOverlayOffset_, bssBase_ + sacOverlayOffset_ + sacOverlayLen_ - 1,
                 sacOverlayLen_, sacOverlaySaved_);
        out << buf;
    }
    if (zarTotal_ > 0) {
        char buf[120];
        snprintf(buf, sizeof(buf), "  (base-page blocks $%02X - $%02X, %u bytes; %u asked for by %zu functions)\n",
                 ZAR_LO, ZAR_LO + zarUsed_ - 1, zarUsed_, zarTotal_, zarOffset_.size());
        out << buf;
    }
    if (zpVarTotal_ > 0) {
        char buf[120];
        snprintf(buf, sizeof(buf), "  (__zp variables $%02X - $%02X, %u bytes, %zu variables)\n",
                 ZAR_HI - zpVarTotal_, ZAR_HI - 1, zpVarTotal_, zpVarAddr_.size());
        out << buf;
    }
    printSeg("ZP", zpBase_, mergedZpLen_);
    // code overlays: where they run, and where each is kept in far memory
    // (bank, then the address in the bank)
    auto far = [](uint32_t a) {
        char b[24];
        snprintf(b, sizeof(b), "bank %X $%04X", a >> 16, a & 0xFFFF);
        return std::string(b);
    };
    if (!ovlNames_.empty()) {
        char buf[200];
        snprintf(buf, sizeof(buf), "  %-6s $%04X - $%04X  (%zu %s, %s)\n", "OVL", ovlBase_, ovlBase_ + ovlSize_ - 1,
                 ovlNames_.size(), ovlNames_.size() == 1 ? "overlay" : "overlays",
                 ovlMap_ ? "mapped: MAP points this window at each" : "copied here by DMA when called");
        out << buf;
        for (size_t i = 0; i < ovlNames_.size(); i++) {
            uint32_t len = (uint32_t)ovlText_[i].size();
            snprintf(buf, sizeof(buf), "    %-14s kept in %s - $%04X  (%u bytes)\n", ovlNames_[i].c_str(),
                     far(ovlAt_[i]).c_str(), (ovlAt_[i] + (len ? len - 1 : 0)) & 0xFFFF, len);
            out << buf;
        }
        for (size_t r = 0; r < ovlFiles_.size(); r++) {
            snprintf(buf, sizeof(buf), "    (file %s, %zu bytes, loaded into %s at start)\n",
                     ovlFileNameOf(r).c_str(), ovlFiles_[r].size(), far(ovlStores_[r].first).c_str());
            out << buf;
        }
    }
    out << "\n";

    if (!discardedObjects_.empty()) {
        out << "Discarded Library Members (nothing live references them)\n";
        out << "--------------------------\n";
        for (const auto& n : discardedObjects_) out << "  " << n << "\n";
        out << "\n";
    }
    if (!discardedSectionNames_.empty()) {
        uint32_t total = 0;
        for (const auto& d : discardedSections_) total += d.bytes;
        out << "Discarded Sections (" << discardedSectionNames_.size() << ", " << total << " bytes of text and data)\n";
        out << "------------------\n";
        for (const auto& n : discardedSectionNames_) out << "  " << n << "\n";
        out << "\n";
    }

    // --- Per-object contributions ---
    out << "Object Files\n";
    out << "------------\n";
    for (const auto& input : objects_) {
        out << "  " << input.filename;
        if (input.ovl) out << "  [overlay " << ovlNames_[input.ovl - 1] << "]";
        out << "\n";
        char buf[160];
        // an object whose text went to several places (__far code): a line per chunk
        bool split = false;
        for (const auto& r : input.textRemaps) if (r.ovl != input.ovl) split = true;
        if (input.obj.tlen > 0 && split) {
            for (const auto& r : input.textRemaps) {
                if (!r.srcLen) continue;
                if (r.ovl) {
                    uint32_t start = ovlBase_ + r.destOffset, kept = ovlAt_[r.ovl - 1] + r.destOffset;
                    snprintf(buf, sizeof(buf), "    TEXT  $%04X - $%04X  (%u bytes; overlay %s, kept in %s - $%04X)\n",
                             start, start + r.srcLen - 1, r.srcLen, ovlNames_[r.ovl - 1].c_str(), far(kept).c_str(),
                             (kept + r.srcLen - 1) & 0xFFFF);
                } else {
                    uint32_t start = textBase_ + r.destOffset;
                    snprintf(buf, sizeof(buf), "    TEXT  $%04X - $%04X  (%u bytes)\n", start, start + r.srcLen - 1, r.srcLen);
                }
                out << buf;
            }
        } else if (input.obj.tlen > 0 && input.ovl) {
            uint32_t start = ovlBase_ + input.textOffset, kept = ovlAt_[input.ovl - 1] + input.textOffset;
            snprintf(buf, sizeof(buf), "    TEXT  $%04X - $%04X  (%u bytes; kept in %s - $%04X)\n",
                     start, start + input.obj.tlen - 1, input.obj.tlen, far(kept).c_str(),
                     (kept + input.obj.tlen - 1) & 0xFFFF);
            out << buf;
        } else if (input.obj.tlen > 0) {
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
        if (input.bssKeptLen > 0) {
            uint32_t start = bssBase_ + input.bssOffset;
            snprintf(buf, sizeof(buf), "    BSS   $%04X - $%04X  (%u bytes)\n",
                     start, start + input.bssKeptLen - 1, input.bssKeptLen);
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
        auto o = symOvl_.find(name);
        if (o != symOvl_.end() && o->second && !ovlStubs_.count(name)) continue;   // (Overlay Symbols)
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
        // the stub of a function in a code overlay
        if (ovlStubs_.count(name)) {
            auto o = symOvl_.find(name);
            auto ra = realAddr_.find(name);
            if (o != symOvl_.end() && o->second && ra != realAddr_.end())
            {
                char b[64];
                snprintf(b, sizeof(b), "  [stub: overlay %s, $%04X]", ovlNames_[o->second - 1].c_str(), ra->second);
                out << b;
            }
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

    // The code overlays' symbols: where they run (the same addresses for every
    // overlay) and where they are kept
    if (!ovlNames_.empty()) {
        out << "\nOverlay Symbols\n";
        out << "---------------\n";
        for (size_t i = 0; i < ovlNames_.size(); i++) {
            std::vector<std::pair<uint32_t, std::string>> os;
            for (const auto& [n, k] : symOvl_)
                if (k == (int)i + 1 && n.rfind("cc45.", 0) != 0 && realAddr_.count(n))
                    os.push_back({realAddr_.at(n), n});
            std::sort(os.begin(), os.end());
            out << "  overlay " << ovlNames_[i] << " (" << far(ovlAt_[i]) << ")\n";
            for (const auto& [a, n] : os) {
                char buf[200], stub[24] = "";
                auto srcIt = symbolSource_.find(n);
                auto st = ovlStubs_.find(n);
                if (st != ovlStubs_.end()) snprintf(stub, sizeof(stub), "  (stub $%04X)", st->second);
                snprintf(buf, sizeof(buf), "    $%04X  %-30s %-14s %s%s\n", a, n.c_str(),
                         far(ovlAt_[i] + (a - ovlBase_)).c_str(),
                         srcIt != symbolSource_.end() ? srcIt->second.c_str() : "", stub);
                out << buf;
            }
        }
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
            if (entry.ovl) continue;   // (Overlay Lines)
            auto key = std::make_pair(entry.file, entry.line);
            if (seen.insert(key).second) {
                char buf[256];
                snprintf(buf, sizeof(buf), "  $%04X  %s:%d\n",
                         entry.address, entry.file.c_str(), entry.line);
                out << buf;
            }
        }
        // (the overlays' lines run at the same addresses: one list per overlay)
        bool header = false;
        seen.clear();
        for (size_t i = 0; i < ovlNames_.size(); i++) {
            bool first = true;
            for (const auto& entry : mergedLineMap_) {
                if (entry.ovl != (int)i + 1) continue;
                if (!seen.insert(std::make_pair(entry.file, entry.line)).second) continue;
                if (!header) { out << "\nOverlay Lines\n-------------\n"; header = true; }
                if (first) { out << "  overlay " << ovlNames_[i] << " (" << far(ovlAt_[i]) << ")\n"; first = false; }
                char buf[256];
                snprintf(buf, sizeof(buf), "    $%04X  %s:%d\n", entry.address, entry.file.c_str(), entry.line);
                out << buf;
            }
        }
    }
}

