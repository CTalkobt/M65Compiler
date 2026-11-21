// The stream formats a file unpacks (#666): see UnpackFormats.hpp.
#include "UnpackFormats.hpp"

#include <cctype>
#include <fstream>
#include <iterator>
#include <map>

#include "AST.hpp"

namespace unpackfmt {

namespace {

// the array an address argument names: x, &x, &x[0], x + 0, through casts
std::string arrayOf(Expression* e) {
    while (true) {
        if (auto* c = dynamic_cast<CastExpression*>(e)) { e = c->expression.get(); continue; }
        if (auto* u = dynamic_cast<UnaryOperation*>(e)) {
            if (u->op != "&") return "";
            e = u->operand.get();
            continue;
        }
        if (auto* a = dynamic_cast<ArrayAccess*>(e)) {
            auto* i = dynamic_cast<IntegerLiteral*>(a->indexExpr.get());
            if (!i || i->value != 0) return "";
            e = a->arrayExpr.get();
            continue;
        }
        break;
    }
    auto* v = dynamic_cast<VariableReference*>(e);
    return v ? v->name : "";
}

// an initializer's first byte (an integer constant, or a string's)
bool firstByte(Expression* init, int& b) {
    if (auto* l = dynamic_cast<InitializerList*>(init)) {
        if (l->elements.empty() || l->getDesignator(0).isDesignated()) return false;
        return firstByte(l->elements[0].get(), b);
    }
    if (auto* c = dynamic_cast<CastExpression*>(init)) return firstByte(c->expression.get(), b);
    if (auto* i = dynamic_cast<IntegerLiteral*>(init)) { b = (int)(i->value & 0xFF); return true; }
    return false;
}

std::string lower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}
std::string upper(std::string s) {
    for (auto& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

class Finder : public ASTWalker {
public:
    Finder(const std::vector<std::string>& dirs) : dirs_(dirs) {}
    using ASTWalker::visit;
    std::vector<Site> sites;
    std::map<std::string, int> arrays;      // name -> its first byte (-1: two declarations differ)

    void visit(VariableDeclaration& node) override {
        int b;
        if (!node.arrayDims.empty() && node.initializer && firstByte(node.initializer.get(), b)) {
            auto it = arrays.find(node.name);
            if (it == arrays.end()) arrays[node.name] = b;
            else if (it->second != b) it->second = -1;
        }
        ASTWalker::visit(node);
    }
    void visit(FunctionCall& node) override {
        if (!node.callExpr) {
            if ((node.name == "__unpack" || node.name == "__unpack_to") && !node.arguments.empty()) {
                std::string a = arrayOf(node.arguments[0].get());
                if (!a.empty()) calls_.push_back({a, &node});
            } else if (node.name == "load_unpack" && !node.arguments.empty()) {
                if (auto* s = dynamic_cast<StringLiteral*>(node.arguments[0].get())) fileCall(s->value, node);
            }
        }
        ASTWalker::visit(node);
    }
    // the array calls once every declaration is seen
    void finish() {
        for (const auto& c : calls_) {
            auto it = arrays.find(c.first);
            if (it == arrays.end() || (it->second != 1 && it->second != 2)) continue;
            add(it->second, *c.second, "array '" + c.first + "'");
        }
    }

private:
    void add(int f, FunctionCall& at, const std::string& what) {
        Site s;
        s.format = f;
        s.file = at.sourceFile;
        s.line = at.line;
        s.column = at.column;
        s.what = what;
        if (!s.file.empty() && s.line > 0) sites.push_back(s);
    }
    // load_unpack("NAME", ...): NAME's file beside the source or in an
    // include directory (as written, in lower or upper case, or with .prg /
    // .pak / .bin); its format byte follows the two-byte load address. More
    // than one file of different formats: not known
    void fileCall(const std::string& name, FunctionCall& at) {
        if (name.empty() || name.find('/') != std::string::npos) return;
        std::vector<std::string> dirs;
        std::string src = at.sourceFile;
        size_t slash = src.rfind('/');
        dirs.push_back(slash == std::string::npos ? "." : src.substr(0, slash));
        dirs.insert(dirs.end(), dirs_.begin(), dirs_.end());
        int format = 0;
        std::string found;
        for (const auto& d : dirs)
            for (const std::string& n : {name, lower(name), upper(name)})
                for (const char* ext : {"", ".prg", ".pak", ".bin", ".PRG", ".PAK", ".BIN"}) {
                    std::string path = d + "/" + n + ext;
                    std::ifstream in(path, std::ios::binary);
                    if (!in) continue;
                    char head[3];
                    if (!in.read(head, 3)) continue;
                    int f = (unsigned char)head[2];
                    if (f != 1 && f != 2) return;               // (not a stream: do not guess)
                    if (format && f != format) return;
                    format = f;
                    if (found.empty()) found = path;
                }
        if (format) add(format, at, "file " + found);
    }
    const std::vector<std::string>& dirs_;
    std::vector<std::pair<std::string, FunctionCall*>> calls_;
};

}  // namespace

std::vector<Site> find(TranslationUnit& unit, const std::vector<std::string>& dirs) {
    Finder f(dirs);
    unit.accept(f);
    f.finish();
    return f.sites;
}

std::set<int> declared(const std::vector<std::string>& fileAsm) {
    std::set<int> out;
    for (const auto& a : fileAsm)
        for (int n : {1, 2})
            if (a.find("___unpack_format" + std::to_string(n)) != std::string::npos) out.insert(n);
    return out;
}

}  // namespace unpackfmt
