#include "Ca65Parser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"
#include "DialectConditionals.hpp"
#include <regex>
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
