#include "X65Parser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"

namespace {
DialectSyntax syntax() {
    DialectSyntax s;
    s.name = "x65";
    s.directives = {{"byte", "byte"}, {"byt", "byte"}, {"dc.b", "byte"}, {"word", "word"},
                    {"dc.w", "word"}, {"long", "long"}, {"dc.l", "long"}, {"dword", "dword"},
                    {"org", "org"}, {"align", "align"}, {"fill", "fill"}, {"text", "text"},
                    {"import", "extern"}, {"xref", "extern"}, {"export", "global"},
                    {"xdef", "global"}, {"incbin", "incbin"}, {"cpu", "cpu"}};
    s.columnLabels = true;
    s.bare = {"dc.b", "dc.w", "dc.l", "xref", "xdef"};
    s.strings = {{"text", 'r'}};
    return s;
}
}  // namespace

AsmIR::Module X65Parser::parse(const std::string& source) {
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
