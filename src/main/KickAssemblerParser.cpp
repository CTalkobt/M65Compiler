#include "KickAssemblerParser.hpp"
#include "DialectReader.hpp"
#include "MacroUtils.hpp"
#include <regex>

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
