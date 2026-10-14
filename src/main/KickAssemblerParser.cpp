#include "KickAssemblerParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"

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
        // macros: their definitions are skipped, their calls expanded
        MacroUtils::extractKickAssemblerMacros(processedSource, module);
        static const DialectSyntax syn = syntax();
        readDialect(processedSource, syn, module, warnings_);
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
