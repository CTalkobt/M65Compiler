#pragma once
#include "AsmIR.hpp"
#include <map>
#include <set>
#include <string>
#include <vector>

// The reader the non-ca45 cvt_asm input formats share (ca65, ACME,
// KickAssembler, Merlin 64, Oscar, x65). A line is read as ca45's own reader
// reads ca45 (Ca45Parser): a label, then a constant, an origin, a directive
// or an instruction, its comment kept. An instruction's addressing mode comes
// from its operand's syntax (#e, e,x, e,y, (e,x), (e),y, (e), e); an operand
// of another form is kept as written (RAW), so it reaches ca45 unchanged.
// Each dialect gives its comment and label conventions and its directive
// names; a directive written ".name" (or the dialect's prefix) that the
// table does not know is kept with that name, as cvt_asm's writers pass on
// ca45-only directives.
struct DialectSyntax {
    std::string name;                  // for messages
    char prefix = '.';                 // directive prefix ('!' for ACME); keyword directives also work
    std::map<std::string, std::string> directives;  // dialect name (lower case) -> ca45 name
    std::set<std::string> ignored;     // directives with no ca45 meaning: dropped with a warning
    bool keywords = false;             // directives without the prefix (Merlin, Oscar: ORG, DFB)
    std::set<std::string> bare;        // ... only these (x65's dc.b, xdef)
    bool slashComments = false;        // // and /* */ (KickAssembler)
    bool starComments = false;         // a '*' in column 1 starts a comment line (Merlin)
    bool columnLabels = false;         // a word in column 1 is a label without ':' (Merlin, Oscar, ACME)
    bool columnLabelsAlways = false;   // ... even when it is a mnemonic (Merlin, Oscar)
    // string directives (lower case) and how they encode: 'r' as written
    // (ASCII), 'p' PETSCII as ACME's !pet, 's' screen codes (ACME !scr,
    // KickAssembler's default), 'k' KickAssembler's .encoding; strings in a
    // byte directive and 'c' constants are 'r' ('k' with kickEncoding)
    std::map<std::string, char> strings;
    bool kickEncoding = false;         // .encoding "name" (KickAssembler) sets 'k'
    bool plusMacros = false;           // +name args invokes a macro (ACME)
    bool callMacros = false;           // name(args) invokes a macro (KickAssembler)
};

// Read a dialect's (preprocessed) source into `module`; warnings name the
// lines that were dropped or kept as written
void readDialect(const std::string& source, const DialectSyntax& syntax, AsmIR::Module& module,
                 std::vector<std::string>& warnings);
