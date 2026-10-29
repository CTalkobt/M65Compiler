#include "AcmeParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"
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
    s.ignored = {"to", "zone", "zn", "sl", "symbollist", "initmem", "warn"};
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
