// ca45 as a library (#117): include/ca45/ca45.hpp
#include "ca45/ca45.hpp"

#include "AsmMacro.hpp"
#include "AssemblerLexer.hpp"
#include "AssemblerOptimizer.hpp"
#include "AssemblerParser.hpp"
#include "Version.hpp"

#include <cstring>
#include <exception>
#include <sstream>

namespace ca45 {

namespace {

std::string hexAddress(uint32_t a) {
    std::ostringstream s;
    s << "$" << std::hex << std::uppercase << a;
    return s.str();
}

const char* const kSourceName = "<source>";

// "line 3: <source>:3:5: error: ..." (the generator's prefix before the
// parser's own) → "<source>:3:5: error: ..."
std::vector<std::string> cleanErrors(const std::vector<std::string>& in) {
    std::vector<std::string> out;
    for (std::string e : in) {
        if (e.compare(0, 5, "line ") == 0) {
            size_t colon = e.find(": ");
            if (colon != std::string::npos && e.compare(colon + 2, std::strlen(kSourceName), kSourceName) == 0)
                e = e.substr(colon + 2);
        }
        out.push_back(e);
    }
    return out;
}

// "<source>:1:5: error: x" → "error: x" (one line: the location says nothing)
std::string withoutLocation(const std::string& e) {
    const std::string prefix = std::string(kSourceName) + ":";
    if (e.compare(0, prefix.size(), prefix) != 0) return e;
    size_t p = e.find(": ", prefix.size());
    return p == std::string::npos ? e : e.substr(p + 2);
}

}  // namespace

AssembleResult assemble(const std::string& source, const AssembleOptions& options) {
    AssembleResult result;
    try {
        // The origin: a `.org` of tokens on line 0 before the source's own
        // tokens, so messages give the source's line numbers
        AssemblerLexer orgLexer("    .org " + hexAddress(options.origin) + "\n");
        std::vector<AssemblerToken> tokens = orgLexer.tokenize();
        while (!tokens.empty() && tokens.back().type == AssemblerTokenType::END_OF_FILE) tokens.pop_back();
        for (auto& t : tokens) t.line = 0;
        // .macro ... .endmacro, as ca45 expands them (its C preprocessor,
        // #define and #include, is not part of the library)
        std::string expanded, macroError;
        if (!expandAsmMacros(source + "\n", kSourceName, expanded, macroError)) {
            result.errors.push_back(macroError);
            return result;
        }
        AssemblerLexer lexer(expanded);
        std::vector<AssemblerToken> sourceTokens = lexer.tokenize();
        tokens.insert(tokens.end(), sourceTokens.begin(), sourceTokens.end());
        AssemblerParser parser(tokens, options.symbols);
        parser.setSourceFile(kSourceName);
        parser.setTokenFiles(lexer.files());
        parser.setBadNumbers(lexer.badNumbers());
        parser.optimizationLevel = options.optimizationLevel;
        parser.optFlags = OptimizationFlags::fromLevel(options.optimizationLevel);
        parser.pass1();
        if (parser.hasErrors()) {
            result.errors = cleanErrors(parser.getErrors());
            return result;
        }
        result.bytes = parser.pass2(false);
        if (parser.hasErrors()) {
            result.errors = cleanErrors(parser.getErrors());
            result.bytes.clear();
        }
    } catch (const std::exception& e) {
        result.errors = cleanErrors({e.what()});
        result.bytes.clear();
    } catch (...) {
        result.errors.push_back("ca45: internal error");
        result.bytes.clear();
    }
    return result;
}

int assembleLine(const std::string& line, uint8_t* buf, int bufsz,
                 uint32_t currentAddr, std::string* error) {
    if (error) error->clear();
    // a line starting in column 1 is the instruction (`LDA #$03`), not a label
    std::string text = line;
    if (!text.empty() && text[0] != ' ' && text[0] != '\t') text = "    " + text;
    AssembleOptions options;
    options.origin = currentAddr;
    AssembleResult r = assemble(text, options);
    if (!r.ok()) {
        if (error) {
            for (size_t i = 0; i < r.errors.size(); i++) {
                if (i) *error += "\n";
                *error += withoutLocation(r.errors[i]);
            }
        }
        return -1;
    }
    if (r.bytes.size() > (size_t)(bufsz < 0 ? 0 : bufsz)) {
        if (error) *error = "the line assembles to " + std::to_string(r.bytes.size()) +
                            " bytes, more than the buffer's " + std::to_string(bufsz);
        return -1;
    }
    if (!r.bytes.empty()) std::memcpy(buf, r.bytes.data(), r.bytes.size());
    return (int)r.bytes.size();
}

const char* version() { return SUITE_VERSION; }

}  // namespace ca45
