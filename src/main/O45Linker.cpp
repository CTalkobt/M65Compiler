#include "O45Linker.hpp"
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
            uint32_t ta = textAlignOf(input.obj);
            if (ta > 1)
                while ((textBase_ + (uint32_t)mergedText_.size() - chunk.srcOffset) % ta) mergedText_.push_back(0);
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
            targetAddr = it->second + addend;

            if (arOffset > 0) {
                ln45debug() << "DEBUG [AR Reloc]: Symbol '" << symName << "' base=0x" << std::hex << it->second
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

            if (r.segment == SEG_TEXT && !input.textRemaps.empty() && input.textRemaps.size() > 1) {
                targetAddr = segBase + input.remapTextOffset(segRelOff);
            } else if (r.segment == SEG_BSS) {
                targetAddr = segBase + input.remapBssOffset(segRelOff);
            } else {
                targetAddr = segBase + segObjOff + segRelOff;
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

    // Selectively pull library members to resolve undefined symbols, then
    // drop the members only dead code needed
    size_t numCommandLine = objects_.size();
    if (!resolveLibraries(errorMsg)) return {};
    if (gcObjectsOn_) {
        gcSections(numCommandLine);
        if (!gcErrors_.empty()) {
            errorMsg = "dropping unused sections: " + gcErrors_[0];
            return {};
        }
    }

    if (!layoutSegments(errorMsg)) return {};
    if (!resolveSymbols(errorMsg)) return {};

    // Phase 3: function attribute integration and debug info
    mergeLineMaps();
    buildFuncAttrs();
    buildCallGraph();
    computeTransitiveClobbers();
    emitDiagnostics();
    verifyStaticAllocSafety();  // Verify SAC constraints before thunk generation
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

    // Generate thunks for convention mismatches (appends to mergedText_)
    generateThunks();

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

    if (!applyRelocations(errorMsg)) return {};

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

// SAC overlay. A static-allocation (SAC) function keeps its parameters and
// locals in a BSS block (cc45: <f>__ar, size cc45.sacsize.<f>). Two SAC
// functions can share storage when neither can be active while the other
// is: neither reaches the other through calls. The blocks of such
// functions are placed first-fit in one overlay area at the end of BSS.
//
// Calls are JSR and JMP references to a function. Any other reference
// (an address loaded or stored in data) makes the function callable
// indirectly from anywhere: it and everything it reaches keep their own
// storage. In objects not made by cc45 (assembly), exported labels are
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

    auto sacBase = [this](std::string n) {
        if (n.size() > 5 && n.compare(n.size() - 5, 5, "__sac") == 0 &&
            funcAttrs_.count(n.substr(0, n.size() - 5)))
            n.resize(n.size() - 5);
        return n;
    };

    std::map<std::string, std::set<std::string>> graph = callGraph_;
    std::set<std::string> addrTaken;

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

    std::set<std::string> unsafe;
    for (const auto& f : addrTaken) {
        unsafe.insert(f);
        auto r = reachFrom(f);
        unsafe.insert(r.begin(), r.end());
    }

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
            bool defined = false;   // this object's definition is the one in use
            for (const auto& e2 : input.obj.exports)
                if (e2.name == fn && e2.segmentId() == SEG_TEXT &&
                    globalSymbols_[fn] == textBase_ + input.remapTextOffset(e2.offset))
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
    if (sacOverlayLen_ > 0) {
        char buf[100];
        snprintf(buf, sizeof(buf), "  (SAC overlay $%04X - $%04X, %u bytes; %u saved by sharing)\n",
                 bssBase_ + sacOverlayOffset_, bssBase_ + sacOverlayOffset_ + sacOverlayLen_ - 1,
                 sacOverlayLen_, sacOverlaySaved_);
        out << buf;
    }
    printSeg("ZP", zpBase_, mergedZpLen_);
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

