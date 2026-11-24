// al45: the assembly linter's rules (#64, #199; doc/architecture/lint.md).
// Phase 2: rules over statements and labels, no data flow. They run on
// what ca45 parsed (pass 1 and 2, the optimizer off), so the code is as
// written and the addressing modes are ca45's own.
#include "AsmLint.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "AssemblerParser.hpp"
#include "AsmStyle.hpp"

namespace asmlint {

using ST = AssemblerParser::Statement;

// (registerRules, with the flow and style rules': LintRules.cpp, #708)

namespace {

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

bool isStore(const std::string& m) { return m == "sta" || m == "stx" || m == "sty" || m == "stz"; }

bool unconditional(const ST& s) {
    std::string m = lower(s.instr.mnemonic);
    return m == "jmp" || m == "bra" || m == "lbra" || m == "rts" || m == "rti" || m == "rtn";
}

bool isBranch(const std::string& m) {
    static const std::set<std::string> b = {"bcc", "bcs", "beq", "bne", "bmi", "bpl", "bvc", "bvs", "bra", "bsr",
                                            "lbcc", "lbcs", "lbeq", "lbne", "lbmi", "lbpl", "lbvc", "lbvs", "lbra", "lbsr"};
    return b.count(m) || (m.size() == 4 && (m.compare(0, 3, "bbr") == 0 || m.compare(0, 3, "bbs") == 0));
}

}  // namespace

std::string defaultProfile(const AssemblerParser&, bool relocMode) {
    // (a relocatable object is linked by ln45, with crt0 and c45.lib; a flat
    // binary stands alone)
    return relocMode ? "cc45" : "standalone";
}

void run(lint::Engine& e, AssemblerParser& p, const std::string& mainFile, bool suggest) {
    auto fileOf = [&](const ST& s) { return s.asmFile.empty() ? mainFile : s.asmFile; };

    // NOLINT comments of every file the statements come from
    {
        std::set<std::string> files;
        files.insert(mainFile);
        for (const auto& s : p.statements) files.insert(fileOf(*s));
        for (const auto& f : files) {
            std::ifstream in(f);
            if (!in) continue;
            std::stringstream ss;
            ss << in.rdbuf();
            e.scanSuppressions(f, ss.str());
            asmstyle::check(e, f, ss.str());   // (the style rules: the text's layout, #611)
        }
    }

    // the parser's own findings (no base-page mode, attributes in an unnamed proc)
    for (const auto& n : p.lintNotes) e.report(n.id, n.file, n.line, n.col, n.message);

    // an instruction's absolute operand value (false: none or not evaluable)
    auto operand = [&](const ST& s, uint32_t& v) {
        AddressingMode md = s.instr.mode;
        if (md == AddressingMode::IMPLIED || md == AddressingMode::ACCUMULATOR || md == AddressingMode::IMMEDIATE ||
            md == AddressingMode::IMMEDIATE16 || md == AddressingMode::RELATIVE || md == AddressingMode::RELATIVE16)
            return false;
        // (a relocatable symbol's value is not its address until linked)
        if (p.lintRelocatable(s.instr.operandTokenIndex, s.scopePrefix)) return false;
        return p.lintEval(s.instr.operandTokenIndex, s.scopePrefix, v);
    };

    // labels whose instruction a store patches (sta lbl+1): self-modified
    // operands, placeholders until run (jsr $FFFF ; patched)
    std::set<std::string> patched;
    {
        static const std::set<std::string> writes = {"sta", "stx", "sty", "stz", "stq", "inc", "dec", "inw", "dew",
                                                     "asl", "lsr", "rol", "ror", "tsb", "trb"};
        const auto& toks = p.getTokens();
        for (const auto& sp : p.statements) {
            const ST& s = *sp;
            if (s.type != ST::INSTRUCTION || !writes.count(lower(s.instr.mnemonic))) continue;
            for (int i = s.instr.operandTokenIndex; i >= 0 && i < (int)toks.size(); i++) {
                const auto& t = toks[i];
                if (t.type == AssemblerTokenType::NEWLINE || t.type == AssemblerTokenType::END_OF_FILE) break;
                if (t.type == AssemblerTokenType::IDENTIFIER) patched.insert(t.value);
            }
        }
    }
    // (the instruction's own label, or labels on lines of their own just before it)
    std::vector<std::string> pendingLabels;
    auto isPatched = [&](const ST& s) {
        std::vector<std::string> ls = pendingLabels;
        if (!s.label.empty()) ls.push_back(s.label);
        for (std::string l : ls) {
            if (patched.count(l)) return true;
            size_t c = l.rfind(':');
            if (c != std::string::npos && patched.count(l.substr(c + 1))) return true;
        }
        return false;
    };

    // the rules on data flow, with the statements a store patches
    {
        std::set<const void*> patchedStmts;
        std::vector<std::string> pend;
        for (const auto& sp : p.statements) {
            const ST& s = *sp;
            if (!s.label.empty()) pend.push_back(s.label);
            if (s.type == ST::NONE || s.type == ST::DIRECTIVE) continue;
            for (std::string l : pend) {
                size_t c = l.rfind(':');
                if (patched.count(l) || (c != std::string::npos && patched.count(l.substr(c + 1)))) patchedStmts.insert(&s);
            }
            pend.clear();
        }
        runFlow(e, p, mainFile, patchedStmts, suggest, &std::cout);
    }

    bool cle = false;                 // the file selects the 16-bit stack
    bool knock = false;               // a store to $D02F
    const ST* firstVic4 = nullptr;    // the first store to $D030-$D07F
    for (const auto& sp : p.statements) {
        const ST& s = *sp;
        if (s.type != ST::INSTRUCTION) continue;
        std::string m = lower(s.instr.mnemonic);
        if (m == "cle") cle = true;
        uint32_t v;
        if (isStore(m) && operand(s, v)) {
            if (v == 0xD02F) knock = true;
            if (v >= 0xD030 && v <= 0xD07F && !firstVic4) firstVic4 = &s;
        }
    }

    // straight-line state, reset at labels and control transfers
    bool dead = false;                // after an unconditional transfer, no label since
    int dmaTrigger = 0;               // the line of a trigger write in this straight-line run (0: none)
    // an address byte written after a trigger: reported when the straight
    // line ends without another trigger (one would make it the next job's)
    struct LateDma { std::string file; int line; uint32_t reg; int trigger; };
    std::vector<LateDma> lateDma;
    auto flushDma = [&](bool report) {
        if (report)
            for (const auto& d : lateDma) {
                char b[200];
                std::snprintf(b, sizeof b, "DMA list address byte $%04X written after the trigger (line %d) "
                              "and no trigger follows: the job started with the old value", d.reg, d.trigger);
                e.report("mega65-dma-trigger-order", d.file, d.line, 1, b);
            }
        lateDma.clear();
        dmaTrigger = 0;
    };
    bool bpRom = false;               // jsr __bp_rom since the last label / proc
    for (const auto& sp : p.statements) {
        const ST& s = *sp;
        if (s.deleted) continue;
        bool isProc = s.type == ST::INSTRUCTION && (s.instr.mnemonic == "proc" || s.instr.mnemonic == "endproc");
        if (!s.label.empty() || isProc) {
            dead = false;
            flushDma(true);
            if (isProc) bpRom = false;
        }
        if (s.type == ST::NONE) {
            if (!s.label.empty()) pendingLabels.push_back(s.label);
            continue;
        }
        if (s.type == ST::DIRECTIVE) {
            // (a segment change ends the straight line; data is not code)
            if (s.dir.name == "segment" || s.dir.name == "org" || s.dir.name == "code" || s.dir.name == "text") {
                dead = false;
                flushDma(true);
            }
            continue;
        }
        if (s.type != ST::INSTRUCTION || isProc) {
            // (simulated and 16/32-bit operations are code, but carry no rule here)
            if (s.type != ST::NONE && s.type != ST::INSTRUCTION && dead) {
                e.report("mega65-unreachable", fileOf(s), s.line, 1, "unreachable code: nothing jumps here");
                dead = false;   // (once per run)
            }
            continue;
        }
        std::string m = lower(s.instr.mnemonic);
        const std::string file = fileOf(s);
        if (dead) {
            e.report("mega65-unreachable", file, s.line, 1,
                     "unreachable code: '" + m + "' follows an unconditional jump or return with no label");
            dead = false;   // (once per run)
        }
        uint32_t v = 0;
        bool hasV = operand(s, v);

        // a stack read through page 1
        if (hasV && v >= 0x0100 && v <= 0x01FF &&
            (s.instr.mode == AddressingMode::ABSOLUTE_X || s.instr.mode == AddressingMode::ABSOLUTE_Y) &&
            (e.profile == "cc45" || cle))
            e.report("mega65-page1-stack", file, s.line, 1,
                     "'" + m + " $" + [&] { char b[8]; std::snprintf(b, sizeof b, "%04X", v); return std::string(b); }() +
                     (s.instr.mode == AddressingMode::ABSOLUTE_X ? ",x" : ",y") +
                     "' reads page 1: under the 16-bit stack the stack is elsewhere (tsx; tsy; read through a pointer)");

        // DMA: an address byte after the trigger
        if (isStore(m) && hasV) {
            if (v == 0xD700 || v == 0xD705) {
                lateDma.clear();   // (the bytes before it were this job's)
                dmaTrigger = s.line;
            } else if ((v == 0xD701 || v == 0xD702 || v == 0xD704) && dmaTrigger) {
                lateDma.push_back({file, s.line, v, dmaTrigger});
            }
        }

        // ROM calls under the program's base page (a routine that sets B
        // itself, tab, manages it: not followed)
        if (m == "tab") bpRom = true;
        if (m == "jsr" && !isPatched(s)) {
            std::string op = s.instr.operand;
            op.erase(std::remove_if(op.begin(), op.end(), ::isspace), op.end());
            if (op == "__bp_rom") bpRom = true;
            else if (op == "__bp_prog") bpRom = false;
            else if (hasV && v >= 0xE000 && v <= 0xFFFF && !bpRom && e.profile == "cc45") {
                char b[200];
                std::snprintf(b, sizeof b, "'jsr $%04X' calls the ROM with the program's base page: "
                              "bracket it with jsr __bp_rom / jsr __bp_prog, or use __rom_call", v);
                e.report("mega65-rom-call-bp", file, s.line, 1, b);
            }
        }

        pendingLabels.clear();
        if (unconditional(s)) { dead = true; flushDma(true); }
        else if (isBranch(m) || m == "jsr" || m == "bsr") flushDma(true);
    }

    flushDma(true);
    if (firstVic4 && !knock && e.profile == "standalone")
        e.report("mega65-io-personality", fileOf(*firstVic4), firstVic4->line, 1,
                 "a MEGA65 I/O register is written, but nothing writes $47, $53 to $D02F (the knock that makes it visible)");

    // local labels nothing refers to: their names among the tokens (a
    // definition is the name and a colon at the start of a line); not in
    // cc45's procs (.var cc45.fp: its block labels, @entry, @__return)
    std::set<std::string> cc45Labels;   // local labels defined in cc45's procs (bare names)
    {
        bool inProc = false, cc = false;
        std::vector<std::string> procLabels;
        for (const auto& sp : p.statements) {
            const ST& s = *sp;
            bool open = s.type == ST::INSTRUCTION && s.instr.mnemonic == "proc";
            bool close = s.type == ST::INSTRUCTION && s.instr.mnemonic == "endproc";
            if (open) { inProc = true; cc = false; procLabels.clear(); }
            if (inProc && !s.label.empty()) {
                std::string l = s.label;
                size_t c = l.rfind(':');
                procLabels.push_back(c == std::string::npos ? l : l.substr(c + 1));
            }
            if (inProc && s.type == ST::DIRECTIVE && s.dir.name == "var" && s.dir.varName.find("cc45.fp") != std::string::npos)
                cc = true;
            if (close && inProc) {
                if (cc) cc45Labels.insert(procLabels.begin(), procLabels.end());
                inProc = false;
            }
        }
    }
    {
        const auto& toks = p.getTokens();
        std::map<std::string, int> refs;
        std::map<std::string, const AssemblerToken*> defs;
        for (size_t i = 0; i < toks.size(); i++) {
            const auto& t = toks[i];
            if (t.type != AssemblerTokenType::IDENTIFIER || t.value.empty()) continue;
            bool def = i + 1 < toks.size() && toks[i + 1].type == AssemblerTokenType::COLON &&
                       (i == 0 || toks[i - 1].type == AssemblerTokenType::NEWLINE);
            if (def && t.value[0] == '@') { if (!defs.count(t.value)) defs[t.value] = &t; }
            else refs[t.value]++;
        }
        for (const auto& [name, t] : defs)
            if (!refs.count(name) && !cc45Labels.count(name))
                e.report("mega65-unused-label", p.lintTokenFile(*t), t->line, t->column, "local label '" + name + "' is never used");
    }

    runPatterns(e, p, mainFile);   // (the configuration's own rules)
}

// --- user-written rules (#765): .cc45-lint.yml asm_rules entries with a
// pattern, a sequence of instructions in one straight line ---

namespace {

// an instruction's operand as written: its tokens after the mnemonic, to the
// end of the line, without spaces
std::string operandText(AssemblerParser& p, const ST& s) {
    const auto& toks = p.getTokens();
    int i = s.instr.operandTokenIndex;
    if (i < 0 || i >= (int)toks.size())
        return s.instr.mode == AddressingMode::ACCUMULATOR ? "a" : "";
    int line = toks[i].line;
    // (back to the mnemonic: the index is not always the operand's first token, lda #n)
    int k = i;
    while (k > 0 && toks[k - 1].line == line && toks[k - 1].type != AssemblerTokenType::NEWLINE &&
           toks[k - 1].type != AssemblerTokenType::INSTRUCTION)
        k--;
    if (k > 0 && toks[k - 1].type == AssemblerTokenType::INSTRUCTION && toks[k - 1].line == line) i = k;
    std::string out;
    for (; i < (int)toks.size(); i++) {
        const auto& t = toks[i];
        if (t.type == AssemblerTokenType::NEWLINE || t.type == AssemblerTokenType::END_OF_FILE || t.line != line) break;
        if (t.type == AssemblerTokenType::STRING_LITERAL) out += "\"" + t.value + "\"";
        else if (t.type == AssemblerTokenType::CHAR_LITERAL) out += "'" + t.value + "'";
        else out += t.value;
    }
    std::string o;
    for (char c : out)
        if (c != ' ' && c != '\t') o += c;
    return o;
}

}  // namespace

void runPatterns(lint::Engine& e, AssemblerParser& p, const std::string& mainFile) {
    const auto& rules = e.config.patterns;
    bool any = false;
    for (const auto& r : rules) any = any || e.levelOf(r.id) != lint::Level::Off;
    if (!any) return;
    auto fileOf = [&](const ST& s) { return s.asmFile.empty() ? mainFile : s.asmFile; };

    // the straight-line runs of instructions: a label, proc / endproc, a
    // segment change, data, a simulated operation or another file ends one
    struct Insn { const ST* s; std::string mn, op, file; };
    std::vector<std::vector<Insn>> runs(1);
    auto cut = [&] { if (!runs.back().empty()) runs.emplace_back(); };
    static const std::set<std::string> moves = {"segment", "org", "code", "text", "data", "bss", "align", "overlay"};
    for (const auto& sp : p.statements) {
        const ST& s = *sp;
        if (s.deleted) continue;
        if (!s.label.empty()) cut();
        if (s.type == ST::NONE || s.type == ST::EXPR) continue;
        if (s.type == ST::DIRECTIVE) {
            if (s.size > 0 || moves.count(lower(s.dir.name))) cut();
            continue;
        }
        if (s.type != ST::INSTRUCTION || s.instr.mnemonic == "proc" || s.instr.mnemonic == "endproc") {
            cut();
            continue;
        }
        std::string f = fileOf(s);
        if (!runs.back().empty() && runs.back().back().file != f) cut();
        runs.back().push_back({&s, lower(s.instr.mnemonic), operandText(p, s), f});
    }

    for (const auto& r : rules) {
        if (e.levelOf(r.id) == lint::Level::Off || r.steps.empty()) continue;
        for (const auto& run : runs) {
            for (size_t i = 0; i + r.steps.size() <= run.size();) {
                std::map<std::string, std::string> caps;
                bool ok = true;
                for (size_t k = 0; k < r.steps.size() && ok; k++) {
                    const lint::PatternStep& st = r.steps[k];
                    const Insn& in = run[i + k];
                    ok = false;
                    for (const auto& m : st.mnemonics) ok = ok || lint::globMatch(m, in.mn);
                    if (ok && !st.anyOperand) ok = lint::matchOperand(st.operand, in.op, caps);
                }
                if (!ok) { i++; continue; }
                std::string msg = r.message;
                for (const auto& [name, v] : caps) {
                    std::string key = "{" + name + "}";
                    for (size_t at = msg.find(key); at != std::string::npos; at = msg.find(key, at + v.size()))
                        msg.replace(at, key.size(), v);
                }
                e.report(r.id, run[i].file, run[i].s->line, 1, msg);
                i += r.steps.size();   // (matches do not overlap)
            }
        }
    }
}

}  // namespace asmlint
