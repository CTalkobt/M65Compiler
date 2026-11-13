// al45: the assembly linter's rules (#64, #199; doc/architecture/lint.md).
// Phase 2: rules over statements and labels, no data flow. They run on
// what ca45 parsed (pass 1 and 2, the optimizer off), so the code is as
// written and the addressing modes are ca45's own.
#include "AsmLint.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "AssemblerParser.hpp"

namespace asmlint {

using ST = AssemblerParser::Statement;

void registerRules(lint::Engine& e) {
    using L = lint::Level;
    e.add({"mega65-unreachable", L::Warning, "all",
           "Code after an unconditional jmp/bra/rts/rti/rtn with no label: nothing reaches it",
           "An instruction that follows jmp, bra, lbra, rts, rti or rtn with no label in between can only be\n"
           "reached through a computed address. Data after a return is a .byte/.word directive, not code,\n"
           "and is not reported."});
    e.add({"mega65-unused-label", L::Info, "all",
           "A local label (@name) that nothing refers to",
           "Local labels (@name) exist for branches inside a routine; one no instruction or expression\n"
           "names is left over from an edit. Global labels are not reported (another file may use them)."});
    e.add({"mega65-no-bp-mode", L::Warning, "all",
           "A base-page operand on an instruction with no base-page mode: assembled as absolute $00xx",
           "ldz $xx, lda/sta $xx,y, jmp ($xx) and phw $00xx have no base-page form: ca45 assembles them as\n"
           "absolute $00xx, which ignores the base page register B. Under cc45's relocated base page that\n"
           "reaches the ROM's page 0, not the program's. Use ldq, lda+taz, ($xx),y or an absolute address\n"
           "on purpose."});
    e.add({"mega65-page1-stack", L::Warning, "all",
           "Stack read as $0100,x: wrong under the 16-bit stack",
           "With the 16-bit stack (cc45's default: crt0 clears E and puts SP at $BFFF) the stack is not in\n"
           "page 1. Read it through a pointer: tsx; tsy; stx $08; sty $09; lda ($08),y. Reported in the cc45\n"
           "profile, or when the file itself selects the 16-bit stack (cle)."});
    e.add({"mega65-dma-trigger-order", L::Warning, "all",
           "A DMA list address byte written after the trigger register",
           "Writing $D700 (or $D705, the enhanced list's low byte) starts the DMA job with the list address\n"
           "in $D701/$D702 (and $D704) as they are then; one of those written after the trigger, in the\n"
           "same straight-line code, belongs to the next job, not this one."});
    e.add({"mega65-io-personality", L::Info, "standalone",
           "VIC-III/IV registers written with no $47 $53 knock to $D02F",
           "The MEGA65's own I/O ($D030-$D07F and above) is visible only after the knock: $47 then $53\n"
           "written to $D02F. Programs linked with cc45's crt0 have it done at startup; a standalone file\n"
           "should do it itself."});
    e.add({"mega65-rom-call-bp", L::Warning, "cc45",
           "A ROM call ($E000 and up) not bracketed by __bp_rom / __bp_prog",
           "The ROM expects the base page at 0; cc45 programs run with their own (crt0 moves B). Call the ROM\n"
           "between jsr __bp_rom and jsr __bp_prog, or through __rom_call / __rom_calli (which also switch\n"
           "the stack, as the KERNAL's disk code needs). A routine that sets B itself (tab) and a call whose\n"
           "address a store patches (sta label+1) are not reported."});
    e.add({"mega65-unnamed-proc-attrs", L::Warning, "all",
           "Function attributes (.func_flags) in an unnamed proc: ignored",
           ".func_flags attaches to the global symbol 'proc <name>' names; in a bare 'proc' it is dropped.\n"
           "Name the function: proc <name> (and .global <name>)."});
}

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

void run(lint::Engine& e, AssemblerParser& p, const std::string& mainFile) {
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
}

}  // namespace asmlint
