#include "AcmeParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"

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
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
