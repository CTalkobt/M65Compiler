// test_validation_ca45lib.cpp — libca45, ca45 as a library (#117):
// include/ca45/ca45.hpp's assemble() and assembleLine()
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "ca45/ca45.hpp"
#include "AssemblerOpcodeDatabase.hpp"
#include "test_harness.hpp"

namespace {

std::vector<uint8_t> line(const std::string& text, uint32_t addr = 0x2000, std::string* err = nullptr) {
    uint8_t buf[32];
    int n = ca45::assembleLine(text, buf, sizeof buf, addr, err);
    if (n < 0) return {0xDE, 0xAD};   // (no instruction assembles to that)
    return std::vector<uint8_t>(buf, buf + n);
}

bool is(const std::vector<uint8_t>& got, std::vector<uint8_t> want) { return got == want; }

// The issue's examples and the instructions the 6502 assembler does not know
void test_45gs02_lines() {
    CHECK(is(line("MAP"), {0x5C}), "MAP");
    CHECK(is(line("EOM"), {0xEA}), "EOM");
    CHECK(is(line("LDA [$6C],Z"), {0xEA, 0xB2, 0x6C}), "LDA [$6C],Z (32-bit indirect)");
    CHECK(is(line("NEG A"), {0x42}), "NEG A");
    CHECK(is(line("ASR A"), {0x43}), "ASR A");
    CHECK(is(line("LDQ ($20),Z"), {0x42, 0x42, 0xB2, 0x20}), "LDQ ($20),Z (quad)");
    CHECK(is(line("LDQ [$20],Z"), {0x42, 0x42, 0xEA, 0xB2, 0x20}), "LDQ [$20],Z (quad, flat)");
    CHECK(is(line("PHW #$1234"), {0xF4, 0x34, 0x12}), "PHW #$1234");
    CHECK(is(line("LDA ($12,SP),Y"), {0xE2, 0x12}), "LDA ($12,SP),Y");
    CHECK(is(line("lda #$42"), {0xA9, 0x42}), "lower case");
    CHECK(is(line("    sta $d020"), {0x8D, 0x20, 0xD0}), "an indented line");
    CHECK(is(line("loop: inx"), {0xE8}), "a label before the instruction");
    CHECK(is(line(".byte 1,2,3"), {1, 2, 3}), "a directive");
    CHECK(is(line("rtn #$12"), {0x62, 0x12}), "RTN #$12 (hex: it took only decimal)");
    CHECK(is(line("rtn #%101"), {0x62, 0x05}), "RTN #%101");
    CHECK(is(line("rtn #0"), {0x60}), "RTN #0 is RTS");
}

// Branches are relative to currentAddr
void test_branches() {
    CHECK(is(line("BNE $2000", 0x2005), {0xD0, 0xF9}), "BNE back");
    CHECK(is(line("BEQ $2010", 0x2000), {0xF0, 0x0E}), "BEQ forward");
    CHECK(is(line("BRA $1F82", 0x2000), {0x80, 0x80}), "BRA -128, the farthest back");
    CHECK(is(line("BNE $5000", 0x2005), {0xD3, 0xF9, 0x2F}), "BNE out of range: the 16-bit branch");
    CHECK(is(line("BBR0 $12,$2010", 0x2000), {0x0F, 0x12, 0x0D}), "BBR0 zp,rel");
    CHECK(is(line("JMP $1234", 0x8000), {0x4C, 0x34, 0x12}), "JMP: not relative");
}

// Errors: -1 and a message without the library's location
void test_errors() {
    std::string err;
    uint8_t buf[8];
    CHECK(ca45::assembleLine("FOO #1", buf, sizeof buf, 0, &err) == -1, "an unknown instruction fails");
    CHECK(err == "error: Unknown instruction 'FOO'", ("its message: " + err).c_str());
    CHECK(ca45::assembleLine("lda #", buf, sizeof buf, 0, &err) == -1, "a missing operand fails");
    CHECK(err == "error: Expected expression", ("its message: " + err).c_str());
    CHECK(ca45::assembleLine("sta $1234", buf, 2, 0, &err) == -1, "3 bytes in a 2-byte buffer fail");
    CHECK(err.find("more than the buffer") != std::string::npos, "the buffer's message");
    CHECK(ca45::assembleLine("sta $1234", buf, 3, 0, &err) == 3 && err.empty(), "3 bytes fit 3, error cleared");
    CHECK(ca45::assembleLine("inx", buf, sizeof buf, 0, nullptr) == 1, "no error string given");
    CHECK(ca45::assembleLine("", buf, sizeof buf, 0, &err) == 0, "an empty line: no bytes");
}

// assemble(): several lines, symbols, the source's own line numbers
void test_assemble() {
    ca45::AssembleOptions o;
    o.origin = 0x1000;
    o.symbols["screen"] = 0x0800;
    auto r = ca45::assemble("start: lda #1\n  sta screen,x\n  bne start\n", o);
    CHECK(r.ok(), "a three-line source");
    CHECK(is(r.bytes, {0xA9, 0x01, 0x9D, 0x00, 0x08, 0xD0, 0xF9}), "its bytes (a label, a symbol given)");

    r = ca45::assemble("  lda #1\n  inx\n  bogus\n", o);
    CHECK(!r.ok() && r.bytes.empty(), "an error: no bytes");
    CHECK(r.errors.size() == 1 && r.errors[0].rfind("<source>:3:", 0) == 0,
          ("the error is on the source's line 3: " + (r.errors.empty() ? std::string() : r.errors[0])).c_str());

    r = ca45::assemble("  lda #1\n  lda #\n", o);
    CHECK(r.errors.size() == 1 && r.errors[0].rfind("<source>:2:", 0) == 0,
          ("a pass-2 error on line 2, one location: " + (r.errors.empty() ? std::string() : r.errors[0])).c_str());

    r = ca45::assemble(".macro twice\n  inx\n  inx\n.endmacro\n  twice\n", o);
    CHECK(r.ok() && is(r.bytes, {0xE8, 0xE8}), ".macro");

    // optimizationLevel 0 keeps code as written; ca45's -O2 drops code after a return
    r = ca45::assemble("  rts\n  inx\n", o);
    CHECK(is(r.bytes, {0x60, 0xE8}), "-O0: as written");
    o.optimizationLevel = 2;
    r = ca45::assemble("  rts\n  inx\n", o);
    CHECK(is(r.bytes, {0x60}), "-O2: the optimizer's");

    CHECK(std::string(ca45::version()).size() > 0, "version()");
}

// Every instruction of ca45's opcode table, written as source: the opcode
// byte (after the $42 $42 quad and $EA flat prefixes) is the table's
std::string operandFor(AddressingMode m, bool& known) {
    known = true;
    switch (m) {
        case AddressingMode::IMPLIED: return "";
        case AddressingMode::ACCUMULATOR: return "a";
        case AddressingMode::IMMEDIATE: return "#$12";
        case AddressingMode::IMMEDIATE16: return "#$1234";
        case AddressingMode::BASE_PAGE: return "$12";
        case AddressingMode::BASE_PAGE_X: return "$12,x";
        case AddressingMode::BASE_PAGE_Y: return "$12,y";
        case AddressingMode::ABSOLUTE: return "$1234";
        case AddressingMode::ABSOLUTE_X: return "$1234,x";
        case AddressingMode::ABSOLUTE_Y: return "$1234,y";
        case AddressingMode::INDIRECT: return "($1234)";
        case AddressingMode::ABSOLUTE_INDIRECT: return "($1234)";
        case AddressingMode::ABSOLUTE_X_INDIRECT: return "($1234,x)";
        case AddressingMode::BASE_PAGE_X_INDIRECT: return "($12,x)";
        case AddressingMode::BASE_PAGE_INDIRECT_Y: return "($12),y";
        case AddressingMode::BASE_PAGE_INDIRECT_Z: return "($12),z";
        case AddressingMode::BASE_PAGE_INDIRECT_SP_Y: return "($12,sp),y";
        case AddressingMode::STACK_RELATIVE_INDIRECT_Y: return "($12,sp),y";
        case AddressingMode::STACK_RELATIVE: return "$12,sp";
        case AddressingMode::FLAT_INDIRECT_Z: return "[$12],z";
        case AddressingMode::RELATIVE: return "$2010";
        case AddressingMode::RELATIVE16: return "$4000";
        case AddressingMode::BASE_PAGE_RELATIVE: return "$12,$2010";
        default: known = false; return "";
    }
}

void test_opcode_table() {
    int checked = 0, unknown = 0;
    for (const auto& [key, opcode] : AssemblerOpcodeDatabase::getOpcodeMap()) {
        bool known;
        std::string text = key.first + " " + operandFor(key.second, known);
        if (!known) { unknown++; continue; }
        std::string err;
        std::vector<uint8_t> b = line(text, 0x2000, &err);
        size_t i = 0;
        while (i + 1 < b.size() && i < 3 && (b[i] == 0x42 || b[i] == 0xEA) && b[i] != opcode) {
            // a prefix: $42 $42 (quad), $EA (flat)
            if (b[i] == 0x42 && i + 1 < b.size() && b[i + 1] == 0x42) i += 2;
            else if (b[i] == 0xEA && key.second == AddressingMode::FLAT_INDIRECT_Z) i += 1;
            else break;
        }
        bool ok = i < b.size() && b[i] == opcode;
        checked++;
        if (!ok) {
            char msg[160];
            snprintf(msg, sizeof msg, "%s: opcode $%02X (%s)", text.c_str(), opcode,
                     err.empty() ? "other bytes" : err.c_str());
            CHECK(false, msg);
        }
    }
    CHECK(unknown == 0, "every addressing mode of the table has an operand form");
    CHECK(checked > 250, ("the opcode table, " + std::to_string(checked) + " instructions").c_str());
    if (tvVerbose()) printf("opcode table: %d instructions\n", checked);
}

}  // namespace

int main() {
    test_45gs02_lines();
    test_branches();
    test_errors();
    test_assemble();
    test_opcode_table();
    printf("\nlibca45 Tests: %d passed, %d failed\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
