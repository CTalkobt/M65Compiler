// test_segment_emission.cpp — Tests for correct segment assignment in .o45 emission
// and multi-object linking with mixed segments.
//
// Group A: End-to-end tests (assemble with ca45, verify with O45Reader)
// Group B: API-level linking tests (O45Writer → O45Linker)
// Group C: Regression tests for issue45-2

#include "O45Types.hpp"
#include "O45Writer.hpp"
#include "O45Reader.hpp"
#include "O45Linker.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <set>

static int tests_passed = 0;
static int tests_failed = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { \
        printf("FAIL: %s (line %d)\n", msg, __LINE__); \
        tests_failed++; \
    } else { \
        tests_passed++; \
    } \
} while(0)

// --- Helpers for end-to-end tests ---

static bool writeAsmFile(const char* path, const char* content) {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << content;
    return f.good();
}

static bool assembleToO45(const char* src, const char* dst) {
    std::string cmd = "./bin/ca45 -c ";
    cmd += src;
    cmd += " -o ";
    cmd += dst;
    cmd += " 2>/dev/null";
    return system(cmd.c_str()) == 0;
}

static bool readO45File(const char* path, O45File& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    std::string err;
    return O45Reader::read(data, out, err);
}

static const O45File::Export* findExport(const O45File& obj, const std::string& name) {
    for (const auto& exp : obj.exports) {
        if (exp.name == name) return &exp;
    }
    return nullptr;
}

// =============================================================================
// Group A: End-to-end segment assignment
// =============================================================================

// A1: Data symbol after .data gets SEG_DATA
void test_data_symbol_segment() {
    const char* src = "build/test_seg_a1.s";
    const char* dst = "build/test_seg_a1.o45";

    writeAsmFile(src,
        ".cpu _45gs02\n"
        ".global code_func\n"
        ".global data_var\n"
        ".code\n"
        "code_func:\n"
        "    lda #$42\n"
        "    rts\n"
        ".data\n"
        "data_var:\n"
        "    .byte $AA, $BB\n"
    );

    CHECK(assembleToO45(src, dst), "A1: assembly succeeded");

    O45File obj;
    CHECK(readO45File(dst, obj), "A1: read .o45");

    auto* cf = findExport(obj, "code_func");
    auto* dv = findExport(obj, "data_var");
    CHECK(cf != nullptr, "A1: code_func exported");
    CHECK(dv != nullptr, "A1: data_var exported");
    if (cf) CHECK(cf->segmentId() == SEG_TEXT, "A1: code_func is TEXT");
    if (dv) CHECK(dv->segmentId() == SEG_DATA, "A1: data_var is DATA");
}

// A2: BSS symbol after .bss gets SEG_BSS
void test_bss_symbol_segment() {
    const char* src = "build/test_seg_a2.s";
    const char* dst = "build/test_seg_a2.o45";

    writeAsmFile(src,
        ".cpu _45gs02\n"
        ".global bss_var\n"
        ".code\n"
        "    lda #$00\n"
        ".bss\n"
        "bss_var:\n"
        "    .res 4\n"
    );

    CHECK(assembleToO45(src, dst), "A2: assembly succeeded");

    O45File obj;
    CHECK(readO45File(dst, obj), "A2: read .o45");

    auto* bv = findExport(obj, "bss_var");
    CHECK(bv != nullptr, "A2: bss_var exported");
    if (bv) CHECK(bv->segmentId() == SEG_BSS, "A2: bss_var is BSS");
}

// A3: Boundary case — first label after segment switch (issue45-2 pattern)
void test_boundary_symbol_segment() {
    const char* src = "build/test_seg_a3.s";
    const char* dst = "build/test_seg_a3.o45";

    writeAsmFile(src,
        ".cpu _45gs02\n"
        ".global code_end\n"
        ".global data_start\n"
        ".code\n"
        "code_end:\n"
        "    .fill 256, $EA\n"
        ".data\n"
        "data_start:\n"
        "    .fill 512, $AA\n"
    );

    CHECK(assembleToO45(src, dst), "A3: assembly succeeded");

    O45File obj;
    CHECK(readO45File(dst, obj), "A3: read .o45");

    auto* ce = findExport(obj, "code_end");
    auto* ds = findExport(obj, "data_start");
    CHECK(ce != nullptr, "A3: code_end exported");
    CHECK(ds != nullptr, "A3: data_start exported");
    if (ce) CHECK(ce->segmentId() == SEG_TEXT, "A3: code_end is TEXT");
    if (ds) CHECK(ds->segmentId() == SEG_DATA, "A3: data_start is DATA (not TEXT)");
}

// A4: Multiple symbols across code/data/bss segments
void test_multi_segment_symbols() {
    const char* src = "build/test_seg_a4.s";
    const char* dst = "build/test_seg_a4.o45";

    writeAsmFile(src,
        ".cpu _45gs02\n"
        ".global sym_code\n"
        ".global sym_data\n"
        ".global sym_bss\n"
        ".code\n"
        "sym_code:\n"
        "    lda #$00\n"
        ".data\n"
        "sym_data:\n"
        "    .byte $FF\n"
        ".bss\n"
        "sym_bss:\n"
        "    .res 2\n"
    );

    CHECK(assembleToO45(src, dst), "A4: assembly succeeded");

    O45File obj;
    CHECK(readO45File(dst, obj), "A4: read .o45");

    auto* sc = findExport(obj, "sym_code");
    auto* sd = findExport(obj, "sym_data");
    auto* sb = findExport(obj, "sym_bss");
    CHECK(sc != nullptr, "A4: sym_code exported");
    CHECK(sd != nullptr, "A4: sym_data exported");
    CHECK(sb != nullptr, "A4: sym_bss exported");
    if (sc) CHECK(sc->segmentId() == SEG_TEXT, "A4: sym_code is TEXT");
    if (sd) CHECK(sd->segmentId() == SEG_DATA, "A4: sym_data is DATA");
    if (sb) CHECK(sb->segmentId() == SEG_BSS, "A4: sym_bss is BSS");
}

// A5: proc label gets correct segment
void test_proc_symbol_segment() {
    const char* src = "build/test_seg_a5.s";
    const char* dst = "build/test_seg_a5.o45";

    writeAsmFile(src,
        ".cpu _45gs02\n"
        ".global myfunc\n"
        ".code\n"
        "proc myfunc\n"
        "    lda #$00\n"
        "    rts\n"
        "endproc\n"
    );

    CHECK(assembleToO45(src, dst), "A5: assembly succeeded");

    O45File obj;
    CHECK(readO45File(dst, obj), "A5: read .o45");

    auto* mf = findExport(obj, "myfunc");
    CHECK(mf != nullptr, "A5: myfunc exported");
    if (mf) CHECK(mf->segmentId() == SEG_TEXT, "A5: myfunc is TEXT");
}

// =============================================================================
// Group B: Multi-object linking (API-level)
// =============================================================================

static O45File makeObj(const std::vector<uint8_t>& text,
                       const std::vector<uint8_t>& data,
                       uint32_t bssLen,
                       const std::vector<std::pair<std::string, std::pair<O45Segment, uint32_t>>>& exports) {
    O45Writer w;
    w.addDefaultOptions("ca45");
    if (!text.empty()) w.setTextSegment(0, text);
    if (!data.empty()) w.setDataSegment(0, data);
    if (bssLen > 0) w.setBssSegment(0, bssLen);
    for (const auto& [name, segOff] : exports) {
        w.addExport(name, segOff.first, segOff.second);
    }
    auto bytes = w.emit();
    O45File obj;
    std::string err;
    O45Reader::read(bytes, obj, err);
    return obj;
}

// B1: Two objects with code+data link without overlap
void test_link_no_overlap() {
    auto obj1 = makeObj({0xA9, 0x42, 0x60},  // LDA #$42; RTS
                        {0xAA, 0xBB},         // data
                        0,
                        {{"func_a", {SEG_TEXT, 0}}, {"var_x", {SEG_DATA, 0}}});
    auto obj2 = makeObj({0xA9, 0x00, 0xA2, 0x01},  // LDA #0; LDX #1
                        {0xCC, 0xDD, 0xEE},         // data
                        0,
                        {{"func_b", {SEG_TEXT, 0}}, {"var_y", {SEG_DATA, 0}}});

    O45Linker linker;
    linker.addObject("obj1.o45", obj1);
    linker.addObject("obj2.o45", obj2);
    linker.setTextBase(0x2000);

    std::string err;
    auto binary = linker.link(err);
    CHECK(!binary.empty(), "B1: link succeeded");

    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("func_a"), "B1: func_a in map");
    CHECK(syms.count("func_b"), "B1: func_b in map");
    CHECK(syms.count("var_x"),  "B1: var_x in map");
    CHECK(syms.count("var_y"),  "B1: var_y in map");

    if (syms.count("func_a") && syms.count("func_b")) {
        CHECK(syms.at("func_a") != syms.at("func_b"), "B1: func_a != func_b");
    }
    if (syms.count("var_x") && syms.count("var_y")) {
        CHECK(syms.at("var_x") != syms.at("var_y"), "B1: var_x != var_y");
    }
    // Text and data shouldn't overlap
    if (syms.count("func_a") && syms.count("var_x")) {
        CHECK(syms.at("func_a") != syms.at("var_x"), "B1: func_a != var_x (no cross-segment overlap)");
    }
}

// B2: BSS doesn't overlap data or code
void test_bss_no_overlap() {
    auto obj1 = makeObj({0x60}, {0xAA}, 10,
                        {{"f1", {SEG_TEXT, 0}}, {"d1", {SEG_DATA, 0}}, {"b1", {SEG_BSS, 0}}});
    auto obj2 = makeObj({0x60}, {0xBB}, 20,
                        {{"f2", {SEG_TEXT, 0}}, {"d2", {SEG_DATA, 0}}, {"b2", {SEG_BSS, 0}}});

    O45Linker linker;
    linker.addObject("obj1.o45", obj1);
    linker.addObject("obj2.o45", obj2);
    linker.setTextBase(0x2000);

    std::string err;
    auto binary = linker.link(err);
    CHECK(!binary.empty(), "B2: link succeeded");

    auto& syms = linker.getSymbolMap();
    // BSS symbols should be after data
    if (syms.count("d1") && syms.count("b1")) {
        CHECK(syms.at("b1") > syms.at("d1"), "B2: bss after data");
    }
    // BSS symbols should be after all text
    if (syms.count("f2") && syms.count("b1")) {
        CHECK(syms.at("b1") > syms.at("f2"), "B2: bss after text");
    }
}

// B3: Symbol map has correct unique addresses
void test_symbol_map_unique() {
    auto obj1 = makeObj({0xA9, 0x42, 0x60}, {0xAA, 0xBB}, 4,
                        {{"fa", {SEG_TEXT, 0}}, {"va", {SEG_DATA, 0}}, {"ba", {SEG_BSS, 0}}});
    auto obj2 = makeObj({0xA2, 0x00, 0x60}, {0xCC}, 8,
                        {{"fb", {SEG_TEXT, 0}}, {"vb", {SEG_DATA, 0}}, {"bb", {SEG_BSS, 0}}});

    O45Linker linker;
    linker.addObject("obj1.o45", obj1);
    linker.addObject("obj2.o45", obj2);
    linker.setTextBase(0x2000);

    std::string err;
    auto binary = linker.link(err);
    CHECK(!binary.empty(), "B3: link succeeded");

    auto& syms = linker.getSymbolMap();

    // All 6 symbols should have unique addresses
    std::set<uint32_t> addrs;
    for (const auto& [name, addr] : syms) {
        if (name.substr(0, 2) == "__") continue; // skip linker-defined
        addrs.insert(addr);
    }
    CHECK(addrs.size() == 6, "B3: all 6 symbols have unique addresses");
}

// B4: Cross-object reference resolves correctly
void test_cross_object_data_reloc() {
    // Obj1 imports data_var, Obj2 exports it in DATA
    O45Writer w1;
    w1.addDefaultOptions("ca45");
    // LDA data_var (absolute addressing: opcode $AD + 16-bit address)
    std::vector<uint8_t> code1 = {0xAD, 0x00, 0x00, 0x60};
    w1.setTextSegment(0, code1);
    w1.addImport("data_var");
    // R_WORD relocation at offset 1 (the address bytes) referencing external symbol 0
    std::vector<O45Reloc> relocs = {{1, R_WORD, SEG_EXTERNAL, 0, 0}};
    w1.setTextRelocations(O45RelocEncoder::encode(relocs));
    auto obj1bytes = w1.emit();

    O45Writer w2;
    w2.addDefaultOptions("ca45");
    std::vector<uint8_t> data2 = {0x42, 0x43};
    w2.setDataSegment(0, data2);
    w2.addExport("data_var", SEG_DATA, 0);
    auto obj2bytes = w2.emit();

    O45File file1, file2;
    std::string e1, e2;
    O45Reader::read(obj1bytes, file1, e1);
    O45Reader::read(obj2bytes, file2, e2);

    O45Linker linker;
    linker.addObject("obj1.o45", file1);
    linker.addObject("obj2.o45", file2);
    linker.setTextBase(0x2000);

    std::string err;
    auto binary = linker.link(err, false);
    CHECK(!binary.empty(), "B4: link succeeded");

    auto& syms = linker.getSymbolMap();
    CHECK(syms.count("data_var"), "B4: data_var resolved");

    if (syms.count("data_var") && binary.size() >= 4) {
        uint32_t expected = syms.at("data_var");
        uint16_t patched = binary[1] | (binary[2] << 8);
        CHECK(patched == (uint16_t)expected, "B4: relocation patched correctly");
    }
}

// =============================================================================
// Group C: Regression tests for issue45-2
// =============================================================================

// C1: Code function + data table don't share addresses
void test_issue45_2_regression() {
    const char* src = "build/test_seg_c1.s";
    const char* dst = "build/test_seg_c1.o45";

    writeAsmFile(src,
        ".cpu _45gs02\n"
        ".global _code_func\n"
        ".global frac_table\n"
        ".code\n"
        "_code_func:\n"
        "    lda #$42\n"
        "    rts\n"
        ".data\n"
        "frac_table:\n"
        "    .fill 1000, $00\n"
    );

    CHECK(assembleToO45(src, dst), "C1: assembly succeeded");

    O45File obj;
    CHECK(readO45File(dst, obj), "C1: read .o45");

    auto* cf = findExport(obj, "_code_func");
    auto* ft = findExport(obj, "frac_table");
    CHECK(cf != nullptr, "C1: _code_func exported");
    CHECK(ft != nullptr, "C1: frac_table exported");
    if (cf) CHECK(cf->segmentId() == SEG_TEXT, "C1: _code_func is TEXT");
    if (ft) CHECK(ft->segmentId() == SEG_DATA, "C1: frac_table is DATA (not TEXT)");

    // When linked, they should be at different addresses
    if (cf && ft) {
        O45Linker linker;
        linker.addObject("test.o45", obj);
        linker.setTextBase(0x2000);
        std::string err;
        auto binary = linker.link(err);
        CHECK(!binary.empty(), "C1: link succeeded");
        auto& syms = linker.getSymbolMap();
        if (syms.count("_code_func") && syms.count("frac_table")) {
            CHECK(syms.at("_code_func") != syms.at("frac_table"),
                  "C1: code_func and frac_table at different addresses");
        }
    }
}

// C2: Relocation for data symbol references SEG_DATA
void test_data_reloc_segment() {
    const char* src = "build/test_seg_c2.s";
    const char* dst = "build/test_seg_c2.o45";

    writeAsmFile(src,
        ".cpu _45gs02\n"
        ".global _code_func\n"
        ".global data_table\n"
        ".code\n"
        "_code_func:\n"
        "    lda data_table\n"
        "    rts\n"
        ".data\n"
        "data_table:\n"
        "    .byte $11, $22, $33\n"
    );

    CHECK(assembleToO45(src, dst), "C2: assembly succeeded");

    O45File obj;
    CHECK(readO45File(dst, obj), "C2: read .o45");

    // The text relocation for data_table should reference SEG_DATA
    auto textRelocs = O45RelocDecoder::decode(obj.textRelocs);
    bool foundDataReloc = false;
    for (const auto& r : textRelocs) {
        if (r.segment == SEG_DATA) {
            foundDataReloc = true;
            break;
        }
    }
    CHECK(foundDataReloc, "C2: text reloc references DATA segment");
}

// =============================================================================

int main() {
    printf("=== Segment Emission Tests ===\n\n");

    // Group A: End-to-end
    printf("Group A: End-to-end segment assignment\n");
    test_data_symbol_segment();
    test_bss_symbol_segment();
    test_boundary_symbol_segment();
    test_multi_segment_symbols();
    test_proc_symbol_segment();

    // Group B: API-level linking
    printf("\nGroup B: Multi-object linking\n");
    test_link_no_overlap();
    test_bss_no_overlap();
    test_symbol_map_unique();
    test_cross_object_data_reloc();

    // Group C: Regression
    printf("\nGroup C: Regression tests\n");
    test_issue45_2_regression();
    test_data_reloc_segment();

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
