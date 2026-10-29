#include "BasicEmitter.hpp"
#include "BasicVarRenamer.hpp"
#include "PETSCIIEncoder.hpp"
#include <cstring>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <fstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <iostream>
#include <cctype>

BasicEmitter::BasicEmitter(uint16_t loadAddr, uint16_t lineInc)
    : loadAddress(loadAddr), lineIncrement(lineInc) {}

std::vector<uint8_t> BasicEmitter::emitBinary(const std::string& sourceCode, bool useLabels) {
    BasicTokenizer tokenizer;
    PETSCIIEncoder petscii;

    std::vector<BasicLine> lines;
    std::vector<BasicToken> currentLineTokens;
    uint16_t currentLineNum = 0;
    bool lineNumExpected = true;

    auto tokens = tokenizer.tokenize(sourceCode);

    for (size_t i = 0; i < tokens.size(); i++) {
        const auto& token = tokens[i];

        if (token.type == BasicToken::EOL) {
            if (currentLineNum > 0 && !currentLineTokens.empty()) {
                BasicLine line;
                line.lineNumber = currentLineNum;
                line.tokens = currentLineTokens;
                lines.push_back(line);

                currentLineTokens.clear();
                currentLineNum = 0;
            }
            lineNumExpected = true;
        } else if (token.type == BasicToken::END_OF_FILE) {
            // Save the last line if it has tokens
            if (currentLineNum > 0 && !currentLineTokens.empty()) {
                BasicLine line;
                line.lineNumber = currentLineNum;
                line.tokens = currentLineTokens;
                lines.push_back(line);
            }
            break;
        } else if (token.type == BasicToken::LABEL) {
            // Label at start of line (no explicit line number)
            if (useLabels) {
                currentLineTokens.push_back(token);  // Keep label token for processing
                lineNumExpected = false;
                if (currentLineNum == 0) {
                    currentLineNum = 1;  // Placeholder - will be replaced by processLabels
                }
            }
        } else if (lineNumExpected && token.type == BasicToken::NUMBER) {
            currentLineNum = static_cast<uint16_t>(std::stoi(token.value));
            lineNumExpected = false;
        } else if (lineNumExpected && useLabels && token.type != BasicToken::LABEL) {
            // In label mode, if we see a statement without a line number or label, use placeholder
            currentLineNum = 1;
            lineNumExpected = false;
            currentLineTokens.push_back(token);
        } else {
            currentLineTokens.push_back(token);
        }
    }

    if (useLabels) {
        lines = processLabels(lines);
    }

    return emit(lines);
}

std::vector<uint8_t> BasicEmitter::emit(const std::vector<BasicLine>& lines) {
    std::vector<uint8_t> result;

    result = emitLoadAddress();

    // First pass: emit all lines with dummy address (0) to get their exact sizes
    std::vector<std::vector<uint8_t>> lineDataList;
    for (size_t i = 0; i < lines.size(); i++) {
        auto lineData = emitLineWithSpaces(lines[i], 0);  // Use dummy address, with spaces
        lineDataList.push_back(lineData);
    }

    // Line addresses: the first line is at the load address (the PRG's
    // 2-byte header is not loaded); each link points at the next line, the
    // last one at the $00 $00 that ends the program
    std::vector<uint16_t> lineAddrs;
    uint16_t currentAddr = loadAddress;
    lineAddress.clear();
    for (size_t i = 0; i < lines.size(); i++) {
        lineAddrs.push_back(currentAddr);
        lineAddress[lines[i].lineNumber] = currentAddr;
        currentAddr += lineDataList[i].size();
    }

    // Second pass: emit lines with correct addresses
    std::vector<uint8_t> programData;
    for (size_t i = 0; i < lines.size(); i++) {
        uint16_t nextLineAddr = (i + 1 < lines.size()) ? lineAddrs[i + 1] : currentAddr;
        auto lineData = emitLineWithSpaces(lines[i], nextLineAddr);
        programData.insert(programData.end(), lineData.begin(), lineData.end());
    }

    result.insert(result.end(), programData.begin(), programData.end());

    // Program end marker
    result.push_back(0x00);
    result.push_back(0x00);

    return result;
}

std::vector<uint8_t> BasicEmitter::emitLoadAddress() {
    std::vector<uint8_t> result;
    uint16_t addr = loadAddress;

    result.push_back(static_cast<uint8_t>(addr & 0xFF));
    result.push_back(static_cast<uint8_t>((addr >> 8) & 0xFF));

    return result;
}

std::vector<uint8_t> BasicEmitter::emitLine(const BasicLine& line, uint16_t nextLineAddr) {
    std::vector<uint8_t> result;

    result.push_back(static_cast<uint8_t>(nextLineAddr & 0xFF));
    result.push_back(static_cast<uint8_t>((nextLineAddr >> 8) & 0xFF));

    result.push_back(static_cast<uint8_t>(line.lineNumber & 0xFF));
    result.push_back(static_cast<uint8_t>((line.lineNumber >> 8) & 0xFF));

    for (const auto& token : line.tokens) {
        auto tokenData = emitToken(token);
        result.insert(result.end(), tokenData.begin(), tokenData.end());
    }

    result.push_back(0x00);

    return result;
}

std::vector<uint8_t> BasicEmitter::emitToken(const BasicToken& token) {
    PETSCIIEncoder petscii;
    std::vector<uint8_t> result;

    switch (token.type) {
        case BasicToken::KEYWORD:
            result.push_back(token.tokenByte);
            if (token.escapeByte != 0) {
                result.push_back(token.escapeByte);
            }
            break;

        case BasicToken::STRING: {
            result.push_back(0x22);
            auto encoded = petscii.encodeAs(token.value, token.prefix);
            result.insert(result.end(), encoded.begin(), encoded.end());
            result.push_back(0x22);
            break;
        }

        case BasicToken::NUMBER: {
            auto encoded = petscii.encode(token.value);
            result.insert(result.end(), encoded.begin(), encoded.end());
            break;
        }

        case BasicToken::IDENTIFIER: {
            auto encoded = petscii.encode(token.value);
            result.insert(result.end(), encoded.begin(), encoded.end());
            break;
        }

        case BasicToken::RAW: {
            auto encoded = petscii.encode(token.value);
            result.insert(result.end(), encoded.begin(), encoded.end());
            break;
        }

        case BasicToken::OPERATOR: {
            if (token.tokenByte != 0) {
                result.push_back(token.tokenByte);   // + - * / ^ > = <
                if (token.escapeByte) result.push_back(token.escapeByte);   // << >>: $FE $52/$53
            } else if (token.value == ":") {
                result.push_back(0x3A);
            } else {
                auto encoded = petscii.encode(token.value);
                result.insert(result.end(), encoded.begin(), encoded.end());
            }
            break;
        }

        case BasicToken::EOL:
        case BasicToken::END_OF_FILE:
            break;

        case BasicToken::LABEL:
            // Labels are not emitted in the binary
            break;
    }

    return result;
}

std::vector<uint8_t> BasicEmitter::emitLineWithSpaces(const BasicLine& line, uint16_t nextLineAddr) {
    std::vector<uint8_t> result;

    result.push_back(static_cast<uint8_t>(nextLineAddr & 0xFF));
    result.push_back(static_cast<uint8_t>((nextLineAddr >> 8) & 0xFF));

    result.push_back(static_cast<uint8_t>(line.lineNumber & 0xFF));
    result.push_back(static_cast<uint8_t>((line.lineNumber >> 8) & 0xFF));

    for (size_t i = 0; i < line.tokens.size(); i++) {
        const auto& token = line.tokens[i];
        // --preserve-spaces: the blanks the source had before the token
        // (not before the first: BASIC lists a space after the line number)
        if (preserveSpaces && i > 0) result.insert(result.end(), token.spaceBefore, 0x20);
        auto tokenData = emitToken(token);
        result.insert(result.end(), tokenData.begin(), tokenData.end());

        // Add space after keywords when followed by identifier, number, or variable
        if (!preserveSpaces && token.type == BasicToken::KEYWORD && i + 1 < line.tokens.size()) {
            const auto& nextToken = line.tokens[i + 1];
            if (token.value.back() == '(') {
                // TAB( SPC(: the parenthesis is part of the token
            } else if (token.value == "GO" && nextToken.type == BasicToken::NUMBER && nextToken.value == "64") {
                // GO64, as BASIC stores it
            } else if (nextToken.type == BasicToken::RAW) {
                // REM text keeps its own spacing; DATA items follow a space
                if (token.value == "DATA") result.push_back(0x20);
            } else if (nextToken.type == BasicToken::IDENTIFIER ||
                nextToken.type == BasicToken::NUMBER ||
                nextToken.type == BasicToken::OPERATOR) {
                // Don't add space before certain operators like (),:
                if (nextToken.type == BasicToken::OPERATOR &&
                    (nextToken.value == "(" || nextToken.value == "," || nextToken.value == ":")) {
                    // Skip space for these operators
                } else {
                    result.push_back(0x20);  // Space character
                }
            }
        }
    }

    result.push_back(0x00);

    return result;
}

std::vector<BasicLine> BasicEmitter::processLabels(std::vector<BasicLine>& lines) {
    std::vector<BasicLine> processedLines;
    labelMap.clear();

    uint16_t nextLineNum = lineIncrement;

    // First pass: collect labels and build mapping
    for (size_t idx = 0; idx < lines.size(); idx++) {
        auto& line = lines[idx];
        if (!line.tokens.empty() && line.tokens[0].type == BasicToken::LABEL) {
            labelMap[line.tokens[0].value] = nextLineNum;
        }
        nextLineNum += lineIncrement;
    }

    // Second pass: process tokens and generate line numbers
    nextLineNum = lineIncrement;
    for (auto& line : lines) {
        BasicLine newLine;
        newLine.lineNumber = nextLineNum;

        // Skip label token if present, process remaining tokens
        size_t startIdx = 0;
        if (!line.tokens.empty() && line.tokens[0].type == BasicToken::LABEL) {
            startIdx = 1;
        }

        // Process tokens, resolving label references
        for (size_t i = startIdx; i < line.tokens.size(); i++) {
            const auto& token = line.tokens[i];

            // Check if this is a GOTO/GOSUB followed by a label reference
            if ((token.type == BasicToken::KEYWORD &&
                 (token.value == "GOTO" || token.value == "GOSUB")) &&
                i + 1 < line.tokens.size() &&
                line.tokens[i + 1].type == BasicToken::IDENTIFIER) {

                // Add the GOTO/GOSUB token
                newLine.tokens.push_back(token);

                // Replace the label reference with its line number
                const std::string& labelName = line.tokens[i + 1].value;
                auto it = labelMap.find(labelName);
                if (it != labelMap.end()) {
                    BasicToken lineNumToken;
                    lineNumToken.type = BasicToken::NUMBER;
                    lineNumToken.value = std::to_string(it->second);
                    newLine.tokens.push_back(lineNumToken);
                    i++;  // Skip the label reference token
                } else {
                    // Undefined label - keep as-is (will cause runtime error)
                    newLine.tokens.push_back(line.tokens[i + 1]);
                    i++;
                }
            } else {
                newLine.tokens.push_back(token);
            }
        }

        processedLines.push_back(newLine);
        nextLineNum += lineIncrement;
    }

    return processedLines;
}

size_t BasicEmitter::listedLength(const std::vector<uint8_t>& d) const {
    static const std::map<uint8_t, size_t> kw = [] {
        std::map<uint8_t, size_t> m;
        BasicTokenizer t;
        for (const auto& [name, tok] : t.getKeywords()) m[tok] = name.size();
        for (uint8_t op = 0xAA; op <= 0xB3; op++) m[op] = 1;
        return m;
    }();
    static const std::map<uint8_t, size_t> kwEsc = [] {
        std::map<uint8_t, size_t> m;
        BasicTokenizer t;
        for (const auto& [name, tok] : t.getEscapeKeywords()) m[tok] = name.size();
        m[0x52] = m[0x53] = 2;   // << >>
        return m;
    }();
    static const std::map<uint8_t, size_t> kwCe = [] {
        std::map<uint8_t, size_t> m;
        BasicTokenizer t;
        for (const auto& [name, tok] : t.getCeKeywords()) m[tok] = name.size();
        return m;
    }();
    size_t n = 0;
    bool inString = false;
    for (size_t i = 0; i < d.size(); i++) {
        uint8_t b = d[i];
        if (b == 0x22) inString = !inString;
        if (inString || b < 0x80) { n++; continue; }
        if ((b == 0xFE || b == 0xCE) && i + 1 < d.size()) {
            const auto& m = b == 0xFE ? kwEsc : kwCe;
            auto it = m.find(d[++i]);
            n += it != m.end() ? it->second : 2;
            continue;
        }
        auto it = kw.find(b);
        n += it != kw.end() ? it->second : 1;
    }
    return n;
}

// A word that is a keyword in another BASIC (7.0, 3.5) or under the MEGA65
// Book's name, and what the ROM's BASIC 65 has instead: " (BASIC 65 has
// SPEED)", or ""
std::string BasicEmitter::otherKeywordHint(const std::string& word) {
    static const std::map<std::string, std::string> other = {
        {"SLOW", "SPEED"}, {"PEEKW", "WPEEK"}, {"POKEW", "WPOKE"},          // the MEGA65 Book's names
        {"DRAW", "LINE"}, {"LOCATE", ""}, {"SCALE", ""}, {"RDOT", "PIXEL"},   // BASIC 7.0 / 3.5
        {"RCLR", "RCOLOR"}, {"RLUM", ""}, {"RGR", "RGRAPHIC"}, {"SSHAPE", ""}, {"GSHAPE", ""},
        {"WIDTH", ""}, {"SPRDEF", ""}, {"PUDEF", ""}, {"QUIT", ""},
        {"STASH", "EDMA"}, {"FETCH", "EDMA"}, {"SWAP", "EDMA"},
    };
    auto it = other.find(word);
    if (it == other.end()) return "";
    return it->second.empty() ? " (a keyword of BASIC 7.0 / 3.5)" : " (BASIC 65 has " + it->second + ")";
}

void BasicEmitter::checkVariables(const std::vector<std::vector<BasicToken>>& lines,
                                  const std::vector<int>& sourceOf) {
    struct Use { int firstLine = 0; bool assigned = false, read = false, exempt = false, dimmed = false; };
    std::map<std::string, Use> vars;   // NAME[$|%][(]
    static const std::set<std::string> reserved = {"ST", "TI", "TI$", "DS", "DS$", "ER", "EL"};
    auto up = [](std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::toupper); return s; };

    for (size_t li = 0; li < lines.size(); li++) {
        const auto& t = lines[li];
        size_t i = 0;
        // one statement at a time: a new one after ':' / THEN / ELSE
        while (i < t.size()) {
            size_t end = i;
            while (end < t.size() && !(t[end].type == BasicToken::OPERATOR && t[end].value == ":") &&
                   !(end > i && t[end].type == BasicToken::KEYWORD && (t[end].value == "THEN" || t[end].value == "ELSE")))
                end++;
            size_t s = i;
            if (s < end && t[s].type == BasicToken::KEYWORD && (t[s].value == "THEN" || t[s].value == "ELSE")) s++;
            if (s < end && t[s].type == BasicToken::KEYWORD && t[s].value == "LET") s++;
            std::string kw = (s < end && t[s].type == BasicToken::KEYWORD) ? t[s].value : "";
            // a statement that starts with a name is an assignment; anything
            // else is a word BASIC 65 does not know (?SYNTAX ERROR when run):
            // a typo, or a keyword of another BASIC or ROM version
            if (s < end && t[s].type == BasicToken::IDENTIFIER && std::isalpha((unsigned char)t[s].value[0])) {
                size_t q = s + 1;
                if (q < end && t[q].type == BasicToken::OPERATOR && (t[q].value == "%" || t[q].value == "&")) q++;
                if (q < end && t[q].type == BasicToken::OPERATOR && t[q].value == "(") {
                    int d = 0;
                    for (; q < end; q++) {
                        if (t[q].type == BasicToken::OPERATOR && t[q].value == "(") d++;
                        if (t[q].type == BasicToken::OPERATOR && t[q].value == ")" && --d == 0) { q++; break; }
                    }
                }
                if (!(q < end && t[q].type == BasicToken::OPERATOR && t[q].value == "="))
                    diagnostics.push_back({sourceOf[li], "'" + t[s].value + "' is not a BASIC 65 keyword" +
                                           otherKeywordHint(up(t[s].value)) +
                                           ": the statement is a ?SYNTAX ERROR when run", false});
            }
            for (size_t k = s + 1; k + 1 < end; k++)   // PEEKW(...) in an expression
                if (t[k].type == BasicToken::IDENTIFIER && t[k + 1].type == BasicToken::OPERATOR &&
                    t[k + 1].value == "(" && !otherKeywordHint(up(t[k].value)).empty())
                    diagnostics.push_back({sourceOf[li], "'" + t[k].value + "' is not a BASIC 65 function" +
                                           otherKeywordHint(up(t[k].value)) + ": it is read as an array", false});
            bool listAssign = kw == "INPUT" || kw == "INPUT#" || kw == "READ" || kw == "GET" || kw == "RREG";
            int depth = 0;
            bool expectTarget = listAssign;   // the next name at depth 0 is assigned
            for (size_t k = s; k < end; k++) {
                const auto& tok = t[k];
                if (tok.type == BasicToken::OPERATOR && tok.value == "(") depth++;
                if (tok.type == BasicToken::OPERATOR && tok.value == ")") depth--;
                if (tok.type == BasicToken::OPERATOR && tok.value == "," && depth == 0 && listAssign) expectTarget = true;
                if (tok.type != BasicToken::IDENTIFIER) continue;
                if (!std::isalpha((unsigned char)tok.value[0])) continue;
                if (isDiskParameter(t, s, k)) continue;   // DOPEN ..., L20, U8
                if (k > s && t[k - 1].type == BasicToken::OPERATOR && (t[k - 1].value == "$" || t[k - 1].value == "%"))
                    continue;   // $FF
                if (k > s && t[k - 1].type == BasicToken::KEYWORD && t[k - 1].value == "FN") continue;   // FN name
                std::string name = up(tok.value);
                if (k + 1 < end && t[k + 1].type == BasicToken::OPERATOR &&
                    (t[k + 1].value == "%" || t[k + 1].value == "&"))
                    name += t[k + 1].value;   // integer %, byte &
                size_t after = k + 1 + ((name.back() == '%' || name.back() == '&') ? 1 : 0);
                bool isArray = after < end && t[after].type == BasicToken::OPERATOR && t[after].value == "(";
                std::string key = name + (isArray ? "(" : "");
                Use& u = vars[key];
                if (!u.firstLine) u.firstLine = sourceOf[li];
                if (reserved.count(name)) u.exempt = true;
                bool first = k == s;
                if (kw == "DIM" && depth == 0) { u.dimmed = true; continue; }
                if (kw == "FOR" && k == s + 1) { u.assigned = true; u.exempt = true; continue; }
                if (kw == "DEF") { u.exempt = true; continue; }   // DEF FN parameter
                if (listAssign && depth == 0 && expectTarget) {
                    u.assigned = true; u.exempt = true; expectTarget = false; continue;
                }
                if (first && !isArray) {
                    size_t eq = after;
                    if (eq < end && t[eq].type == BasicToken::OPERATOR && t[eq].value == "=") { u.assigned = true; continue; }
                }
                if (first && isArray) {
                    // A(i) = ...: the matching ')' then '='
                    int d = 0;
                    size_t q = after;
                    for (; q < end; q++) {
                        if (t[q].type == BasicToken::OPERATOR && t[q].value == "(") d++;
                        if (t[q].type == BasicToken::OPERATOR && t[q].value == ")" && --d == 0) break;
                    }
                    if (q + 1 < end && t[q + 1].type == BasicToken::OPERATOR && t[q + 1].value == "=") {
                        u.assigned = true;
                        continue;
                    }
                }
                u.read = true;
            }
            i = end;
            if (i < t.size() && t[i].type == BasicToken::OPERATOR && t[i].value == ":") i++;
        }
    }
    scalarCount = arrayCount = 0;
    for (const auto& [key, u] : vars) (key.back() == '(' ? arrayCount : scalarCount)++;
    if (scalarCount > 256)
        diagnostics.push_back({sourceOf.empty() ? 0 : sourceOf[0], std::to_string(scalarCount) +
            " scalar variables: BASIC 65 holds 256 ($F700-$FEFF)", false});
    for (const auto& [key, u] : vars) {
        if (u.exempt) continue;
        std::string shown = key.back() == '(' ? key + ")" : key;
        if (u.read && !u.assigned && !u.dimmed)
            diagnostics.push_back({u.firstLine, "variable " + shown + " is read but never assigned (a typo?)", false});
        else if (u.assigned && !u.read)
            diagnostics.push_back({u.firstLine, "variable " + shown + " is assigned but never read", false});
    }
}

void BasicEmitter::checkReachable(const std::vector<BasicLine>& lines, const std::vector<int>& sourceOf,
                                  const std::set<uint16_t>& targets) {
    auto isLoneJump = [](const BasicLine& l) {
        const auto& t = l.tokens;
        return !t.empty() && t[0].type == BasicToken::KEYWORD &&
               ((t[0].value == "RETURN" && t.size() == 1) || (t[0].value == "GOTO" && t.size() == 2));
    };
    for (size_t i = 0; i + 1 < lines.size(); i++) {
        const auto& t = lines[i].tokens;
        // the last statement of the line, and whether an IF/ON comes first
        bool conditional = false;
        size_t lastStart = 0;
        for (size_t k = 0; k < t.size(); k++) {
            if (t[k].type == BasicToken::KEYWORD && (t[k].value == "IF" || t[k].value == "ON")) conditional = true;
            if (t[k].type == BasicToken::OPERATOR && t[k].value == ":") lastStart = k + 1;
        }
        if (conditional || lastStart >= t.size() || t[lastStart].type != BasicToken::KEYWORD) continue;
        const std::string& kw = t[lastStart].value;
        bool jump = kw == "GOTO" || kw == "END" || kw == "RETURN" || kw == "STOP" || kw == "RUN" ||
                    (kw == "GO" && lastStart + 1 < t.size() && t[lastStart + 1].value == "TO");
        if (!jump) continue;
        const BasicLine& next = lines[i + 1];
        if (targets.count(next.lineNumber) || next.tokens.empty() || isLoneJump(next)) continue;
        const std::string& first = next.tokens[0].value;
        if (next.tokens[0].type == BasicToken::KEYWORD && (first == "DATA" || first == "REM")) continue;
        if (next.tokens[0].type == BasicToken::OPERATOR && first == ":") continue;
        diagnostics.push_back({sourceOf[i + 1], "line " + std::to_string(next.lineNumber) +
            " can never run (no label, after an unconditional " + kw + ")", false});
    }
}

char BasicEmitter::encodingFromName(const std::string& n) {
    if (n == "upper" || n == "default") return 'u';
    if (n == "petscii" || n == "mixed") return 'p';
    if (n == "screencode" || n == "screen") return 's';
    if (n == "ascii") return 'a';
    return 0;
}

const std::map<std::string, uint16_t>& BasicEmitter::getLabelMap() const {
    return labelMap;
}

void BasicEmitter::outputLabelTable(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file) {
        std::cerr << "Warning: Could not open label table file: " << filename << std::endl;
        return;
    }

    file << "Label\tLine Number\tMemory Address\n";
    // in line order
    std::vector<std::pair<uint16_t, std::string>> rows;
    for (const auto& [label, lineNum] : labelMap) rows.push_back({lineNum, label});
    std::sort(rows.begin(), rows.end());
    for (const auto& [lineNum, label] : rows) {
        auto it = lineAddress.find(lineNum);
        uint16_t memAddr = it != lineAddress.end() ? it->second : 0;
        file << label << "\t" << lineNum << "\t0x" << std::hex << std::setw(4)
             << std::setfill('0') << memAddr << std::dec << "\n";
    }
}

std::vector<uint8_t> BasicEmitter::emitFromLabels(const LabelBasedSourceParser& parser) {
    // Each source line with code is one BASIC line, numbered by the parser
    // (assignLineNumbers); label references become line numbers
    BasicTokenizer tokenizer;
    std::vector<BasicLine> lines;
    std::vector<int> sourceOf;   // per line: the parser's source line

    const auto& labels = parser.getLabelMap();   // upper-case label -> line number
    auto isLabel = [&](const std::string& n) { return labels.count(n) > 0; };
    labelMap.clear();
    variableMap.clear();
    emittedLines.clear();
    diagnostics.clear();
    longestLine = 0;
    longestLineNumber = 0;
    char encoding = defaultEncoding;
    PETSCIIEncoder petscii;

    std::vector<std::vector<BasicToken>> tokenLines;
    for (const auto& srcLine : parser.getSourceLines()) {
        if (!srcLine.pragma.empty()) {
            std::istringstream ps(srcLine.pragma);
            std::string what, value;
            ps >> what >> value;
            if (what == "encoding") {
                char e = encodingFromName(value);
                if (e) encoding = e;
                else diagnostics.push_back({srcLine.sourceLineNum, "unknown encoding '" + value +
                                            "' (upper, petscii, screencode, ascii)", true});
            } else {
                diagnostics.push_back({srcLine.sourceLineNum, "unknown #pragma '" + what + "' ignored", false});
            }
            continue;
        }
        if (!srcLine.label.empty()) {
            auto it = labels.find(LabelBasedSourceParser::upper(srcLine.label));
            if (it != labels.end()) labelMap[srcLine.label] = it->second;
        }
        if (srcLine.code.empty()) continue;

        auto tokens = tokenizer.tokenize(srcLine.code);
        LabelBasedSourceParser::forEachLabelRef(tokens, isLabel,
            [&](size_t idx, const std::string& name) {
                auto it = labels.find(name);
                if (it == labels.end()) return;   // undefined: the validator reports it
                tokens[idx].type = BasicToken::NUMBER;
                tokens[idx].value = std::to_string(it->second);
            });
        std::vector<BasicToken> kept;
        for (auto token : tokens) {
            if (token.type == BasicToken::EOL || token.type == BasicToken::END_OF_FILE ||
                token.type == BasicToken::LABEL)
                continue;
            if (token.type == BasicToken::STRING) {
                if (!token.prefix) token.prefix = encoding;
                // a BASIC line cannot hold $00 (it ends the line) or a quote
                for (uint8_t b : petscii.encodeAs(token.value, token.prefix)) {
                    if (b == 0x00 || b == 0x22) {
                        diagnostics.push_back({srcLine.sourceLineNum, std::string("a string cannot hold the byte ") +
                            (b ? "$22 (a quote ends it)" : "$00 (it ends the BASIC line; screen code 0 is '@')"), true});
                        break;
                    }
                }
            }
            kept.push_back(token);
        }
        if (kept.empty()) continue;
        tokenLines.push_back(kept);
        BasicLine line;
        line.lineNumber = srcLine.basicLineNum;
        lines.push_back(line);
        sourceOf.push_back(srcLine.sourceLineNum);
    }

    checkVariables(tokenLines, sourceOf);
    {
        std::set<uint16_t> targets;
        for (const auto& [l, n] : labels) targets.insert(n);
        // and the lines jumped to by number (GOSUB 300: a numbered program)
        for (const auto& tl : tokenLines)
            LabelBasedSourceParser::forEachLineNumberRef(tl, [&](size_t, uint16_t n) { targets.insert(n); });
        checkReachable(lines.empty() ? lines : [&] {
            std::vector<BasicLine> withTokens = lines;
            for (size_t i = 0; i < withTokens.size(); i++) withTokens[i].tokens = tokenLines[i];
            return withTokens;
        }(), sourceOf, targets);
    }

    if (renameVariables) {
        BasicVarRenamer renamer;
        renamer.rename(tokenLines);
        variableMap = renamer.mapping();
    }
    for (size_t i = 0; i < lines.size(); i++) lines[i].tokens = tokenLines[i];

    // Labels after the last line of code name a line of their own (":")
    if (parser.getTrailingLineNum() != 0) {
        BasicLine line;
        line.lineNumber = parser.getTrailingLineNum();
        BasicToken colon;
        colon.type = BasicToken::OPERATOR;
        colon.value = ":";
        line.tokens.push_back(colon);
        lines.push_back(line);
        sourceOf.push_back(0);
    }

    // Emit the lines
    auto bytes = emit(lines);
    // A line BASIC lists longer than its 160-character input buffer cannot
    // be edited on the machine
    {
        size_t pos = 2;
        for (size_t i = 0; i < lines.size() && pos + 4 <= bytes.size(); i++) {
            size_t end = pos + 4;
            while (end < bytes.size() && bytes[end] != 0) end++;
            std::vector<uint8_t> body(bytes.begin() + pos + 4, bytes.begin() + end);
            size_t len = std::to_string(lines[i].lineNumber).size() + 1 + listedLength(body);
            if (len > longestLine) { longestLine = len; longestLineNumber = lines[i].lineNumber; }
            if (len > 160)
                diagnostics.push_back({sourceOf[i], "line " + std::to_string(lines[i].lineNumber) + " lists as " +
                    std::to_string(len) + " characters: longer than BASIC's 160-character input line, it cannot be "
                    "edited on the MEGA65", false});
            pos = end + 1;
        }
    }
    for (size_t i = 0; i < lines.size(); i++)
        emittedLines.push_back({lines[i].lineNumber, sourceOf[i], lineAddress[lines[i].lineNumber]});
    return bytes;
}
