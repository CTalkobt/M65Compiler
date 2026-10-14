#include "Ca65Parser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"

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
        // macros: their definitions are skipped, their calls expanded
        MacroUtils::extractCa65Macros(processedSource, module);
        static const DialectSyntax syn = syntax();
        readDialect(processedSource, syn, module, warnings_);
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
