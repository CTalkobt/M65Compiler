// objdump45_tree.cpp — objdump45 --tree (#201): the model --json writes as a
// tree in the terminal: files, then their header, segments, link sections,
// symbols, functions, relocations, disassembly (by label), data and call
// graph. Arrow keys move, expand and collapse; Enter on an instruction that
// branches, jumps or calls goes to its target (Backspace comes back); / finds.
//
// The terminal is driven with termios and the ANSI (VT100) sequences every
// terminal in use understands: no curses library. Not on a terminal (a pipe,
// a file), the whole tree is written as indented text.
#include <algorithm>
#include <cctype>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
#include <sys/ioctl.h>
#include <sys/select.h>   // select, fd_set (glibc has them through other headers; musl does not)
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include "MiniJson.hpp"

namespace {

struct Node {
    std::string text;
    int parent = -1;
    std::vector<int> kids;
    bool open = false;
    int jump = -1;          // the node Enter goes to (a branch's target)
    long jumpAddr = -1;     // (before the targets are resolved)
    int file = -1;          // the file node it belongs to
};

std::vector<Node> T;

int add(int parent, const std::string& text) {
    Node n;
    n.text = text;
    n.parent = parent;
    n.file = parent >= 0 ? T[parent].file : -1;
    T.push_back(n);
    int i = (int)T.size() - 1;
    if (parent >= 0) T[parent].kids.push_back(i);
    else T[i].file = i;
    return i;
}

std::string hx(double v, int w = 4) {
    char b[16];
    snprintf(b, sizeof b, "$%0*lx", w, (unsigned long)v);
    return b;
}

std::string pad(std::string s, size_t w) {
    if (s.size() < w) s.append(w - s.size(), ' ');
    return s;
}

const std::vector<Json>& arr(const Json& m, const char* k) {
    static const std::vector<Json> none;
    const Json* v = m.at(k);
    return v && v->kind == Json::ARR ? v->arr : none;
}

std::string attrText(const Json& a) {
    std::string s = a.strAt("conv") + "_call";
    for (const auto& f : arr(a, "flags")) s += " " + f.str;
    s += "  regs:" + (a.strAt("regs").empty() ? "-" : a.strAt("regs"));
    s += " flags:" + (a.strAt("cflags").empty() ? "-" : a.strAt("cflags"));
    return s;
}

// per file: address -> node (labels, instruction lines) for the jumps
std::map<int, std::map<long, int>> g_at;

void buildFile(int parent, const Json& m);

void buildDisasm(int fileNode, int parent, const Json& sec) {
    int secNode = add(parent, "Disassembly: " + sec.strAt("name") + "  (" + sec.strAt("cpu") + ", " +
                      std::to_string((long)sec.numAt("size")) + " bytes at " + hx(sec.numAt("base")) + ")");
    int cur = -1;
    auto& at = g_at[fileNode];
    // who reaches each address (for "called from")
    std::map<long, std::vector<long>> from;
    for (const auto& l : arr(sec, "lines"))
        if (l.strAt("t") == "I" && l.at("g")) from[(long)l.numAt("g")].push_back((long)l.numAt("a"));
    for (const auto& l : arr(sec, "lines")) {
        std::string t = l.strAt("t");
        long a = (long)l.numAt("a");
        if (t == "L") {
            std::string text = hx(a) + " <" + l.strAt("name") + ">";
            auto f = from.find(a);
            if (f != from.end()) text += "   (reached from " + std::to_string(f->second.size()) + ")";
            cur = add(secNode, text);
            if (!at.count(a)) at[a] = cur;
            continue;
        }
        if (cur < 0) cur = add(secNode, hx(a) + " (start)");
        if (t == "S") {
            add(cur, "; " + l.strAt("file") + ":" + std::to_string((int)l.numAt("line")) + "  " + l.strAt("text"));
            continue;
        }
        std::string line = "  " + hx(a).substr(1) + ":  " + pad(l.strAt("b"), 16);
        if (t == "B") line += ".byte $" + l.strAt("b");
        else {
            if (!l.strAt("c").empty()) line += pad(l.strAt("c"), 6);
            line += pad(l.strAt("m"), 6) + l.strAt("o");
            if (!l.strAt("n").empty()) line += "   ; " + l.strAt("n");
            if (l.boolAt("u")) line += " (undocumented)";
        }
        int n = add(cur, line);
        if (l.at("g")) T[n].jumpAddr = (long)l.numAt("g");
        if (!at.count(a)) at[a] = n;
    }
}

void buildFile(int parent, const Json& m) {
    std::string kind = m.strAt("kind");
    std::string head = m.strAt("file") + "  (" + kind;
    if (!m.strAt("format").empty()) head += ", " + m.strAt("format") + ", " + m.strAt("cpu");
    head += ")";
    int f = add(parent, head);
    T[f].file = f;
    if (kind == "library") {
        int idx = add(f, "Symbol index (" + std::to_string(arr(m, "index").size()) + ")");
        const auto& members = arr(m, "members");
        for (const auto& e : arr(m, "index")) {
            size_t mi = (size_t)e.numAt("member");
            add(idx, pad(e.strAt("name"), 32) + (mi < members.size() ? members[mi].strAt("file") : "?"));
        }
        for (const auto& mm : members) buildFile(f, mm);
        return;
    }
    if (kind == "unreadable") {
        add(f, "not an object: " + m.strAt("error"));
        return;
    }
    if (kind == "comparison") {
        auto sign = [](double d) { return (d > 0 ? "+" : "") + std::to_string((long)d); };
        int s = add(f, "Sizes");
        for (const auto& g : arr(m, "segments"))
            add(s, pad(g.strAt("name"), 8) + pad(std::to_string((long)g.numAt("a")), 8) + "-> " + pad(std::to_string((long)g.numAt("b")), 8) +
                   "(" + sign(g.numAt("delta")) + ")");
        auto list = [&](const char* key, const char* title) {
            const auto& v = arr(m, key);
            int n = add(f, std::string(title) + " (" + std::to_string(v.size()) + ")");
            for (const auto& y : v)
                add(n, pad(y.strAt("name"), 28) + (y.at("change") ? y.strAt("change") : pad(y.strAt("seg"), 6) + y.strAt("attr")));
        };
        list("added", "Symbols added");
        list("removed", "Symbols removed");
        list("changed", "Symbols changed");
        int r = add(f, "Relocations changed (" + std::to_string(arr(m, "relocs").size()) + ")");
        for (const auto& y : arr(m, "relocs"))
            add(r, pad(y.strAt("name"), 28) + std::to_string((long)y.numAt("a")) + " -> " + std::to_string((long)y.numAt("b")));
        int fn = add(f, "Functions changed (" + std::to_string(arr(m, "functions").size()) + "; " +
                        std::to_string((long)m.numAt("unchanged")) + " unchanged)");
        for (const auto& y : arr(m, "functions")) {
            int n = add(fn, pad(y.strAt("name"), 28) + std::to_string((long)y.numAt("a")) + " -> " + std::to_string((long)y.numAt("b")) +
                            " instructions  (+" + std::to_string((long)y.numAt("added")) + " -" + std::to_string((long)y.numAt("removed")) + ")");
            for (const auto& l : arr(y, "lines")) add(n, l.str);
        }
        return;
    }
    int h = add(f, "Header");
    if (!m.strAt("format").empty()) {
        add(h, "Format: " + m.strAt("format"));
        add(h, "CPU: " + m.strAt("cpu"));
        add(h, "Mode: " + hx(m.numAt("mode")));
    }
    if (m.at("base")) add(h, "Loaded at " + hx(m.numAt("base")) + ", " + std::to_string((long)m.numAt("size")) + " bytes");
    if (const Json* st = m.at("stub"))
        add(h, "BASIC stub: " + std::to_string((long)st->numAt("line")) + " SYS " + std::to_string((long)st->numAt("sys")) +
               " (" + hx(st->numAt("sys")) + ")");
    if (!m.strAt("map").empty()) add(h, "Symbols from " + m.strAt("map"));
    if (m.at("region")) add(h, std::string("Overlays run at ") + hx(m.numAt("region")) + " (" + (m.boolAt("mapped") ? "mapped" : "copied") + ")");
    for (const auto& o : arr(m, "options"))
        add(h, "Option " + o.strAt("name") + ": " + (o.at("text") ? o.strAt("text") : std::to_string((long)o.numAt("size")) + " bytes"));
    if (!arr(m, "segments").empty()) {
        int s = add(f, "Segments (" + std::to_string(arr(m, "segments").size()) + ")");
        for (const auto& g : arr(m, "segments"))
            add(s, pad(g.strAt("name"), 6) + pad(g.strAt("type"), 6) + hx(g.numAt("base")) + "  " +
                   pad(std::to_string((long)g.numAt("size")) + " bytes", 12) + std::to_string((long)g.numAt("relocs")) + " relocations");
    }
    if (!arr(m, "sections").empty()) {
        int s = add(f, "Link sections (" + std::to_string(arr(m, "sections").size()) + ")");
        for (const auto& g : arr(m, "sections"))
            add(s, pad(g.strAt("seg"), 6) + hx(g.numAt("offset")) + "  " + pad(std::to_string((long)g.numAt("size")), 6) +
                   "align " + pad(std::to_string((long)g.numAt("align")), 4) + g.strAt("name") + (g.boolAt("keep") ? " [keep]" : ""));
    }
    if (!arr(m, "overlays").empty()) {
        int s = add(f, "Overlays (" + std::to_string(arr(m, "overlays").size()) + ")");
        for (const auto& o : arr(m, "overlays"))
            add(s, pad(o.strAt("name"), 16) + pad(std::to_string((long)o.numAt("size")) + " bytes", 12) + "stored at " + hx(o.numAt("at"), 5));
    }
    if (m.at("symbols")) {
        int s = add(f, "Symbols (" + std::to_string(arr(m, "symbols").size()) + ")");
        for (const auto& y : arr(m, "symbols")) {
            std::string seg = y.strAt("seg");
            std::string text = seg == "U" ? std::string(6, ' ') + pad("U", 6) + y.strAt("name")
                                          : hx(y.numAt("value")) + "  " + pad(seg, 6) + y.strAt("name");
            if (y.boolAt("weak")) text += " [weak]";
            int n = add(s, text);
            if (seg != "U" && y.at("value")) T[n].jumpAddr = (long)y.numAt("value");
            if (const Json* a = y.at("attr")) {
                add(n, "Convention: " + attrText(*a));
                add(n, "Parameters: " + std::to_string((int)a->numAt("params")) + " bytes, frame " + std::to_string((int)a->numAt("frame")));
                add(n, "Base page: uses " + a->strAt("zpUses") + ", clobbers " + a->strAt("zpClobbers") + ", releases " + a->strAt("zpRelease"));
            }
        }
        int fn = 0;
        for (const auto& y : arr(m, "symbols")) fn += y.at("attr") != nullptr;
        if (fn) {
            int ff = add(f, "Functions (" + std::to_string(fn) + ")");
            for (const auto& y : arr(m, "symbols")) {
                const Json* a = y.at("attr");
                if (!a) continue;
                int n = add(ff, pad(y.strAt("name"), 28) + attrText(*a));
                T[n].jumpAddr = (long)y.numAt("value");
            }
        }
    }
    if (m.at("relocs")) {
        int s = add(f, "Relocations (" + std::to_string(arr(m, "relocs").size()) + ")");
        for (const auto& r : arr(m, "relocs"))
            add(s, pad(r.strAt("seg"), 6) + hx(r.numAt("offset")) + "  " + pad(r.strAt("type"), 12) +
                   (r.at("symbol") ? r.strAt("symbol") : r.strAt("target")));
    }
    if (m.at("checks")) {
        const auto& cs = arr(m, "checks");
        int warn = 0;
        for (const auto& c : cs) warn += c.strAt("severity") == "warning";
        int s = add(f, "Checks (" + std::to_string(warn) + " warning" + (warn == 1 ? "" : "s") + ", " +
                       std::to_string(cs.size() - warn) + " note" + (cs.size() - warn == 1 ? "" : "s") + ")");
        for (const auto& c : cs) {
            int n = add(s, pad(c.strAt("severity"), 8) + pad(c.at("a") ? hx(c.numAt("a")) : "", 7) + c.strAt("message"));
            if (c.at("a")) T[n].jumpAddr = (long)c.numAt("a");
        }
    }
    for (const auto& sec : arr(m, "disasm")) buildDisasm(f, f, sec);
    for (const auto& d : arr(m, "data")) {
        std::string hex = d.strAt("hex");
        long base = (long)d.numAt("base");
        int s = add(f, "Data: " + d.strAt("name") + "  (" + std::to_string(hex.size() / 2) + " bytes at " + hx(base) + ")");
        for (size_t i = 0; i < hex.size(); i += 32) {
            std::string line = hx(base + (long)i / 2).substr(1) + "  ", asc;
            for (size_t j = i; j < i + 32 && j < hex.size(); j += 2) {
                line += hex.substr(j, 2) + " ";
                int c = std::stoi(hex.substr(j, 2), nullptr, 16);
                asc += c >= 32 && c < 127 ? (char)c : '.';
            }
            add(s, pad(line, 54) + asc);
        }
    }
}

// --- the call graph: from the disassembly nodes (jsr / jmp / bsr to a label) ---
void buildCalls() {
    for (int f = 0; f < (int)T.size(); f++) {
        if (T[f].file != f) continue;
        std::map<std::string, std::map<std::string, int>> calls;
        auto& at = g_at[f];
        for (int s : T[f].kids) {
            if (T[s].text.rfind("Disassembly:", 0) != 0) continue;
            for (int lab : T[s].kids) {
                size_t lt = T[lab].text.find('<'), gt = T[lab].text.find('>');
                if (lt == std::string::npos || gt == std::string::npos) continue;
                std::string fn = T[lab].text.substr(lt + 1, gt - lt - 1);
                for (int ins : T[lab].kids) {
                    const std::string& x = T[ins].text;
                    size_t mpos = x.find_first_not_of(' ', 26);
                    bool call = x.find("jsr ", 20) != std::string::npos || x.find("bsr ", 20) != std::string::npos ||
                                x.find("jmp ", 20) != std::string::npos;
                    if (!call || mpos == std::string::npos) continue;
                    std::string callee;
                    size_t semi = x.find("; <");
                    if (semi != std::string::npos) callee = x.substr(semi + 3, x.find('>', semi) - semi - 3);
                    else if (T[ins].jumpAddr >= 0 && at.count(T[ins].jumpAddr)) {
                        const std::string& t = T[at[T[ins].jumpAddr]].text;
                        size_t a = t.find('<'), b = t.find('>');
                        if (a != std::string::npos && b != std::string::npos) callee = t.substr(a + 1, b - a - 1);
                    }
                    if (!callee.empty() && (callee != fn || x.find("jsr ", 20) != std::string::npos)) calls[fn][callee]++;
                }
            }
        }
        if (calls.empty()) continue;
        std::map<std::string, std::vector<std::string>> by;
        for (const auto& [fn, k] : calls) for (const auto& [c, n] : k) by[c].push_back(fn);
        int g = add(f, "Call graph (" + std::to_string(calls.size()) + ")");
        for (const auto& [fn, k] : calls) {
            int n = add(g, fn);
            std::string list;
            for (const auto& [c, cnt] : k) list += (list.empty() ? "" : ", ") + c + (cnt > 1 ? " x" + std::to_string(cnt) : "");
            add(n, "calls: " + list);
            auto b = by.find(fn);
            if (b != by.end()) {
                std::string l2;
                for (const auto& c : b->second) l2 += (l2.empty() ? "" : ", ") + c;
                add(n, "called by: " + l2);
            }
        }
    }
}

void resolveJumps() {
    for (auto& n : T) {
        if (n.jumpAddr < 0) continue;
        auto f = g_at.find(n.file);
        if (f == g_at.end()) continue;
        auto t = f->second.find(n.jumpAddr);
        if (t != f->second.end()) n.jump = t->second;
    }
}

void printAll(int i, int depth) {
    printf("%s%s\n", std::string(depth * 2, ' ').c_str(), T[i].text.c_str());
    for (int k : T[i].kids) printAll(k, depth + 1);
}

// --- the terminal --------------------------------------------------------

struct termios g_saved;
volatile sig_atomic_t g_resized = 0;

void restore() {
    fputs("\x1b[0m\x1b[?25h\x1b[?1049l", stdout);
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_saved);
}
void onSignal(int sig) {
    restore();
    signal(sig, SIG_DFL);
    raise(sig);
}
void onResize(int) { g_resized = 1; }

int depthOf(int i) { int d = 0; while (T[i].parent >= 0) { i = T[i].parent; d++; } return d; }

void visible(const std::vector<int>& roots, std::vector<int>& out) {
    out.clear();
    std::vector<int> st(roots.rbegin(), roots.rend());
    while (!st.empty()) {
        int i = st.back();
        st.pop_back();
        out.push_back(i);
        if (T[i].open) for (auto it = T[i].kids.rbegin(); it != T[i].kids.rend(); ++it) st.push_back(*it);
    }
}

void reveal(int i) { for (int p = T[i].parent; p >= 0; p = T[p].parent) T[p].open = true; }

// the next node after `from` (in the whole tree's order) whose text holds `what`
int find(const std::vector<int>& roots, int from, const std::string& what, bool back) {
    std::vector<int> all;
    std::vector<int> st(roots.rbegin(), roots.rend());
    while (!st.empty()) {
        int i = st.back();
        st.pop_back();
        all.push_back(i);
        for (auto it = T[i].kids.rbegin(); it != T[i].kids.rend(); ++it) st.push_back(*it);
    }
    auto lower = [](std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; };
    std::string w = lower(what);
    size_t pos = std::find(all.begin(), all.end(), from) - all.begin();
    for (size_t k = 1; k <= all.size(); k++) {
        size_t j = back ? (pos + all.size() - k) % all.size() : (pos + k) % all.size();
        if (lower(T[all[j]].text).find(w) != std::string::npos) return all[j];
    }
    return -1;
}

int readKey() {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return -1;
    if (c != 0x1b) return c;
    unsigned char s[4] = {0};
    // (a lone Esc: nothing follows at once)
    struct timeval tv = {0, 50000};
    fd_set fs;
    FD_ZERO(&fs);
    FD_SET(STDIN_FILENO, &fs);
    if (select(STDIN_FILENO + 1, &fs, nullptr, nullptr, &tv) <= 0) return 27;
    if (read(STDIN_FILENO, &s[0], 1) != 1) return 27;
    if (s[0] != '[' && s[0] != 'O') return 27;
    if (read(STDIN_FILENO, &s[1], 1) != 1) return 27;
    switch (s[1]) {
        case 'A': return 1000; case 'B': return 1001; case 'C': return 1002; case 'D': return 1003;
        case 'H': return 1006; case 'F': return 1007;
    }
    if (s[1] >= '0' && s[1] <= '9') {
        if (read(STDIN_FILENO, &s[2], 1) != 1) return 27;
        if (s[2] == '~') {
            switch (s[1]) { case '5': return 1004; case '6': return 1005; case '1': case '7': return 1006; case '4': case '8': return 1007; }
        }
    }
    return 27;
}

std::string prompt(const std::string& label, int rows) {
    std::string s;
    for (;;) {
        printf("\x1b[%d;1H\x1b[0m\x1b[2K%s%s", rows, label.c_str(), s.c_str());
        fflush(stdout);
        int k = readKey();
        if (k == '\r' || k == '\n') return s;
        if (k == 27 || k == -1) return "";
        if ((k == 127 || k == 8) && !s.empty()) s.pop_back();
        else if (k >= 32 && k < 127) s += (char)k;
    }
}

int browse(const std::vector<int>& roots) {
    tcgetattr(STDIN_FILENO, &g_saved);
    struct termios raw = g_saved;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    signal(SIGTERM, onSignal);
    signal(SIGHUP, onSignal);
    signal(SIGWINCH, onResize);
    fputs("\x1b[?1049h\x1b[?25l", stdout);

    std::vector<int> vis;
    std::vector<int> back;   // Enter's jumps, for Backspace
    int cur = roots[0], top = 0;
    std::string lastFind, message;
    for (bool quit = false; !quit;) {
        struct winsize ws{};
        int rows = 24, cols = 80;
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 2) { rows = ws.ws_row; cols = ws.ws_col; }
        int page = rows - 2;
        visible(roots, vis);
        int ci = (int)(std::find(vis.begin(), vis.end(), cur) - vis.begin());
        if (ci >= (int)vis.size()) { ci = 0; cur = vis[0]; }
        if (ci < top) top = ci;
        if (ci >= top + page) top = ci - page + 1;
        std::string out = "\x1b[H";
        for (int r = 0; r < page; r++) {
            int k = top + r;
            out += "\x1b[2K";
            if (k < (int)vis.size()) {
                int i = vis[k];
                std::string line = std::string(depthOf(i) * 2, ' ') + (T[i].kids.empty() ? "  " : T[i].open ? "- " : "+ ") + T[i].text;
                if (T[i].jump >= 0) line += "  ->";
                if ((int)line.size() > cols) line = line.substr(0, cols);
                if (k == ci) out += "\x1b[7m" + pad(line, cols) + "\x1b[0m";
                else if (!T[i].kids.empty()) out += "\x1b[1m" + line + "\x1b[0m";
                else out += line;
            }
            out += "\r\n";
        }
        // the status line: where the cursor is, the keys
        std::string path;
        for (int p = T[cur].parent; p >= 0; p = T[p].parent) {
            std::string t = T[p].text.substr(0, std::min<size_t>(T[p].text.size(), 28));
            path = t + (path.empty() ? "" : " > " + path);
        }
        std::string status = message.empty() ? " " + std::to_string(ci + 1) + "/" + std::to_string(vis.size()) + "  " + path : " " + message;
        std::string keys = "  arrows move/open/close  Enter: go  BS: back  /: find  n N  q: quit ";
        if ((int)(status.size() + keys.size()) > cols) status = status.substr(0, std::max(0, cols - (int)keys.size()));
        out += "\x1b[2K\x1b[7m" + pad(status, std::max(0, cols - (int)keys.size())) + keys.substr(0, cols) + "\x1b[0m";
        fputs(out.c_str(), stdout);
        fflush(stdout);
        message.clear();

        int k = readKey();
        if (k == -1) { if (g_resized) { g_resized = 0; fputs("\x1b[2J", stdout); continue; } break; }
        Node& n = T[cur];
        switch (k) {
        case 'q': case 'Q': case 3: quit = true; break;
        case 1000: case 'k': if (ci > 0) cur = vis[ci - 1]; break;
        case 1001: case 'j': if (ci + 1 < (int)vis.size()) cur = vis[ci + 1]; break;
        case 1004: cur = vis[std::max(0, ci - page)]; top = std::max(0, top - page); break;
        case 1005: cur = vis[std::min((int)vis.size() - 1, ci + page)]; top += page; break;
        case 1006: case 'g': cur = vis[0]; break;
        case 1007: case 'G': cur = vis.back(); break;
        case 1002: case 'l':
            if (!n.kids.empty() && !n.open) n.open = true;
            else if (!n.kids.empty()) cur = n.kids[0];
            break;
        case 1003: case 'h':
            if (!n.kids.empty() && n.open) n.open = false;
            else if (n.parent >= 0) cur = n.parent;
            break;
        case ' ': if (!n.kids.empty()) n.open = !n.open; break;
        case '\r': case '\n':
            if (n.jump >= 0) { back.push_back(cur); cur = n.jump; reveal(cur); top = std::max(0, top); }
            else if (!n.kids.empty()) n.open = !n.open;
            else message = "nothing to go to";
            break;
        case 127: case 8: case 'b':
            if (!back.empty()) { cur = back.back(); back.pop_back(); reveal(cur); }
            break;
        case '*': {   // everything under it open
            std::vector<int> st{cur};
            while (!st.empty()) { int i = st.back(); st.pop_back(); T[i].open = true; for (int c : T[i].kids) st.push_back(c); }
            break;
        }
        case 'c': {   // everything under it closed
            std::vector<int> st{cur};
            while (!st.empty()) { int i = st.back(); st.pop_back(); T[i].open = false; for (int c : T[i].kids) st.push_back(c); }
            break;
        }
        case '/': {
            std::string w = prompt("/", rows);
            if (!w.empty()) lastFind = w;
            [[fallthrough]];
        }
        case 'n': case 'N': {
            if (lastFind.empty()) break;
            int f = find(roots, cur, lastFind, k == 'N');
            if (f < 0) message = "not found: " + lastFind;
            else { cur = f; reveal(f); }
            break;
        }
        default: break;
        }
    }
    restore();
    return 0;
}

}  // namespace

// --tree: the models (objdump45 --json's) as a tree; on a terminal to browse
int objdump45Tree(const Json& models) {
    T.clear();
    g_at.clear();
    std::vector<int> roots;
    for (const auto& m : models.arr) {
        size_t before = T.size();
        buildFile(-1, m);
        roots.push_back((int)before);
    }
    buildCalls();
    resolveJumps();
    if (roots.empty()) return 0;
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        for (int r : roots) printAll(r, 0);
        return 0;
    }
    for (int r : roots) T[r].open = true;
    return browse(roots);
}
