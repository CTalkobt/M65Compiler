// The linters' shared core (#199, #278): see include/Lint.hpp and
// doc/architecture/lint.md.
#include "Lint.hpp"

#include <algorithm>
#include <cctype>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <ostream>
#include <sstream>

namespace lint {

const char* levelName(Level l) {
    switch (l) {
        case Level::Off: return "off";
        case Level::Info: return "info";
        case Level::Warning: return "warning";
        case Level::Error: return "error";
    }
    return "warning";
}

bool parseLevel(const std::string& s, Level& out) {
    if (s == "off" || s == "none") { out = Level::Off; return true; }
    if (s == "info" || s == "note") { out = Level::Info; return true; }
    if (s == "warning" || s == "warn") { out = Level::Warning; return true; }
    if (s == "error") { out = Level::Error; return true; }
    return false;
}

bool globMatch(const std::string& pat, const std::string& s) {
    // (iterative * matching)
    size_t p = 0, i = 0, star = std::string::npos, mark = 0;
    while (i < s.size()) {
        if (p < pat.size() && pat[p] == '*') { star = p++; mark = i; }
        else if (p < pat.size() && pat[p] == s[i]) { p++; i++; }
        else if (star != std::string::npos) { p = star + 1; i = ++mark; }
        else return false;
    }
    while (p < pat.size() && pat[p] == '*') p++;
    return p == pat.size();
}

// --- YAML subset ---

const Yaml* Yaml::get(const std::string& key) const {
    for (const auto& kv : map)
        if (kv.first == key) return &kv.second;
    return nullptr;
}

namespace {

struct YLine { int indent; std::string text; int no; };

// a comment starts at a # outside quotes, at the start or after a space
std::string stripComment(const std::string& s) {
    char q = 0;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (q) { if (c == q) q = 0; continue; }
        if (c == '"' || c == '\'') { q = c; continue; }
        if (c == '#' && (i == 0 || s[i - 1] == ' ' || s[i - 1] == '\t')) return s.substr(0, i);
    }
    return s;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r"), b = s.find_last_not_of(" \t\r");
    return a == std::string::npos ? "" : s.substr(a, b - a + 1);
}

bool unquote(const std::string& in, std::string& out, std::string& err) {
    std::string s = trim(in);
    if (s.size() >= 2 && s[0] == '"' && s.back() == '"') {
        out.clear();
        for (size_t i = 1; i + 1 < s.size(); i++) {
            if (s[i] == '\\' && i + 2 < s.size()) {
                char n = s[++i];
                out += n == 'n' ? '\n' : n == 't' ? '\t' : n;
            } else out += s[i];
        }
        return true;
    }
    if (s.size() >= 2 && s[0] == '\'' && s.back() == '\'') {
        out.clear();
        for (size_t i = 1; i + 1 < s.size(); i++) {
            if (s[i] == '\'' && i + 2 < s.size() && s[i + 1] == '\'') i++;
            out += s[i];
        }
        return true;
    }
    if (!s.empty() && (s[0] == '"' || s[0] == '\'')) { err = "unterminated quoted string"; return false; }
    if (!s.empty() && (s[0] == '[' || s[0] == '{')) { err = "flow collections ([...], {...}) are not supported"; return false; }
    out = s;
    return true;
}

// the ": " (or a final ':') separating a key outside quotes
size_t keyColon(const std::string& s) {
    char q = 0;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (q) { if (c == q) q = 0; continue; }
        if (c == '"' || c == '\'') { q = c; continue; }
        if (c == ':' && (i + 1 == s.size() || s[i + 1] == ' ' || s[i + 1] == '\t')) return i;
    }
    return std::string::npos;
}

struct YParser {
    std::vector<YLine> lines;
    size_t i = 0;
    std::string err;

    bool fail(int no, const std::string& m) { if (err.empty()) err = "line " + std::to_string(no) + ": " + m; return false; }

    bool block(int indent, Yaml& out) {
        if (i >= lines.size() || lines[i].indent < indent) { out.kind = Yaml::Scalar; return true; }
        indent = lines[i].indent;
        out.line = lines[i].no;
        bool isList = lines[i].text == "-" || lines[i].text.compare(0, 2, "- ") == 0;
        out.kind = isList ? Yaml::List : Yaml::Map;
        while (i < lines.size() && lines[i].indent == indent) {
            YLine& l = lines[i];
            bool item = l.text == "-" || l.text.compare(0, 2, "- ") == 0;
            if (item != isList) return fail(l.no, isList ? "a map entry among list items" : "a list item among map entries");
            if (isList) {
                Yaml el;
                std::string rest = l.text == "-" ? "" : l.text.substr(2);
                size_t off = l.text.size() - trim(rest).size();
                rest = trim(rest);
                if (rest.empty()) {
                    i++;
                    if (!block(indent + 1, el)) return false;
                } else if (keyColon(rest) != std::string::npos) {
                    // "- key: v": a map at the key's column
                    l.indent += (int)off;
                    l.text = rest;
                    if (!block(l.indent, el)) return false;
                } else {
                    if (!unquote(rest, el.value, err)) return fail(l.no, err);
                    el.line = l.no;
                    i++;
                }
                out.list.push_back(std::move(el));
                continue;
            }
            size_t c = keyColon(l.text);
            if (c == std::string::npos) return fail(l.no, "expected 'key: value'");
            std::string key;
            if (!unquote(l.text.substr(0, c), key, err)) return fail(l.no, err);
            std::string val = trim(l.text.substr(c + 1));
            Yaml v;
            v.line = l.no;
            i++;
            if (val.empty()) {
                if (i < lines.size() && lines[i].indent > indent) {
                    if (!block(lines[i].indent, v)) return false;
                } else if (i < lines.size() && lines[i].indent == indent &&
                           (lines[i].text == "-" || lines[i].text.compare(0, 2, "- ") == 0)) {
                    // (a list at the key's own indentation, as YAML allows)
                    if (!block(indent, v)) return false;
                }
            } else if (!unquote(val, v.value, err)) {
                return fail(v.line, err);
            }
            for (const auto& kv : out.map)
                if (kv.first == key) return fail(v.line, "duplicate key '" + key + "'");
            out.map.emplace_back(key, std::move(v));
        }
        if (i < lines.size() && lines[i].indent > indent) return fail(lines[i].no, "unexpected indentation");
        return true;
    }
};

}  // namespace

bool parseYaml(const std::string& text, Yaml& out, std::string& err) {
    YParser p;
    std::istringstream in(text);
    std::string l;
    int no = 0;
    while (std::getline(in, l)) {
        no++;
        if (l.find('\t') != std::string::npos && l.find_first_not_of(" \t") != std::string::npos &&
            l.find('\t') < l.find_first_not_of(" \t")) {
            err = "line " + std::to_string(no) + ": a tab in the indentation";
            return false;
        }
        std::string s = stripComment(l);
        if (trim(s).empty()) continue;
        if (trim(s) == "---" || trim(s) == "...") continue;
        int ind = (int)s.find_first_not_of(' ');
        std::string t = s.substr(ind);
        while (!t.empty() && (t.back() == ' ' || t.back() == '\r' || t.back() == '\t')) t.pop_back();
        p.lines.push_back({ind, t, no});
    }
    out = Yaml();
    if (p.lines.empty()) { out.kind = Yaml::Map; return true; }
    if (!p.block(0, out) || p.i < p.lines.size()) {
        err = p.err.empty() ? "line " + std::to_string(p.lines[p.i].no) + ": unexpected text" : p.err;
        return false;
    }
    return true;
}

// --- configuration ---

namespace {
std::vector<std::string> splitList(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ',') { if (!trim(cur).empty()) out.push_back(trim(cur)); cur.clear(); }
        else cur += c;
    }
    if (!trim(cur).empty()) out.push_back(trim(cur));
    return out;
}
}  // namespace

// --- user-written rules (#765) ---

namespace {
bool isWordChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }
}  // namespace

bool parsePatternStep(const std::string& text, PatternStep& out, std::string& err) {
    out = PatternStep();
    out.text = trim(text);
    if (out.text.empty()) { err = "an empty step"; return false; }
    size_t sp = out.text.find_first_of(" \t");
    std::string mn = out.text.substr(0, sp);
    std::string op = sp == std::string::npos ? "" : trim(out.text.substr(sp));
    std::string cur;
    for (size_t i = 0; i <= mn.size(); i++) {
        if (i < mn.size() && mn[i] != '|') { cur += (char)std::tolower((unsigned char)mn[i]); continue; }
        bool ok = !cur.empty() && std::all_of(cur.begin(), cur.end(), [](char c) {
                      return std::isalnum((unsigned char)c) || c == '*' || c == '.';
                  });
        if (!ok) { err = "'" + mn + "': a mnemonic (* any letters: ld*), or several separated by |"; return false; }
        out.mnemonics.push_back(cur);
        cur.clear();
    }
    for (char c : op)
        if (c != ' ' && c != '\t') out.operand += (char)std::tolower((unsigned char)c);
    out.anyOperand = out.operand.empty();
    // {name}: letters, digits, _
    for (size_t i = 0; i < out.operand.size(); i++) {
        char c = out.operand[i];
        if (c == '}') { err = "'" + out.text + "': a } with no {"; return false; }
        if (c != '{') continue;
        size_t e = out.operand.find('}', i);
        std::string name = e == std::string::npos ? "" : out.operand.substr(i + 1, e - i - 1);
        if (e == std::string::npos || name.empty() || !std::all_of(name.begin(), name.end(), isWordChar)) {
            err = "'" + out.text + "': a capture is {name} (letters, digits, _)";
            return false;
        }
        i = e;
    }
    return true;
}

namespace {
bool matchAt(const std::string& pat, size_t p, const std::string& s, size_t i, std::map<std::string, std::string>& caps) {
    auto eq = [](char a, char b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); };
    while (p < pat.size()) {
        char c = pat[p];
        if (c == '*') {
            for (size_t k = s.size() + 1; k-- > i;)   // (longest first)
                if (matchAt(pat, p + 1, s, k, caps)) return true;
            return false;
        }
        if (c == '{') {
            size_t e = pat.find('}', p);
            std::string name = pat.substr(p + 1, e - p - 1);
            auto it = caps.find(name);
            if (it != caps.end()) {   // (bound: the same text)
                const std::string v = it->second;
                if (s.size() - i < v.size()) return false;
                for (size_t k = 0; k < v.size(); k++)
                    if (!eq(v[k], s[i + k])) return false;
                i += v.size();
                p = e + 1;
                continue;
            }
            for (size_t k = s.size(); k > i; k--) {
                caps[name] = s.substr(i, k - i);
                if (matchAt(pat, e + 1, s, k, caps)) return true;
            }
            caps.erase(name);
            return false;
        }
        if (i >= s.size() || !eq(c, s[i])) return false;
        p++;
        i++;
    }
    return i == s.size();
}
}  // namespace

bool matchOperand(const std::string& pat, const std::string& text, std::map<std::string, std::string>& caps) {
    std::map<std::string, std::string> keep = caps;
    if (matchAt(pat, 0, text, 0, caps)) return true;
    caps = keep;
    return false;
}

bool Config::patternRule(const Yaml& r, const std::string& file, std::set<std::string>& idsHere, std::string& err) {
    auto bad = [&](const Yaml& at, const std::string& m) {
        err = file + ":" + std::to_string(at.line) + ": " + m;
        return false;
    };
    PatternRule pr;
    const Yaml& idv = *r.get("id");
    pr.id = idv.value;
    pr.file = file;
    pr.line = idv.line;
    if (pr.id.empty() || !std::all_of(pr.id.begin(), pr.id.end(), [](char c) {
            return std::isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.';
        }))
        return bad(idv, "a user-written rule's id: letters, digits, '-', '_', '.' ('" + pr.id + "')");
    if (pr.id.compare(0, 7, "mega65-") == 0 || knownRules().count(pr.id))
        return bad(idv, "'" + pr.id + "': the mega65- ids are the built-in rules'; a user-written rule needs an id of "
                        "its own (a project prefix: myproj-...)");
    if (!idsHere.insert(pr.id).second) return bad(idv, "a second user-written rule '" + pr.id + "' in this file");
    for (const auto& [k, v] : r.map) {
        if (k == "id") continue;
        if (k == "severity") {
            if (v.kind != Yaml::Scalar || !parseLevel(v.value, pr.level)) return bad(v, "severity: off, info, warning or error");
        } else if (k == "message") {
            if (v.kind != Yaml::Scalar || v.value.empty()) return bad(v, "message: the text to report");
            pr.message = v.value;
        } else if (k == "pattern") {
            std::vector<std::pair<std::string, int>> steps;
            if (v.kind == Yaml::Scalar) {
                std::string cur;
                for (size_t i = 0; i <= v.value.size(); i++) {
                    if (i == v.value.size() || v.value[i] == ';') { steps.push_back({cur, v.line}); cur.clear(); }
                    else cur += v.value[i];
                }
            } else if (v.kind == Yaml::List) {
                for (const auto& el : v.list) {
                    if (el.kind != Yaml::Scalar) return bad(el, "pattern: each step an instruction (\"lda #0\")");
                    steps.push_back({el.value, el.line});
                }
            } else {
                return bad(v, "pattern: a list of instructions, or one string with ; between them");
            }
            if (steps.empty() || (steps.size() == 1 && trim(steps[0].first).empty()))
                return bad(v, "pattern: no instructions");
            if (steps.size() > 32) return bad(v, "pattern: more than 32 instructions");
            for (const auto& [t, ln] : steps) {
                PatternStep st;
                std::string e;
                if (!parsePatternStep(t, st, e)) {
                    err = file + ":" + std::to_string(ln) + ": pattern: " + e;
                    return false;
                }
                pr.steps.push_back(st);
            }
        } else {
            return bad(v, "unknown key '" + k + "' in a user-written rule (id, severity, message, pattern)");
        }
    }
    if (pr.message.empty()) return bad(idv, "user-written rule '" + pr.id + "' needs a message");
    // (the message's {name}s must be the pattern's captures)
    for (size_t i = pr.message.find('{'); i != std::string::npos; i = pr.message.find('{', i + 1)) {
        size_t e = pr.message.find('}', i);
        if (e == std::string::npos) break;
        std::string name = pr.message.substr(i + 1, e - i - 1);
        if (name.empty() || !std::all_of(name.begin(), name.end(), isWordChar)) continue;
        bool found = false;
        for (const auto& st : pr.steps) found = found || st.operand.find("{" + name + "}") != std::string::npos;
        if (!found) return bad(*r.get("message"), "message: {" + name + "} is not a capture of the pattern");
    }
    for (auto& old : patterns)
        if (old.id == pr.id) { old = pr; return true; }   // (a later file's rule replaces an earlier one's)
    patterns.push_back(pr);
    return true;
}

void Config::addPatternRules(Engine& e) const {
    for (const auto& pr : patterns) {
        if (e.find(pr.id)) continue;
        Rule r;
        r.id = pr.id;
        r.level = pr.level;
        r.profile = "all";
        r.summary = pr.message;
        r.explanation = "A user-written rule (" + pr.file + ":" + std::to_string(pr.line) + "): the instructions";
        for (const auto& st : pr.steps) r.explanation += "\n    " + st.text;
        e.add(r);
    }
}

bool Config::apply(const Yaml& y, const std::string& file, std::string& err) {
    auto bad = [&](const Yaml& at, const std::string& m) {
        err = file + ":" + std::to_string(at.line) + ": " + m;
        return false;
    };
    if (y.kind != Yaml::Map) return bad(y, "the configuration must be a map");
    // (the user-written rules' ids first: overrides and the like before them know them)
    if (const Yaml* ar = y.get("asm_rules"))
        for (const auto& r : ar->list)
            if (r.kind == Yaml::Map && r.get("id") && r.get("pattern")) userIds_.insert(r.get("id")->value);
    for (const auto& [key, v] : y.map) {
        if (key == "profile") {
            if (v.kind != Yaml::Scalar || (v.value != "cc45" && v.value != "standalone"))
                return bad(v, "profile: cc45 or standalone");
            profile = v.value;
        } else if (key == "c_checks") {
            if (v.kind != Yaml::Map) return bad(v, "c_checks: a map (enabled, options)");
            for (const auto& [k2, v2] : v.map) {
                if (k2 == "enabled") {
                    if (v2.kind != Yaml::Scalar) return bad(v2, "c_checks.enabled: a list of rule globs, \"mega65-*,-mega65-x\"");
                    for (const auto& g : splitList(v2.value)) {
                        ruleGlob(file + ":" + std::to_string(v2.line) + ": c_checks.enabled", g);
                        cChecks.push_back(g);
                    }
                } else if (k2 == "options") {
                    if (v2.kind != Yaml::Map) return bad(v2, "c_checks.options: a map");
                    for (const auto& [k3, v3] : v2.map) {
                        if (v3.kind != Yaml::Scalar) return bad(v3, "an option's value must be a scalar");
                        option(file + ":" + std::to_string(v3.line) + ": c_checks.options", k3, v3);
                    }
                } else if (k2 == "pattern" || k2 == "patterns") {
                    return bad(v2, "c_checks: user-written rules (pattern:) are al45's only, in asm_rules; cl45 has none");
                } else return bad(v2, "unknown key 'c_checks." + k2 + "'");
            }
        } else if (key == "asm_rules") {
            if (v.kind != Yaml::List) return bad(v, "asm_rules: a list of rules (- id: ...)");
            std::set<std::string> idsHere;
            for (const auto& r : v.list) {
                if (r.kind != Yaml::Map || !r.get("id")) return bad(r, "an asm_rules entry needs an id");
                std::string id = r.get("id")->value;
                if (r.get("pattern")) {
                    if (!patternRule(r, file, idsHere, err)) return false;
                    continue;
                }
                ruleId(file + ":" + std::to_string(r.get("id")->line) + ": asm_rules", id);
                for (const auto& [k2, v2] : r.map) {
                    if (k2 == "id") continue;
                    if (k2 == "severity") {
                        Level l;
                        if (!parseLevel(v2.value, l)) return bad(v2, "severity: off, info, warning or error");
                        levels[id] = l;
                    } else if (k2 == "message") {
                        messages[id] = v2.value;
                    } else if (k2 == "patterns") {
                        return bad(v2, "unknown key 'patterns' in asm_rules (a user-written rule's key is pattern:)");
                    } else if (k2 == "options") {
                        if (v2.kind != Yaml::Map) return bad(v2, "options: a map");
                        for (const auto& [k3, v3] : v2.map)
                            option(file + ":" + std::to_string(v3.line) + ": asm_rules options", k3, v3);
                    } else return bad(v2, "unknown key '" + k2 + "' in asm_rules");
                }
            }
        } else if (key == "overrides") {
            if (v.kind != Yaml::Map) return bad(v, "overrides: a map of rule id: severity");
            for (const auto& [id, lv] : v.map) {
                Level l;
                if (lv.kind != Yaml::Scalar || !parseLevel(lv.value, l)) return bad(lv, "overrides." + id + ": off, info, warning or error");
                ruleId(file + ":" + std::to_string(lv.line) + ": overrides", id);
                levels[id] = l;
            }
        } else if (key == "options") {
            if (v.kind != Yaml::Map) return bad(v, "options: a map");
            for (const auto& [k3, v3] : v.map) option(file + ":" + std::to_string(v3.line) + ": options", k3, v3);
        } else {
            return bad(v, "unknown key '" + key + "' (profile, c_checks, asm_rules, overrides, options)");
        }
    }
    sources.push_back(file);
    return true;
}

bool Config::loadFile(const std::string& path, std::string& err) {
    std::ifstream f(path);
    if (!f) { err = path + ": cannot be read"; return false; }
    std::stringstream ss;
    ss << f.rdbuf();
    Yaml y;
    std::string e;
    if (!parseYaml(ss.str(), y, e)) { err = path + ": " + e; return false; }
    return apply(y, path, err);
}

bool Config::loadDefaults(const std::string& dir, std::string& err) {
    if (const char* home = std::getenv("HOME")) {
        std::string user = std::string(home) + "/.config/m65/cc45-lint.yml";
        if (std::ifstream(user) && !loadFile(user, err)) return false;
    }
    std::string d = dir.empty() ? "." : dir;
    for (int depth = 0; depth < 64; depth++) {
        std::string f = d + "/.cc45-lint.yml";
        if (std::ifstream(f)) return loadFile(f, err);
        // up one directory (by path text; stop at the root)
        std::string abs = d;
        if (abs == "/" ) break;
        if (abs == "." || abs.compare(0, 2, "./") == 0 || abs[0] != '/') {
            char* rp = realpath(abs.c_str(), nullptr);
            if (!rp) break;
            abs = rp;
            free(rp);
        }
        size_t s = abs.find_last_of('/');
        if (s == std::string::npos || abs == "/") break;
        d = s == 0 ? "/" : abs.substr(0, s);
        if (abs == d) break;
    }
    return true;
}

bool Config::applyOption(const std::string& arg, std::string& err) {
    auto val = [&](const char* p) { return arg.substr(std::string(p).size()); };
    if (arg.rfind("--checks=", 0) == 0) {
        for (const auto& g : splitList(val("--checks="))) {
            ruleGlob("--checks", g);
            cliChecks.push_back(g);
        }
    } else if (arg.rfind("--disable=", 0) == 0) {
        for (const auto& g : splitList(val("--disable="))) {
            ruleGlob("--disable", g);
            cliChecks.push_back("-" + g);
        }
    } else if (arg.rfind("--profile=", 0) == 0) {
        std::string p = val("--profile=");
        if (p != "cc45" && p != "standalone") { err = "--profile: cc45 or standalone"; return false; }
        profile = p;
    } else if (arg.rfind("--severity=", 0) == 0) {
        std::string s = val("--severity=");
        size_t eq = s.find('=');
        Level l;
        if (eq == std::string::npos || !parseLevel(s.substr(eq + 1), l)) { err = "--severity=ID=off|info|warning|error"; return false; }
        ruleId("--severity", s.substr(0, eq));
        levels[s.substr(0, eq)] = l;
    } else {
        err = "unknown option " + arg;
        return false;
    }
    return true;
}

// What the configuration names that nothing reads (#708): a warning, the rest
// of the file applied. Rule ids and options are al45's and cl45's together
// (LintRules.cpp): a file names both tools' rules.
void Config::option(const std::string& where, const std::string& key, const Yaml& v) {
    const OptionInfo* info = nullptr;
    for (const auto& o : knownOptions())
        if (key == o.id) info = &o;
    if (!info) {
        warnings.push_back(where + ": unknown option '" + key + "' ignored");
        return;
    }
    if (info->number && (v.value.empty() || v.value.find_first_not_of("0123456789") != std::string::npos)) {
        warnings.push_back(where + ": " + key + ": '" + v.value + "' is not a number; " + info->dflt + " is used");
        options.erase(key);   // (the default, over what an earlier file set: as the warning says)
        return;
    }
    options[key] = v.value;
}

bool Config::known(const std::string& id) const { return knownRules().count(id) || userIds_.count(id); }

void Config::ruleId(const std::string& where, const std::string& id) {
    if (!known(id)) warnings.push_back(where + ": unknown rule '" + id + "' ignored");
}

void Config::ruleGlob(const std::string& where, const std::string& glob) {
    std::string g = !glob.empty() && glob[0] == '-' ? glob.substr(1) : glob;
    if (g.find('*') == std::string::npos) return ruleId(where, g);
    for (const auto& id : knownRules())
        if (globMatch(g, id)) return;
    for (const auto& id : userIds_)
        if (globMatch(g, id)) return;
    warnings.push_back(where + ": '" + g + "' matches no rule");
}

void Config::printWarnings(std::ostream& os, const std::string& tool) {
    for (; warningsPrinted_ < warnings.size(); warningsPrinted_++)
        os << tool << ": warning: " << warnings[warningsPrinted_] << "\n";
    os.flush();
}

// --- the engine ---

void Engine::add(const Rule& r) { rules_.push_back(r); }

const Rule* Engine::find(const std::string& id) const {
    for (const auto& r : rules_)
        if (r.id == id) return &r;
    return nullptr;
}

Level Engine::levelOf(const std::string& id) const {
    const Rule* r = find(id);
    if (!r) return Level::Off;
    if (r->profile != "all" && r->profile != profile) return Level::Off;
    bool on = r->level != Level::Off;
    auto applyGlobs = [&](const std::vector<std::string>& gs) {
        for (const auto& g : gs) {
            bool neg = !g.empty() && g[0] == '-';
            if (globMatch(neg ? g.substr(1) : g, id)) on = !neg;
        }
    };
    if (lang == "c") applyGlobs(config.cChecks);
    auto lv = config.levels.find(id);
    if (lv != config.levels.end()) on = lv->second != Level::Off;   // (a severity in the files enables, off disables)
    applyGlobs(config.cliChecks);
    if (!on) return Level::Off;
    if (lv != config.levels.end() && lv->second != Level::Off) return lv->second;
    return r->level == Level::Off ? Level::Warning : r->level;
}

std::string Engine::option(const std::string& id, const std::string& dflt) const {
    auto it = config.options.find(id);
    return it == config.options.end() ? dflt : it->second;
}

void Engine::scanSuppressions(const std::string& file, const std::string& text) {
    std::istringstream in(text);
    std::string l;
    int no = 0;
    while (std::getline(in, l)) {
        no++;
        size_t at = l.find("NOLINT");
        if (at == std::string::npos) continue;
        // (in a comment: after ";", "//" or "/*")
        size_t c1 = l.find(';'), c2 = l.find("//"), c3 = l.find("/*");
        size_t c = std::min({c1, c2, c3});
        if (c == std::string::npos || c > at) continue;
        bool next = l.compare(at, 14, "NOLINTNEXTLINE") == 0;
        size_t p = at + (next ? 14 : 6);
        std::set<std::string> ids;
        if (p < l.size() && l[p] == '(') {
            size_t e = l.find(')', p);
            for (const auto& id : splitList(l.substr(p + 1, (e == std::string::npos ? l.size() : e) - p - 1))) ids.insert(id);
        }
        if (ids.empty()) ids.insert("*");
        auto& set = nolint_[file][next ? no + 1 : no];
        set.insert(ids.begin(), ids.end());
    }
}

void Engine::suppressFrom(const std::string& file, int line, const std::string& id, bool restore) {
    auto& rs = ranges_[file];
    if (restore) {
        for (auto& r : rs)
            if ((r.id == id || id == "*") && r.to == 1 << 30) r.to = line;
        return;
    }
    rs.push_back({line, 1 << 30, id});
}

bool Engine::suppressed(const std::string& id, const std::string& file, int line) const {
    auto f = nolint_.find(file);
    if (f != nolint_.end()) {
        auto l = f->second.find(line);
        if (l != f->second.end() && (l->second.count("*") || l->second.count(id))) return true;
    }
    auto r = ranges_.find(file);
    if (r != ranges_.end())
        for (const auto& x : r->second)
            if (line >= x.from && line < x.to && (x.id == id || x.id == "*" || globMatch(x.id, id))) return true;
    return false;
}

void Engine::report(const std::string& id, const std::string& file, int line, int col, const std::string& message) {
    Level l = levelOf(id);
    if (l == Level::Off || suppressed(id, file, line)) return;
    auto m = config.messages.find(id);
    diags_.push_back({file, line, col < 1 ? 1 : col, l, id, m != config.messages.end() ? m->second : message});
}

bool Engine::anyError() const {
    for (const auto& d : diags_)
        if (d.level == Level::Error) return true;
    return false;
}

namespace {
std::string jstr(const std::string& s) {
    std::string o = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\t': o += "\\t"; break;
            case '\r': o += "\\r"; break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += (char)c;
        }
    }
    return o + "\"";
}
const char* sarifLevel(Level l) {
    return l == Level::Error ? "error" : l == Level::Warning ? "warning" : "note";
}
}  // namespace

void Engine::write(std::ostream& os, const std::string& format, const std::string& tool) const {
    std::vector<Diagnostic> ds = diags_;
    std::stable_sort(ds.begin(), ds.end(), [](const Diagnostic& a, const Diagnostic& b) {
        if (a.file != b.file) return a.file < b.file;
        if (a.line != b.line) return a.line < b.line;
        return a.col < b.col;
    });
    if (format == "json") {
        os << "[";
        for (size_t i = 0; i < ds.size(); i++) {
            const auto& d = ds[i];
            os << (i ? ",\n " : "\n ") << "{\"file\": " << jstr(d.file) << ", \"line\": " << d.line << ", \"column\": " << d.col
               << ", \"severity\": \"" << levelName(d.level) << "\", \"rule\": " << jstr(d.ruleId) << ", \"message\": " << jstr(d.message) << "}";
        }
        os << (ds.empty() ? "]\n" : "\n]\n");
        return;
    }
    if (format == "sarif") {
        os << "{\n  \"$schema\": \"https://json.schemastore.org/sarif-2.1.0.json\",\n  \"version\": \"2.1.0\",\n  \"runs\": [{\n"
           << "    \"tool\": {\"driver\": {\"name\": " << jstr(tool) << ", \"informationUri\": \"https://github.com/CTalkobt/M65Compiler\",\n"
           << "      \"rules\": [";
        bool first = true;
        for (const auto& r : rules_) {
            if (levelOf(r.id) == Level::Off) continue;
            os << (first ? "\n        " : ",\n        ") << "{\"id\": " << jstr(r.id) << ", \"shortDescription\": {\"text\": " << jstr(r.summary)
               << "}, \"fullDescription\": {\"text\": " << jstr(r.explanation.empty() ? r.summary : r.explanation)
               << "}, \"defaultConfiguration\": {\"level\": \"" << sarifLevel(levelOf(r.id)) << "\"}}";
            first = false;
        }
        os << "]}},\n    \"results\": [";
        for (size_t i = 0; i < ds.size(); i++) {
            const auto& d = ds[i];
            os << (i ? ",\n      " : "\n      ") << "{\"ruleId\": " << jstr(d.ruleId) << ", \"level\": \"" << sarifLevel(d.level)
               << "\", \"message\": {\"text\": " << jstr(d.message) << "}, \"locations\": [{\"physicalLocation\": {\"artifactLocation\": {\"uri\": "
               << jstr(d.file) << "}, \"region\": {\"startLine\": " << d.line << ", \"startColumn\": " << d.col << "}}}]}";
        }
        os << "]\n  }]\n}\n";
        return;
    }
    for (const auto& d : ds)
        os << d.file << ":" << d.line << ":" << d.col << ": " << levelName(d.level) << ": " << d.message << " [" << d.ruleId << "]\n";
}

void Engine::listRules(std::ostream& os) const {
    size_t w = 0;
    for (const auto& r : rules_) w = std::max(w, r.id.size());
    for (const auto& r : rules_) {
        Level now = levelOf(r.id);
        os << r.id << std::string(w + 2 - r.id.size(), ' ') << levelName(now);
        os << std::string(9 - std::string(levelName(now)).size(), ' ') << (r.profile == "all" ? "" : r.profile + ": ") << r.summary << "\n";
    }
}

bool Engine::explain(std::ostream& os, const std::string& id) const {
    const Rule* r = find(id);
    if (!r) return false;
    os << r->id << " (default " << levelName(r->level) << (r->profile == "all" ? "" : ", profile " + r->profile) << ")\n  "
       << r->summary << "\n";
    if (!r->explanation.empty()) os << "\n" << r->explanation << "\n";
    return true;
}

}  // namespace lint
