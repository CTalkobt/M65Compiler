#pragma once
// research/pcode: a function's IR as p-code (research/pcode/pcode_rt.s45 runs
// it). Not part of the toolchain: cc45 uses it only with CC45_PCODE set.
#include "IR.hpp"
#include <set>
#include <string>

struct PcodeResult {
    bool ok = false;
    std::string text;       // ca45 source: the frame, the native entry, the p-code
    std::string why;        // when not ok: what it does not translate
    int pcodeBytes = 0;     // the p-code's bytes
    int stubBytes = 0;      // the native entry's bytes (about)
    int frameBytes = 0;     // the frame's bytes
    bool enterable = false; // CALLP can enter it (the interpreter copies its parameters)
};

// The p-code of fn (ZP convention, no recursion: its frame is static).
PcodeResult pcodeEmit(const ir::Function& fn);

// Far p-code's plan (two passes, as -fzp-plan): CC45_PCODE_PLAN_OUT=FILE, the
// first, appends "P f" for each function made p-code that CALLP can enter
// (its parameters copied by the interpreter) and "E g" for each name that needs g's native entry (a
// native call, an address taken, a call CALLP cannot make); CC45_PCODE_PLAN=
// FILE, the second: calls from p-code to a P function are CALLP, and a P
// function no E names has no native entry.
// The names pcodeEmit found needing an entry (a call left CALL, an address)
std::set<std::string>& pcodeEntryRefs();
// The module's functions (a CALLP to another's p-code: .extern <g>__pc)
std::set<std::string>& pcodeModuleFunctions();
// Far: the module's __noescape literals (#pragma cc45 noescape_strings) that
// p-code may copy into its frame (CSTR) from a copy in its bank, <L>__pcs:
// each label's bytes with its 0 (at most 255; none in an initializer)
#include <map>
std::map<std::string, int>& pcodeFarStrings();
