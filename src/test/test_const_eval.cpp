// test_const_eval.cpp — ConstEval, cc45's constant-expression evaluator (#93)
//
// Each row is an expression (parsed as a global's initializer, not folded)
// and its value, or that it is not a constant. The variables' sizes come from
// a test context, as IRBuilder and the parser give them.

#include "AST.hpp"
#include "ConstEval.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include <cmath>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

class TestContext : public ConstEvalContext {
public:
    std::map<std::string, int64_t> varSize{{"x", 2}, {"lx", 4}, {"a4", 4}};
    std::map<std::string, int64_t> typeSize{{"int", 2}, {"long", 4}, {"char", 1}};
    bool sizeOf(SizeofExpression& node, int64_t& out) override {
        if (node.isType) {
            if (node.pointerLevel) { out = 2; return true; }
            auto it = typeSize.find(node.typeName);
            if (it == typeSize.end()) return false;
            out = it->second;
            return true;
        }
        if (auto* vr = dynamic_cast<VariableReference*>(node.expression.get())) {
            auto it = varSize.find(vr->name);
            if (it == varSize.end()) return false;
            out = it->second;
            return true;
        }
        if (auto* aa = dynamic_cast<ArrayAccess*>(node.expression.get()))   // a4[i]: a char
            if (auto* vr = dynamic_cast<VariableReference*>(aa->arrayExpr.get()); vr && vr->name == "a4") {
                out = 1;
                return true;
            }
        return false;
    }
    bool alignOf(AlignofExpression& node, int64_t& out) override {
        if (node.expression) return false;
        out = 1;   // (cc45 packs everything)
        return true;
    }
};

struct Row {
    const char* expr;
    enum { INT, FLOAT, NONE } kind;
    double value;
};

const std::vector<Row> kRows = {
    {"1 + 2", Row::INT, 3},
    {"256 * 256", Row::INT, 65536},                     // two plain literals: 32 bits
    {"40000 / 2", Row::INT, 20000},
    {"(unsigned)65535 + 1", Row::INT, 0},              // unsigned int wraps
    {"(unsigned char)300", Row::INT, 44},
    {"(signed char)200", Row::INT, -56},
    {"(unsigned char)-1 == 255", Row::INT, 1},
    {"(signed)40000 < 0", Row::INT, 1},
    {"-1 > 0u", Row::INT, 1},                           // -1 converted to unsigned
    {"-1 / 2", Row::INT, 0},
    {"~0x8000", Row::INT, 0x7FFF},                      // an unsigned int (#289)
    {"(long)1 << 20", Row::INT, 1048576},
    {"'0' + 1", Row::INT, 49},                          // ('A' is PETSCII $C1)
    {"1 ? 3 : 5", Row::INT, 3},
    {"0 ? 3 : 5", Row::INT, 5},
    {"0 && 1 / 0", Row::INT, 0},                        // the right operand is not evaluated
    {"1 || 1 / 0", Row::INT, 1},
    {"1 ? 7 : 1 / 0", Row::INT, 7},
    {"sizeof x << 1", Row::INT, 4},                     // #93: these were errors in initializers
    {"sizeof lx > 2 ? 7 : 9", Row::INT, 7},
    {"sizeof x % 3", Row::INT, 2},
    {"(sizeof x == 2) && (sizeof lx == 4)", Row::INT, 1},
    {"sizeof a4 / sizeof a4[0]", Row::INT, 4},
    {"sizeof(long) * 2 + sizeof(char *)", Row::INT, 10},
    {"sizeof x - 3 < 0", Row::INT, 1},                  // sizeof is a plain (signed) int in cc45
    {"_Alignof(int)", Row::INT, 1},
    {"(int)2.75", Row::INT, 2},
    {"(int)-2.75", Row::INT, 65534},                    // plain int is unsigned in cc45
    {"(signed int)-2.75", Row::INT, -2},
    {"1.5 * 2", Row::FLOAT, 3.0},
    {"(float)1 / 4", Row::FLOAT, 0.25},
    {"(double)(unsigned char)255 + 0.5", Row::FLOAT, 255.5},
    {"2.0 > 1", Row::INT, 1},
    {"!0.0", Row::INT, 1},
    {"x + 1", Row::NONE, 0},                            // not constants
    {"1 / 0", Row::NONE, 0},
    {"1 % 0", Row::NONE, 0},
    {"2.5 % 2", Row::NONE, 0},
    {"1 && x", Row::NONE, 0},
    {"sizeof y", Row::NONE, 0},
};

}  // namespace

int main() {
    std::string src = "int x; long lx; char a4[4];\n";
    for (size_t i = 0; i < kRows.size(); i++)
        src += "long row" + std::to_string(i) + " = " + kRows[i].expr + ";\n";
    Lexer lexer(src);
    lexer.setSourceFile("const_eval.c");
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    auto unit = parser.parse();

    std::map<std::string, Expression*> init;
    for (auto& d : unit->topLevelDecls)
        if (auto* vd = dynamic_cast<VariableDeclaration*>(d.get())) init[vd->name] = vd->initializer.get();

    TestContext ctx;
    int failed = 0;
    for (size_t i = 0; i < kRows.size(); i++) {
        const Row& row = kRows[i];
        ConstValue v;
        bool ok = ConstEval::eval(init["row" + std::to_string(i)], ctx, v);
        std::string got = !ok ? "not a constant"
                        : v.isFloat() ? std::to_string(v.fvalue) + " (float)"
                        : std::to_string(v.intValue());
        bool pass = row.kind == Row::NONE  ? !ok
                  : row.kind == Row::FLOAT ? ok && v.isFloat() && std::fabs(v.fvalue - row.value) < 1e-9
                  : ok && v.isInt() && v.intValue() == (int64_t)row.value;
        if (!pass) {
            std::cout << "FAIL: " << row.expr << " gave " << got << std::endl;
            failed++;
        }
    }
    if (failed) return 1;
    std::cout << "ConstEval: all " << kRows.size() << " expressions" << std::endl;
    return 0;
}
