// SacSlotShare.cpp — static-allocation (SAC) locals that are never live at
// the same time share their storage (#324)
//
// cc45 gives every frame vreg of a SAC function a slot of its own in the
// function's block (_f__local_<id>: .res n), even temporaries of different
// statements or branches, and slots of values kept in registers or folded
// away. This pass works on the finished assembly, the ground truth of what
// reads and writes each slot: a liveness of every byte of the slots over the
// function's code (between proc and endproc, following its labels and
// branches), then slots that are never live together get one storage, the
// slot labels of a group on one .res. The names stay, so nothing else
// changes; cc45.sacsize.<f> gives the new size.
//
// Conservative where the code is not understood: an instruction it does not
// know, or an indexed or indirect operand, reads every byte of the slots it
// names and writes none; a jump through a pointer or to a label not in the
// function, or data in the code, has every slot live; a slot whose address
// is taken (#_f__local_N) or that is named outside its function keeps its
// own storage; a function that calls setjmp is left alone (longjmp returns
// into it). A slot read before any write is live from the entry: slots live
// together at the entry are kept apart.
#include "SacSlotShare.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t"), b = s.find_last_not_of(" \t\r");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

// The line without its comment (a ';' outside quotes)
std::string code(const std::string& s) {
    bool q = false;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '"') q = !q;
        else if (s[i] == ';' && !q) return s.substr(0, i);
    }
    return s;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

struct Slot {
    int size = 0;
    size_t line = 0;      // its label's line (the .res is the next)
    bool keep = false;    // its own storage (address taken, named elsewhere)
    int base = 0;         // its first byte in the liveness bit vector
};

// One statement of the function's code
struct Node {
    std::vector<int> reads, writes;   // byte bits
    std::vector<int> succ;            // next statements (-1: the function's exit)
    bool opaque = false;              // every byte live
    bool fall = true;
    std::string jumpTo;
};

class Function {
public:
    std::string name;
    std::map<int, Slot> slots;   // by vreg id
    int bytes = 0;

    // The slot a symbol names (name__local_<id>), and the byte offset after it
    bool slotRef(const std::string& operand, size_t at, int& id, int& off, size_t& end) const {
        std::string pre = name + "__local_";
        if (operand.compare(at, pre.size(), pre) != 0) return false;
        size_t e = at + pre.size();
        if (e >= operand.size() || !std::isdigit((unsigned char)operand[e])) return false;
        while (e < operand.size() && std::isdigit((unsigned char)operand[e])) e++;
        if (e < operand.size() && (std::isalnum((unsigned char)operand[e]) || operand[e] == '_')) return false;
        id = std::stoi(operand.substr(at + pre.size(), e - at - pre.size()));
        off = 0;
        if (e + 1 < operand.size() && operand[e] == '+' && std::isdigit((unsigned char)operand[e + 1])) {
            size_t f = e + 1;
            while (f < operand.size() && std::isdigit((unsigned char)operand[f])) f++;
            off = std::stoi(operand.substr(e + 1, f - e - 1));
            e = f;
        }
        end = e;
        return slots.count(id) != 0;
    }
};

void bitsOf(const Slot& s, int from, int n, std::vector<int>& out) {
    for (int k = from; k < from + n && k < s.size; k++) out.push_back(s.base + k);
}

}  // namespace

std::string shareSacSlots(const std::string& text) {
    std::vector<std::string> lines;
    {
        std::istringstream in(text);
        for (std::string l; std::getline(in, l); ) lines.push_back(l);
    }
    bool endsWithNewline = !text.empty() && text.back() == '\n';

    // The SAC functions' slots: name__local_<id>: then .res n
    std::map<std::string, Function> fns;
    for (size_t i = 0; i + 1 < lines.size(); i++) {
        std::string l = trim(code(lines[i]));
        size_t p = l.find("__local_");
        if (p == std::string::npos || l.back() != ':' || l.find_first_of(" \t") != std::string::npos) continue;
        std::string fn = l.substr(0, p);
        std::string idText = l.substr(p + 8, l.size() - 1 - (p + 8));
        if (idText.empty() || idText.find_first_not_of("0123456789") != std::string::npos) continue;
        std::string r = trim(code(lines[i + 1]));
        if (r.rfind(".res ", 0) != 0) continue;
        std::string n = trim(r.substr(5));
        if (n.empty() || n.find_first_not_of("0123456789") != std::string::npos) continue;
        Function& f = fns[fn];
        f.name = fn;
        Slot s;
        s.size = std::stoi(n);
        s.line = i;
        f.slots[std::stoi(idText)] = s;
    }

    std::map<size_t, std::vector<std::string>> rewrite;   // block's first line -> new lines
    std::set<size_t> drop;
    std::map<std::string, std::pair<size_t, int>> sizeLine;  // cc45.sacsize.<f> line, bytes saved
    for (auto& [fnName, f] : fns) {
        // the slots one after another (nothing between them: an .align keeps the function as it is)
        std::vector<size_t> at;
        for (auto& [id, s] : f.slots) at.push_back(s.line);
        std::sort(at.begin(), at.end());
        bool contiguous = true;
        for (size_t k = 1; k < at.size(); k++) if (at[k] != at[k - 1] + 2) contiguous = false;
        if (!contiguous || f.slots.size() < 2) continue;
        for (auto& [id, s] : f.slots) { s.base = f.bytes; f.bytes += s.size; }

        // the code: proc <name> ... endproc
        size_t from = lines.size(), to = lines.size();
        for (size_t i = 0; i < lines.size(); i++) {
            std::string c = trim(code(lines[i]));
            if (from == lines.size()) {
                if (c.rfind("proc ", 0) == 0) {
                    std::string nm = trim(c.substr(5));
                    nm = nm.substr(0, nm.find_first_of(", \t"));
                    if (nm == fnName) from = i;
                }
            } else if (c == "endproc") { to = i; break; }
        }
        if (from == lines.size() || to == lines.size()) continue;

        // a slot named outside its function and its block keeps its storage
        for (size_t i = 0; i < lines.size(); i++) {
            if ((i >= from && i <= to) || (i >= at.front() && i <= at.back() + 1)) continue;
            std::string c = code(lines[i]);
            for (size_t p = c.find(fnName + "__local_"); p != std::string::npos; p = c.find(fnName + "__local_", p + 1)) {
                int id, off; size_t e;
                if ((p == 0 || !(std::isalnum((unsigned char)c[p - 1]) || c[p - 1] == '_')) && f.slotRef(c, p, id, off, e))
                    f.slots[id].keep = true;
            }
        }

        // the statements, their labels
        std::vector<Node> nodes;
        std::map<std::string, int> labelAt;
        bool skip = false;
        for (size_t i = from + 1; i < to && !skip; i++) {
            std::string c = trim(code(lines[i]));
            while (!c.empty()) {                       // labels (several, or before an instruction)
                size_t colon = c.find(':');
                if (colon == std::string::npos) break;
                std::string lb = c.substr(0, colon);
                if (lb.empty() || lb.find_first_of(" \t\"#$(") != std::string::npos) break;
                labelAt[lb] = (int)nodes.size();
                c = trim(c.substr(colon + 1));
            }
            if (c.empty()) continue;
            Node nd;
            std::string m = lower(c.substr(0, c.find_first_of(" \t")));
            std::string operand = c.size() > m.size() ? trim(c.substr(m.size())) : "";
            if (m[0] == '.') {
                static const std::set<std::string> quiet = {
                    ".loc", ".var", ".func_flags", ".reg_clobbers", ".flag_clobbers", ".frame_size", ".zp_uses",
                    ".zp_clobbers", ".zp_release", ".global", ".extern", ".weak", ".local", ".bptop", ".bpindex", ".sac",
                    ".param_sizes", ".zar"};
                if (quiet.count(m)) continue;
                nd.opaque = true;                      // data in the code, or anything else
                nodes.push_back(nd);
                continue;
            }
            // the slots this statement names
            std::vector<std::pair<int, int>> refs;      // (id, byte offset)
            bool addrTaken = false;
            for (size_t p = operand.find(fnName + "__local_"); p != std::string::npos;
                 p = operand.find(fnName + "__local_", p + 1)) {
                if (p > 0 && (std::isalnum((unsigned char)operand[p - 1]) || operand[p - 1] == '_')) continue;
                int id, off; size_t e;
                if (!f.slotRef(operand, p, id, off, e)) continue;
                refs.push_back({id, off});
                std::string before = operand.substr(0, p);
                if (before.find('#') != std::string::npos) { f.slots[id].keep = true; addrTaken = true; }
            }
            if (m == "jsr" && (operand == "_setjmp" || operand == "___builtin_setjmp" || operand == "__setjmp")) {
                skip = true;
                break;
            }
            // control flow
            static const std::set<std::string> cond = {"bcc", "bcs", "beq", "bne", "bmi", "bpl", "bvc", "bvs",
                "lbcc", "lbcs", "lbeq", "lbne", "lbmi", "lbpl", "lbvc", "lbvs"};
            bool bbx = m.size() == 4 && (m.compare(0, 3, "bbr") == 0 || m.compare(0, 3, "bbs") == 0);
            if (cond.count(m) || bbx) {
                nd.jumpTo = trim(operand.substr(operand.find(',') == std::string::npos ? 0 : operand.find(',') + 1));
            } else if (m == "bra" || m == "lbra" || (m == "jmp" && operand.find('(') == std::string::npos)) {
                nd.jumpTo = operand;
                nd.fall = false;
            } else if (m == "jmp" || m == "rti" || m == "brk") {
                nd.opaque = m == "jmp" || m == "brk";   // through a pointer
                nd.fall = false;
            } else if (m == "rts") {
                nd.fall = false;                       // the locals are dead after the return
            }
            if (!refs.empty()) {
                const std::string& op = operand;
                bool indexed = op.find(",x") != std::string::npos || op.find(",y") != std::string::npos ||
                               op.find(",z") != std::string::npos || op.find(",X") != std::string::npos ||
                               op.find(",Y") != std::string::npos || op.find(",Z") != std::string::npos ||
                               op.find('(') != std::string::npos || op.find('[') != std::string::npos;
                static const std::set<std::string> read1 = {"lda", "ldx", "ldy", "ldz", "adc", "sbc", "and", "ora",
                    "eor", "cmp", "cpx", "cpy", "cpz", "bit"};
                static const std::set<std::string> write1 = {"sta", "stx", "sty", "stz"};
                static const std::set<std::string> rmw1 = {"inc", "dec", "asl", "lsr", "rol", "ror", "asr", "tsb", "trb"};
                static const std::set<std::string> rmw2 = {"inw", "dew", "asw", "row"};
                static const std::set<std::string> read2 = {"ldax", "lday", "ldaz", "ldxy"};
                static const std::set<std::string> write2 = {"stax", "stay", "staz", "stxy"};
                bool known = refs.size() == 1 && !indexed && !addrTaken;
                const Slot& s = f.slots[refs[0].first];
                int k = refs[0].second;
                if (known && read1.count(m)) bitsOf(s, k, 1, nd.reads);
                else if (known && m == "ldq") bitsOf(s, k, 4, nd.reads);
                else if (known && read2.count(m)) bitsOf(s, k, 2, nd.reads);
                else if (known && write1.count(m)) bitsOf(s, k, 1, nd.writes);
                else if (known && m == "stq") bitsOf(s, k, 4, nd.writes);
                else if (known && write2.count(m)) bitsOf(s, k, 2, nd.writes);
                else if (known && rmw1.count(m)) { bitsOf(s, k, 1, nd.reads); bitsOf(s, k, 1, nd.writes); }
                else if (known && rmw2.count(m)) { bitsOf(s, k, 2, nd.reads); bitsOf(s, k, 2, nd.writes); }
                else
                    for (const auto& [id, off] : refs) bitsOf(f.slots[id], 0, f.slots[id].size, nd.reads);
            }
            nodes.push_back(nd);
        }
        if (skip) continue;
        const int n = (int)nodes.size();
        for (int i = 0; i < n; i++) {
            Node& nd = nodes[i];
            if (nd.fall && !nd.opaque) nd.succ.push_back(i + 1 < n ? i + 1 : -1);
            if (!nd.jumpTo.empty()) {
                auto it = labelAt.find(nd.jumpTo);
                if (it != labelAt.end()) nd.succ.push_back(it->second < n ? it->second : -1);
                else nd.opaque = true;   // out of the function, or an expression (ca45 makes the tail calls)
            }
        }

        // liveness: live in = reads | (live out - writes), every byte
        const int B = f.bytes;
        std::vector<std::vector<char>> in(n, std::vector<char>(B, 0)), out(n, std::vector<char>(B, 0));
        for (bool more = true; more; ) {
            more = false;
            for (int i = n - 1; i >= 0; i--) {
                const Node& nd = nodes[i];
                std::vector<char> o(B, 0);
                if (nd.opaque) std::fill(o.begin(), o.end(), 1);
                for (int s : nd.succ) if (s >= 0) for (int b = 0; b < B; b++) o[b] |= in[s][b];
                std::vector<char> l = o;
                if (!nd.opaque) for (int b : nd.writes) l[b] = 0;
                for (int b : nd.reads) l[b] = 1;
                if (nd.opaque) std::fill(l.begin(), l.end(), 1);
                if (l != in[i]) { in[i] = l; more = true; }
                out[i] = o;
            }
        }

        // interference: a slot written while another is live; slots live at the entry
        std::vector<int> ids;
        std::map<int, int> slotOfBit;
        for (auto& [id, s] : f.slots) { ids.push_back(id); for (int k = 0; k < s.size; k++) slotOfBit[s.base + k] = id; }
        std::set<std::pair<int, int>> clash;
        auto liveSlots = [&](const std::vector<char>& v) {
            std::set<int> r;
            for (int b = 0; b < B; b++) if (v[b]) r.insert(slotOfBit[b]);
            return r;
        };
        for (int i = 0; i < n; i++) {
            std::set<int> written;
            for (int b : nodes[i].writes) written.insert(slotOfBit[b]);
            if (written.empty()) continue;
            std::set<int> live = liveSlots(out[i]);
            for (int a : written) for (int c : live) if (a != c) { clash.insert({a, c}); clash.insert({c, a}); }
        }
        if (n > 0) {
            std::set<int> entry = liveSlots(in[0]);
            for (int a : entry) for (int c : entry) if (a != c) clash.insert({a, c});
        }

        // groups: the largest slots first, each into the first group with none it clashes with
        std::vector<int> order = ids;
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return f.slots[a].size > f.slots[b].size; });
        std::vector<std::vector<int>> groups;
        for (int id : order) {
            bool placed = false;
            if (!f.slots[id].keep)
                for (auto& g : groups) {
                    if (f.slots[g[0]].keep) continue;
                    bool ok = true;
                    for (int o : g) if (clash.count({id, o})) { ok = false; break; }
                    if (ok) { g.push_back(id); placed = true; break; }
                }
            if (!placed) groups.push_back({id});
        }
        if (groups.size() == ids.size()) continue;   // nothing shared

        std::vector<std::string> nl;
        int newBytes = 0;
        for (auto& g : groups) {
            int sz = 0;
            std::sort(g.begin(), g.end());
            for (int id : g) { nl.push_back(fnName + "__local_" + std::to_string(id) + ":"); sz = std::max(sz, f.slots[id].size); }
            nl.push_back("    .res " + std::to_string(sz));
            newBytes += sz;
        }
        rewrite[at.front()] = nl;
        for (size_t l = at.front(); l <= at.back() + 1; l++) drop.insert(l);
        sizeLine[fnName] = {0, f.bytes - newBytes};
    }
    if (rewrite.empty()) return text;

    // the new blocks, and the sizes (cc45.sacsize.<f> = n)
    std::ostringstream o;
    for (size_t i = 0; i < lines.size(); i++) {
        auto rw = rewrite.find(i);
        if (rw != rewrite.end()) for (const auto& l : rw->second) o << l << "\n";
        if (drop.count(i)) continue;
        std::string c = trim(code(lines[i]));
        if (c.rfind("cc45.sacsize.", 0) == 0) {
            size_t eq = c.find('=');
            std::string fn = trim(c.substr(13, eq == std::string::npos ? 0 : eq - 13));
            auto it = sizeLine.find(fn);
            if (it != sizeLine.end() && eq != std::string::npos) {
                int v = std::stoi(trim(c.substr(eq + 1))) - it->second.second;
                o << "    cc45.sacsize." << fn << " = " << v;
                if (i + 1 < lines.size() || endsWithNewline) o << "\n";
                continue;
            }
        }
        o << lines[i];
        if (i + 1 < lines.size() || endsWithNewline) o << "\n";
    }
    return o.str();
}
