// objdump45_analysis.cpp — objdump45's checks and comparisons over its model
// (#201; the model is --json's: objdump45_main.cpp builds it).
//
// Checks (each file's "checks": severity, kind, message, the address):
//   self-modifying   a store, inc/dec, shift or tsb/trb into the code itself
//   mid-instruction  a branch, jump or call into the middle of an instruction
//   undecodable      bytes in code that are no instruction (data among the code)
//   relocation       a relocation of a type the format does not have
//   isr-sac          an __interrupt handler calling a function with static
//                    allocation (ln45 rejects that: the handler may interrupt it)
//   convention       a call between the ZP and stack conventions (ln45 adds a
//                    thunk; through a pointer it would not)
//   unresolved       a library member's import no member defines
//
// Comparison (--compare A B: a "comparison" entry): the segments' sizes, the
// symbols added, removed and changed (value, segment, the function's
// attributes), the relocations by symbol, and each function's instructions,
// those of A against those of B (addresses and operands that are only the
// code's own addresses left out).
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "MiniJson.hpp"

namespace {

const std::vector<Json>& arr(const Json& m, const char* k) {
    static const std::vector<Json> none;
    const Json* v = m.at(k);
    return v && v->kind == Json::ARR ? v->arr : none;
}

std::string hx(long v) {
    char b[16];
    snprintf(b, sizeof b, "$%04lx", v);
    return b;
}

struct FnInfo { std::string conv; bool isr = false, sac = false; };

// every function's attributes, by name, over all the files (and members)
void collectFns(const Json& m, std::map<std::string, FnInfo>& fns) {
    for (const auto& s : arr(m, "symbols")) {
        const Json* a = s.at("attr");
        if (!a) continue;
        FnInfo f;
        f.conv = a->strAt("conv");
        for (const auto& fl : arr(*a, "flags")) {
            if (fl.str == "isr") f.isr = true;
            if (fl.str == "static_alloc") f.sac = true;
        }
        fns[s.strAt("name")] = f;
    }
    for (const auto& mm : arr(m, "members")) collectFns(mm, fns);
}

// a name in a note: "<_foo+3> lo" -> _foo
std::string noteName(const std::string& n) {
    if (n.size() < 3 || n[0] != '<') return "";
    size_t e = n.find_first_of("+->");
    return n.substr(1, e == std::string::npos ? std::string::npos : e - 1);
}

// the address an absolute operand names: $d020, $2005,x (-1: none)
long operandAddr(const std::string& o) {
    if (o.size() < 5 || o[0] != '$') return -1;
    size_t e = 1;
    while (e < o.size() && isxdigit((unsigned char)o[e])) e++;
    if (e - 1 != 4) return -1;   // (a base-page operand: not code)
    return std::stol(o.substr(1, 4), nullptr, 16);
}

void check(Json& out, const char* sev, const char* kind, const std::string& msg, long addr = -1) {
    Json c = Json::object();
    c.set("severity", sev).set("kind", kind).set("message", msg);
    if (addr >= 0) c.set("a", (double)addr);
    out.push(std::move(c));
}

void checkFile(Json& m, const std::map<std::string, FnInfo>& fns, const std::set<std::string>* libExports) {
    Json checks = Json::array();
    std::map<std::string, std::string> segOf;   // symbol -> its segment
    for (const auto& s : arr(m, "symbols")) segOf[s.strAt("name")] = s.strAt("seg");
    bool object = m.strAt("kind") == "object";
    // (a PRG or binary without its map is decoded to its end, data too: no
    // branch or byte there tells anything)
    bool codeKnown = object || m.strAt("kind") == "overlays" || !m.strAt("map").empty();
    static const std::set<std::string> writes = {"sta", "stx", "sty", "stz", "stq", "inc", "dec", "asl", "lsr", "rol", "ror",
                                                 "asr", "tsb", "trb", "inw", "dew", "asw", "row", "inq", "deq", "aslq", "lsrq",
                                                 "rolq", "rorq", "asrq", "smb", "rmb"};
    std::set<std::pair<std::string, std::string>> reported;   // (caller, callee) per kind
    for (const auto& sec : arr(m, "disasm")) {
        long base = (long)sec.numAt("base"), end = base + (long)sec.numAt("size");
        std::set<long> starts;
        for (const auto& l : arr(sec, "lines")) if (l.strAt("t") != "S") starts.insert((long)l.numAt("a"));
        std::string fn;
        long runAt = -1, runLen = 0;
        auto flushRun = [&]() {
            if (runLen >= 1 && codeKnown) check(checks, "info", "undecodable", std::to_string(runLen) + " byte" + (runLen > 1 ? "s" : "") +
                                   " in " + sec.strAt("name") + " that " + (runLen > 1 ? "are" : "is") +
                                   " no instruction (data among the code?)", runAt);
            runAt = -1;
            runLen = 0;
        };
        for (const auto& l : arr(sec, "lines")) {
            std::string t = l.strAt("t");
            long a = (long)l.numAt("a");
            if (t == "L") { fn = l.strAt("name"); continue; }
            if (t == "B") { if (runAt < 0) runAt = a; runLen++; continue; }
            if (t != "I") continue;
            flushRun();
            std::string mn = l.strAt("m");
            std::string base3 = mn.size() == 4 && (mn.compare(0, 3, "smb") == 0 || mn.compare(0, 3, "rmb") == 0) ? mn.substr(0, 3) : mn;
            // a store into the code
            if (writes.count(base3)) {
                std::string target;
                if (object) {
                    std::string n = noteName(l.strAt("n"));
                    if (!n.empty() && (n == "TEXT" || (segOf.count(n) && segOf[n] == "TEXT"))) target = l.strAt("n");
                } else {
                    long t2 = operandAddr(l.strAt("o"));
                    if (t2 >= base && t2 < end) target = hx(t2);
                }
                if (!target.empty())
                    check(checks, "info", "self-modifying", (fn.empty() ? std::string() : fn + ": ") + mn + " writes into the code (" + target + ")", a);
            }
            // into the middle of an instruction
            if (const Json* g = l.at("g")) {
                long t2 = (long)g->num;
                if (codeKnown && t2 >= base && t2 < end && !starts.count(t2))
                    check(checks, "warning", "mid-instruction", (fn.empty() ? std::string() : fn + ": ") + mn + " to " + hx(t2) +
                          ", inside an instruction (overlapping code, or data decoded as code)", a);
            }
            // calls: the callee's convention and allocation
            if ((mn == "jsr" || mn == "jmp" || mn == "bsr") && !fn.empty()) {
                std::string callee = noteName(l.strAt("n"));
                if (callee.empty()) continue;
                auto ce = fns.find(callee), cr = fns.find(fn);
                if (ce == fns.end() || cr == fns.end() || callee == fn) continue;
                if (cr->second.isr && ce->second.sac && reported.insert({fn, "sac:" + callee}).second)
                    check(checks, "warning", "isr-sac", "the interrupt handler " + fn + " calls " + callee +
                          ", which has static allocation: ln45 rejects that (#pragma cc45 recurse before it gives it a frame)", a);
                if (!ce->second.conv.empty() && !cr->second.conv.empty() && ce->second.conv != cr->second.conv &&
                    reported.insert({fn, "conv:" + callee}).second)
                    check(checks, "info", "convention", fn + " (" + cr->second.conv + " convention) calls " + callee + " (" +
                          ce->second.conv + "): ln45 bridges it with a thunk", a);
            }
        }
        flushRun();
    }
    for (const auto& r : arr(m, "relocs"))
        if (r.strAt("type") == "R_???")
            check(checks, "warning", "relocation", "a relocation of an unknown type at " + r.strAt("seg") + "+" + hx((long)r.numAt("offset")));
    if (libExports) {
        for (const auto& s : arr(m, "symbols")) {
            std::string n = s.strAt("name");
            if (s.strAt("seg") != "U" || libExports->count(n) || n.rfind("__", 0) == 0 || n.rfind("cc45.", 0) == 0) continue;
            check(checks, "info", "unresolved", "imports " + n + ", which no member of the library defines");
        }
    }
    m.set("checks", std::move(checks));
}

void checkAll(Json& m, const std::map<std::string, FnInfo>& fns) {
    std::string kind = m.strAt("kind");
    if (kind == "library") {
        std::set<std::string> exports;
        for (const auto& mm : arr(m, "members"))
            for (const auto& s : arr(mm, "symbols")) if (s.strAt("seg") != "U") exports.insert(s.strAt("name"));
        for (auto& [k, v] : m.obj)
            if (k == "members") for (auto& mm : v.arr) if (mm.strAt("kind") == "object") checkFile(mm, fns, &exports);
        return;
    }
    if (kind == "object" || kind == "prg" || kind == "binary" || kind == "overlays") checkFile(m, fns, nullptr);
}

// ---------------------------------------------------------------- compare

struct Ins { std::string text; long a; };

// each function's instructions, by label: "mnemonic operand" (a branch's or
// a jump's own-code target left out: it moves with the code)
std::map<std::string, std::vector<Ins>> functions(const Json& m) {
    std::map<std::string, std::vector<Ins>> out;
    for (const auto& sec : arr(m, "disasm")) {
        std::string fn = sec.strAt("name") + " (start)";
        for (const auto& l : arr(sec, "lines")) {
            std::string t = l.strAt("t");
            if (t == "L") { fn = l.strAt("name"); continue; }
            if (t != "I" && t != "B") continue;
            std::string s;
            if (t == "B") s = ".byte $" + l.strAt("b");
            else {
                s = l.strAt("m");
                std::string o = l.strAt("o"), n = l.strAt("n");
                if (l.at("g")) o = "<label>";
                if (!n.empty() && n[0] == '<') o = n;   // (a relocated operand: by its symbol)
                if (!o.empty()) s += " " + o;
            }
            out[fn].push_back({s, (long)l.numAt("a")});
        }
    }
    return out;
}

// the lines of a against b: " ", "-" (only in a), "+" (only in b)
std::vector<std::pair<char, std::string>> diff(const std::vector<Ins>& a, const std::vector<Ins>& b) {
    size_t n = a.size(), m = b.size();
    std::vector<std::pair<char, std::string>> out;
    if (n * m > 4000000) {   // (too big for the table: the lines as they are)
        for (const auto& x : a) out.push_back({'-', x.text});
        for (const auto& x : b) out.push_back({'+', x.text});
        return out;
    }
    std::vector<std::vector<int>> L(n + 1, std::vector<int>(m + 1, 0));
    for (size_t i = n; i-- > 0;)
        for (size_t j = m; j-- > 0;)
            L[i][j] = a[i].text == b[j].text ? L[i + 1][j + 1] + 1 : std::max(L[i + 1][j], L[i][j + 1]);
    size_t i = 0, j = 0;
    while (i < n || j < m) {
        if (i < n && j < m && a[i].text == b[j].text) { out.push_back({' ', a[i].text}); i++; j++; }
        else if (j < m && (i == n || L[i][j + 1] >= L[i + 1][j])) { out.push_back({'+', b[j].text}); j++; }
        else { out.push_back({'-', a[i].text}); i++; }
    }
    return out;
}

std::string attrText(const Json* a) {
    if (!a) return "";
    std::string s = a->strAt("conv");
    for (const auto& f : arr(*a, "flags")) s += " " + f.str;
    s += " regs:" + a->strAt("regs") + " flags:" + a->strAt("cflags") + " params:" + std::to_string((int)a->numAt("params")) +
         " frame:" + std::to_string((int)a->numAt("frame"));
    return s;
}

}  // namespace

void objdump45Checks(Json& models) {
    std::map<std::string, FnInfo> fns;
    for (const auto& m : models.arr) collectFns(m, fns);
    for (auto& m : models.arr) checkAll(m, fns);
}

Json objdump45Compare(const Json& A, const Json& B) {
    Json c = Json::object();
    c.set("kind", "comparison").set("file", A.strAt("file") + " → " + B.strAt("file")).set("a", A.strAt("file")).set("b", B.strAt("file"));
    // segments
    Json segs = Json::array();
    std::map<std::string, std::pair<long, long>> sz;
    for (const auto& s : arr(A, "segments")) sz[s.strAt("name")].first = (long)s.numAt("size");
    for (const auto& s : arr(B, "segments")) sz[s.strAt("name")].second = (long)s.numAt("size");
    if (A.at("size") && !A.at("segments")) sz["(file)"] = {(long)A.numAt("size"), (long)B.numAt("size")};
    for (const auto& [n, p] : sz) {
        Json j = Json::object();
        j.set("name", n).set("a", (double)p.first).set("b", (double)p.second).set("delta", (double)(p.second - p.first));
        segs.push(std::move(j));
    }
    c.set("segments", std::move(segs));
    // symbols
    std::map<std::string, const Json*> sa, sb;
    for (const auto& s : arr(A, "symbols")) sa[s.strAt("name")] = &s;
    for (const auto& s : arr(B, "symbols")) sb[s.strAt("name")] = &s;
    Json added = Json::array(), removed = Json::array(), changed = Json::array();
    for (const auto& [n, s] : sb)
        if (!sa.count(n)) { Json j = Json::object(); j.set("name", n).set("seg", s->strAt("seg")); if (s->at("attr")) j.set("attr", attrText(s->at("attr"))); added.push(std::move(j)); }
    for (const auto& [n, s] : sa)
        if (!sb.count(n)) { Json j = Json::object(); j.set("name", n).set("seg", s->strAt("seg")); if (s->at("attr")) j.set("attr", attrText(s->at("attr"))); removed.push(std::move(j)); }
    for (const auto& [n, s] : sa) {
        auto o = sb.find(n);
        if (o == sb.end()) continue;
        std::vector<std::string> what;
        if (s->strAt("seg") != o->second->strAt("seg")) what.push_back("segment " + s->strAt("seg") + " → " + o->second->strAt("seg"));
        if (s->boolAt("weak") != o->second->boolAt("weak")) what.push_back(o->second->boolAt("weak") ? "now weak" : "no longer weak");
        std::string ta = attrText(s->at("attr")), tb = attrText(o->second->at("attr"));
        if (ta != tb) what.push_back("attributes " + (ta.empty() ? "-" : ta) + " → " + (tb.empty() ? "-" : tb));
        if (what.empty()) continue;
        Json j = Json::object();
        std::string w;
        for (const auto& x : what) w += (w.empty() ? "" : "; ") + x;
        j.set("name", n).set("change", w);
        changed.push(std::move(j));
    }
    c.set("added", std::move(added)).set("removed", std::move(removed)).set("changed", std::move(changed));
    // relocations by symbol
    std::map<std::string, std::pair<int, int>> rel;
    for (const auto& r : arr(A, "relocs")) rel[r.at("symbol") ? r.strAt("symbol") : r.strAt("target")].first++;
    for (const auto& r : arr(B, "relocs")) rel[r.at("symbol") ? r.strAt("symbol") : r.strAt("target")].second++;
    Json rels = Json::array();
    for (const auto& [n, p] : rel) {
        if (p.first == p.second) continue;
        Json j = Json::object();
        j.set("name", n).set("a", p.first).set("b", p.second);
        rels.push(std::move(j));
    }
    c.set("relocs", std::move(rels));
    // functions
    auto fa = functions(A), fb = functions(B);
    std::set<std::string> names;
    for (const auto& [n, v] : fa) names.insert(n);
    for (const auto& [n, v] : fb) names.insert(n);
    Json fns = Json::array();
    int same = 0;
    for (const auto& n : names) {
        const auto& x = fa[n];
        const auto& y = fb[n];
        auto d = diff(x, y);
        int plus = 0, minus = 0;
        for (const auto& [k, t] : d) { plus += k == '+'; minus += k == '-'; }
        if (!plus && !minus) { same++; continue; }
        Json j = Json::object();
        j.set("name", n).set("a", (double)x.size()).set("b", (double)y.size()).set("added", plus).set("removed", minus);
        Json lines = Json::array();
        for (const auto& [k, t] : d) lines.push(Json::string(std::string(1, k) + " " + t));
        j.set("lines", std::move(lines));
        fns.push(std::move(j));
    }
    c.set("functions", std::move(fns)).set("unchanged", same);
    return c;
}
