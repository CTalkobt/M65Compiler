#include "OscarParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"

namespace {
DialectSyntax syntax() {
    DialectSyntax s;
    s.name = "oscar";
    s.directives = {{"org", "org"}, {"byte", "byte"}, {"db", "byte"}, {"dfb", "byte"},
                    {"word", "word"}, {"dw", "word"}, {"global", "global"}, {"extern", "extern"}};
    s.columnLabels = true;
    s.columnLabelsAlways = true;
    s.keywords = true;
    return s;
}
}  // namespace

AsmIR::Module OscarParser::parse(const std::string& source) {
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
