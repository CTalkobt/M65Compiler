#include "CompilerLog.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>
#ifdef __linux__
#include <unistd.h>
#endif
#include <fstream>
#include <sstream>
#include "ConfigLoader.hpp"
#include "CompilationPipeline.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "ConstantFolder.hpp"
#include "FunctionAnalyzer.hpp"
#include "OptimizationSelector.hpp"
#include "InlineSelector.hpp"
#include "CallGraphAnalyzer.hpp"
#include "DevirtualizationDetector.hpp"
#include "CoOptimizationSelector.hpp"
#include "DevirtualizationHints.hpp"
#include "CoOptimizationApplier.hpp"
#include "LoopOptimizer.hpp"
#include "LoopInterchange.hpp"
#include "Preprocessor.hpp"
#include "AssemblerLexer.hpp"
#include "AssemblerParser.hpp"
#include "AssemblerGenerator.hpp"
#include "M65Emitter.hpp"
#include "IRBuilder.hpp"
#include "IRCodeGen.hpp"
#include "IROptimizer.hpp"
#include "OptimizationCatalog.hpp"
#include "IPOAnalyzer.hpp"
#include "SpecializationCodeGenerator.hpp"
#include "SpecializationOptimizer.hpp"
#include "IRSpecializationGenerator.hpp"
#include "Version.hpp"
#include "Diagnostic.hpp"
#include "O45Reader.hpp"
#include "O45Writer.hpp"
#include "O45IRSerializer.hpp"
#include "HookIntegration.hpp"
#include "AdaptiveLearnerIntegration.hpp"
#include "TemplateOptimizationSystem.hpp"

class ASTPrinter : public ASTVisitor {
public:
    int indent = 0;
    void printIndent() { for (int i = 0; i < indent; i++) std::cout << "  "; }
    void visit(IntegerLiteral& node) override { printIndent(); std::cout << "IntegerLiteral: " << node.value << std::endl; }
    void visit(FloatLiteral& node) override { printIndent(); std::cout << "FloatLiteral: " << node.value << std::endl; }
    void visit(StringLiteral& node) override { printIndent(); std::cout << "StringLiteral: \"" << node.value << "\"" << std::endl; }
    void visit(VariableReference& node) override { printIndent(); std::cout << "VariableReference: " << node.name << std::endl; }
    void visit(Assignment& node) override {
        printIndent(); std::cout << "Assignment: " << node.op << std::endl;
        indent++;
        node.target->accept(*this);
        node.expression->accept(*this);
        indent--;
    }
    void visit(BinaryOperation& node) override {
        printIndent(); std::cout << "BinaryOperation: " << node.op << std::endl;
        indent++;
        node.left->accept(*this);
        node.right->accept(*this);
        indent--;
    }
    void visit(UnaryOperation& node) override {
        printIndent(); std::cout << "UnaryOperation: " << node.op << std::endl;
        indent++;
        node.operand->accept(*this);
        indent--;
    }
    void visit(FunctionCall& node) override {
        printIndent(); std::cout << "FunctionCall: " << node.name << std::endl;
        indent++;
        for (auto& arg : node.arguments) arg->accept(*this);
        indent--;
    }
    void visit(MemberAccess& node) override {
        printIndent(); std::cout << "MemberAccess: " << (node.isArrow ? "->" : ".") << node.memberName << std::endl;
        indent++;
        node.structExpr->accept(*this);
        indent--;
    }
    void visit(ConditionalExpression& node) override {
        printIndent(); std::cout << "ConditionalExpression:" << std::endl;
        indent++;
        printIndent(); std::cout << "Condition:" << std::endl;
        indent++;
        node.condition->accept(*this);
        indent--;
        printIndent(); std::cout << "Then:" << std::endl;
        indent++;
        if (node.thenExpr) node.thenExpr->accept(*this);
        else { printIndent(); std::cout << "(elvis: condition)" << std::endl; }
        indent--;
        printIndent(); std::cout << "Else:" << std::endl;
        indent++;
        node.elseExpr->accept(*this);
        indent--;
        indent--;
    }
    void visit(GenericSelection& node) override {
        printIndent(); std::cout << "GenericSelection:" << std::endl;
        indent++;
        printIndent(); std::cout << "Control:" << std::endl;
        indent++;
        node.control->accept(*this);
        indent--;
        printIndent(); std::cout << "Associations:" << std::endl;
        indent++;
        for (auto& assoc : node.associations) {
            printIndent();
            if (assoc.isDefault) std::cout << "default:";
            else {
                std::cout << assoc.typeName;
                for (int i = 0; i < assoc.pointerLevel; i++) std::cout << "*";
                std::cout << ":";
            }
            std::cout << std::endl;
            indent++;
            assoc.result->accept(*this);
            indent--;
        }
        indent--;
        indent--;
    }
    void visit(InitializerList& node) override {
        printIndent(); std::cout << "InitializerList:" << std::endl;
        indent++;
        for (auto& elem : node.elements) elem->accept(*this);
        indent--;
    }
    void visit(ArrayAccess& node) override {
        printIndent(); std::cout << "ArrayAccess:" << std::endl;
        indent++;
        printIndent(); std::cout << "Array:" << std::endl;
        indent++;
        node.arrayExpr->accept(*this);
        indent--;
        printIndent(); std::cout << "Index:" << std::endl;
        indent++;
        node.indexExpr->accept(*this);
        indent--;
        indent--;
    }
    void visit(CastExpression& node) override {
        printIndent(); std::cout << "CastExpression: (" << node.targetType;
        for (int i = 0; i < node.pointerLevel; i++) std::cout << "*";
        std::cout << ")" << std::endl;
        indent++;
        node.expression->accept(*this);
        indent--;
    }
    void visit(CompoundLiteral& node) override {
        printIndent(); std::cout << "CompoundLiteral: (" << node.targetType;
        for (int i = 0; i < node.pointerLevel; i++) std::cout << "*";
        if (!node.arrayDims.empty()) { std::cout << "["; for (int d : node.arrayDims) std::cout << d; std::cout << "]"; }
        std::cout << ")" << std::endl;
        indent++;
        node.initializer->accept(*this);
        indent--;
    }
    void visit(AlignofExpression& node) override {
        printIndent(); std::cout << "Alignof: " << node.typeName;
        for (int i = 0; i < node.pointerLevel; i++) std::cout << "*";
        std::cout << std::endl;
    }
    void visit(VariableDeclaration& node) override {
        printIndent(); std::cout << "VariableDeclaration: " << node.name << " (" << node.type;
        for (int i = 0; i < node.pointerLevel; ++i) std::cout << "*";
        std::cout << ")" << std::endl;
        if (node.initializer) {
            indent++;
            node.initializer->accept(*this);
            indent--;
        }
    }

    void visit(ReturnStatement& node) override {
        printIndent(); std::cout << "ReturnStatement" << std::endl;
        if (node.expression) {
            indent++;
            node.expression->accept(*this);
            indent--;
        }
    }
    void visit(BreakStatement& node) override {
        printIndent(); std::cout << "BreakStatement"; if (node.levels > 1) std::cout << " " << node.levels; std::cout << std::endl;
    }
    void visit(ContinueStatement&) override {
        printIndent(); std::cout << "ContinueStatement" << std::endl;
    }
    void visit(SwitchContinueStatement& node) override {
        printIndent(); std::cout << "SwitchContinueStatement";
        if (node.target) {
            std::cout << ":" << std::endl;
            indent++;
            node.target->accept(*this);
            indent--;
        } else {
            std::cout << " (default)" << std::endl;
        }
    }
    void visit(GotoStatement& node) override {
        printIndent(); std::cout << "GotoStatement: " << node.label;
        if (node.target) {
            std::cout << " *";
            indent++; node.target->accept(*this); indent--;
        }
        std::cout << std::endl;
    }
    void visit(LabelledStatement& node) override {
        printIndent(); std::cout << "Label: " << node.label << ":" << std::endl;
        indent++;
        node.statement->accept(*this);
        indent--;
    }
    void visit(SizeofExpression& node) override {
        printIndent(); std::cout << "Sizeof: ";
        if (node.isType) {
            std::cout << node.typeName;
            for (int i = 0; i < node.pointerLevel; i++) std::cout << "*";
        } else {
            std::cout << "expr" << std::endl;
            indent++;
            node.expression->accept(*this);
            indent--;
        }
        std::cout << std::endl;
    }
    void visit(ExpressionStatement& node) override {
        printIndent(); std::cout << "ExpressionStatement" << std::endl;
        indent++;
        node.expression->accept(*this);
        indent--;
    }
    void visit(IfStatement& node) override {
        printIndent(); std::cout << "IfStatement" << (node.hint > 0 ? " (likely)" : node.hint < 0 ? " (unlikely)" : "") << std::endl;
        indent++;
        printIndent(); std::cout << "Condition:" << std::endl;
        indent++;
        node.condition->accept(*this);
        indent--;
        printIndent(); std::cout << "Then:" << std::endl;
        indent++;
        node.thenBranch->accept(*this);
        indent--;
        if (node.elseBranch) {
            printIndent(); std::cout << "Else:" << std::endl;
            indent++;
            node.elseBranch->accept(*this);
            indent--;
        }
        indent--;
    }
    void visit(WhileStatement& node) override {
        printIndent(); std::cout << "WhileStatement" << std::endl;
        indent++;
        printIndent(); std::cout << "Condition:" << std::endl;
        indent++;
        node.condition->accept(*this);
        indent--;
        printIndent(); std::cout << "Body:" << std::endl;
        indent++;
        node.body->accept(*this);
        indent--;
        indent--;
    }
    void visit(DoWhileStatement& node) override {
        printIndent(); std::cout << "DoWhileStatement" << std::endl;
        indent++;
        printIndent(); std::cout << "Body:" << std::endl;
        indent++;
        node.body->accept(*this);
        indent--;
        printIndent(); std::cout << "Condition:" << std::endl;
        indent++;
        node.condition->accept(*this);
        indent--;
        indent--;
    }
    void visit(ForStatement& node) override {
        printIndent(); std::cout << "ForStatement" << std::endl;
        if (node.initializer) {
            printIndent(); std::cout << "Initializer:" << std::endl;
            indent++;
            node.initializer->accept(*this);
            indent--;
        }
        if (node.condition) {
            printIndent(); std::cout << "Condition:" << std::endl;
            indent++;
            node.condition->accept(*this);
            indent--;
        }
        if (node.increment) {
            printIndent(); std::cout << "Increment:" << std::endl;
            indent++;
            node.increment->accept(*this);
            indent--;
        }
        printIndent(); std::cout << "Body:" << std::endl;
        indent++;
        node.body->accept(*this);
        indent--;
    }
    void visit(RepeatStatement& node) override {
        printIndent(); std::cout << "RepeatStatement: count=" << node.count;
        if (!node.varName.empty()) std::cout << " var=" << node.varType << " " << node.varName;
        std::cout << std::endl;
        indent++; node.body->accept(*this); indent--;
    }
    void visit(SwitchStatement& node) override {
        printIndent(); std::cout << "SwitchStatement" << std::endl;
        indent++;
        printIndent(); std::cout << "Expression:" << std::endl;
        indent++;
        node.expression->accept(*this);
        indent--;
        printIndent(); std::cout << "Body:" << std::endl;
        indent++;
        node.body->accept(*this);
        indent--;
        indent--;
    }
    void visit(CaseStatement& node) override {
        printIndent(); std::cout << "CaseStatement" << (node.rangeEnd ? " (range)" : "") << ": " << std::endl;
        indent++;
        node.value->accept(*this);
        if (node.rangeEnd) { printIndent(); std::cout << "..." << std::endl; node.rangeEnd->accept(*this); }
        indent--;
    }
    void visit(DefaultStatement&) override {
        printIndent(); std::cout << "DefaultStatement" << std::endl;
    }
    void visit(AsmStatement& node) override {
        printIndent(); std::cout << "AsmStatement: " << node.code << std::endl;
    }
    void visit(StaticAssert& node) override {
        printIndent(); std::cout << "StaticAssert: " << node.message << std::endl;
    }
    void visit(EnumDefinition& node) override {
        printIndent(); std::cout << "EnumDefinition: " << node.name << std::endl;
        indent++;
        for (const auto& enumerator : node.enumerators) {
            printIndent();
            std::cout << enumerator.first << " = " << enumerator.second << std::endl;
        }
        indent--;
    }
    void visit(StructDefinition& node) override {
        printIndent(); std::cout << (node.isUnion ? "UnionDefinition: " : "StructDefinition: ") << node.name << std::endl;
        indent++;
        for (const auto& member : node.members) {
            std::string ptrs = "";
            for (int i = 0; i < member.pointerLevel; i++) ptrs += "*";
            printIndent(); 
            if (member.isAnonymous) std::cout << "(Anonymous) ";
            std::cout << "Member: " << member.name << " (" << member.type << ptrs << ")" << std::endl;
        }
        indent--;
    }
    void visit(CompoundStatement& node) override {
        printIndent(); std::cout << "CompoundStatement" << std::endl;
        indent++;
        for (auto& stmt : node.statements) stmt->accept(*this);
        indent--;
    }
    void visit(BuiltinVaStart& node) override {
        printIndent(); std::cout << "BuiltinVaStart: last=" << node.lastParamName << std::endl;
        indent++; node.ap->accept(*this); indent--;
    }
    void visit(BuiltinVaArg& node) override {
        printIndent(); std::cout << "BuiltinVaArg: " << node.typeName << std::endl;
        indent++; node.ap->accept(*this); indent--;
    }
    void visit(CpuRegisterAccess& node) override {
        printIndent(); std::cout << "CpuRegisterAccess: " << node.regName << std::endl;
    }
    void visit(CpuFlagAccess& node) override {
        printIndent(); std::cout << "CpuFlagAccess: " << node.flagName << std::endl;
    }
    void visit(LabelAddressExpression& node) override {
        printIndent(); std::cout << "LabelAddressExpression: &&" << node.label << std::endl;
    }
    void visit(FunctionDeclaration& node) override {
        printIndent(); std::cout << "FunctionDeclaration: " << node.name << " (" << (node.isSigned ? "signed " : "") << node.returnType << ")";
        if (node.isNoreturn) std::cout << " [noreturn]";
        if (node.isVariadic) std::cout << " [variadic]";
        if (node.isNested) std::cout << " [nested]";
        std::cout << std::endl;
        indent++;
        for (const auto& param : node.parameters) {
            std::string ptrs = "";
            for (int i = 0; i < param.pointerLevel; i++) ptrs += "*";
            printIndent(); std::cout << "Parameter: " << param.name << " (" << (param.isSigned ? "signed " : "") << param.type << ptrs << ")" << std::endl;
        }
        if (node.body) node.body->accept(*this);
        indent--;
    }
    void visit(TranslationUnit& node) override {
        printIndent(); std::cout << "TranslationUnit" << std::endl;
        indent++;
        for (auto& decl : node.topLevelDecls) if (decl) decl->accept(*this);
        indent--;
    }
};

int main(int argc, char** argv) {
    std::string programName = argv[0];
    size_t lastSlash = programName.find_last_of("/\\");
    if (lastSlash != std::string::npos) programName = programName.substr(lastSlash + 1);

    // Load configuration from ~/.config/m65/<program>.conf
    std::vector<std::string> configTokens = ConfigLoader::loadConfig(programName);
    std::vector<std::string> allArgs;
    for (const auto& tok : configTokens) allArgs.push_back(tok);
    for (int i = 1; i < argc; ++i) allArgs.push_back(std::string(argv[i]));

    // Parse arguments
    CompilationConfig config;
    config.preprocessOnly = (programName == "cp45");
    std::vector<std::string> includePaths;
    std::map<std::string, std::string> symbols;
    bool outputFileSet = false;

    // Add paths from CC45_INCLUDE environment variable
    if (const char* envInc = std::getenv("CC45_INCLUDE")) {
        std::string s(envInc);
        size_t pos = 0, found;
        while ((found = s.find(':', pos)) != std::string::npos) {
            if (found > pos) includePaths.push_back(s.substr(pos, found - pos));
            pos = found + 1;
        }
        if (pos < s.size()) includePaths.push_back(s.substr(pos));
    }

    // Add default system include path
    {
        std::string exePath;
#ifdef __linux__
        char buf[4096];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len > 0) { buf[len] = '\0'; exePath = buf; }
#endif
        if (exePath.empty()) {
            char* resolved = realpath(argv[0], nullptr);
            if (resolved) { exePath = resolved; free(resolved); }
            else exePath = argv[0];
        }
        size_t sep = exePath.find_last_of("/\\");
        std::string baseDir = (sep != std::string::npos) ? exePath.substr(0, sep + 1) : "";
        // The system headers, after CC45_INCLUDE: the build tree's
        // bin/../lib/include, else an installed tree's bin/../include/cc45
        // (make install's PREFIX/include/cc45 next to PREFIX/bin)
        std::string sysInc = baseDir + "../lib/include";
        for (const char* rel : {"../lib/include", "../include/cc45"}) {
            std::string dir = baseDir + rel;
            if (access((dir + "/stdio.h").c_str(), R_OK) == 0) { sysInc = dir; break; }
        }
        includePaths.push_back(sysInc);
        config.toolDir = baseDir;
    }

    std::vector<std::string> requestedOpts;  // catalog optimizations named by -f flags
    // Parse all arguments
    for (size_t i = 0; i < allArgs.size(); ++i) {
        std::string arg = allArgs[i];
        if (arg == "-V" || arg == "--version") {
            std::cout << suiteVersionString("cc45") << std::endl;
            return 0;
        } else if (arg == "-?" || arg == "--help") {
            // Phase 5.1: Dynamic program name in help output
            std::string inputType = (programName == "cp45") ? "<input_file.c>" : "[options] <input_file.c>";
            std::cout << "Usage: " << programName << " " << inputType << std::endl;
            if (programName != "cp45") {
                std::cout << "  -E             Preprocess only" << std::endl;
                std::cout << "  -S             Generate assembly only" << std::endl;
                std::cout << "  -g             With -o prog.prg: also write prog.dbg, the debug information (ln45 -g)" << std::endl;
                std::cout << "  -c             Generate object file only" << std::endl;
            } else {
                std::cout << "  (cp45 performs C preprocessing)" << std::endl;
            }
            std::cout << "  -o <file>      Output filename" << std::endl;
            if (programName != "cp45") {
                std::cout << "  -O0..9         Optimization level" << std::endl;
            }
            std::cout << "  -v,-vv         Verbose output" << std::endl;
            std::cout << "  -I<path>       Include path" << std::endl;
            std::cout << "  -D<name>=<val> Define symbol" << std::endl;
            if (programName != "cp45") {
                std::cout << "  -fzpcall       ZP parameter calling convention (default)" << std::endl;
                std::cout << "  -fno-zpcall    Stack parameter calling convention" << std::endl;
                std::cout << "  -fno-stack16   Keep the ROM's 8-bit CPU stack on page 1 (default: 16-bit stack topped at $BFFF)" << std::endl;
                std::cout << "  -fstack-top=ADDR  Top byte of the 16-bit CPU stack (e.g. 0x9FFF or $9FFF)" << std::endl;
                std::cout << "  -fbss-fill=V   Byte crt0 fills BSS with at startup (default $00; e.g. $FF to find" << std::endl;
                std::cout << "                 code that relies on uninitialized data)" << std::endl;
                std::cout << "  -fstaticalloc  Use static allocation (default)" << std::endl;
                std::cout << "  -fno-zp-locals  No base-page blocks ($80-$E9) for pointer variables of functions" << std::endl;
                std::cout << "                 that make calls (for call chains that do not fit there)" << std::endl;
                std::cout << "  -fzp-locals-max=N  Bytes of such a block (default 16; 0 = none)" << std::endl;
                std::cout << "  -fzp-plan=FILE     The blocks' variables as ln45 --zp-plan-out chose them program-wide" << std::endl;
                std::cout << "  -fmul-call     Multiplies as calls of the library's __mul16/__mul32 (smaller," << std::endl;
                std::cout << "                 slower; -Os = -O2 -fmul-call)" << std::endl;
                std::cout << "  -finline-functions  Inline small functions" << std::endl;
                std::cout << "  -fmax-inline-ir-size=N  Max IR instructions for an inlined function (default 11)" << std::endl;
                std::cout << "  -fmax-unroll-bytes=N    Max estimated code bytes of an unrolled loop (default 128)" << std::endl;
                std::cout << "  -fsigned-default  Plain char/short/int/long are signed (standard C); default is unsigned" << std::endl;
                std::cout << "  -funsigned-char   With -fsigned-default, keep plain char unsigned" << std::endl;
                std::cout << "  -f<opt> / -fno-<opt>  Enable / disable an optimization (e.g. -fno-cse,"
                          << " -floop-invariant-code-motion; see doc/architecture/optimizations.md)" << std::endl;
                std::cout << "  --future       Warn when an optimization that is not implemented yet is requested" << std::endl;
                std::cout << "  -Roptir        Report IR optimizer actions to stderr" << std::endl;
                std::cout << "  --pragma <p>   Inject pragma" << std::endl;
                std::cout << "  --prg-base <a> PRG load address (hex)" << std::endl;
                std::cout << "  --save-temps   Keep intermediate files" << std::endl;
            }
            std::cout << "Configuration: See ~/.config/m65/" << programName << ".conf or doc/bin/CONFIGURATION.md" << std::endl;
            return 0;
        } else if (arg == "-c") {
            config.objectOnly = true;
        } else if (arg == "-S") {
            config.assemblyOnly = true;
        } else if (arg == "-E") {
            config.preprocessOnly = true;
        } else if (arg == "--save-temps") {
            config.saveTemps = true;
        } else if (arg == "-o" && i + 1 < allArgs.size()) {
            config.outputFile = allArgs[++i];
            outputFileSet = true;
        } else if (arg == "-fzpcall") {
            config.zpCallMode = true;
        } else if (arg == "-fno-zpcall") {
            config.zpCallMode = false;
        } else if (arg == "-fno-stack16") {
            config.stackTop = 0;
        } else if (arg == "-fstack16") {
            config.stackTop = -1;
        } else if (arg.rfind("-fstack-top=", 0) == 0) {
            std::string v = arg.substr(12);
            int base = 10;
            if (!v.empty() && v[0] == '$') { v = v.substr(1); base = 16; }
            else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) { v = v.substr(2); base = 16; }
            try { config.stackTop = (int)(std::stoul(v, nullptr, base) & 0xFFFF); }
            catch (...) { std::cerr << "cc45: error: invalid -fstack-top value '" << arg.substr(12) << "'" << std::endl; return 1; }
        } else if (arg == "-g") {
            config.debugFile = true;   // (one-step build: ln45 -g writes <program>.dbg)
        } else if (arg == "-ffunction-sections") {
            config.functionSections = true;
        } else if (arg == "-fno-function-sections") {
            config.functionSections = false;
        } else if (arg.rfind("-foverlay=", 0) == 0) {
            config.overlayName = arg.substr(10);
            bool ok = !config.overlayName.empty();
            for (char ch : config.overlayName) if (!std::isalnum((unsigned char)ch) && ch != '_') ok = false;
            if (!ok) { std::cerr << "cc45: error: invalid -foverlay name '" << config.overlayName << "' (letters, digits, _)" << std::endl; return 1; }
        } else if (arg.rfind("-fstack-size=", 0) == 0) {
            std::string v = arg.substr(13);
            int base = 10;
            if (!v.empty() && v[0] == '$') { v = v.substr(1); base = 16; }
            else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) { v = v.substr(2); base = 16; }
            unsigned long sv = 0;
            try { size_t used = 0; sv = std::stoul(v, &used, base); if (used != v.size() || sv > 0xFFFF) throw 0; }
            catch (...) { std::cerr << "cc45: error: invalid -fstack-size value '" << arg.substr(13) << "' (0-65535, $1000, 0x1000)" << std::endl; return 1; }
            config.stackSize = (int)sv;
        } else if (arg.rfind("-fbss-fill=", 0) == 0) {
            std::string v = arg.substr(11);
            int base = 10;
            if (!v.empty() && v[0] == '$') { v = v.substr(1); base = 16; }
            else if (v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) { v = v.substr(2); base = 16; }
            unsigned long fv = 0;
            try { size_t used = 0; fv = std::stoul(v, &used, base); if (used != v.size() || fv > 255) throw 0; }
            catch (...) { std::cerr << "cc45: error: invalid -fbss-fill value '" << arg.substr(11) << "' (a byte: 0-255, $FF, 0xFF)" << std::endl; return 1; }
            config.bssFill = (int)fv;
        } else if (arg == "-fstaticalloc") {
            config.staticAllocMode = true;
        } else if (arg == "-fno-staticalloc") {
            config.staticAllocMode = false;
        } else if (arg == "-fmul-call") {
            // multiplies as calls of __mul16 / __mul32 (smaller, slower)
            setenv("CC45_MUL_CALL", "1", 1);
        } else if (arg == "-fno-mul-call") {
            // (inline everywhere; without either, calls outside loops)
            setenv("CC45_MUL_CALL", "0", 1);
        } else if (arg == "-fno-zp-locals") {
            // no base-page blocks ($80-$E9) for pointer variables of functions that make calls
            setenv("CC45_NO_ZP_LOCALS", "1", 1);
        } else if (arg == "-fzp-locals") {
            unsetenv("CC45_NO_ZP_LOCALS");
        } else if (arg.rfind("-fzp-plan=", 0) == 0) {
            // ln45's program-wide choice of base-page block variables (#304)
            setenv("CC45_ZP_PLAN", arg.substr(10).c_str(), 1);
        } else if (arg.rfind("-fzp-locals-max=", 0) == 0) {
            // bytes of a function's base-page block (default 16)
            setenv("CC45_ZP_LOCALS_MAX", arg.substr(16).c_str(), 1);
        } else if (arg == "--prg-base" && i + 1 < allArgs.size()) {
            config.prgBase = std::stoul(allArgs[++i], nullptr, 16);
            config.prgBaseSet = true;
        } else if (arg == "-fimplicit-function-declaration") {
            config.allowImplicitFunctionDecl = true;
        } else if (arg == "-fno-implicit-function-declaration") {
            config.allowImplicitFunctionDecl = false;
        } else if (arg == "-finline-functions") {
            config.inlineSmallFunctions = true;
        } else if (arg == "-fsigned-default") {
            config.signedDefault = true;
        } else if (arg == "-fno-signed-default") {
            config.signedDefault = false;
        } else if (arg == "-funsigned-char") {
            config.unsignedChar = true;
        } else if (arg == "-fsigned-char") {
            config.unsignedChar = false;
        } else if (arg.rfind("-fmax-unroll-bytes=", 0) == 0) {
            std::string sizeStr = arg.substr(19);
            try {
                config.maxUnrollBytes = std::stoi(sizeStr);
            } catch (...) {
                std::cerr << "Invalid size for -fmax-unroll-bytes: " << sizeStr << std::endl;
            }
        } else if (arg.rfind("-fmax-inline-ir-size=", 0) == 0) {
            std::string sizeStr = arg.substr(21);
            try {
                config.maxInlineIRSize = std::stoi(sizeStr);
            } catch (...) {
                std::cerr << "Invalid IR size for -fmax-inline-ir-size: " << sizeStr << std::endl;
            }
        } else if (arg == "--pragma" && i + 1 < allArgs.size()) {
            config.cliPragmas.push_back(allArgs[++i]);
        } else if (arg == "--emit-ir") {
            config.emitIR = true;
        } else if (arg == "-Rcodegen") {
            config.emitReasons = true;
        } else if (arg == "-Roptir") {
            config.traceIROpt = true;
        } else if (arg == "--future") {
            config.futureWarnings = true;
        } else if (arg.substr(0, 2) == "-O") {
            std::string levelStr = arg.substr(2);
            if (!levelStr.empty() && levelStr[0] >= '0' && levelStr[0] <= '9') {
                config.optimizationLevel = levelStr[0] - '0';
            } else if (levelStr == "s" || levelStr == "size") {
                // -Os: -O2, and code size over speed (multiplies as calls, no
                // loop rotation unless -frotate-loops)
                config.optimizationLevel = 2;
                setenv("CC45_MUL_CALL", "1", 1);
                config.optOverrides.emplace("LoopRotation", false);
            } else if (levelStr == "speed") {
                config.optimizationLevel = 3;
            } else {
                config.optimizationLevel = 0;
            }
        } else if (arg == "-vv") {
            config.verboseLevel = 2;
        } else if (arg == "-v") {
            config.verboseLevel = 1;
        } else if (arg == "-I" && i + 1 < allArgs.size()) {
            includePaths.push_back(allArgs[++i]);
        } else if (arg.substr(0, 2) == "-I") {
            includePaths.push_back(arg.substr(2));
        } else if (arg.substr(0, 2) == "-D") {
            size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                std::string name = arg.substr(2, eq - 2);
                std::string valStr = arg.substr(eq + 1);
                symbols[name] = valStr;
            } else {
                symbols[arg.substr(2)] = "1";
            }
        } else if (arg.size() > 2 && arg.compare(0, 2, "-f") == 0) {
            // -f<optimization> / -fno-<optimization> (OptimizationCatalog)
            bool enable = arg.compare(0, 5, "-fno-") != 0;
            std::string name = enable ? arg.substr(2) : arg.substr(5);
            if (const OptimizationInfo* o = OptimizationCatalog::find(name)) {
                config.optOverrides[o->name] = enable;
                requestedOpts.push_back(o->name);
            } else {
                std::cerr << programName << ": warning: unknown option '" << arg << "' ignored" << std::endl;
            }
        } else if (arg.size() > 1 && arg[0] == '-') {
            std::cerr << programName << ": warning: unknown option '" << arg << "' ignored" << std::endl;
        } else {
            config.inputFile = arg;
        }
    }

    // --future: a requested optimization that is not implemented (any order of
    // --future and the -f flags); -fno-<always on> cannot take effect
    for (const auto& n : requestedOpts) {
        const OptimizationInfo* o = OptimizationCatalog::getOptimization(n);
        if (!o) continue;
        if (!o->implemented()) {
            if (config.futureWarnings)
                std::cerr << programName << ": warning: optimization '" << o->kebabName()
                          << "' is not implemented yet; -f" << (config.optOverrides[n] ? "" : "no-")
                          << o->kebabName() << " has no effect" << std::endl;
        } else if (o->impl == OptimizationImpl::ALWAYS_ON && !config.optOverrides[n]) {
            std::cerr << programName << ": warning: '" << o->kebabName()
                      << "' is part of code generation and cannot be disabled" << std::endl;
        }
    }

    if (config.inputFile.empty()) {
        // Phase 5.1: Dynamic program name in error message
        std::cerr << "Usage: " << programName << " [options] <input_file.c>" << std::endl;
        return 1;
    }

    // Set compilation config
    config.includePaths = includePaths;
    config.symbols = symbols;

    // Use CompilationPipeline to compile
    g_cc45Verbosity = config.verboseLevel;
    CompilationPipeline pipeline(config);
    CompilationResult result = pipeline.compile();

    if (!result.success) {
        if (!result.errorPrinted) std::cerr << "Compilation error: " << result.error << std::endl;
        return result.exitCode;
    }

    if (config.verboseLevel >= 1) {
        std::cout << "Compilation successful: " << result.outputFile << std::endl;
    }

    return 0;
}
