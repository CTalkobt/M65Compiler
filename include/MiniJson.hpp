// MiniJson.hpp — enough JSON for the suite's own files and protocols: ln45
// -g's debug file (<program>.dbg) and the Debug Adapter Protocol (dbg45).
// Parse a document, read values, build and write them. No dependencies.
#pragma once
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

struct Json {
    enum Kind { NUL, BOOL, NUM, STR, ARR, OBJ } kind = NUL;
    double num = 0;
    bool b = false;
    std::string str;
    std::vector<Json> arr;
    std::vector<std::pair<std::string, Json>> obj;

    Json() = default;
    static Json null() { return Json(); }
    static Json boolean(bool v) { Json j; j.kind = BOOL; j.b = v; return j; }
    static Json number(double v) { Json j; j.kind = NUM; j.num = v; return j; }
    static Json string(const std::string& v) { Json j; j.kind = STR; j.str = v; return j; }
    static Json array() { Json j; j.kind = ARR; return j; }
    static Json object() { Json j; j.kind = OBJ; return j; }

    // an object's member (nullptr when it is not there)
    const Json* at(const std::string& k) const {
        for (const auto& [n, v] : obj) if (n == k) return &v;
        return nullptr;
    }
    // a member's value with a default
    double numAt(const std::string& k, double def = 0) const {
        const Json* v = at(k);
        return v && v->kind == NUM ? v->num : def;
    }
    std::string strAt(const std::string& k, const std::string& def = "") const {
        const Json* v = at(k);
        return v && v->kind == STR ? v->str : def;
    }
    bool boolAt(const std::string& k, bool def = false) const {
        const Json* v = at(k);
        return v && v->kind == BOOL ? v->b : def;
    }
    bool isNull() const { return kind == NUL; }

    // building
    Json& set(const std::string& k, Json v) {
        kind = OBJ;
        for (auto& [n, old] : obj) if (n == k) { old = std::move(v); return *this; }
        obj.push_back({k, std::move(v)});
        return *this;
    }
    Json& set(const std::string& k, const std::string& v) { return set(k, string(v)); }
    Json& set(const std::string& k, const char* v) { return set(k, string(v)); }
    Json& set(const std::string& k, double v) { return set(k, number(v)); }
    Json& set(const std::string& k, int v) { return set(k, number(v)); }
    Json& set(const std::string& k, unsigned v) { return set(k, number(v)); }
    Json& set(const std::string& k, long v) { return set(k, number((double)v)); }
    Json& set(const std::string& k, bool v) { return set(k, boolean(v)); }
    Json& push(Json v) { kind = ARR; arr.push_back(std::move(v)); return *this; }

    // writing (compact)
    std::string dump() const {
        std::string out;
        write(out);
        return out;
    }
    void write(std::string& out) const {
        switch (kind) {
            case NUL: out += "null"; break;
            case BOOL: out += b ? "true" : "false"; break;
            case NUM: {
                char buf[32];
                if (num == (double)(long long)num) snprintf(buf, sizeof buf, "%lld", (long long)num);
                else snprintf(buf, sizeof buf, "%.17g", num);
                out += buf;
                break;
            }
            case STR: quote(str, out); break;
            case ARR:
                out += '[';
                for (size_t i = 0; i < arr.size(); i++) { if (i) out += ','; arr[i].write(out); }
                out += ']';
                break;
            case OBJ:
                out += '{';
                for (size_t i = 0; i < obj.size(); i++) {
                    if (i) out += ',';
                    quote(obj[i].first, out);
                    out += ':';
                    obj[i].second.write(out);
                }
                out += '}';
                break;
        }
    }
    static void quote(const std::string& s, std::string& out) {
        out += '"';
        for (unsigned char c : s) {
            if (c == '"' || c == '\\') { out += '\\'; out += (char)c; }
            else if (c == '\n') out += "\\n";
            else if (c == '\r') out += "\\r";
            else if (c == '\t') out += "\\t";
            else if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); out += b; }
            else out += (char)c;
        }
        out += '"';
    }

    // reading: a whole document (false on a syntax error)
    static bool parse(const std::string& s, Json& out) {
        size_t p = 0;
        if (!parseAt(s, p, out)) return false;
        while (p < s.size() && isspace((unsigned char)s[p])) p++;
        return p == s.size();
    }
    static bool parseAt(const std::string& s, size_t& p, Json& out) {
        auto ws = [&] { while (p < s.size() && isspace((unsigned char)s[p])) p++; };
        ws();
        if (p >= s.size()) return false;
        char c = s[p];
        if (c == '"') {
            out = Json();
            out.kind = STR;
            for (p++; p < s.size() && s[p] != '"'; p++) {
                if (s[p] != '\\' || p + 1 >= s.size()) { out.str += s[p]; continue; }
                char e = s[++p];
                switch (e) {
                    case 'n': out.str += '\n'; break;
                    case 'r': out.str += '\r'; break;
                    case 't': out.str += '\t'; break;
                    case 'b': out.str += '\b'; break;
                    case 'f': out.str += '\f'; break;
                    case 'u':
                        if (p + 4 < s.size()) {
                            unsigned v = (unsigned)std::stoul(s.substr(p + 1, 4), nullptr, 16);
                            p += 4;
                            if (v < 0x80) out.str += (char)v;              // (the suite's files are ASCII)
                            else if (v < 0x800) { out.str += (char)(0xC0 | (v >> 6)); out.str += (char)(0x80 | (v & 0x3F)); }
                            else { out.str += (char)(0xE0 | (v >> 12)); out.str += (char)(0x80 | ((v >> 6) & 0x3F)); out.str += (char)(0x80 | (v & 0x3F)); }
                        }
                        break;
                    default: out.str += e; break;
                }
            }
            if (p >= s.size()) return false;
            p++;
            return true;
        }
        if (c == '[' || c == '{') {
            bool isObj = c == '{';
            out = isObj ? object() : array();
            p++;
            ws();
            if (p < s.size() && s[p] == (isObj ? '}' : ']')) { p++; return true; }
            for (;;) {
                std::string key;
                if (isObj) {
                    Json k;
                    if (!parseAt(s, p, k) || k.kind != STR) return false;
                    key = k.str;
                    ws();
                    if (p >= s.size() || s[p] != ':') return false;
                    p++;
                }
                Json v;
                if (!parseAt(s, p, v)) return false;
                if (isObj) out.obj.push_back({key, std::move(v)}); else out.arr.push_back(std::move(v));
                ws();
                if (p < s.size() && s[p] == ',') { p++; continue; }
                if (p < s.size() && s[p] == (isObj ? '}' : ']')) { p++; return true; }
                return false;
            }
        }
        if (s.compare(p, 4, "null") == 0) { p += 4; out = Json(); return true; }
        if (s.compare(p, 4, "true") == 0) { p += 4; out = boolean(true); return true; }
        if (s.compare(p, 5, "false") == 0) { p += 5; out = boolean(false); return true; }
        size_t e = p;
        while (e < s.size() && (isdigit((unsigned char)s[e]) || s[e] == '-' || s[e] == '.' || s[e] == 'e' ||
                                s[e] == 'E' || s[e] == '+')) e++;
        if (e == p) return false;
        out = number(std::stod(s.substr(p, e - p)));
        p = e;
        return true;
    }
};
