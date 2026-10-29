#include "AcmeParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"
#include <set>
#include <map>
#include <cctype>
#include <sstream>
#include <regex>
#include "DialectConditionals.hpp"
#include <cstdlib>

namespace {
DialectSyntax syntax() {
    DialectSyntax s;
    s.name = "acme";
    s.prefix = '!';
    s.directives = {{"by", "byte"}, {"byte", "byte"}, {"8", "byte"}, {"08", "byte"},
                    {"wo", "word"}, {"word", "word"}, {"16", "word"}, {"32", "long"},
                    {"tx", "text"}, {"text", "text"}, {"scr", "screencode"},
                    {"fi", "fill"}, {"fill", "fill"}, {"align", "align"},
                    {"bin", "incbin"}, {"binary", "incbin"}, {"cpu", "cpu"}};
    s.ignored = {"to", "sl", "symbollist", "initmem", "warn"};
    s.handled = {"zone", "zn"};   // (its .locals renamed per zone before reading)
    s.columnLabels = true;
    s.plusMacros = true;
    s.strings = {{"tx", 'r'}, {"text", 'r'}, {"raw", 'r'}, {"pet", 'p'}, {"scr", 's'}};
    return s;
}
}  // namespace

AsmIR::Module AcmeParser::parse(const std::string& source) {
    AsmIR::Module module;
    try {
        std::set<std::string> processedFiles;
        std::vector<std::string> includeErrors;
        std::string withIncludes = MacroUtils::processIncludes(source, module, processedFiles, includeErrors);
        for (const auto& err : includeErrors) addWarning(err);
        std::string processedSource = MacroUtils::processConditionals(withIncludes, module);
        // the dialect's own conditional assembly, resolved (ca45 has none, #224)
        processedSource = dialectcond::resolve(processedSource, "acme", warnings_);
        // zone-local labels (.name: from one !zone to the next) as
        // <zone>__name, unique in ca45, whose @locals scope per label (#224):
        // only names a line of that zone defines (a .name at its start), not
        // ca45's register operands kept as written (.A, .AX, .Q)
        {
            static const std::regex zoneRe(R"re(^\s*!(zone|zn)\b\s*([A-Za-z_][A-Za-z0-9_]*)?.*$)re", std::regex::icase);
            static const std::regex defRe(R"re(^\s*\.([A-Za-z_][A-Za-z0-9_]*)\s*(:|$|\s).*)re");
            std::vector<std::string> lines, zoneOf;
            std::map<std::string, std::set<std::string>> defined;
            {
                std::istringstream in(processedSource);
                std::string line, zone = "zone0";
                int anon = 0;
                while (std::getline(in, line)) {
                    std::smatch m;
                    if (std::regex_match(line, m, zoneRe)) zone = m[2].matched ? m[2].str() : "zone" + std::to_string(++anon);
                    else if (std::regex_match(line, m, defRe)) defined[zone].insert(m[1].str());
                    lines.push_back(line);
                    zoneOf.push_back(zone);
                }
            }
            std::ostringstream out;
            for (size_t n = 0; n < lines.size(); n++) {
                const std::string& line = lines[n];
                const auto& names = defined[zoneOf[n]];
                std::string r;
                char q = 0;
                for (size_t i = 0; i < line.size(); i++) {
                    char c = line[i];
                    if (q) { r += c; if (c == q) q = 0; continue; }
                    if (c == ';') { r += line.substr(i); break; }
                    if (c == '"' || c == '\'') { q = c; r += c; continue; }
                    bool start = i == 0 || !(std::isalnum((unsigned char)line[i - 1]) || line[i - 1] == '_');
                    if (c == '.' && start && i + 1 < line.size() &&
                        (std::isalpha((unsigned char)line[i + 1]) || line[i + 1] == '_')) {
                        size_t e = i + 1;
                        while (e < line.size() && (std::isalnum((unsigned char)line[e]) || line[e] == '_')) e++;
                        std::string nm = line.substr(i + 1, e - i - 1);
                        if (names.count(nm)) {
                            r += zoneOf[n] + "__" + nm;
                            i = e - 1;
                            continue;
                        }
                    }
                    r += c;
                }
                out << r << "\n";
            }
            processedSource = out.str();
        }
        // macros: their definitions are skipped, their calls expanded
        MacroUtils::extractACMEMacros(processedSource, module);
        static const DialectSyntax syn = syntax();
        readDialect(processedSource, syn, module, warnings_);
        // !align takes a mask and a value (!align 15, 0 pads to 16): ca45's
        // .align n[, fill] when the value is 0 and the mask a number (#223)
        for (auto& st : module.statements) {
            if (st.type != AsmIR::Statement::Type::DIRECTIVE || st.dir.name != "align" || st.dir.arguments.size() < 2)
                continue;
            auto& a = st.dir.arguments;
            char* end = nullptr;
            long m = a[0][0] == '$' ? std::strtol(a[0].c_str() + 1, &end, 16) : std::strtol(a[0].c_str(), &end, 0);
            if (!end || *end || m < 0 || a[1] != "0") {
                addWarning("!align " + a[0] + ", " + a[1] + ": only a numeric mask with value 0 converts (kept as written)");
                continue;
            }
            std::vector<std::string> ca45 = {std::to_string(m + 1)};
            if (a.size() > 2) ca45.push_back(a[2]);
            a = ca45;
        }
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
