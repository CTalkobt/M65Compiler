// The linters' rules, al45's and cl45's, in one place (#708): the ids, default
// levels, profiles and texts, and the options they read. One .cc45-lint.yml
// serves both linters (and af45), so the shared core checks a file's rule ids
// and option names against all of them (lint::knownRules, lint::knownOptions),
// and both tools link this file. What the rules check is in AsmLint.cpp,
// AsmLintFlow.cpp, AsmStyle.cpp and CLint.cpp.
#include "AsmLint.hpp"
#include "AsmStyle.hpp"
#include "CLint.hpp"
#include "Lint.hpp"

namespace asmlint {

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
           "should do it itself, with interrupts masked (php; sei; ...; plp): between the two\n"
           "writes the KERNAL's keyboard scan reads $D611 as $FF and turns its Ctrl-S pause on."});
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
    registerFlowRules(e);
    asmstyle::registerRules(e);   // (mega65-style-*, off until enabled: #611)
}

void registerFlowRules(lint::Engine& e) {
    using L = lint::Level;
    e.add({"mega65-flag-stale", L::Warning, "all",
           "A comparison whose flags are overwritten before anything tests them",
           "cmp / cpx / cpy / cpz set N, Z and C for a branch that follows. An instruction between the two\n"
           "that sets the same flags (a load, a transfer, a pull, inx, a call) leaves the branch testing\n"
           "that instruction's result: 'cmp #5 / ldx #0 / beq' branches on X, always. Reported where the\n"
           "branch reads a flag the comparison set and something else has set since, with nothing having\n"
           "read the comparison; and where every flag of a comparison is set again unread. Save the flags\n"
           "(php / plp), or compare after the load."});
    e.add({"mega65-stack-balance", L::Warning, "all",
           "A return with bytes still pushed",
           "rts and rti take the return address from the top of the stack: a byte pushed and not pulled on\n"
           "the way there is taken for it. Followed from each routine's entry (a proc, a jsr's target, an\n"
           "exported label) along every path. Two bytes pushed before rts are a jump through the stack\n"
           "(lda #>addr / pha / lda #<addr / pha / rts) and are not reported; nor is a routine that pulls\n"
           "its own return address, sets the stack pointer (txs), or reaches a place with two different\n"
           "depths."});
    e.add({"mega65-dead-reg-write", L::Info, "all",
           "A register loaded with a value nothing uses",
           "A load or transfer whose register is written again on every path before anything reads it,\n"
           "and whose N and Z nothing tests. Calls, returns, jumps out of the file and writes to the\n"
           "hypervisor's trap registers ($D640-$D67F) count as reading every register. A load from an I/O\n"
           "register ($D000-$DFFF), where the read itself matters, is not reported."});
    e.add({"mega65-redundant-load", L::Info, "all",
           "A register loaded with the value it already holds",
           "The same constant or the same memory byte loaded again with the register untouched and the\n"
           "byte not written in between, in straight-line code (a label or a call ends what al45 knows),\n"
           "and with nothing needing the N and Z the load sets. A register just stored to a byte holds\n"
           "that byte's value. I/O registers are never the same twice."});
    e.add({"mega65-dead-store", L::Info, "all",
           "A base-page byte stored twice with no read between",
           "Two stores to the same base-page byte in straight-line code with nothing reading it between\n"
           "them: the first value is lost. An indexed or indirect read, a call, a branch or a label ends\n"
           "what al45 knows. $00 and $01 (the CPU port) are not judged."});
    e.add({"mega65-zp-param-block", L::Warning, "cc45",
           "A parameter-block byte ($10-$2F) read after a call that may have changed it",
           "cc45's functions take their arguments in $10-$2F and use what is left of it as they like: after\n"
           "a call of a C function (jsr _name) those bytes are not what they were. A routine that needs\n"
           "its own arguments after such a call keeps them elsewhere (a register, the stack, its own\n"
           "storage). Reported for a read of a byte no store has set since the call, in straight-line code.\n"
           "A callee in the same file that declares what it changes (.zp_clobbers) is believed."});
    e.add({"mega65-lib-temps", L::Warning, "cc45",
           "A library temporary ($F6-$FC) read after a call that may have changed it",
           "$F6-$FC are the library's temporaries: any library routine may use them. A value kept there\n"
           "across a call of a C function (jsr _name) may be gone. As mega65-zp-param-block."});
    e.add({"mega65-clobber-mismatch", L::Warning, "all",
           "A proc changes a register or flag its .reg_clobbers / .flag_clobbers does not list",
           "ca45's optimizer and ln45 believe a proc's attributes: a caller keeps a value in a register the\n"
           "callee is declared not to change. A register the proc writes (and does not save and restore\n"
           "with a push and pull) must be in .reg_clobbers, a flag in .flag_clobbers; a proc declared leaf\n"
           "(.func_flags leaf) makes no calls. Calls of procs in the file count with what they declare;\n"
           "other calls are not judged. al45 --suggest-clobbers prints the lists for every proc."});
}

}  // namespace asmlint

namespace asmstyle {

void registerRules(lint::Engine& e) {
    using L = lint::Level;
    e.add({"mega65-style-whitespace", L::Off, "all",
           "Style: a tab, trailing spaces, or no newline at the end of the file",
           "Spaces only (a tab outside a string or character literal; af45 expands it to 8-column stops),\n"
           "no spaces at the end of a line, and a newline after the last line."});
    e.add({"mega65-style-label", L::Off, "all",
           "Style: a label (or a constant's definition) not at column 1",
           "Labels (name:, @name:) and constants (NAME = value) start at the beginning of the line; what\n"
           "follows a label on its line is left where it is."});
    e.add({"mega65-style-indent", L::Off, "all",
           "Style: an instruction not at the instruction column, or a directive neither there nor at column 1",
           "An instruction (or a macro's name used as one) starts after mega65-style-instruction-column spaces\n"
           "(default 4). A directive, proc or endproc starts at column 1 or at the instruction column. Lines\n"
           "with a label are not checked."});
    e.add({"mega65-style-operand", L::Off, "all",
           "Style: an instruction's operand not one space after the mnemonic (or at the operand column)",
           "One space between a mnemonic and its operand, or, with mega65-style-operand-column set, the operand\n"
           "at that column (one space when the mnemonic reaches it). Directives' operands are not checked."});
    e.add({"mega65-style-comment", L::Off, "all",
           "Style: a trailing comment not at the comment column, too close to the code, or a // comment",
           "Comments start with ';'. A trailing comment starts at mega65-style-comment-column (default 28: after\n"
           "28 characters, column 29), or at least two spaces after code that reaches past it. Comments on a\n"
           "line of their own are not checked."});
    e.add({"mega65-style-label-name", L::Off, "all",
           "Style: a global label's or proc's name does not match mega65-style-label-name",
           "Global labels (name:) and proc names are matched against the regular expression\n"
           "mega65-style-label-name (default ^[A-Za-z_.][A-Za-z0-9_.]*$: any)."});
    e.add({"mega65-style-local-name", L::Off, "all",
           "Style: a local label's name does not match mega65-style-local-name",
           "Local labels (@name:) are matched against mega65-style-local-name (default ^@[A-Za-z0-9_]+$)."});
    e.add({"mega65-style-constant-name", L::Off, "all",
           "Style: a constant's name does not match mega65-style-constant-name",
           "Constants (NAME = value) are matched against mega65-style-constant-name (default: any name; for\n"
           "UPPER_SNAKE_CASE, ^[A-Z][A-Z0-9_]*$). A name the file exports or imports (.global, .weak, .extern)\n"
           "is a symbol, not a constant, and is not checked."});
}

}  // namespace asmstyle

namespace clint {

void registerRules(lint::Engine& e) {
    using L = lint::Level;
    e.add({"mega65-dma-trigger-order", L::Warning, "all",
           "A DMA list address byte written after the trigger register",
           "Writing $D700 (or $D705, the enhanced list's low byte) starts the DMA job with the list address\n"
           "in $D701/$D702 (and $D704) as they are then; one of those written after the trigger, with no\n"
           "other trigger after it in the same block, belongs to no job. Write the address bytes first."});
    e.add({"mega65-io-not-volatile", L::Warning, "all",
           "An I/O register ($D000-$DFFF) reached through a pointer or variable that is not volatile",
           "A read of an I/O register has an effect or a value that changes by itself, and a write one\n"
           "that the next write does not replace. Only 'volatile' tells the compiler so: cast the address\n"
           "to (volatile T *), declare the pointer 'volatile T *p', the placed variable 'volatile'.\n"
           "cc45 today leaves accesses through pointers alone; it does reuse the value read from a\n"
           "variable placed in I/O with __section, and other compilers merge both."});
    e.add({"mega65-io-personality", L::Info, "standalone",
           "VIC-III/IV registers written with no $47 $53 knock to $D02F",
           "The MEGA65's own I/O ($D030-$D07F and above) is visible only after the knock: $47 then $53\n"
           "written to $D02F. Programs linked with cc45's crt0 have it done at startup; code built to run\n"
           "without it should do it itself, with interrupts masked (php; sei; ...; plp): between the two\n"
           "writes the KERNAL's keyboard scan reads $D611 as $FF and turns its Ctrl-S pause on."});
    e.add({"mega65-isr-rom-call", L::Warning, "cc45",
           "An __interrupt handler, or a function it calls, calls the ROM",
           "The KERNAL and the BASIC ROM are not reentrant: a handler that calls them while the program\n"
           "is inside one of their routines overwrites that routine's work variables (the cursor, the\n"
           "current device, the float accumulators). The library keeps its own state across a handler\n"
           "(__jsr's register bytes, __rom_call's saved stack pointer); it cannot keep the ROM's. Set a\n"
           "flag in the handler and make the call from the main program, or make sure the program is\n"
           "never inside the ROM when the interrupt can arrive."});
    e.add({"mega65-stack-frame-limit", L::Warning, "all",
           "A stack frame larger than the limit (option mega65-stack-frame-limit, default 256 bytes)",
           "The CPU stack is 4 KB by default (-fstack-size) and a function with a stack frame (recursive,\n"
           "__interrupt, #pragma cc45 recurse, -fno-staticalloc) takes its whole frame on every call.\n"
           "Make large locals static, or raise the limit:\n"
           "  c_checks:\n    options:\n      mega65-stack-frame-limit: 512"});
    e.add({"mega65-recursion-frame", L::Info, "all",
           "A function on a call cycle: it gets a stack frame instead of static allocation",
           "cc45 keeps the parameters and locals of a function that cannot be active twice in static\n"
           "storage, which is smaller and faster than a stack frame. A function that calls itself, directly\n"
           "or through others of the file, cannot. #pragma cc45 recurse before it says the recursion is\n"
           "meant (and silences this)."});
    e.add({"mega65-sac-interrupt", L::Warning, "cc45",
           "A static-allocation function reachable from an __interrupt handler of the same file",
           "A function with static allocation has one copy of its parameters and locals. Called from an\n"
           "interrupt handler while the main program is inside it, the handler's call overwrites them;\n"
           "ln45 refuses to link such a program. Put #pragma cc45 recurse before the function (it then\n"
           "uses a stack frame), or keep what handlers call to themselves."});
    e.add({"mega65-narrow-param", L::Info, "all",
           "An int parameter that every call in the file passes a small constant",
           "A static function's int parameter that all its calls give a constant of 0-255 could be an\n"
           "unsigned char: a byte passed and compared instead of two."});
    e.add({"mega65-wide-arith", L::Info, "all",
           "long arithmetic whose operands and result fit 16 bits",
           "Both operands were widened from int or char (or are small constants) and the result is used\n"
           "as an int or char: the 32-bit operation gives the same bits as a 16-bit one, at twice the\n"
           "code. Drop the long cast or the L suffix. (cc45's type narrowing does this itself from -O1.)"});
    e.add({"mega65-signedness", L::Warning, "all",
           "A plain (unsigned in cc45) integer used where standard C would have a signed one",
           "cc45's plain char, short, int and long are unsigned unless declared signed. A negative constant\n"
           "stored in one or compared with one becomes a large positive value; x < 0 is always false; a\n"
           "difference is never negative, so (a - b) / 2 and (a - b) >> 1 differ from standard C when\n"
           "a < b. Declare the variable signed, or build with -fsigned-default."});
    e.add({"mega65-implicit-narrowing", L::Info, "all",
           "A wider integer assigned to a narrower one without a cast",
           "The upper bytes are dropped silently. When that is meant, a cast says so: c = (char)n."});
    e.add({"mega65-unpack-format", L::Warning, "cc45",
           "A pack45 stream unpacked whose format's decoder the program does not link",
           "Each pack45 format's decoder is linked only when the program names it at file scope:\n"
           "UNPACK_FORMAT1; or UNPACK_FORMAT2; (<unpack.h>; UNPACK_IMAGES; is UNPACK_FORMAT2). <unpack.h>\n"
           "names format 1 itself unless UNPACK_EXPLICIT_FORMATS is defined. Without the decoder, unpack(),\n"
           "unpack_to() and load_unpack() return UNPACK_ERROR for such a stream and write nothing. The rule\n"
           "reads the format of streams it can see (an array initialized in the file, #embed too; load_unpack of a\n"
           "name whose file is beside the source or in an include directory) and checks the file with\n"
           "main(): another file may name the decoder, so ln45 checks the whole program as well."});
}

}  // namespace clint

namespace lint {

const std::set<std::string>& knownRules() {
    static const std::set<std::string> ids = [] {
        Engine e;
        asmlint::registerRules(e);
        clint::registerRules(e);
        std::set<std::string> s;
        for (const auto& r : e.rules()) s.insert(r.id);
        return s;
    }();
    return ids;
}

const std::vector<OptionInfo>& knownOptions() {
    // (the style options' values are checked by asmstyle::optionsFrom: an error in al45 and af45)
    static const std::vector<OptionInfo> opts = {
        {"mega65-stack-frame-limit", "256", true},          // cl45 (CLint.cpp runFrames)
        {"mega65-style-instruction-column", "4", false},    // al45 and af45 (AsmStyle.hpp Options)
        {"mega65-style-operand-column", "0", false},
        {"mega65-style-comment-column", "28", false},
        {"mega65-style-label-name", "", false},
        {"mega65-style-local-name", "", false},
        {"mega65-style-constant-name", "", false},
    };
    return opts;
}

}  // namespace lint
