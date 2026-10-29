#include "Ca45Parser.hpp"
#include "Preprocessor.hpp"
#include "MacroUtils.hpp"
#include "AsmMacro.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <regex>
#include <sstream>

// ca45 source -> AsmIR. The text is taken from the (preprocessed) source as
// written: labels keep their spelling, operands and directive arguments their
// expressions; ca45's own parse supplies the addressing modes. Whatever only
// ca45 understands (simulated instructions, proc/endproc, .repeat, (zp),z,
// [zp],z) is kept verbatim (AddressingMode::RAW) with a warning.

AsmIR::Module Ca45Parser::parse(const std::string& source) {
    AsmIR::Module module;

    try {
        // Preprocess the source
        Preprocessor preprocessor(false);  // not compiler mode
        std::map<std::string, std::string> symbols;
        std::vector<std::string> includePaths;
        std::string processed = preprocessor.process(source, symbols, includePaths, "");
        // .macro ... .endmacro: expanded as ca45 does (the output has the expansions)
        {
            std::string expanded, merr;
            if (!expandAsmMacros(processed, "", expanded, merr)) throw std::runtime_error(merr);
            processed = std::move(expanded);
        }

        // Tokenize
        AssemblerLexer lexer(processed);
        auto tokens = lexer.tokenize();

        // Parse
        AssemblerParser parser(tokens);
        parser.pass1();

        if (parser.hasErrors()) {
            for (const auto& err : parser.getErrors()) {
                addError(err);
            }
            return module;
        }

        // Extract module information
        module.cpu = "45GS02";
        module.is_o45 = false;

        extractSymbols(parser, module);
        extractStatements(parser, module, processed);

        for (const auto& warn : parser.getWarnings()) {
            addWarning(warn);
        }

    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }

    return module;
}

AsmIR::AddressingMode Ca45Parser::mapAddressingMode(AddressingMode mode) {
    switch (mode) {
        case AddressingMode::IMPLIED: return AsmIR::AddressingMode::IMPLIED;
        case AddressingMode::ACCUMULATOR: return AsmIR::AddressingMode::IMPLIED;
        case AddressingMode::IMMEDIATE: return AsmIR::AddressingMode::IMMEDIATE;
        case AddressingMode::BASE_PAGE: return AsmIR::AddressingMode::ZERO_PAGE;
        case AddressingMode::BASE_PAGE_X: return AsmIR::AddressingMode::ZERO_PAGE_X;
        case AddressingMode::BASE_PAGE_Y: return AsmIR::AddressingMode::ZERO_PAGE_Y;
        case AddressingMode::ABSOLUTE: return AsmIR::AddressingMode::ABSOLUTE;
        case AddressingMode::ABSOLUTE_X: return AsmIR::AddressingMode::ABSOLUTE_X;
        case AddressingMode::ABSOLUTE_Y: return AsmIR::AddressingMode::ABSOLUTE_Y;
        case AddressingMode::INDIRECT: return AsmIR::AddressingMode::INDIRECT;
        case AddressingMode::ABSOLUTE_INDIRECT: return AsmIR::AddressingMode::INDIRECT;
        case AddressingMode::BASE_PAGE_X_INDIRECT: return AsmIR::AddressingMode::INDIRECT_X;
        case AddressingMode::BASE_PAGE_INDIRECT_Y: return AsmIR::AddressingMode::INDIRECT_Y;
        case AddressingMode::RELATIVE: return AsmIR::AddressingMode::ABSOLUTE;
        case AddressingMode::RELATIVE16: return AsmIR::AddressingMode::ABSOLUTE;
        default: return AsmIR::AddressingMode::RAW;  // 45GS02-only syntax, as written
    }
}

AsmIR::DirectiveType Ca45Parser::mapDirectiveType(const std::string& name) {
    if (name == "byte") return AsmIR::DirectiveType::BYTE_DATA;
    if (name == "word") return AsmIR::DirectiveType::WORD_DATA;
    if (name == "long" || name == "dword") return AsmIR::DirectiveType::LONG_DATA;
    if (name == "fill") return AsmIR::DirectiveType::FILL;
    if (name == "align") return AsmIR::DirectiveType::ALIGN;
    if (name == "org") return AsmIR::DirectiveType::ORG;
    if (name == "segment" || name == "code" || name == "data" || name == "bss")
        return AsmIR::DirectiveType::SEGMENT;
    if (name == "global") return AsmIR::DirectiveType::GLOBAL;
    if (name == "extern") return AsmIR::DirectiveType::EXTERN;
    if (name == "weak") return AsmIR::DirectiveType::WEAK;
    if (name == "const") return AsmIR::DirectiveType::CONST;
    if (name == "cpu") return AsmIR::DirectiveType::CPU;
    if (name == "func_flags") return AsmIR::DirectiveType::FUNCTION;
    if (name == "zp_uses") return AsmIR::DirectiveType::ZP_USES;
    if (name == "zp_clobbers") return AsmIR::DirectiveType::ZP_CLOBBERS;
    if (name == "reg_clobbers") return AsmIR::DirectiveType::REG_CLOBBERS;
    if (name == "flag_clobbers") return AsmIR::DirectiveType::FLAG_CLOBBERS;
    if (name == "loc") return AsmIR::DirectiveType::LOC;
    return AsmIR::DirectiveType::OTHER;
}

void Ca45Parser::extractSymbols(const AssemblerParser& parser, AsmIR::Module& module) {
    const auto& symbolTable = parser.getSymbolTable();
    for (const auto& [name, sym] : symbolTable) {
        AsmIR::Symbol irSym;
        irSym.name = name;
        irSym.value = sym.value;
        irSym.is_constant = sym.isConstant;
        irSym.is_global = parser.isGlobalSymbol(name);
        irSym.is_extern = parser.isExternSymbol(name);
        irSym.is_weak = parser.isWeakSymbol(name);
        module.symbols[name] = irSym;
    }
}

namespace {

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

// The code and the comment of a line (';' outside quotes)
void splitComment(const std::string& line, std::string& code, std::string& comment) {
    bool dq = false, sq = false;
    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '\\' && (dq || sq)) { i++; continue; }
        if (c == '"' && !sq) dq = !dq;
        else if (c == '\'' && !dq) sq = !sq;
        else if (c == ';' && !dq && !sq) {
            code = line.substr(0, i);
            comment = line.substr(i + 1);
            return;
        }
    }
    code = line;
    comment.clear();
}

// Arguments separated by commas outside quotes and parentheses
std::vector<std::string> splitArgs(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    int depth = 0;
    bool dq = false, sq = false;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if ((dq || sq) && c == '\\' && i + 1 < s.size()) { cur += c; cur += s[++i]; continue; }
        if (c == '"' && !sq) dq = !dq;
        else if (c == '\'' && !dq) sq = !sq;
        else if (!dq && !sq) {
            if (c == '(' || c == '[') depth++;
            else if ((c == ')' || c == ']') && depth > 0) depth--;
            else if (c == ',' && depth == 0) { out.push_back(trim(cur)); cur.clear(); continue; }
        }
        cur += c;
    }
    if (!trim(cur).empty() || !out.empty()) out.push_back(trim(cur));
    return out;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// Strip the syntax of an addressing mode from an operand: the expression
bool operandCore(AsmIR::AddressingMode mode, const std::string& op, std::string& core) {
    std::string t = trim(op);
    auto endsWithReg = [&](const std::string& s, char reg, std::string& rest) {
        size_t comma = s.rfind(',');
        if (comma == std::string::npos) return false;
        if (lower(trim(s.substr(comma + 1))) != std::string(1, reg)) return false;
        rest = trim(s.substr(0, comma));
        return true;
    };
    std::string rest;
    switch (mode) {
        case AsmIR::AddressingMode::IMPLIED: core = ""; return t.empty() || lower(t) == "a";
        case AsmIR::AddressingMode::IMMEDIATE:
            if (t.empty() || t[0] != '#') return false;
            core = trim(t.substr(1));
            return true;
        case AsmIR::AddressingMode::ZERO_PAGE:
        case AsmIR::AddressingMode::ABSOLUTE:
            core = t;
            return !t.empty();
        case AsmIR::AddressingMode::ZERO_PAGE_X:
        case AsmIR::AddressingMode::ABSOLUTE_X:
            return endsWithReg(t, 'x', core);
        case AsmIR::AddressingMode::ZERO_PAGE_Y:
        case AsmIR::AddressingMode::ABSOLUTE_Y:
            return endsWithReg(t, 'y', core);
        case AsmIR::AddressingMode::INDIRECT:
            if (t.size() < 2 || t.front() != '(' || t.back() != ')') return false;
            core = trim(t.substr(1, t.size() - 2));
            return true;
        case AsmIR::AddressingMode::INDIRECT_X:
            if (t.size() < 2 || t.front() != '(' || t.back() != ')') return false;
            return endsWithReg(t.substr(1, t.size() - 2), 'x', core);
        case AsmIR::AddressingMode::INDIRECT_Y:
            if (!endsWithReg(t, 'y', rest) || rest.size() < 2 || rest.front() != '(' || rest.back() != ')') return false;
            core = trim(rest.substr(1, rest.size() - 2));
            return true;
        default:
            return false;
    }
}

}  // namespace

void Ca45Parser::extractStatements(const AssemblerParser& parser, AsmIR::Module& module, const std::string& source) {
    std::vector<std::string> lines;
    std::stringstream ss(source);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);

    // ca45's parse of each line: the instruction and its addressing mode, by
    // (file, line) as ca45's lexer numbers them: a '#line N "file"' marker
    // of the preprocessor (after an #include, a macro's expansion) makes the
    // next line line N of that file ("" the main source). A macro's expanded
    // lines all have the line of its call: a line's statements in order
    std::map<std::pair<std::string, int>, std::vector<const AssemblerParser::Statement*>> instrAt;
    std::map<std::pair<std::string, int>, size_t> instrNext;
    for (const auto& stmt : parser.statements)
        if (stmt->type != AssemblerParser::Statement::Type::NONE &&
            stmt->type != AssemblerParser::Statement::Type::DIRECTIVE)
            instrAt[{stmt->asmFile, stmt->line}].push_back(stmt.get());
    static const std::regex lineMarkRe(R"re(^#line\s+(\d+)(?:\s+"([^"]*)")?\s*$)re");
    std::string curFile;
    int curLine = 0;
    int rawWarnings = 0;

    static const std::regex labelRe(R"(^\s*([A-Za-z_@.][A-Za-z0-9_@.]*)\s*:(.*)$)");
    static const std::regex constRe(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$)");
    static const std::regex orgRe(R"(^\s*\*\s*=\s*(.+)$)");
    static const std::regex directiveRe(R"(^\s*\.([A-Za-z_][A-Za-z0-9_]*)\b\s*(.*)$)");
    static const std::regex instrRe(R"(^\s*([A-Za-z][A-Za-z0-9_]*(?:\.[A-Za-z0-9_]+)*)\b\s*(.*)$)");
    for (size_t n = 0; n < lines.size(); n++) {
        std::smatch m;
        // (the preprocessor's markers: not part of the program)
        if (std::regex_match(lines[n], m, lineMarkRe)) {
            curLine = std::stoi(m[1].str()) - 1;
            if (m[2].matched) curFile = m[2].str();
            continue;
        }
        int lineNo = ++curLine;
        std::string code, comment;
        splitComment(lines[n], code, comment);
        auto add = [&](AsmIR::Statement st) {
            st.source_line = lineNo;
            module.statements.push_back(std::move(st));
        };
        if (trim(code).empty()) {
            AsmIR::Statement st;
            if (!comment.empty()) {
                st.type = AsmIR::Statement::Type::COMMENT;
                st.comment = comment;
            } else {
                st.type = AsmIR::Statement::Type::BLANK;
            }
            add(st);
            continue;
        }
        // a label, maybe followed by a statement on the same line
        if (std::regex_match(code, m, labelRe) && !std::regex_match(code, constRe)) {
            AsmIR::Statement st;
            st.type = AsmIR::Statement::Type::LABEL;
            st.label = m[1].str();
            add(st);
            code = m[2].str();
            if (trim(code).empty()) {
                if (!comment.empty()) {
                    AsmIR::Statement c;
                    c.type = AsmIR::Statement::Type::COMMENT;
                    c.comment = comment;
                    add(c);
                }
                continue;
            }
        }
        AsmIR::Statement st;
        if (std::regex_match(code, m, orgRe)) {
            st.type = AsmIR::Statement::Type::DIRECTIVE;
            st.dir.type = AsmIR::DirectiveType::ORG;
            st.dir.name = "org";
            st.dir.arguments = {trim(m[1].str())};
        } else if (std::regex_match(code, m, constRe)) {
            st.type = AsmIR::Statement::Type::DIRECTIVE;
            st.dir.type = AsmIR::DirectiveType::CONST;
            st.dir.name = "const";
            st.dir.arguments = {m[1].str(), trim(m[2].str())};
        } else if (std::regex_match(code, m, directiveRe)) {
            std::string name = m[1].str();
            std::string args = trim(m[2].str());
            st.type = AsmIR::Statement::Type::DIRECTIVE;
            st.dir.name = name;
            st.dir.type = mapDirectiveType(name);
            if (name == "const") {
                // .const name = value
                std::smatch c;
                if (std::regex_match(args, c, constRe)) st.dir.arguments = {c[1].str(), trim(c[2].str())};
                else st.dir.arguments = splitArgs(args);
            } else {
                st.dir.arguments = splitArgs(args);
            }
        } else if (std::regex_match(code, m, instrRe)) {
            std::string mnemonic = lower(m[1].str());
            std::string operand = trim(m[2].str());
            st.type = AsmIR::Statement::Type::INSTRUCTION;
            st.instr.mnemonic = mnemonic;
            const AssemblerParser::Statement* ps = nullptr;
            auto it = instrAt.find({curFile, lineNo});
            if (it != instrAt.end()) {
                size_t& k = instrNext[{curFile, lineNo}];
                if (k < it->second.size()) ps = it->second[k++];
            }
            AsmIR::AddressingMode mode = AsmIR::AddressingMode::RAW;
            if (ps && ps->type == AssemblerParser::Statement::Type::INSTRUCTION &&
                mnemonic.find('.') == std::string::npos)
                mode = mapAddressingMode(ps->instr.mode);
            std::string core;
            if (mode != AsmIR::AddressingMode::RAW && operandCore(mode, operand, core)) {
                st.instr.mode = mode;
                st.instr.operand.text = core;
            } else {
                st.instr.mode = AsmIR::AddressingMode::RAW;
                st.instr.operand.text = operand;
                // (each such line, up to 20; then how many more)
                if (++rawWarnings <= 20)
                    addWarning((curFile.empty() ? "" : curFile + ": ") + "line " + std::to_string(lineNo) + ": '" +
                               trim(code) + "' is ca45-specific and kept as written (other assemblers will not accept it)");
            }
        } else {
            st.type = AsmIR::Statement::Type::INSTRUCTION;
            st.instr.mnemonic = trim(code);
            st.instr.mode = AsmIR::AddressingMode::RAW;
        }
        if (!comment.empty()) st.comment = comment;
        add(st);
    }
    if (rawWarnings > 20)
        addWarning(std::to_string(rawWarnings - 20) + " more ca45-specific lines kept as written");
}
