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
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "ConstantFolder.hpp"
#include "LoopOptimizer.hpp"
#include "Preprocessor.hpp"
#include "AssemblerLexer.hpp"
#include "AssemblerParser.hpp"
#include "AssemblerGenerator.hpp"
#include "M65Emitter.hpp"
#include "IRBuilder.hpp"
#include "IRCodeGen.hpp"
#include "IROptimizer.hpp"
#include "Version.hpp"
#include "Diagnostic.hpp"

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
    void visit(BreakStatement&) override {
        printIndent(); std::cout << "BreakStatement" << std::endl;
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
        printIndent(); std::cout << "IfStatement" << std::endl;
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
    // Config tokens are parsed first, then real argv (which overrides config)
    std::vector<std::string> configTokens = ConfigLoader::loadConfig(programName);
    std::vector<std::string> allArgs;
    // Add config tokens first
    for (const auto& tok : configTokens) allArgs.push_back(tok);
    // Add real argv (skip argv[0] which is program name)
    for (int i = 1; i < argc; ++i) allArgs.push_back(std::string(argv[i]));

    // Now parse all arguments (config + CLI)
    std::string input_file;
    bool preprocessOnly = (programName == "cp45");
    std::string output_file = "";
    bool outputFileSet = false;
    bool objectFileOnly = false;  // -c: stop after assembling to .o45
    bool assemblyOnly = false;    // -S: stop after code generation (assembly text)
    bool saveTemps = false;       // --save-temps: keep .s and .o45 files
    int verboseLevel = 0;
    bool optimize = true;
    OptimizationFlags irOptFlags = OptimizationFlags::fromLevel(2);  // IR optimizer flags
    int listingLevel = 1;
    uint32_t zeroPageStart = 0x08;
    bool zpCallMode = false;
    bool inlineFunctions = false;
    bool emitIR = false;
    bool emitReasons = false;
    bool traceIROpt = false;
    uint32_t zeroPageAvail = 9;
    std::string defineFlag = "";
    std::map<std::string, std::string> initialSymbols;
    std::vector<std::string> includePaths;
    std::vector<std::string> cliPragmas;

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

    // Add default system include path relative to the resolved binary location.
    // Uses /proc/self/exe (Linux) or realpath(argv[0]) to handle symlinks,
    // PATH lookups, and invocation from any working directory.
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
        includePaths.push_back(baseDir + "../lib/include");
    }

    for (size_t i = 0; i < allArgs.size(); ++i) {
        std::string arg = allArgs[i];
        if (arg == "-V" || arg == "--version") {
            std::cout << suiteVersionString("cc45") << std::endl;
            return 0;
        } else if (arg == "-?" || arg == "--help") {
            std::cout << "Usage: cc45 [options] <input_file.c>" << std::endl;
            std::cout << std::endl;
            std::cout << "Compilation Pipeline:" << std::endl;
            std::cout << "  Default:   C → Assembly → Object → Link → Executable (.prg)" << std::endl;
            std::cout << "  With -S:   C → Assembly (.s only)" << std::endl;
            std::cout << "  With -c:   C → Assembly → Object (.o45 relocatable)" << std::endl;
            std::cout << "  With -E:   C → Preprocessor output (C only)" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -E             Run only the preprocessor (output to stdout or -o file)" << std::endl;
            std::cout << "  -S             Produce assembly text only (stop after code generation)" << std::endl;
            std::cout << "  -c             Produce .o45 relocatable object file (compile+assemble, no link)" << std::endl;
            std::cout << "  -o <filename>  Specify output filename" << std::endl;
            std::cout << "                   (default: out.prg for full pipeline, out.o45 with -c, out.s with -S)" << std::endl;
            std::cout << "                 Automatically infers type from extension (.prg, .o45, .s)" << std::endl;
            std::cout << "  -l <level>     Listing level: 1=Standard (default), 2=Expanded" << std::endl;
            std::cout << "  -v             Enable verbose output (phase info)" << std::endl;
            std::cout << "  -vv            Extra verbose output (token dumps, AST)" << std::endl;
            std::cout << "  -O0            Disable optimizations" << std::endl;
            std::cout << "  --save-temps   Keep intermediate .s and .o45 files" << std::endl;
            std::cout << "  -fzpcall       Use ZP parameter block calling convention" << std::endl;
            std::cout << "  -fno-zpcall    Use stack-based calling convention (default)" << std::endl;
            std::cout << "  -finline-functions  Inline small functions at call sites" << std::endl;
            std::cout << "  --pragma \"...\"  Inject a #pragma directive (e.g., --pragma \"cc45 heap\")" << std::endl;
            std::cout << "  -Dname=val     Define a symbol (e.g., -Dcc45.zeroPageStart=$10)" << std::endl;
            std::cout << "  -I<path>       Add include search path" << std::endl;
            std::cout << "  -Rcodegen      Annotate assembly output with codegen reasoning comments" << std::endl;
            std::cout << "  -Roptir        Trace IR optimizer actions to stderr" << std::endl;
            std::cout << "  -Roptimizer    Report assembler optimizer actions to stderr" << std::endl;
            std::cout << "  -Rmachstate    Trace assembler MachineState register/flag tracking" << std::endl;
            std::cout << "  -?             Display this help message" << std::endl;
            return 0;
        } else if (arg == "-c") {
            objectFileOnly = true;
        } else if (arg == "-S") {
            assemblyOnly = true;
        } else if (arg == "-E") {
            preprocessOnly = true;
        } else if (arg == "--save-temps") {
            saveTemps = true;
        } else if (arg == "-o" && i + 1 < allArgs.size()) {
            output_file = allArgs[++i];
            outputFileSet = true;
        } else if (arg == "-l" && i + 1 < allArgs.size()) {
            listingLevel = std::stoi(allArgs[++i]);
        } else if (arg == "-fzpcall") {
            zpCallMode = true;
        } else if (arg == "-fno-zpcall") {
            zpCallMode = false;
        } else if (arg == "-finline-functions") {
            inlineFunctions = true;
        } else if (arg == "-fno-inline-functions") {
            inlineFunctions = false;
        } else if (arg.rfind("-fset-bp=", 0) == 0) {
            cliPragmas.push_back("cc45 set_bp " + arg.substr(9));
        } else if (arg == "--pragma" && i + 1 < allArgs.size()) {
            cliPragmas.push_back(allArgs[++i]);
        } else if (arg == "--emit-ir") {
            emitIR = true;
        } else if (arg == "-Rcodegen") {
            emitReasons = true;
        } else if (arg == "-Roptir") {
            traceIROpt = true;
        } else if (arg == "-Roptimizer") {
            emitReasons = true; // also enable codegen reasons for context
            // Flag will be passed to ca45 subprocess below
        } else if (arg.substr(0, 2) == "-P") {
            // Named optimization flags: apply to IR optimizer and forward to ca45
            std::string flagName = arg.substr(2);
            bool enable = true;
            if (flagName.substr(0, 2) == "No") {
                enable = false;
                flagName = flagName.substr(2);
            }
            // IR-level optimizations
            if (flagName == "StrengthReduction") irOptFlags.strengthReduction = enable;
            else if (flagName == "AlgebraicSimplify") irOptFlags.algebraicSimplify = enable;
            else if (flagName == "TypeNarrowing") irOptFlags.typeNarrowing = enable;
            else if (flagName == "BranchFold") irOptFlags.branchFold = enable;
            else if (flagName == "CSE") irOptFlags.cse = enable;
            else if (flagName == "LICM") irOptFlags.licm = enable;
            else if (flagName == "CopyChains") irOptFlags.copyChains = enable;
            else if (flagName == "AddrElemFusion") irOptFlags.addrElemFusion = enable;
            // Assembler-level: will be forwarded to ca45 subprocess
        } else if (arg.substr(0, 2) == "-O") {
            std::string levelStr = arg.substr(2);
            if (!levelStr.empty() && levelStr[0] >= '0' && levelStr[0] <= '3') {
                int level = levelStr[0] - '0';
                optimize = (level > 0);
                irOptFlags = OptimizationFlags::fromLevel(level);
            } else {
                optimize = false;
                irOptFlags = OptimizationFlags::fromLevel(0);
            }
        } else if (arg == "-vv") {
            verboseLevel = 2;
        } else if (arg == "-v") {
            verboseLevel = 1;
        } else if (arg == "-I" && i + 1 < allArgs.size()) {
            includePaths.push_back(allArgs[++i]);
        } else if (arg.substr(0, 2) == "-I") {
            includePaths.push_back(arg.substr(2));
        } else if (arg.substr(0, 2) == "-D") {
            defineFlag = arg;
            size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                std::string name = arg.substr(2, eq - 2);
                std::string valStr = arg.substr(eq + 1);
                initialSymbols[name] = valStr;
                uint32_t val = 0;
                if (valStr.empty()) {}
                else if (valStr.substr(0, 1) == "$") val = std::stoul(valStr.substr(1), nullptr, 16);
                else if (valStr.substr(0, 1) == "%") val = std::stoul(valStr.substr(1), nullptr, 2);
                else val = std::stoul(valStr);

                if (name == "cc45.zeroPageStart") {
                    zeroPageStart = val;
                } else if (name == "cc45.zeroPageAvail") {
                    zeroPageAvail = val;
                }
            } else {
                initialSymbols[arg.substr(2)] = "1";
            }
        } else {
            input_file = arg;
        }
    }

    if (input_file.empty()) {
        std::cerr << "Usage: cc45 [options] <input_file.c>" << std::endl;
        std::cerr << "Use -? for a list of options." << std::endl;
        return 1;
    }

    std::ifstream file(input_file);
    if (!file.is_open()) {
        std::cerr << "Failed to open input file: " << input_file << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sourceRaw = buffer.str();

    if (verboseLevel >= 1) {
        std::cout << "Preprocessing " << input_file << "..." << std::endl;
    }

    // Inject --pragma arguments as #pragma lines before source
    if (!cliPragmas.empty()) {
        std::string prefix;
        for (const auto& p : cliPragmas) {
            prefix += "#pragma " + p + "\n";
        }
        sourceRaw = prefix + sourceRaw;
    }

    Preprocessor preprocessor(true);
    std::string source;
    try {
        source = preprocessor.process(sourceRaw, initialSymbols, includePaths, input_file);
    } catch (const std::exception& e) {
        std::cerr << formatDiagnostic(input_file, 1, 1, Severity::Error, e.what()) << std::endl;
        return 1;
    }

    if (preprocessOnly) {
        if (outputFileSet) {
            std::ofstream out(output_file);
            if (!out.is_open()) {
                std::cerr << "Failed to open output file: " << output_file << std::endl;
                return 1;
            }
            out << source;
            out.close();
        } else {
            std::cout << source;
        }
        return 0;
    }

    if (!outputFileSet) {
        std::string base = input_file;
        size_t dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);

        if (objectFileOnly) {
            // -c mode: default output is input.o45
            output_file = base + ".o45";
        } else if (assemblyOnly) {
            // -S mode: default output is input.s
            output_file = base + ".s";
        } else {
            // Default: compile+assemble+link to .prg
            output_file = base + ".prg";
        }
    } else {
        // Infer output type from filename extension when -o is used
        size_t dot = output_file.rfind('.');
        if (dot != std::string::npos) {
            std::string ext = output_file.substr(dot);
            if (ext == ".s") {
                assemblyOnly = true;
            } else if (ext == ".o45") {
                objectFileOnly = true;
            }
            // .prg or other extensions: full pipeline (default)
        }
    }

    std::vector<std::string> sourceLines;
    {
        std::stringstream ss(source);
        std::string line;
        while (std::getline(ss, line)) {
            sourceLines.push_back(line);
        }
    }

    if (verboseLevel >= 1) {
        std::cout << "Lexing " << input_file << "..." << std::endl;
    }

    Lexer lexer(source);
    std::vector<Token> tokens = lexer.tokenize();
    auto lexerLineMap = lexer.getLineToFileMap();

    // Convert Lexer's FileContext map to CodeGenerator's expected format
    std::map<int, std::pair<std::string, int>> lineToFileMap;
    for (const auto& entry : lexerLineMap) {
        lineToFileMap[entry.first] = {entry.second.filename, entry.second.lineOffset};
    }

    if (verboseLevel >= 2) {
        for (const auto& token : tokens) {
            std::cout << "Token: " << token.typeToString() << " (" << token.value << ") at " << token.line << ":" << token.column << std::endl;
        }
    }

    if (verboseLevel >= 1) {
        std::cout << "Parsing " << input_file << "..." << std::endl;
    }

    // Compute assembly file path:
    // - If -S mode: output_file is the final .s, so asmFile = output_file
    // - Otherwise: .s is intermediate, use base + ".s"
    std::string baseName = output_file;
    size_t dotPos = baseName.rfind('.');
    if (dotPos != std::string::npos) baseName = baseName.substr(0, dotPos);
    std::string asmFile = assemblyOnly ? output_file : (baseName + ".s");

    Parser parser(tokens);
    try {
        if (verboseLevel >= 1) std::cout << "Parsing " << input_file << "..." << std::endl;
        auto ast = parser.parse();
        if (verboseLevel >= 1) std::cout << "Parsing complete." << std::endl;

        // IR pipeline: AST → IRBuilder → IR → IRCodeGen → assembly
        IRBuilder irBuilder;
        irBuilder.zpCallMode = zpCallMode;
        irBuilder.inlineFunctions = inlineFunctions;
        irBuilder.setSourceInfo(input_file);

        if (optimize) {
            if (verboseLevel >= 1) std::cout << "Constant folding..." << std::endl;
            ConstantFolder folder;
            ast = folder.foldTranslationUnit(std::move(ast));
            if (verboseLevel >= 1) std::cout << "Constant folding complete." << std::endl;
            irBuilder.setExternalUsedVars(folder.usedVars_);

            if (verboseLevel >= 1) std::cout << "Loop optimization..." << std::endl;
            LoopOptimizer loopOpt;
            loopOpt.optimizeTranslationUnit(*ast);
            if (verboseLevel >= 1) std::cout << "Loop optimization complete." << std::endl;
        }

        if (verboseLevel >= 2 && listingLevel >= 1) {
            ASTPrinter printer;
            ast->accept(printer);
        }

        if (verboseLevel >= 1) std::cout << "Code generation..." << std::endl;

        // Legacy CodeGenerator validator removed (Issue #116).
        // All semantic checks handled by IRBuilder. The legacy validator
        // was only producing false positives (verified: non-fatal mode
        // caused zero regressions, GTE improved from 559→560).

        irBuilder.generate(*ast);

        if (traceIROpt) ir::optTrace = &std::cerr;
        if (optimize) {
            if (irOptFlags.strengthReduction) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (Strength Reduction)..." << std::endl;
                ir::optimizeStrengthReduction(irBuilder.getModule());
            }
            if (irOptFlags.algebraicSimplify) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (Algebraic Simplification)..." << std::endl;
                ir::optimizeAlgebraic(irBuilder.getModule());
            }
            if (irOptFlags.typeNarrowing) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (Type Narrowing)..." << std::endl;
                ir::optimizeTypeNarrowing(irBuilder.getModule());
            }
            if (irOptFlags.branchFold) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (Branch Folding)..." << std::endl;
                ir::optimizeBranchFold(irBuilder.getModule());
            }
            if (irOptFlags.cse) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (CSE and Copy Propagation)..." << std::endl;
                ir::optimizeCSE(irBuilder.getModule());
            }
            if (irOptFlags.licm) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (LICM)..." << std::endl;
                ir::optimizeLICM(irBuilder.getModule());
            }
            if (irOptFlags.copyChains) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (COPY Chain Elimination)..." << std::endl;
                ir::optimizeCopyChains(irBuilder.getModule());
            }
            if (irOptFlags.addrElemFusion) {
                if (verboseLevel >= 1) std::cout << "Optimizing IR (ADDR_ELEM Fusion)..." << std::endl;
                ir::optimizeAddrElemFusion(irBuilder.getModule());
            }
        }

        // Write IR text dump if requested
        if (emitIR) {
            std::string irFile = asmFile;
            size_t dotPos = irFile.rfind('.');
            if (dotPos != std::string::npos) irFile = irFile.substr(0, dotPos);
            irFile += ".ir";
            std::ofstream irOut(irFile);
            if (irOut.is_open()) {
                ir::Printer::print(irOut, irBuilder.getModule());
                irOut.close();
                if (verboseLevel >= 1) std::cout << "Generated IR in " << irFile << std::endl;
            }
        }

        // Emit warnings
        for (const auto& warn : irBuilder.getWarnings()) {
            std::cerr << warn << std::endl;
        }

        // Check for IR errors (const violations, etc.)
        if (irBuilder.hasErrors()) {
            for (const auto& err : irBuilder.getErrors()) {
                std::cerr << err << std::endl;
            }
            return 1;
        }

        // Emit assembly from IR
        std::ofstream asmOut(asmFile);
        if (!asmOut.is_open()) {
            std::cerr << "Failed to open output file for assembly: " << asmFile << std::endl;
            return 1;
        }
        IRCodeGen irCodeGen(asmOut);
        irCodeGen.setLineToFileMap(lineToFileMap);
        irCodeGen.generate(irBuilder.getModule(), zeroPageStart, true, zpCallMode, emitReasons);
        asmOut.close();

        if (verboseLevel >= 1) {
            std::cout << "Generated assembly in " << asmFile << std::endl;
            std::cout << "Code generation complete." << std::endl;
        }

        if (listingLevel == 2) {
            if (verboseLevel >= 1) std::cout << "Generating expanded listing..." << std::endl;
            
            std::ifstream asmIn(asmFile);
            std::stringstream asmBuf;
            asmBuf << asmIn.rdbuf();
            asmIn.close();

            std::map<std::string, uint32_t> predefinedSymbols;
            predefinedSymbols["cc45.zeroPageStart"] = zeroPageStart;
            predefinedSymbols["__sp_base"] = 0x0101;

            AssemblerLexer lex(asmBuf.str());
            auto tokens = lex.tokenize();
            AssemblerParser parser(tokens, predefinedSymbols);
            parser.optimizationLevel = optimize ? 2 : 0;  // Set optimization level based on -O0 flag
            parser.pass1();
            if (parser.hasErrors()) {
                for (const auto& err : parser.getErrors()) {
                    std::cerr << asmFile << ":" << err << std::endl;
                }
                return 1;
            }
            parser.pass2(); // Run optimizer and resolve addresses

            std::ofstream expandedOut(asmFile);
            M65Emitter e(expandedOut, zeroPageStart);
            AssemblerGenerator::generate(&parser, e);
            expandedOut.close();
        }

    } catch (const std::exception& e) {
        std::cerr << "Compiler Error: " << e.what() << std::endl;
        return 1;
    }

    // ===== COMPILATION PIPELINE =====
    // If -S: stop after code generation (assembly text)
    // If -c: compile → assemble → stop (keep .o45, optionally .s)
    // Default: compile → assemble → link → stop (keep .prg, optionally .s/.o45)

    if (assemblyOnly) {
        // -S mode: we're done, assembly was generated to asmFile (which equals output_file)
        if (verboseLevel >= 1) std::cout << "Assembly generated in " << asmFile << std::endl;
        return 0;
    }

    // Compute base name for intermediate files
    std::string finalOutputBase = output_file;
    size_t finalDotPos = finalOutputBase.rfind('.');
    if (finalDotPos != std::string::npos) {
        finalOutputBase = finalOutputBase.substr(0, finalDotPos);
    }

    std::string objFile = finalOutputBase + ".o45";
    std::string prgFile = finalOutputBase + ".prg";

    // Find ca45 and ln45 relative to cc45's own location
    std::string cc45Path = argv[0];
    std::string ca45Path = "ca45";
    std::string ln45Path = "ln45";
    size_t lastSep = cc45Path.find_last_of("/\\");
    if (lastSep != std::string::npos) {
        std::string binDir = cc45Path.substr(0, lastSep + 1);
        ca45Path = binDir + "ca45";
        ln45Path = binDir + "ln45";
    }

    // Collect flags for subprocesses
    std::string rOptFlag;
    std::string rMachFlag;
    std::string pFlags;
    std::string optLevelFlag = " -O2";  // Default optimization level
    if (!optimize) {
        optLevelFlag = " -O0";  // No optimization if -O0 was passed to cc45
    }
    for (size_t ai = 0; ai < allArgs.size(); ai++) {
        std::string arg = allArgs[ai];
        if (arg == "-Roptimizer") rOptFlag = " -Roptimizer";
        if (arg == "-Rmachstate") rMachFlag = " -Rmachstate";
        if (arg.substr(0, 2) == "-P") pFlags += " " + arg;  // Collect all -P flags
    }

    // STEP 1: Assemble .s → .o45
    if (verboseLevel >= 1) std::cout << "Assembling " << asmFile << " → " << objFile << "..." << std::endl;
    std::string asmCommand = ca45Path + " -c" + optLevelFlag + " " + defineFlag + rOptFlag + rMachFlag + pFlags + " -o " + objFile + " " + asmFile;

    // Capture and display assembler output
    FILE* asmPipe = popen(asmCommand.c_str(), "r");
    if (!asmPipe) {
        std::cerr << "Failed to run assembler: " << asmCommand << std::endl;
        return 1;
    }

    char asmBuf[256];
    while (fgets(asmBuf, sizeof(asmBuf), asmPipe) != nullptr) {
        // Display assembler output with prefix
        if (verboseLevel >= 1) std::cout << "[ca45] ";
        std::cout << asmBuf;
    }

    int asmRet = pclose(asmPipe);
    if (asmRet != 0) {
        std::cerr << "Assembler failed with return code " << asmRet << std::endl;
        return 1;
    }

    // STEP 2: Link .o45 → .prg (unless -c specified)
    if (!objectFileOnly) {
        if (verboseLevel >= 1) std::cout << "Linking " << objFile << " → " << prgFile << "..." << std::endl;

        // Determine which library and startup object to link based on calling convention
        std::string libName = "c45.lib";
        std::string crt0Name = "crt0.o45";
        if (zpCallMode) {
            libName = "c45_zp.lib";
            crt0Name = "crt0_zp.o45";
        }

        // Try to find library and startup object in multiple locations (build tree, then installed prefix)
        std::string libPath = libName;
        std::string crt0Path = crt0Name;
        if (lastSep != std::string::npos) {
            std::string binDir = cc45Path.substr(0, lastSep + 1);
            // Try build tree location first: ../lib/build/
            std::string tryPath = binDir + "../lib/build/" + libName;
            std::ifstream test(tryPath);
            if (test.good()) {
                libPath = tryPath;
                test.close();
            } else {
                // Try installed prefix location: ../lib/cc45/
                tryPath = binDir + "../lib/cc45/" + libName;
                test.open(tryPath);
                if (test.good()) {
                    libPath = tryPath;
                    test.close();
                }
            }

            // Look for crt0 in the same locations
            tryPath = binDir + "../lib/build/" + crt0Name;
            test.open(tryPath);
            if (test.good()) {
                crt0Path = tryPath;
                test.close();
            } else {
                // Try installed prefix location
                tryPath = binDir + "../lib/cc45/" + crt0Name;
                test.open(tryPath);
                if (test.good()) {
                    crt0Path = tryPath;
                    test.close();
                }
            }
        }

        // Link with crt0 first (sets entry point), then user object, then libraries
        std::string linkCommand = ln45Path + " " + crt0Path + " " + objFile + " " + libPath + " -o " + prgFile;

        // Capture and display linker output with a prefix so it's clear where it comes from
        FILE* linkerPipe = popen(linkCommand.c_str(), "r");
        if (!linkerPipe) {
            std::cerr << "Failed to run linker: " << linkCommand << std::endl;
            return 1;
        }

        char linkerBuf[256];
        while (fgets(linkerBuf, sizeof(linkerBuf), linkerPipe) != nullptr) {
            // Display linker output with prefix
            if (verboseLevel >= 1) std::cout << "[ln45] ";
            std::cout << linkerBuf;
        }

        int linkRet = pclose(linkerPipe);
        if (linkRet != 0) {
            std::cerr << "Linker failed with return code " << linkRet << std::endl;
            return 1;
        }
    }

    // STEP 3: Cleanup intermediate files (unless --save-temps)
    if (!saveTemps) {
        if (!assemblyOnly) {
            // Remove assembly file
            if (std::remove(asmFile.c_str()) == 0 && verboseLevel >= 1) {
                std::cout << "Removed intermediate assembly file: " << asmFile << std::endl;
            }
        }
        if (!objectFileOnly && !assemblyOnly) {
            // Remove object file (unless -c mode keeps it)
            if (std::remove(objFile.c_str()) == 0 && verboseLevel >= 1) {
                std::cout << "Removed intermediate object file: " << objFile << std::endl;
            }
        }
    }

    if (objectFileOnly) {
        if (verboseLevel >= 1) std::cout << "Object file: " << objFile << std::endl;
    } else {
        if (verboseLevel >= 1) std::cout << "Executable: " << prgFile << std::endl;
    }

    return 0;
}
