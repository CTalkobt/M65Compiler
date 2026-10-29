#include "Ca65Parser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"
#include "DialectConditionals.hpp"
#include <regex>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

namespace {
DialectSyntax syntax() {
    DialectSyntax s;
    s.name = "ca65";
    s.directives = {{"byte", "byte"}, {"byt", "byte"}, {"word", "word"}, {"addr", "word"},
                    {"dword", "dword"}, {"res", "res"}, {"org", "org"}, {"align", "align"},
                    {"segment", "segment"}, {"code", "code"}, {"data", "data"}, {"bss", "bss"},
                    {"rodata", "data"}, {"import", "extern"}, {"importzp", "extern"},
                    {"export", "global"}, {"exportzp", "global"}, {"global", "global"},
                    {"setcpu", "cpu"}, {"incbin", "incbin"}};
    s.ignored = {"p02", "pc02", "p816", "p4510", "debuginfo", "autoimport", "feature", "smart"};
    return s;
}
}  // namespace

AsmIR::Module Ca65Parser::parse(const std::string& source) {
    AsmIR::Module module;
    try {
        std::set<std::string> processedFiles;
        std::vector<std::string> includeErrors;
        std::string withIncludes = MacroUtils::processIncludes(source, module, processedFiles, includeErrors);
        for (const auto& err : includeErrors) addWarning(err);
        std::string processedSource = MacroUtils::processConditionals(withIncludes, module);
        // the dialect's own conditional assembly, resolved (ca45 has none, #224)
        processedSource = dialectcond::resolve(processedSource, "ca65", warnings_);
        // macros: their definitions are skipped, their calls expanded
        MacroUtils::extractCa65Macros(processedSource, module);
        static const DialectSyntax syn = syntax();
        // unnamed labels (#224): ":" at the start of a line (alone or before
        // an instruction); ":-" / ":+" the previous / next one, ":--" / ":++"
        // the second, ... Each gets a name of its own.
        {
            std::vector<std::string> lines;
            {
                std::istringstream in(processedSource);
                std::string l;
                while (std::getline(in, l)) lines.push_back(l);
            }
            static const std::regex defRe(R"re(^(\s*):(\s.*|$))re");
            std::vector<size_t> defs;
            std::map<size_t, std::string> defName;
            for (size_t n = 0; n < lines.size(); n++)
                if (std::regex_match(lines[n], defRe)) {
                    defName[n] = "__ca65_anon_" + std::to_string(defs.size() + 1);
                    defs.push_back(n);
                }
            static const std::regex refRe(R"re(:([+]+|[-]+))re");
            std::ostringstream out;
            for (size_t n = 0; n < lines.size(); n++) {
                std::string l = lines[n];
                std::smatch m;
                if (defName.count(n) && std::regex_match(l, m, defRe)) l = m[1].str() + defName[n] + ":" + m[2].str();
                std::string code = l, cmt;
                {
                    char q = 0;
                    for (size_t i = 0; i < l.size(); i++) {
                        if (q) { if (l[i] == q) q = 0; continue; }
                        if (l[i] == '"' || l[i] == '\'') { q = l[i]; continue; }
                        if (l[i] == ';') { code = l.substr(0, i); cmt = l.substr(i); break; }
                    }
                }
                std::string r;
                auto it = code.cbegin();
                for (std::sregex_iterator rx(code.cbegin(), code.cend(), refRe), end; rx != end; ++rx) {
                    const std::smatch& mm = *rx;
                    std::string sg = mm[1].str();
                    int count = (int)sg.size();
                    std::string target;
                    if (sg[0] == '-') {
                        int seen = 0;
                        for (size_t j = defs.size(); j-- > 0;) if (defs[j] <= n && ++seen == count) { target = defName[defs[j]]; break; }
                    } else {
                        int seen = 0;
                        for (size_t j = 0; j < defs.size(); j++) if (defs[j] > n && ++seen == count) { target = defName[defs[j]]; break; }
                    }
                    r.append(it, mm[0].first);
                    r += target.empty() ? mm[0].str() : target;
                    it = mm[0].second;
                }
                r.append(it, code.cend());
                out << r << cmt << "\n";
            }
            processedSource = out.str();
        }
        // .define NAME value (a constant): NAME = value
        {
            std::string out, line;
            std::istringstream in(processedSource);
            static const std::regex defRe(R"re(^\s*\.define\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.+?)\s*$)re");
            while (std::getline(in, line)) {
                std::smatch m;
                if (std::regex_match(line, m, defRe)) line = m[1].str() + " = " + m[2].str();
                out += line + "\n";
            }
            processedSource = out;
        }
        readDialect(processedSource, syn, module, warnings_);
        // (#224) ca65's scoping and list directives in ca45's terms
        std::vector<AsmIR::Statement> out;
        bool warnedScope = false;
        for (auto& st : module.statements) {
            if (st.type == AsmIR::Statement::Type::DIRECTIVE) {
                const std::string& d = st.dir.name;
                // .proc name: the label name (ca45's @locals scope from it to the next label)
                if (d == "proc" && !st.dir.arguments.empty()) {
                    AsmIR::Statement l;
                    l.type = AsmIR::Statement::Type::LABEL;
                    l.label = st.dir.arguments[0];
                    l.source_line = st.source_line;
                    out.push_back(l);
                    continue;
                }
                if (d == "endproc") continue;
                if (d == "scope" || d == "endscope") {
                    if (!warnedScope) addWarning("ca65 scopes (.scope) are not kept: their labels are global in ca45, "
                                                 "and scope::name references do not convert");
                    warnedScope = true;
                    continue;
                }
                // .asciiz "s": the string and a 0
                if (d == "asciiz") {
                    for (const auto& a : st.dir.arguments) {
                        AsmIR::Statement t = st;
                        t.dir.name = "ascii";
                        t.dir.arguments = {a};
                        out.push_back(t);
                    }
                    AsmIR::Statement z = st;
                    z.comment.clear();
                    z.dir.name = "byte";
                    z.dir.type = AsmIR::DirectiveType::BYTE_DATA;
                    z.dir.arguments = {"0"};
                    out.push_back(z);
                    continue;
                }
                // .lobytes / .hibytes a, b: .byte <a, <b / >a, >b
                if (d == "lobytes" || d == "hibytes") {
                    for (auto& a : st.dir.arguments) a = std::string(d == "lobytes" ? "<(" : ">(") + a + ")";
                    st.dir.name = "byte";
                    st.dir.type = AsmIR::DirectiveType::BYTE_DATA;
                }
            }
            out.push_back(st);
        }
        // ca65 starts in CODE; ca45's first segment is "default", written after
        // code and data (#224): .code at the start when content comes before
        // the first segment switch. .segment "DEFAULT" (the ca65 writer's
        // name for ca45's default segment) is that segment again
        bool contentFirst = false, switches = false, isDefault = false;
        for (const auto& st : out) {
            if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::SEGMENT) {
                switches = true;
                isDefault = st.dir.arguments.size() == 1 && (st.dir.arguments[0] == "\"DEFAULT\"" || st.dir.arguments[0] == "DEFAULT");
                break;
            }
            if (st.type == AsmIR::Statement::Type::INSTRUCTION || st.type == AsmIR::Statement::Type::LABEL) contentFirst = true;
        }
        if (isDefault) {
            out.erase(std::find_if(out.begin(), out.end(), [](const AsmIR::Statement& st) {
                return st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.type == AsmIR::DirectiveType::SEGMENT; }));
        } else if (switches && contentFirst) {
            AsmIR::Statement c;
            c.type = AsmIR::Statement::Type::DIRECTIVE;
            c.dir.type = AsmIR::DirectiveType::SEGMENT;
            c.dir.name = "code";
            out.insert(out.begin(), c);
        }
        module.statements = std::move(out);
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
