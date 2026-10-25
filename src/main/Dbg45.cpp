// Dbg45.cpp — dbg45's debugger (Dbg45.hpp)
//
// Breakpoints are xemu's 'k' list (stop before the opcode). Stepping:
// "step" runs an opcode at a time until the source line changes (a call
// into code without lines runs to its return); "next" and "finish" put
// temporary breakpoints where the line or function can go on (the
// function's other line starts, its return address) and run at full
// speed, going on past a stop deeper in the stack (recursion). A stop at a
// breakpoint address whose bytes are not the program's (the ROM's code
// mapped there before the program runs) resumes.
#include "Dbg45.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

static bool isCall(uint8_t op) { return op == 0x20 || op == 0x22 || op == 0x23 || op == 0x63; }

bool Dbg45::open(const std::string& prg, std::string& err) {
    prg_ = prg;
    std::ifstream in(prg, std::ios::binary);
    if (!in) { err = "cannot read " + prg; return false; }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (data.size() < 3) { err = prg + ": not a program"; return false; }
    imageBase = data[0] | (data[1] << 8);
    image.assign(data.begin() + 2, data.end());
    std::string dbgPath = prg;
    size_t d = dbgPath.find_last_of('.'), s = dbgPath.find_last_of('/');
    if (d != std::string::npos && (s == std::string::npos || d > s)) dbgPath = dbgPath.substr(0, d);
    dbgPath += ".dbg";
    if (!dbg.load(dbgPath, err)) return false;
    exitAddr_ = dbg.symbol("__exit");
    return true;
}

int Dbg45::addBreakpoint(const std::string& where, std::string& err) {
    std::vector<uint16_t> addrs;
    std::string desc = where;
    if (!where.empty() && where[0] == '*') {
        addrs.push_back((uint16_t)std::stoul(where.substr(1), nullptr, where.size() > 2 && (where[1] == '$') ? 16 : 0));
        if (where[1] == '$') addrs.back() = (uint16_t)std::stoul(where.substr(2), nullptr, 16);
    } else if (where.find(':') != std::string::npos ||
               std::all_of(where.begin(), where.end(), [](char c) { return isdigit((unsigned char)c); })) {
        std::string file;
        int line;
        size_t c = where.rfind(':');
        if (c != std::string::npos) { file = where.substr(0, c); line = std::stoi(where.substr(c + 1)); }
        else {
            // a line of the file the program stopped in (else main's)
            line = std::stoi(where);
            const DbgLine* l = started_ && !exited_ ? dbg.lineAt(regs.pc) : nullptr;
            if (!l) if (const DbgFunction* m = dbg.functionNamed("main")) l = dbg.lineAt(m->start);
            if (!l) { err = "no current source file: give file:line"; return 0; }
            file = dbg.files[l->file];
        }
        if (dbg.fileIndex(file) < 0) { err = "no source file " + file + " in the program"; return 0; }
        int want = line;
        for (uint32_t a : dbg.addressesOf(file, line)) addrs.push_back((uint16_t)a);
        if (addrs.empty()) { err = "no code at or after line " + std::to_string(want) + " of " + file; return 0; }
        std::string base = file.substr(file.find_last_of('/') == std::string::npos ? 0 : file.find_last_of('/') + 1);
        desc = base + ":" + std::to_string(line);
    } else {
        const DbgFunction* f = dbg.functionNamed(where);
        if (!f) { err = "no function " + where + " in the program"; return 0; }
        // its entries: the function's start and, for a static-allocation
        // function, <f>__sac (callers that know it call that one)
        addrs.push_back((uint16_t)f->start);
        long sac = dbg.symbol(f->name + "__sac");
        if (sac > (long)f->start && sac < (long)f->end) addrs.push_back((uint16_t)sac);
        desc = dbg.cName(f->name);
    }
    int id = nextBp_++;
    breakpoints[id] = {id, desc, addrs};
    return id;
}

bool Dbg45::deleteBreakpoint(int id) { return breakpoints.erase(id) > 0; }

void Dbg45::syncBreakpoints(const std::set<uint16_t>& temp) {
    std::set<uint16_t> want(temp);
    for (const auto& [id, b] : breakpoints) want.insert(b.addrs.begin(), b.addrs.end());
    if (exitAddr_ >= 0) want.insert((uint16_t)exitAddr_);
    for (uint16_t a : active_) if (!want.count(a)) mon.removeBreak(a);
    for (uint16_t a : want) if (!active_.count(a)) mon.addBreak(a);
    active_ = want;
}

uint8_t Dbg45::byteAt(uint32_t addr) {
    uint32_t line = addr & ~15u;
    auto it = cache_.find(line);
    if (it == cache_.end()) {
        std::vector<uint8_t> b;
        if (!mon.read(line, 16, b)) b.assign(16, 0);
        it = cache_.emplace(line, b).first;
    }
    return it->second[addr - line];
}

bool Dbg45::genuine(uint16_t pc) {
    if (pc < imageBase || pc >= imageBase + image.size()) return true;   // (not the program's: can't tell)
    for (uint32_t i = 0; i < 3 && pc + i < imageBase + image.size(); i++)
        if (byteAt(pc + i) != image[pc + i - imageBase]) return false;
    return true;
}

Dbg45::Stop Dbg45::runUntilStop(const std::set<uint16_t>& temp, std::string& err) {
    syncBreakpoints(temp);
    for (;;) {
        if (!mon.resume()) { err = "xemu's monitor does not answer"; return ERROR; }
        CpuRegs r;
        // (waits of a quarter second: the timeout, and an interrupt between)
        auto t0 = std::chrono::steady_clock::now();
        bool stopped = false, interrupted = false;
        for (;;) {
            if (mon.waitStop(r, 250)) { stopped = true; break; }
            if (!mon.running()) { err = "xemu ended"; started_ = false; return ERROR; }
            if (opt.interrupt && opt.interrupt()) { interrupted = true; break; }
            if (opt.timeoutMs >= 0 && std::chrono::steady_clock::now() - t0 > std::chrono::milliseconds(opt.timeoutMs)) break;
        }
        if (!stopped) {
            mon.pause();
            mon.regs(regs);
            cache_.clear();
            return interrupted ? PAUSED : TIMEOUT;
        }
        regs = r;
        cache_.clear();
        if (!genuine(r.pc)) continue;
        if (exitAddr_ >= 0 && r.pc == (uint16_t)exitAddr_) {
            exited_ = true;
            exitStatus_ = r.a | (r.x << 8);
            return EXITED;
        }
        lastBp_ = 0;
        for (const auto& [id, b] : breakpoints)
            if (std::find(b.addrs.begin(), b.addrs.end(), r.pc) != b.addrs.end()) { lastBp_ = id; break; }
        return lastBp_ ? BREAKPOINT : STEP;
    }
}

Dbg45::Stop Dbg45::run(bool toMain, std::string& err) {
    kill();
    if (!mon.launch(opt.xemu, prg_, 0, opt.headless, opt.fast, opt.xemuArgs, err)) return ERROR;
    if (!mon.hasBreakpointList()) {
        err = "this xemu (" + opt.xemu + ") has no breakpoint list (its monitor's 'k' command): dbg45 needs "
              "CTalkobt/xemu's (branch feat/monitor-breakpoints)";
        kill();
        return ERROR;
    }
    started_ = true;
    exited_ = false;
    active_.clear();
    std::set<uint16_t> temp;
    if (toMain) {
        const DbgFunction* m = dbg.functionNamed("main");
        if (!m) { err = "the program has no main"; return ERROR; }
        temp.insert((uint16_t)m->start);
        long sac = dbg.symbol(m->name + "__sac");
        if (sac > (long)m->start && sac < (long)m->end) temp.insert((uint16_t)sac);
    }
    return runUntilStop(temp, err);
}

Dbg45::Stop Dbg45::cont(std::string& err) {
    if (!started_ || exited_) { err = "the program is not running"; return ERROR; }
    return runUntilStop({}, err);
}

void Dbg45::kill() {
    mon.stop();
    started_ = false;
    active_.clear();
    cache_.clear();
}

Dbg45::Stop Dbg45::pause(std::string& err) {
    if (!started_) { err = "the program is not running"; return ERROR; }
    mon.pause();
    if (!mon.regs(regs)) { err = "no registers"; return ERROR; }
    cache_.clear();
    return STEP;
}

Dbg45::Stop Dbg45::stepInstruction(std::string& err) {
    if (!started_ || exited_) { err = "the program is not running"; return ERROR; }
    if (!mon.step(regs)) { err = "xemu's monitor does not answer"; return ERROR; }
    cache_.clear();
    if (exitAddr_ >= 0 && regs.pc == (uint16_t)exitAddr_) { exited_ = true; exitStatus_ = regs.a | (regs.x << 8); return EXITED; }
    return STEP;
}

// The return address of the function f's frame on the stack from sp: a
// word on the stack that is a JSR's (or BSR's) return address into f
// (else the first one returning after any call); -1: none
long Dbg45::returnAddressOf(const DbgFunction* f, uint16_t sp, size_t* at) {
    long top = dbg.symbol("__stack_top");
    uint32_t end = std::min<uint32_t>(top > 0 ? (uint32_t)top : 0xBFFF, sp + 0x400);
    std::vector<uint8_t> st;
    if (end <= sp || !mon.read(sp + 1u, end - sp, st)) return -1;
    long loose = -1;
    size_t looseAt = 0;
    for (size_t k = 0; k + 1 < st.size(); k++) {
        uint32_t ret = (uint32_t)(st[k] | (st[k + 1] << 8)) + 1;
        if (ret < 3 || ret > 0xFFFF) continue;
        uint32_t cs = ret - 3;
        uint8_t op = byteAt(cs);
        if (!isCall(op) || !dbg.functionAt(cs)) continue;
        uint32_t target = byteAt(cs + 1) | (byteAt(cs + 2) << 8);
        if (op == 0x63) target = (uint32_t)(cs + 2 + (int16_t)target) & 0xFFFF;
        if (f && op != 0x22 && op != 0x23 && target >= f->start && target < f->end) {
            if (at) *at = k;
            return ret;
        }
        if (loose < 0) { loose = ret; looseAt = k; }
    }
    if (at) *at = looseAt;
    return loose;
}

std::vector<Dbg45Frame> Dbg45::backtrace() {
    std::vector<Dbg45Frame> out;
    if (!started_) return out;
    Dbg45Frame f0;
    f0.pc = regs.pc;
    f0.function = dbg.functionAt(regs.pc);
    f0.line = dbg.lineAt(regs.pc);
    out.push_back(f0);
    uint16_t sp = regs.sp;
    while (out.back().function && out.size() < 64) {
        std::string name = dbg.cName(out.back().function->name);
        if (name == "main") break;
        size_t at = 0;
        long ret = returnAddressOf(out.back().function, sp, &at);
        if (ret < 0) break;
        Dbg45Frame f;
        f.pc = (uint32_t)ret - 3;
        f.function = dbg.functionAt(f.pc);
        f.line = dbg.lineAt(f.pc);
        if (!f.function) break;
        out.push_back(f);
        sp = (uint16_t)(sp + at + 2);
    }
    return out;
}

Dbg45::Stop Dbg45::finish(std::string& err) {
    if (!started_ || exited_) { err = "the program is not running"; return ERROR; }
    const DbgFunction* f = dbg.functionAt(regs.pc);
    long ret = returnAddressOf(f, regs.sp);
    if (ret < 0) { err = "no return address on the stack"; return ERROR; }
    uint16_t sp0 = regs.sp;
    for (;;) {
        Stop s = runUntilStop({(uint16_t)ret}, err);
        if (s != STEP || regs.pc != (uint16_t)ret || regs.sp > sp0) return s;
        // (a deeper call of the same function returned there: on)
    }
}

Dbg45::Stop Dbg45::stepLine(bool over, std::string& err) {
    if (!started_ || exited_) { err = "the program is not running"; return ERROR; }
    const DbgLine* start = dbg.lineAt(regs.pc);
    const DbgFunction* fn = dbg.functionAt(regs.pc);
    uint16_t sp0 = regs.sp;
    if (over && fn) {
        // the function's line starts but the one being stepped, and its
        // return address: run there at full speed
        std::set<uint16_t> temp;
        const DbgLine* prev = nullptr;
        for (const auto& l : dbg.lines) {
            if (l.addr < fn->start || l.addr >= fn->end) { prev = &l; continue; }
            bool runStart = !(prev && prev->file == l.file && prev->line == l.line && prev->addr >= fn->start);
            if (runStart && !(start && l.addr == start->addr)) temp.insert((uint16_t)l.addr);
            prev = &l;
        }
        long ret = returnAddressOf(fn, regs.sp);
        if (ret >= 0) temp.insert((uint16_t)ret);
        if (temp.size() + breakpoints.size() < 60) {
            for (;;) {
                Stop s = runUntilStop(temp, err);
                if (s != STEP) return s;
                if (regs.sp < sp0 && dbg.functionAt(regs.pc) == fn) continue;   // (a deeper call of it)
                const DbgLine* l = dbg.lineAt(regs.pc);
                if (!l) return stepLine(false, err);                             // (back into code without lines)
                // (the same line again, after an inlined function's lines: on)
                if (start && l->line == start->line && l->file == start->file && regs.sp == sp0 &&
                    dbg.functionAt(regs.pc) == fn) continue;
                return STEP;
            }
        }
    }
    for (int n = 0; n < 20000; n++) {
        uint8_t op = byteAt(regs.pc);
        bool call = isCall(op);
        Stop s = stepInstruction(err);
        if (s != STEP) return s;
        const DbgLine* l = dbg.lineAt(regs.pc);
        if (call && (over || !l)) {
            // a call to run over (next), or into code without lines: to its return
            long ret = returnAddressOf(dbg.functionAt(regs.pc), regs.sp);
            if (!l && regs.sp + 2 < 0x10000) {
                std::vector<uint8_t> w;
                if (mon.read(regs.sp + 1u, 2, w)) ret = (w[0] | (w[1] << 8)) + 1;
            }
            if (ret >= 0) {
                Stop r = runUntilStop({(uint16_t)ret}, err);
                if (r != STEP) return r;
                l = dbg.lineAt(regs.pc);
            }
        }
        if (!l) continue;
        if (!start || l->line != start->line || l->file != start->file || dbg.functionAt(regs.pc) != fn) return STEP;
    }
    return STEP;
}

std::string Dbg45::sourceLine(int file, int line) {
    if (file < 0 || file >= (int)dbg.files.size()) return "";
    auto it = sources_.find(file);
    if (it == sources_.end()) {
        std::vector<std::string> lines;
        // (a path from where cc45 ran: else next to the program)
        std::string path = dbg.files[file];
        std::ifstream in(path);
        if (!in && path[0] != '/') {
            size_t s = prg_.find_last_of('/');
            if (s != std::string::npos) in.open(prg_.substr(0, s + 1) + path);
        }
        for (std::string l; std::getline(in, l); ) lines.push_back(l);
        it = sources_.emplace(file, lines).first;
    }
    return line >= 1 && line <= (int)it->second.size() ? it->second[line - 1] : "";
}

// ---------------------------------------------------------------------------
// Values
// ---------------------------------------------------------------------------

bool Dbg45::readVar(const DbgVar& v, size_t frame, Dbg45Value& out, std::string& err) {
    out.type = v.type;
    int size = dbg.sizeOf(v.type);
    uint32_t addr;
    switch (v.loc) {
        case DbgVar::NONE: err = "<optimised out>"; return false;
        case DbgVar::ADDR: addr = (uint32_t)v.value; break;
        case DbgVar::ZP:
            // (the base page the code runs with: B, the program's or a handler's)
            if (frame) { err = "<not available in a caller's frame>"; return false; }
            addr = (uint32_t)(regs.b << 8) + (uint32_t)v.value;
            break;
        case DbgVar::FRAME: {
            if (frame) { err = "<not available in a caller's frame>"; return false; }
            std::vector<uint8_t> fp;
            if (!mon.read((uint32_t)(regs.b << 8) + 0xFD, 2, fp)) { err = "<unreadable>"; return false; }
            addr = (uint32_t)((fp[0] | (fp[1] << 8)) + v.value) & 0xFFFF;
            break;
        }
        case DbgVar::REG: {
            if (frame) { err = "<not available in a caller's frame>"; return false; }
            out.addr = -1;
            if (v.reg == "AX") out.bytes = {regs.a, regs.x};
            else if (v.reg == "X") out.bytes = {regs.x};
            else if (v.reg == "Y") out.bytes = {regs.y};
            else out.bytes = {regs.z};
            out.bytes.resize((size_t)size, 0);
            return true;
        }
    }
    out.addr = addr;
    if (!mon.read(addr, (size_t)size, out.bytes)) { err = "<unreadable>"; return false; }
    return true;
}

std::vector<std::pair<const DbgVar*, std::string>> Dbg45::locals(size_t frame, bool params) {
    std::vector<std::pair<const DbgVar*, std::string>> out;
    auto bt = backtrace();
    if (frame >= bt.size() || !bt[frame].function) return out;
    for (const DbgVar* v : dbg.variablesOf(bt[frame].function->name)) {
        if ((v->kind == "param") != params) continue;
        Dbg45Value val;
        std::string err;
        out.push_back({v, readVar(*v, frame, val, err) ? format(val) : err});
    }
    return out;
}

static std::string petscii(uint8_t c) {
    if (c >= 0x41 && c <= 0x5A) return std::string(1, (char)(c + 0x20));
    if (c >= 0xC1 && c <= 0xDA) return std::string(1, (char)(c - 0x80));
    if (c >= 0x20 && c < 0x41) return std::string(1, (char)c);
    if (c == 0x5B || c == 0x5D) return std::string(1, (char)c);
    if (c == 0x0D) return "\\r";
    char b[8];
    snprintf(b, sizeof b, "\\x%02x", c);
    return b;
}

static long long intOf(const std::vector<uint8_t>& b, bool sign) {
    unsigned long long v = 0;
    for (size_t i = 0; i < b.size() && i < 8; i++) v |= (unsigned long long)b[i] << (8 * i);
    if (sign && !b.empty() && b.size() < 8 && (b.back() & 0x80)) v |= ~0ULL << (8 * b.size());
    return (long long)v;
}

// CBM 40-bit float: exponent, then the mantissa (its top bit the sign)
static double cbmFloat(const std::vector<uint8_t>& b) {
    if (b.size() < 5 || b[0] == 0) return 0;
    double m = ((double)((b[1] | 0x80) << 24 | b[2] << 16 | b[3] << 8 | b[4])) / 4294967296.0;
    double v = std::ldexp(m, b[0] - 128);
    return (b[1] & 0x80) ? -v : v;
}

std::string Dbg45::format(const Dbg45Value& v, int depth) {
    const std::string& t = v.type;
    char buf[64];
    if (t.empty()) return "?";
    if (t[0] == '&') {           // (a struct parameter's copy: its address)
        Dbg45Value d;
        d.type = t.substr(1);
        d.addr = (long)intOf(v.bytes, false);
        if (!mon.read((uint32_t)d.addr, (size_t)dbg.sizeOf(d.type), d.bytes)) return "<unreadable>";
        return format(d, depth);
    }
    if (t[0] == '*') {
        unsigned a = (unsigned)intOf(v.bytes, false);
        snprintf(buf, sizeof buf, "0x%04x", a);
        std::string s = buf;
        if ((t == "*char" || t == "*u8" || t == "*s8") && a) {
            std::vector<uint8_t> str;
            if (mon.read(a, 48, str)) {
                std::string q;
                for (uint8_t c : str) { if (!c) break; q += petscii(c); }
                s += " \"" + q + "\"";
            }
        }
        return s;
    }
    if (t[0] == '[') {
        size_t close = t.find(']');
        int n = std::stoi(t.substr(1, close - 1));
        std::string et = t.substr(close + 1);
        if (et == "char" || et == "u8" || et == "s8") {
            std::string q;
            for (int i = 0; i < n && i < (int)v.bytes.size(); i++) { if (!v.bytes[i]) break; q += petscii(v.bytes[i]); }
            return "\"" + q + "\"";
        }
        if (depth > 2) return "{...}";
        std::string s = "{";
        int es = dbg.sizeOf(et);
        for (int i = 0; i < n && i < 16; i++) {
            Dbg45Value e;
            e.type = et;
            e.bytes.assign(v.bytes.begin() + std::min<size_t>(v.bytes.size(), (size_t)(i * es)),
                           v.bytes.begin() + std::min<size_t>(v.bytes.size(), (size_t)((i + 1) * es)));
            e.addr = v.addr < 0 ? -1 : v.addr + i * es;
            s += (i ? ", " : "") + format(e, depth + 1);
        }
        if (n > 16) s += ", ...";
        return s + "}";
    }
    if (t.rfind("struct:", 0) == 0 || t.rfind("union:", 0) == 0) {
        if (depth > 2) return "{...}";
        std::string s = "{";
        bool first = true;
        for (const auto& [name, m] : children(v)) {
            s += (first ? "" : ", ") + name + " = " + format(m, depth + 1);
            first = false;
        }
        return s + "}";
    }
    if (t == "bool") return intOf(v.bytes, false) ? "true" : "false";
    if (t == "f40") { snprintf(buf, sizeof buf, "%.9g", cbmFloat(v.bytes)); return buf; }
    if (t == "char") {
        unsigned c = v.bytes.empty() ? 0 : v.bytes[0];
        snprintf(buf, sizeof buf, "%u '%s'", c, petscii((uint8_t)c).c_str());
        return buf;
    }
    bool sign = t[0] == 's';
    if (t == "u64" || t == "s64" || t == "u128" || t == "s128" || v.bytes.size() > 8) {
        std::string s = "0x";
        for (size_t i = v.bytes.size(); i-- > 0; ) { snprintf(buf, sizeof buf, "%02x", v.bytes[i]); s += buf; }
        return s;
    }
    snprintf(buf, sizeof buf, sign ? "%lld" : "%llu", intOf(v.bytes, sign));
    return buf;
}

std::vector<std::pair<std::string, Dbg45Value>> Dbg45::children(const Dbg45Value& v) {
    std::vector<std::pair<std::string, Dbg45Value>> out;
    std::string t = v.type;
    Dbg45Value base = v;
    if (!t.empty() && t[0] == '&') {
        base.type = t.substr(1);
        base.addr = (long)intOf(v.bytes, false);
        if (!mon.read((uint32_t)base.addr, (size_t)dbg.sizeOf(base.type), base.bytes)) return out;
        t = base.type;
    }
    if (!t.empty() && t[0] == '[') {
        size_t close = t.find(']');
        int n = std::stoi(t.substr(1, close - 1));
        std::string et = t.substr(close + 1);
        int es = dbg.sizeOf(et);
        for (int i = 0; i < n && i < 256; i++) {
            Dbg45Value e;
            e.type = et;
            size_t a = std::min<size_t>(base.bytes.size(), (size_t)(i * es)), b = std::min<size_t>(base.bytes.size(), (size_t)((i + 1) * es));
            e.bytes.assign(base.bytes.begin() + a, base.bytes.begin() + b);
            e.addr = base.addr < 0 ? -1 : base.addr + i * es;
            out.push_back({"[" + std::to_string(i) + "]", e});
        }
        return out;
    }
    auto it = dbg.types.find(t);
    if (it == dbg.types.end()) return out;
    for (const auto& m : it->second.members) {
        Dbg45Value e;
        e.type = m.type;
        int ms = dbg.sizeOf(m.type);
        size_t a = std::min<size_t>(base.bytes.size(), (size_t)m.offset), b = std::min<size_t>(base.bytes.size(), (size_t)(m.offset + ms));
        e.bytes.assign(base.bytes.begin() + a, base.bytes.begin() + b);
        e.addr = base.addr < 0 ? -1 : base.addr + m.offset;
        if (m.bit >= 0) {                     // a bit-field: its bits, as a value of its type
            unsigned long long u = (unsigned long long)intOf(e.bytes, false);
            u = (u >> m.bit) & ((m.width >= 64 ? ~0ULL : (1ULL << m.width) - 1));
            if (m.type[0] == 's' && m.width < 64 && (u >> (m.width - 1)) & 1) u |= ~0ULL << m.width;
            for (size_t i = 0; i < e.bytes.size(); i++) e.bytes[i] = (uint8_t)(u >> (8 * i));
            e.addr = -1;
        }
        out.push_back({m.name, e});
    }
    return out;
}

// ---------------------------------------------------------------------------
// Expressions: name, number, $hex, *e, &e, e.member, e->member, e[i], (e)
// ---------------------------------------------------------------------------

namespace {
struct ExprParser {
    Dbg45& d;
    size_t frame;
    const std::string& s;
    size_t p = 0;
    std::string err;
    ExprParser(Dbg45& d_, size_t f, const std::string& s_) : d(d_), frame(f), s(s_) {}
    void ws() { while (p < s.size() && isspace((unsigned char)s[p])) p++; }
    bool eat(const char* t) {
        ws();
        size_t n = strlen(t);
        if (s.compare(p, n, t) == 0) { p += n; return true; }
        return false;
    }
    std::string ident() {
        ws();
        size_t b = p;
        while (p < s.size() && (isalnum((unsigned char)s[p]) || s[p] == '_')) p++;
        return s.substr(b, p - b);
    }
    bool load(Dbg45Value& v) {   // v.addr and v.type set: its bytes
        if (v.addr < 0) { err = "no address"; return false; }
        if (!d.readMemory((uint32_t)v.addr, (size_t)d.dbg.sizeOf(v.type), v.bytes)) { err = "unreadable memory"; return false; }
        return true;
    }
    bool deref(Dbg45Value& v) {
        std::string t = v.type;
        if (!t.empty() && (t[0] == '*' || t[0] == '&')) {
            v.addr = (long)intOf(v.bytes, false);
            v.type = t.substr(1);
            if (v.type == "void") v.type = "u8";
            return load(v);
        }
        if (!t.empty() && t[0] == '[') return index(v, 0);
        err = "not a pointer (" + t + ")";
        return false;
    }
    bool index(Dbg45Value& v, long i) {
        std::string t = v.type;
        if (!t.empty() && t[0] == '[') {
            std::string et = t.substr(t.find(']') + 1);
            if (v.addr < 0) { err = "no address"; return false; }
            v.addr += i * d.dbg.sizeOf(et);
            v.type = et;
            return load(v);
        }
        if (!t.empty() && t[0] == '*') {
            std::string et = t.substr(1);
            v.addr = (long)intOf(v.bytes, false) + i * d.dbg.sizeOf(et);
            v.type = et;
            return load(v);
        }
        err = "not an array or pointer (" + t + ")";
        return false;
    }
    bool member(Dbg45Value& v, const std::string& name) {
        for (auto& [n, m] : d.children(v))
            if (n == name) { v = m; return true; }
        err = "no member " + name + " in " + v.type;
        return false;
    }
    bool primary(Dbg45Value& v) {
        ws();
        if (eat("(")) { if (!unary(v)) return false; if (!eat(")")) { err = "')' expected"; return false; } return true; }
        if (p < s.size() && (isdigit((unsigned char)s[p]) || s[p] == '$')) {
            long n;
            if (s[p] == '$') { p++; size_t b = p; while (p < s.size() && isxdigit((unsigned char)s[p])) p++; n = std::stol(s.substr(b, p - b), nullptr, 16); }
            else { size_t e; n = std::stol(s.substr(p), &e, 0); p += e; }
            v.type = n > 0xFFFF || n < -32768 ? "s32" : "s16";
            int sz = v.type == "s32" ? 4 : 2;
            v.bytes.clear();
            for (int i = 0; i < sz; i++) v.bytes.push_back((uint8_t)(n >> (8 * i)));
            v.addr = -1;
            return true;
        }
        std::string name = ident();
        if (name.empty()) { err = "a name expected at '" + s.substr(p) + "'"; return false; }
        auto bt = d.backtrace();
        std::string fn = frame < bt.size() && bt[frame].function ? bt[frame].function->name : "";
        const DbgVar* var = d.dbg.lookup(name, fn);
        if (!var) {
            long sym = d.dbg.symbol(name);
            if (sym < 0) sym = d.dbg.symbol("_" + name);
            if (sym < 0) { err = "no variable " + name + " here"; return false; }
            v.type = "u8";       // a plain symbol: its byte
            v.addr = sym;
            return load(v);
        }
        std::string e;
        if (!d.evaluateVar(*var, frame, v, e)) { err = name + ": " + e; return false; }
        return true;
    }
    bool postfix(Dbg45Value& v) {
        if (!primary(v)) return false;
        for (;;) {
            if (eat("->")) { if (!deref(v) || !member(v, ident())) return false; }
            else if (eat(".")) { if (!member(v, ident())) return false; }
            else if (eat("[")) {
                Dbg45Value i;
                if (!unary(i)) return false;
                if (!eat("]")) { err = "']' expected"; return false; }
                if (!index(v, (long)intOf(i.bytes, i.type[0] == 's'))) return false;
            } else return true;
        }
    }
    bool unary(Dbg45Value& v) {
        if (eat("*")) return unary(v) && deref(v);
        if (eat("&")) {
            if (!unary(v)) return false;
            if (v.addr < 0) { err = "no address"; return false; }
            long a = v.addr;
            v.type = "*" + v.type;
            v.bytes = {(uint8_t)a, (uint8_t)(a >> 8)};
            v.addr = -1;
            return true;
        }
        return postfix(v);
    }
};
}  // namespace

bool Dbg45::evaluateVar(const DbgVar& v, size_t frame, Dbg45Value& out, std::string& err) {
    return readVar(v, frame, out, err);
}

bool Dbg45::evaluate(const std::string& expr, size_t frame, Dbg45Value& out, std::string& err) {
    if (!started_) { err = "the program is not running"; return false; }
    ExprParser ep(*this, frame, expr);
    if (!ep.unary(out)) { err = ep.err; return false; }
    ep.ws();
    if (ep.p != expr.size()) { err = "unexpected '" + expr.substr(ep.p) + "'"; return false; }
    return true;
}

std::string Dbg45::location(const Dbg45Frame& f) {
    std::string s = f.function ? dbg.cName(f.function->name) : "??";
    char buf[16];
    if (!f.function) { snprintf(buf, sizeof buf, "$%04x", f.pc); return buf; }
    s += " (";
    bool first = true;
    for (const DbgVar* v : dbg.variablesOf(f.function->name)) {
        if (v->kind != "param") continue;
        Dbg45Value val;
        std::string err;
        size_t frame = f.pc == regs.pc ? 0 : 1;
        s += (first ? "" : ", ") + v->name + "=" + (readVar(*v, frame, val, err) ? format(val, 2) : "?");
        first = false;
    }
    s += ")";
    if (f.line) {
        std::string file = dbg.files[f.line->file];
        s += " at " + file.substr(file.find_last_of('/') == std::string::npos ? 0 : file.find_last_of('/') + 1) +
             ":" + std::to_string(f.line->line);
    }
    return s;
}
