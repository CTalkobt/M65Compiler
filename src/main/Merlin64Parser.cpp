#include "Merlin64Parser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"

namespace {
DialectSyntax syntax() {
    DialectSyntax s;
    s.name = "merlin64";
    s.directives = {{"org", "org"}, {"dfb", "byte"}, {"db", "byte"}, {"byte", "byte"},
                    {"hex", "byte"}, {"dw", "word"}, {"da", "word"}, {"word", "word"},
                    {"asc", "text"}, {"ds", "res"}, {"ent", "global"}, {"global", "global"},
                    {"ext", "extern"}, {"extern", "extern"}};
    s.ignored = {"lst", "mx", "xc", "tr", "exp", "sav", "typ", "dsk", "end"};
    s.columnLabels = true;
    s.columnLabelsAlways = true;
    s.starComments = true;
    s.keywords = true;
    s.strings = {{"asc", 'r'}};
    return s;
}
}  // namespace

AsmIR::Module Merlin64Parser::parse(const std::string& source) {
    AsmIR::Module module;
    try {
        std::set<std::string> processedFiles;
        std::vector<std::string> includeErrors;
        std::string withIncludes = MacroUtils::processIncludes(source, module, processedFiles, includeErrors);
        for (const auto& err : includeErrors) addWarning(err);
        std::string processedSource = MacroUtils::processConditionals(withIncludes, module);
        static const DialectSyntax syn = syntax();
        readDialect(processedSource, syn, module, warnings_);
        module.cpu = "45GS02";   // ca45's: a 6502 program assembles unchanged
        module.is_o45 = false;
    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }
    return module;
}
