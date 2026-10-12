#include "BasicVarRenamer.hpp"
#include <algorithm>
#include <cctype>
#include <set>

namespace {

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// A variable token: an identifier that is not a hex digit string after '$'
bool isVariable(const std::vector<BasicToken>& toks, size_t i) {
    if (toks[i].type != BasicToken::IDENTIFIER) return false;
    if (i > 0 && toks[i - 1].type == BasicToken::OPERATOR &&
        (toks[i - 1].value == "$" || toks[i - 1].value == "%"))
        return false;   // $FF, %1010
    const std::string& v = toks[i].value;
    return !v.empty() && std::isalpha((unsigned char)v[0]);
}

// The name without its $ suffix (the % suffix is a separate token)
std::string baseOf(const std::string& v) {
    return (!v.empty() && v.back() == '$') ? v.substr(0, v.size() - 1) : v;
}

// Reserved variables, and every keyword of one or two characters (TO IF ON
// OR GO FN DO ...): such a name would be read as the keyword when the line
// is listed and entered again
bool forbidden(const std::string& s) {
    static const std::set<std::string> bad = [] {
        std::set<std::string> b = {"ST", "TI", "DS", "ER", "EL"};
        BasicTokenizer t;
        for (const auto& [kw, tok] : t.getKeywords()) if (kw.size() <= 2) b.insert(kw);
        for (const auto& [kw, tok] : t.getEscapeKeywords()) if (kw.size() <= 2) b.insert(kw);
        return b;
    }();
    return bad.count(s) > 0;
}

}  // namespace

void BasicVarRenamer::rename(std::vector<std::vector<BasicToken>>& lines) {
    map_.clear();

    // Short names the program uses itself (any suffix: SC and SC$ are
    // separate variables, but a generated name stays clear of both)
    std::set<std::string> used;
    for (const auto& toks : lines)
        for (size_t i = 0; i < toks.size(); i++)
            if (isVariable(toks, i)) {
                std::string b = upper(baseOf(toks[i].value));
                if (b.size() <= 2) used.insert(b);
            }

    auto allocate = [&](const std::string& base) -> std::string {
        std::vector<std::string> tries;
        tries.push_back(base.substr(0, 2));
        for (char c : base.substr(2))
            if (std::isalnum((unsigned char)c)) tries.push_back(std::string(1, base[0]) + c);
        for (char d = '0'; d <= '9'; d++) tries.push_back(std::string(1, base[0]) + d);
        tries.push_back(base.substr(0, 1));
        for (char a = 'A'; a <= 'Z'; a++) {
            tries.push_back(std::string(1, a));
            for (char b = 'A'; b <= 'Z'; b++) tries.push_back(std::string(1, a) + b);
            for (char b = '0'; b <= '9'; b++) tries.push_back(std::string(1, a) + b);
        }
        for (const auto& t : tries)
            if (!forbidden(t) && !used.count(t)) { used.insert(t); return t; }
        return "";   // over 900 names in use
    };

    // FN functions are their own namespace in BASIC but share the
    // two-character rule; they are renamed with the same table
    std::map<std::string, std::string> baseMap;   // long base -> short base
    for (auto& toks : lines) {
        for (size_t i = 0; i < toks.size(); i++) {
            if (!isVariable(toks, i)) continue;
            std::string v = toks[i].value;
            std::string base = upper(baseOf(v));
            if (base.size() <= 2) continue;
            std::string suffix = v.back() == '$' ? "$" : "";
            if (suffix.empty() && i + 1 < toks.size() && toks[i + 1].type == BasicToken::OPERATOR &&
                toks[i + 1].value == "%")
                suffix = "%";
            auto it = baseMap.find(base);
            if (it == baseMap.end()) {
                std::string s = allocate(base);
                if (s.empty()) continue;
                it = baseMap.insert({base, s}).first;
            }
            map_[base + suffix] = it->second + suffix;
            toks[i].value = it->second + (v.back() == '$' ? "$" : "");
        }
    }
}
