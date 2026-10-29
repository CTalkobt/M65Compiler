#include "KickAssemblerParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"
#include "DialectConditionals.hpp"
#include <regex>
#include <vector>
#include <cctype>
#include <sstream>
#include <map>

namespace {
DialectSyntax syntax() {
    DialectSyntax s;
    s.name = "kickassembler";
    s.directives = {{"byte", "byte"}, {"by", "byte"}, {"word", "word"}, {"wo", "word"},
                    {"dword", "dword"}, {"dw", "dword"}, {"text", "text"}, {"fill", "fill"},
                    {"align", "align"}, {"pc", "org"}, {"const", "const"}, {"var", "var"},
                    {"label", "const"}, {"cpu", "cpu"}};
    s.ignored = {"filenamespace", "print", "printnow", "file", "disk", "plugin",
                 "segmentdef", "memblock"};
    s.slashComments = true;
    s.callMacros = true;
    s.strings = {{"text", 'k'}};
    s.kickEncoding = true;
    return s;
}
}  // namespace

AsmIR::Module KickAssemblerParser::parse(const std::string& source) {
    AsmIR::Module module;
    try {
        std::set<std::string> processedFiles;
        std::vector<std::string> includeErrors;
        std::string withIncludes = MacroUtils::processIncludes(source, module, processedFiles, includeErrors);
        for (const auto& err : includeErrors) addWarning(err);
        std::string processedSource = MacroUtils::processConditionals(withIncludes, module);
        // the dialect's own conditional assembly, resolved (ca45 has none, #224)
        processedSource = dialectcond::resolve(processedSource, "kickassembler", warnings_);
        // (#224) multi-labels: "!name:" may be defined again and again (name
        // may be empty: "!:"); "!name-" is the closest one before, "!name+"
        // the closest after ("!name--", "!name++": the second, ...). Each
        // definition gets a name of its own, __kick_<name>_<n>.
        // The 45GS02's ((zp)),z is ca45's [zp],z.
        {
            std::vector<std::string> lines;
            {
                std::istringstream in(processedSource);
                std::string l;
                while (std::getline(in, l)) lines.push_back(l);
            }
            static const std::regex defRe(R"re(^(\s*)!([A-Za-z_][A-Za-z0-9_]*)?:(.*)$)re");
            std::map<std::string, std::vector<size_t>> defs;   // name -> the lines defining it, in order
            std::map<size_t, std::string> defName;             // line -> its unique name
            for (size_t n = 0; n < lines.size(); n++) {
                std::smatch m;
                if (std::regex_match(lines[n], m, defRe)) {
                    std::string nm = m[2].matched ? m[2].str() : "";
                    auto& v = defs[nm];
                    defName[n] = "__kick_" + (nm.empty() ? std::string("anon") : nm) + "_" + std::to_string(v.size() + 1);
                    v.push_back(n);
                }
            }
            static const std::regex flatRe2(R"re(\(\(([^()]*)\)\)\s*,\s*([zZ]))re");
            std::ostringstream out;
            for (size_t n = 0; n < lines.size(); n++) {
                std::string l = lines[n];
                std::smatch m;
                if (defName.count(n) && std::regex_match(l, m, defRe)) l = m[1].str() + defName[n] + ":" + m[3].str();
                // references: !name followed by + or - signs
                std::string r;
                char q = 0;
                for (size_t i = 0; i < l.size(); i++) {
                    char c = l[i];
                    if (q) { r += c; if (c == q) q = 0; continue; }
                    if (c == '"' || c == '\'') { q = c; r += c; continue; }
                    if (c == '/' && i + 1 < l.size() && l[i + 1] == '/') { r += l.substr(i); break; }
                    if (c == '!' && (i == 0 || !std::isalnum((unsigned char)l[i - 1]))) {
                        size_t e = i + 1;
                        while (e < l.size() && (std::isalnum((unsigned char)l[e]) || l[e] == '_')) e++;
                        std::string nm = l.substr(i + 1, e - i - 1);
                        size_t k = e;
                        char dir = k < l.size() ? l[k] : 0;
                        int count = 0;
                        while (k < l.size() && l[k] == dir && (dir == '+' || dir == '-')) { count++; k++; }
                        auto it = defs.find(nm);
                        if (count && it != defs.end()) {
                            const auto& v = it->second;
                            long found = -1;
                            if (dir == '-') {
                                int seen = 0;
                                for (size_t j = v.size(); j-- > 0;) if (v[j] <= n && ++seen == count) { found = (long)v[j]; break; }
                            } else {
                                int seen = 0;
                                for (size_t j = 0; j < v.size(); j++) if (v[j] > n && ++seen == count) { found = (long)v[j]; break; }
                            }
                            if (found >= 0) {
                                r += defName[(size_t)found];
                                i = k - 1;
                                continue;
                            }
                        }
                    }
                    r += c;
                }
                out << std::regex_replace(r, flatRe2, "[$1],$2") << "\n";
            }
            processedSource = out.str();
        }
        // macros: their definitions are skipped, their calls expanded
        MacroUtils::extractKickAssemblerMacros(processedSource, module);
        static const DialectSyntax syn = syntax();
        readDialect(processedSource, syn, module, warnings_);
        // (#223) KickAssembler's 45GS02 flat-memory mode ((zp)),z is ca45's
        // [zp],z; .import binary "f" is .incbin "f"
        static const std::regex flatRe(R"re(^\s*\(\((.*)\)\)\s*,\s*[zZ]\s*$)re");
        static const std::regex binRe(R"re(^\s*binary\s+(.*)$)re");
        for (auto& st : module.statements) {
            std::smatch m;
            if (st.type == AsmIR::Statement::Type::INSTRUCTION && std::regex_match(st.instr.operand.text, m, flatRe)) {
                st.instr.mode = AsmIR::AddressingMode::RAW;
                st.instr.operand.text = "[" + m[1].str() + "],z";
            }
            if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.name == "import" && !st.dir.arguments.empty() &&
                std::regex_match(st.dir.arguments[0], m, binRe)) {
                st.dir.name = "incbin";
                st.dir.type = AsmIR::DirectiveType::OTHER;
                st.dir.arguments = {m[1].str()};
            }
        }
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
