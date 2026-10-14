#include "AsmWriter.hpp"
#include "AssemblerLexer.hpp"
#include <cstdio>

WriterRegistry& WriterRegistry::getInstance() {
    static WriterRegistry instance;
    return instance;
}

void WriterRegistry::registerWriter(const std::string& format, WriterFactory factory) {
    writers_[format] = factory;
}

std::unique_ptr<AsmWriter> WriterRegistry::createWriter(const std::string& format) {
    auto it = writers_.find(format);
    if (it != writers_.end()) {
        return std::unique_ptr<AsmWriter>(it->second());
    }
    return nullptr;
}

std::vector<std::string> WriterRegistry::getAvailableWriters() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : writers_) {
        result.push_back(name);
    }
    return result;
}

namespace {
// 'c' in an expression (outside "strings") as the number ca45 gives it
std::string charsAsNumbers(const std::string& e) {
    std::string out;
    bool dq = false;
    for (size_t i = 0; i < e.size(); i++) {
        char c = e[i];
        if (c == '"') dq = !dq;
        if (!dq && c == '\'' && i + 2 < e.size() && e[i + 2] == '\'' && e[i + 1] != '\\') {
            auto b = asmstr::encode(std::string(1, e[i + 1]), "text");
            char buf[8];
            std::snprintf(buf, sizeof(buf), "$%02X", b.empty() ? 0 : b[0]);
            out += buf;
            i += 2;
            continue;
        }
        out += c;
    }
    return out;
}
}  // namespace

AsmIR::Module AsmWriter::textAsBytes(const AsmIR::Module& module) {
    AsmIR::Module out = module;
    for (auto& st : out.statements) {
        if (st.type == AsmIR::Statement::Type::INSTRUCTION && st.instr.mode != AsmIR::AddressingMode::RAW)
            st.instr.operand.text = charsAsNumbers(st.instr.operand.text);
        if (st.type == AsmIR::Statement::Type::DIRECTIVE && st.dir.name != "text" &&
            st.dir.name != "screencode" && st.dir.name != "ascii")
            for (auto& a : st.dir.arguments) a = charsAsNumbers(a);
        if (st.type != AsmIR::Statement::Type::DIRECTIVE || st.dir.arguments.size() != 1) continue;
        const std::string& name = st.dir.name;
        if (name != "text" && name != "screencode" && name != "ascii") continue;
        std::vector<AssemblerToken> toks;
        try {
            toks = AssemblerLexer(st.dir.arguments[0]).tokenize();
        } catch (...) {
            continue;
        }
        if (toks.empty() || toks[0].type != AssemblerTokenType::STRING_LITERAL) continue;
        std::vector<std::string> bytes;
        for (uint8_t b : asmstr::encode(toks[0].value, name)) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "$%02X", b);
            bytes.push_back(buf);
        }
        std::string was = "." + name + " " + st.dir.arguments[0];
        st.comment = st.comment.empty() ? " " + was : " " + was + ";" + st.comment;
        st.dir.type = AsmIR::DirectiveType::BYTE_DATA;
        st.dir.name = "byte";
        st.dir.arguments = bytes;
    }
    return out;
}
