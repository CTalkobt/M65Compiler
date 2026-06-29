#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#ifdef __linux__
#include <unistd.h>
#endif
#include <fstream>
#include <sstream>
#include "Lexer.hpp"
#include "Parser.hpp"
#include "AST.hpp"
#include "CodeGenerator.hpp"
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

    std::string input_file;
    bool preprocessOnly = (programName == "cp45");
    std::string output_file = "";
    bool outputFileSet = false;
    bool assemble = false;
    int verboseLevel = 0;
    bool optimize = true;
    int listingLevel = 1;
    uint32_t zeroPageStart = 0x08;
    bool zpCallMode = false;
    bool inlineFunctions = false;
    bool emitIR = false;
    bool emitReasons = false;
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

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-V" || arg == "--version") {
            std::cout << suiteVersionString("cc45") << std::endl;
            return 0;
        } else if (arg == "-?" || arg == "--help") {
            std::cout << "Usage: cc45 [options] <input_file.c>" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -E             Run only the preprocessor (output to stdout or -o file)" << std::endl;
            std::cout << "  -c             Compile and assemble to a .o45 relocatable object file" << std::endl;
            std::cout << "  -o <filename>  Specify output assembly filename (default: out.s)" << std::endl;
            std::cout << "  -l <level>     Listing level: 1=Standard (default), 2=Expanded" << std::endl;
            std::cout << "  -v             Enable verbose output (phase info)" << std::endl;
            std::cout << "  -vv            Extra verbose output (token dumps, AST)" << std::endl;
            std::cout << "  -fzpcall       Use ZP parameter block calling convention" << std::endl;
            std::cout << "  -fno-zpcall    Use stack-based calling convention (default)" << std::endl;
            std::cout << "  -finline-functions  Inline small functions at call sites" << std::endl;
            std::cout << "  --pragma \"...\"  Inject a #pragma directive (e.g., --pragma \"cc45 heap\")" << std::endl;
            std::cout << "  -Dname=val     Define a symbol (e.g., -Dcc45.zeroPageStart=$10)" << std::endl;
            std::cout << "  -I<path>       Add include search path" << std::endl;
            std::cout << "  -Rcodegen      Annotate assembly output with codegen reasoning comments" << std::endl;
            std::cout << "  -?             Display this help message" << std::endl;
            return 0;
        } else if (arg == "-c") {
            assemble = true;
        } else if (arg == "-E") {
            preprocessOnly = true;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
            outputFileSet = true;
        } else if (arg == "-l" && i + 1 < argc) {
            listingLevel = std::stoi(argv[++i]);
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
        } else if (arg == "--pragma" && i + 1 < argc) {
            cliPragmas.push_back(argv[++i]);
        } else if (arg == "--emit-ir") {
            emitIR = true;
        } else if (arg == "-Rcodegen") {
            emitReasons = true;
        } else if (arg == "-Roptimizer") {
            emitReasons = true; // also enable codegen reasons for context
            // Flag will be passed to ca45 subprocess below
        } else if (arg == "-O0") {
            optimize = false;
        } else if (arg == "-vv") {
            verboseLevel = 2;
        } else if (arg == "-v") {
            verboseLevel = 1;
        } else if (arg == "-I" && i + 1 < argc) {
            includePaths.push_back(argv[++i]);
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
        std::cerr << "Preprocessor Error: " << e.what() << std::endl;
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
        if (assemble) {
            // -c mode: default output is input.o45
            std::string base = input_file;
            size_t dot = base.rfind('.');
            if (dot != std::string::npos) base = base.substr(0, dot);
            output_file = base + ".o45";
        } else {
            output_file = "out.s";
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

    // In -c mode, the .s file is intermediate; output_file is the .o45
    std::string asmFile = assemble ? (output_file + ".s") : output_file;

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

        if (verboseLevel >= 2 && listingLevel >= 1) {
            ASTPrinter printer;
            ast->accept(printer);
        }

        if (verboseLevel >= 1) std::cout << "Code generation..." << std::endl;

        // Run legacy CodeGenerator for validation (error detection only)
        // Catches struct errors, type errors, etc. that IRBuilder doesn't validate.
        // Skip for files with nested functions — the legacy validator doesn't handle them.
        {
            bool skipValidator = false;
            for (const auto& decl : ast->topLevelDecls) {
                if (auto* fd = dynamic_cast<FunctionDeclaration*>(decl.get())) {
                    if (fd->body) {
                        for (const auto& stmt : fd->body->statements) {
                            if (dynamic_cast<FunctionDeclaration*>(stmt.get())) {
                                skipValidator = true; // nested functions
                                break;
                            }
                        }
                    }
                    if (skipValidator) break;
                }
                // Struct methods also need to skip validator
                if (auto* sd = dynamic_cast<StructDefinition*>(decl.get())) {
                    if (!sd->methods.empty()) { skipValidator = true; break; }
                }
                // Also check struct definitions inside function bodies
                if (auto* fd2 = dynamic_cast<FunctionDeclaration*>(decl.get())) {
                    if (fd2->isMethod) { skipValidator = true; break; }
                }
            }
            if (!skipValidator) {
                std::ostringstream nullOut;
                CodeGenerator validator(nullOut);
                validator.zeroPageStart = zeroPageStart;
                validator.zeroPageAvail = zeroPageAvail;
                validator.relocMode = assemble;
                validator.zpCallMode = zpCallMode;
                validator.setSourceInfo(input_file, sourceLines);
                validator.setLineToFileMap(lineToFileMap);
                try {
                    validator.generate(*ast);
                } catch (const std::exception& e) {
                    std::cerr << "Compile Error: " << e.what() << std::endl;
                    return 1;
                }
            }
        }

        irBuilder.generate(*ast);

        if (optimize) {
            if (verboseLevel >= 1) std::cout << "IR Optimization..." << std::endl;
            ir::IROptimizer optimizer(verboseLevel);
            optimizer.optimize(irBuilder.getModule());
            if (verboseLevel >= 1) std::cout << "IR Optimization complete." << std::endl;
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
            std::cerr << input_file << ":" << warn << std::endl;
        }

        // Check for IR errors (const violations, etc.)
        if (irBuilder.hasErrors()) {
            for (const auto& err : irBuilder.getErrors()) {
                std::cerr << input_file << ":" << err << std::endl;
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
        irCodeGen.generate(irBuilder.getModule(), zeroPageStart, assemble, zpCallMode, emitReasons);
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
    
    if (assemble) {
        if (verboseLevel >= 1) std::cout << "Assembling to " << output_file << "..." << std::endl;
        // Find ca45 relative to cc45's own location
        std::string cc45Path = argv[0];
        std::string ca45Path = "ca45";
        size_t lastSep = cc45Path.find_last_of("/\\");
        if (lastSep != std::string::npos) {
            ca45Path = cc45Path.substr(0, lastSep + 1) + "ca45";
        }
        std::string rOptFlag;
        for (int ai = 1; ai < argc; ai++) {
            if (std::string(argv[ai]) == "-Roptimizer") { rOptFlag = " -Roptimizer"; break; }
        }
        std::string command = ca45Path + " -c -opt " + defineFlag + rOptFlag + " -o " + output_file + " " + asmFile;
        int ret = std::system(command.c_str());
        if (ret != 0) {
            std::cerr << "Assembler failed with return code " << ret << std::endl;
            return 1;
        }
    }

    return 0;
}
