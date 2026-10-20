// test_ast_walker.cpp — ASTWalker reaches every child (#94)
//
// One function with a call in each place an expression or statement can be;
// a walker that overrides only FunctionCall (and walks on below it) must see
// every one of them.

#include "AST.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {

class CallNames : public ASTWalker {
public:
    std::set<std::string> names;
    using ASTWalker::visit;
    void visit(FunctionCall& node) override {
        names.insert(node.name);
        ASTWalker::visit(node);
    }
};

const char* kSource = R"(
struct P { int x; };
typedef int (*fn_t)(int);
fn_t fps[2];
int arr[4];
int if_c(void), if_t(void), if_e(void), wh_c(void), wh_b(void), do_b(void), do_c(void);
int for_i(void), for_c(void), for_n(void), for_b(void), sw_e(void), sw_b(void), ret(void);
int outer(int), arg(void), callee_idx(void), tgt(void), val(void), bin_l(void), bin_r(void);
int un(void), tern_c(void), tern_t(void), tern_e(void), cast(void), szof(void), init(void);
int il_a(void), il_b(void), clit(void), vla(void), gen_c(void), gen_a(void), gen_d(void);
int se(void), lbl(void), cmp(void);
struct P mem(void);
int f(int k) {
    if (if_c()) if_t(); else if_e();
    while (wh_c()) wh_b();
    do do_b(); while (do_c());
    for (for_i(); for_c(); for_n()) for_b();
    switch (sw_e()) { case 1: sw_b(); break; }
    outer(arg());
    fps[callee_idx()](1);
    arr[tgt()] = val();
    k = bin_l() + bin_r();
    k = -un();
    k = tern_c() ? tern_t() : tern_e();
    k = (int)cast();
    k = sizeof(szof());
    int v = init();
    int a[2] = { il_a(), il_b() };
    struct P p = (struct P){ clit() };
    int dyn[vla()];
    k = _Generic(gen_c(), int: gen_a(), default: gen_d());
    k = ({ se(); });
    k = mem().x;
    here: lbl();
    k = (k == cmp());
    return ret() + v + a[0] + p.x + dyn[0];
}
)";

}  // namespace

int main() {
    Lexer lexer(kSource);
    lexer.setSourceFile("walker.c");
    std::vector<Token> tokens = lexer.tokenize();
    Parser parser(tokens);
    auto unit = parser.parse();
    CallNames walker;
    unit->accept(walker);

    const std::vector<std::string> expected = {
        "if_c", "if_t", "if_e", "wh_c", "wh_b", "do_b", "do_c", "for_i", "for_c", "for_n", "for_b",
        "sw_e", "sw_b", "outer", "arg", "callee_idx", "tgt", "val", "bin_l", "bin_r", "un",
        "tern_c", "tern_t", "tern_e", "cast", "szof", "init", "il_a", "il_b", "clit", "vla",
        "gen_c", "gen_a", "gen_d", "se", "mem", "lbl", "cmp", "ret"};
    int missing = 0;
    for (const auto& n : expected)
        if (!walker.names.count(n)) {
            std::cout << "FAIL: ASTWalker did not reach the call to " << n << "()" << std::endl;
            missing++;
        }
    if (missing) return 1;
    std::cout << "ASTWalker: all " << expected.size() << " calls reached" << std::endl;
    return 0;
}
