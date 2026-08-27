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
#include "IPOAnalyzer.hpp"
#include "SpecializationCodeGenerator.hpp"
#include "SpecializationOptimizer.hpp"
#include "IRSpecializationGenerator.hpp"
#include "CompoundAssignmentFusion.hpp"
#include "CompoundChainOptimizer.hpp"
#include "AssemblerPeephole.hpp"
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
        includePaths.push_back(baseDir + "../lib/include");
        config.toolDir = baseDir;
    }

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
                std::cout << "  -fzpcall       Use ZP calling convention" << std::endl;
                std::cout << "  -fstaticalloc  Use static allocation (default)" << std::endl;
                std::cout << "  -finline-functions  Inline small functions" << std::endl;
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
        } else if (arg == "-fstaticalloc") {
            config.staticAllocMode = true;
        } else if (arg == "-fno-staticalloc") {
            config.staticAllocMode = false;
        } else if (arg == "--prg-base" && i + 1 < allArgs.size()) {
            config.prgBase = std::stoul(allArgs[++i], nullptr, 16);
        } else if (arg == "-finline-functions") {
            config.inlineSmallFunctions = true;
        } else if (arg == "--pragma" && i + 1 < allArgs.size()) {
            config.cliPragmas.push_back(allArgs[++i]);
        } else if (arg == "--emit-ir") {
            config.emitIR = true;
        } else if (arg == "-Rcodegen") {
            config.emitReasons = true;
        } else if (arg.substr(0, 2) == "-O") {
            std::string levelStr = arg.substr(2);
            if (!levelStr.empty() && levelStr[0] >= '0' && levelStr[0] <= '9') {
                config.optimizationLevel = levelStr[0] - '0';
            } else if (levelStr == "size") {
                config.optimizationLevel = 2;
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
        } else {
            config.inputFile = arg;
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
    CompilationPipeline pipeline(config);
    CompilationResult result = pipeline.compile();

    if (!result.success) {
        std::cerr << "Compilation error: " << result.error << std::endl;
        return result.exitCode;
    }

    if (config.verboseLevel >= 1) {
        std::cout << "Compilation successful: " << result.outputFile << std::endl;
    }

    return 0;
}
