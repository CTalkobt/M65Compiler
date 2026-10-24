// DbgFile.cpp — loading <program>.dbg and its lookups (DbgFile.hpp)
#include "DbgFile.hpp"
#include "MiniJson.hpp"
#include <algorithm>
#include <fstream>
#include <sstream>

bool DbgFile::load(const std::string& path, std::string& err) {
    std::ifstream in(path);
    if (!in) { err = "cannot read " + path + " (link with ln45 -g, or cc45 -g)"; return false; }
    std::stringstream ss;
    ss << in.rdbuf();
    Json root;
    if (!Json::parse(ss.str(), root) || root.kind != Json::OBJ || root.strAt("format") != "cc45-debug") {
        err = path + ": not a cc45 debug file";
        return false;
    }
    if (root.numAt("version") != 1) { err = path + ": debug file version " + std::to_string((int)root.numAt("version")) + " (1 expected)"; return false; }
    program = root.strAt("program");
    if (const Json* z = root.at("zp_page"); z && z->kind == Json::NUM) { zpPage = (uint32_t)z->num; hasZpPage = true; }
    if (const Json* z = root.at("zp_page_isr"); z && z->kind == Json::NUM) zpPageIsr = (uint32_t)z->num;
    std::map<std::string, int> ovlIndex;
    if (const Json* o = root.at("overlays"))
        for (const auto& v : o->arr) { overlays.push_back(v.strAt("name")); ovlIndex[overlays.back()] = (int)overlays.size(); }
    auto ovlOf = [&](const Json* v) { return v && v->kind == Json::STR && ovlIndex.count(v->str) ? ovlIndex[v->str] : 0; };
    if (const Json* o = root.at("objects")) for (const auto& v : o->arr) objects.push_back(v.str);
    if (const Json* f = root.at("files")) for (const auto& v : f->arr) files.push_back(v.str);
    if (const Json* l = root.at("lines"))
        for (const auto& e : l->arr) {
            if (e.arr.size() < 3) continue;
            DbgLine dl;
            dl.addr = (uint32_t)e.arr[0].num;
            dl.file = (int)e.arr[1].num;
            dl.line = (int)e.arr[2].num;
            dl.overlay = e.arr.size() > 3 ? ovlOf(&e.arr[3]) : 0;
            lines.push_back(dl);
        }
    std::stable_sort(lines.begin(), lines.end(), [](const DbgLine& a, const DbgLine& b) { return a.addr < b.addr; });
    if (const Json* fs = root.at("functions"))
        for (const auto& f : fs->arr) {
            DbgFunction df;
            df.name = f.strAt("name");
            df.start = (uint32_t)f.numAt("start");
            df.end = (uint32_t)f.numAt("end");
            df.overlay = ovlOf(f.at("overlay"));
            df.object = (int)f.numAt("object");
            functions.push_back(df);
        }
    std::sort(functions.begin(), functions.end(), [](const DbgFunction& a, const DbgFunction& b) { return a.start < b.start; });
    if (const Json* ts = root.at("types"))
        for (const auto& [name, t] : ts->obj) {
            DbgType dt;
            dt.size = (int)t.numAt("size");
            if (const Json* ms = t.at("members"))
                for (const auto& m : ms->arr) {
                    DbgMember dm;
                    dm.name = m.strAt("name");
                    dm.offset = (int)m.numAt("offset");
                    if (m.at("bit")) { dm.bit = (int)m.numAt("bit"); dm.width = (int)m.numAt("width"); }
                    dm.type = m.strAt("type");
                    dt.members.push_back(dm);
                }
            types[name] = dt;
        }
    if (const Json* vs = root.at("variables"))
        for (const auto& v : vs->arr) {
            DbgVar dv;
            dv.kind = v.strAt("kind");
            dv.scope = v.strAt("scope");
            dv.name = v.strAt("name");
            dv.type = v.strAt("type");
            dv.object = (int)v.numAt("object");
            if (const Json* l = v.at("location"); l && l->kind == Json::OBJ) {
                if (l->at("addr")) { dv.loc = DbgVar::ADDR; dv.value = (long)l->numAt("addr"); }
                else if (l->at("zp")) { dv.loc = DbgVar::ZP; dv.value = (long)l->numAt("zp"); }
                else if (l->at("frame")) { dv.loc = DbgVar::FRAME; dv.value = (long)l->numAt("frame"); }
                else if (l->at("reg")) { dv.loc = DbgVar::REG; dv.reg = l->strAt("reg"); }
            }
            variables.push_back(dv);
        }
    if (const Json* sy = root.at("symbols"))
        for (const auto& [name, v] : sy->obj) symbols[name] = (uint32_t)v.num;
    return true;
}

const DbgFunction* DbgFile::functionAt(uint32_t addr, int overlay) const {
    for (const auto& f : functions)
        if (addr >= f.start && addr < f.end && (f.overlay == overlay || !overlay || !f.overlay)) return &f;
    return nullptr;
}

std::string DbgFile::cName(const std::string& label) const {
    std::string n = label;
    if (!n.empty() && n[0] == '_') n = n.substr(1);
    // a static function: name__<its file's stem>
    size_t p = n.rfind("__");
    if (p != std::string::npos && p > 0) {
        std::string stem = n.substr(p + 2);
        for (const auto& f : files) {
            std::string b = f.substr(f.find_last_of('/') == std::string::npos ? 0 : f.find_last_of('/') + 1);
            b = b.substr(0, b.find('.'));
            if (b == stem) return n.substr(0, p);
        }
    }
    return n;
}

const DbgFunction* DbgFile::functionNamed(const std::string& name) const {
    for (const auto& f : functions) if (f.name == name) return &f;
    for (const auto& f : functions) if (f.name == "_" + name) return &f;
    for (const auto& f : functions) if (cName(f.name) == name) return &f;
    return nullptr;
}

// Lines starting at one address (a .loc with no code of its own before the
// next: a function's opening line, its first statement): the first, but a
// later line of the file after it wins (the opening line gives way to the
// statement; an inlined function's earlier line does not)
const DbgLine* DbgFile::lineAt(uint32_t addr, int overlay) const {
    const DbgFunction* f = functionAt(addr, overlay);
    const DbgLine* best = nullptr;
    const DbgLine* second = nullptr;
    for (const auto& l : lines) {
        if (l.addr > addr) break;
        if (overlay && l.overlay && l.overlay != overlay) continue;
        if (f && (l.addr < f->start || l.addr >= f->end)) continue;
        if (!best || l.addr != best->addr) { best = &l; second = nullptr; }
        else if (!second) second = &l;
    }
    if (best && second && second->file == best->file && second->line > best->line) return second;
    return best;
}

// A file as given (its path as recorded, the end of that path: t.c for
// src/t.c, or a full path whose end is the recorded one: an editor's)
int DbgFile::fileIndex(const std::string& file) const {
    auto endsWith = [](const std::string& a, const std::string& b) {
        return a.size() > b.size() && a.compare(a.size() - b.size(), b.size(), b) == 0 && a[a.size() - b.size() - 1] == '/';
    };
    for (size_t i = 0; i < files.size(); i++) if (files[i] == file) return (int)i;
    for (size_t i = 0; i < files.size(); i++) if (endsWith(files[i], file) || endsWith(file, files[i])) return (int)i;
    return -1;
}

std::vector<uint32_t> DbgFile::addressesOf(const std::string& file, int& line) const {
    std::vector<uint32_t> out;
    int fi = fileIndex(file);
    if (fi < 0) return out;
    // the line, or the next one with code
    int best = -1;
    for (const auto& l : lines)
        if (l.file == fi && l.line >= line && (best < 0 || l.line < best)) best = l.line;
    if (best < 0) return out;
    line = best;
    // where a run of the line starts (the previous entry is another line)
    const DbgLine* prev = nullptr;
    for (const auto& l : lines) {
        if (l.file == fi && l.line == best && !(prev && prev->file == fi && prev->line == best && prev->overlay == l.overlay))
            out.push_back(l.addr);
        prev = &l;
    }
    return out;
}

std::vector<const DbgVar*> DbgFile::variablesOf(const std::string& function) const {
    std::vector<const DbgVar*> out;
    for (const auto& v : variables) if (v.scope == function) out.push_back(&v);
    return out;
}

const DbgVar* DbgFile::lookup(const std::string& name, const std::string& function) const {
    int obj = -1;
    if (const DbgFunction* f = functionNamed(function)) obj = f->object;
    for (const auto& v : variables) if (v.name == name && !function.empty() && v.scope == function) return &v;
    for (const auto& v : variables) if (v.name == name && v.scope.empty() && v.object == obj) return &v;
    for (const auto& v : variables) if (v.name == name && v.kind == "global") return &v;
    for (const auto& v : variables) if (v.name == name && v.scope.empty()) return &v;
    return nullptr;
}

int DbgFile::sizeOf(const std::string& type) const {
    std::string t = type;
    if (!t.empty() && t[0] == '&') return 2;   // (the address of the value)
    if (!t.empty() && t[0] == '[') {
        size_t close = t.find(']');
        int n = std::stoi(t.substr(1, close - 1));
        return n * sizeOf(t.substr(close + 1));
    }
    if (!t.empty() && t[0] == '*') return 2;
    if (t == "char" || t == "u8" || t == "s8" || t == "bool") return 1;
    if (t == "u16" || t == "s16") return 2;
    if (t == "u32" || t == "s32") return 4;
    if (t == "u64" || t == "s64") return 8;
    if (t == "u128" || t == "s128") return 16;
    if (t == "f40") return 5;
    if (t == "void") return 1;
    auto it = types.find(t);
    return it == types.end() ? 2 : it->second.size;
}

long DbgFile::symbol(const std::string& name) const {
    auto it = symbols.find(name);
    return it == symbols.end() ? -1 : (long)it->second;
}
