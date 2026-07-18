#include "Ca45Parser.hpp"
#include "Preprocessor.hpp"
#include <sstream>

AsmIR::Module Ca45Parser::parse(const std::string& source) {
    AsmIR::Module module;

    try {
        // Preprocess the source
        Preprocessor preprocessor(false);  // not compiler mode
        std::map<std::string, std::string> symbols;
        std::vector<std::string> includePaths;
        std::string processed = preprocessor.process(source, symbols, includePaths, "");

        // Tokenize
        AssemblerLexer lexer(processed);
        auto tokens = lexer.tokenize();

        // Parse
        AssemblerParser parser(tokens);
        parser.pass1();

        if (parser.hasErrors()) {
            for (const auto& err : parser.getErrors()) {
                addError(err);
            }
            return module;
        }

        // Extract module information
        module.cpu = "45GS02";
        module.is_o45 = false;

        // Extract symbols
        extractSymbols(parser, module);

        // Extract statements
        extractStatements(parser, module, source);

        // Extract warnings
        for (const auto& warn : parser.getWarnings()) {
            addWarning(warn);
        }

    } catch (const std::exception& e) {
        addError(std::string("Parse error: ") + e.what());
    }

    return module;
}

AsmIR::AddressingMode Ca45Parser::mapAddressingMode(AddressingMode mode) {
    switch (mode) {
        case AddressingMode::IMPLIED: return AsmIR::AddressingMode::IMPLIED;
        case AddressingMode::IMMEDIATE: return AsmIR::AddressingMode::IMMEDIATE;
        case AddressingMode::BASE_PAGE: return AsmIR::AddressingMode::ZERO_PAGE;
        case AddressingMode::BASE_PAGE_X: return AsmIR::AddressingMode::ZERO_PAGE_X;
        case AddressingMode::BASE_PAGE_Y: return AsmIR::AddressingMode::ZERO_PAGE_Y;
        case AddressingMode::ABSOLUTE: return AsmIR::AddressingMode::ABSOLUTE;
        case AddressingMode::ABSOLUTE_X: return AsmIR::AddressingMode::ABSOLUTE_X;
        case AddressingMode::ABSOLUTE_Y: return AsmIR::AddressingMode::ABSOLUTE_Y;
        case AddressingMode::INDIRECT: return AsmIR::AddressingMode::INDIRECT;
        case AddressingMode::BASE_PAGE_X_INDIRECT: return AsmIR::AddressingMode::INDIRECT_X;
        case AddressingMode::BASE_PAGE_INDIRECT_Y: return AsmIR::AddressingMode::INDIRECT_Y;
        case AddressingMode::STACK_RELATIVE: return AsmIR::AddressingMode::STACK_RELATIVE;
        case AddressingMode::LINEAR_ABSOLUTE: return AsmIR::AddressingMode::FRAME_RELATIVE;
        case AddressingMode::ABSOLUTE_X_INDIRECT: return AsmIR::AddressingMode::INDEXED_PAIR;
        // Handle other modes by falling back to implied
        case AddressingMode::ACCUMULATOR:
        case AddressingMode::IMMEDIATE16:
        case AddressingMode::BASE_PAGE_INDIRECT_Z:
        case AddressingMode::BASE_PAGE_INDIRECT_SP_Y:
        case AddressingMode::ABSOLUTE_INDIRECT:
        case AddressingMode::RELATIVE:
        case AddressingMode::RELATIVE16:
        case AddressingMode::BASE_PAGE_RELATIVE:
        case AddressingMode::FLAT_INDIRECT_Z:
        case AddressingMode::QUAD_Q:
        case AddressingMode::STACK_RELATIVE_INDIRECT_Y:
        case AddressingMode::LINEAR_ABSOLUTE_X:
        case AddressingMode::LINEAR_ABSOLUTE_Y:
        case AddressingMode::NONE:
            return AsmIR::AddressingMode::IMPLIED;
    }
    return AsmIR::AddressingMode::IMPLIED;
}

AsmIR::DirectiveType Ca45Parser::mapDirectiveType(const std::string& name) {
    if (name == "byte") return AsmIR::DirectiveType::BYTE_DATA;
    if (name == "word") return AsmIR::DirectiveType::WORD_DATA;
    if (name == "long") return AsmIR::DirectiveType::LONG_DATA;
    if (name == "fill") return AsmIR::DirectiveType::FILL;
    if (name == "align") return AsmIR::DirectiveType::ALIGN;
    if (name == "org") return AsmIR::DirectiveType::ORG;
    if (name == "segment" || name == "code" || name == "data" || name == "bss")
        return AsmIR::DirectiveType::SEGMENT;
    if (name == "global") return AsmIR::DirectiveType::GLOBAL;
    if (name == "extern") return AsmIR::DirectiveType::EXTERN;
    if (name == "weak") return AsmIR::DirectiveType::WEAK;
    if (name == "const") return AsmIR::DirectiveType::CONST;
    if (name == "cpu") return AsmIR::DirectiveType::CPU;
    if (name == "func_flags") return AsmIR::DirectiveType::FUNCTION;
    if (name == "zp_uses") return AsmIR::DirectiveType::ZP_USES;
    if (name == "zp_clobbers") return AsmIR::DirectiveType::ZP_CLOBBERS;
    if (name == "reg_clobbers") return AsmIR::DirectiveType::REG_CLOBBERS;
    if (name == "flag_clobbers") return AsmIR::DirectiveType::FLAG_CLOBBERS;
    if (name == "loc") return AsmIR::DirectiveType::LOC;
    return AsmIR::DirectiveType::OTHER;
}

void Ca45Parser::extractSymbols(const AssemblerParser& parser, AsmIR::Module& module) {
    const auto& symbolTable = parser.getSymbolTable();
    for (const auto& [name, sym] : symbolTable) {
        AsmIR::Symbol irSym;
        irSym.name = name;
        irSym.value = sym.value;
        irSym.is_constant = sym.isConstant;
        irSym.is_global = parser.isGlobalSymbol(name);
        irSym.is_extern = parser.isExternSymbol(name);
        irSym.is_weak = parser.isWeakSymbol(name);
        module.symbols[name] = irSym;
    }
}

void Ca45Parser::extractStatements(const AssemblerParser& parser, AsmIR::Module& module, const std::string& source) {
    std::vector<std::string> sourceLines;
    std::stringstream ss(source);
    std::string line;
    while (std::getline(ss, line)) {
        sourceLines.push_back(line);
    }

    for (const auto& stmt : parser.statements) {
        if (stmt->deleted) continue;

        AsmIR::Statement irStmt;
        irStmt.source_line = stmt->line;

        if (!stmt->label.empty()) {
            irStmt.type = AsmIR::Statement::Type::LABEL;
            irStmt.label = stmt->label;
        } else if (stmt->type == AssemblerParser::Statement::Type::INSTRUCTION) {
            irStmt.type = AsmIR::Statement::Type::INSTRUCTION;
            irStmt.instr.mnemonic = stmt->instr.mnemonic;
            irStmt.instr.mode = mapAddressingMode(stmt->instr.mode);
            irStmt.instr.operand.text = stmt->instr.operand;
            irStmt.instr.operand.is_symbolic = (parser.getSymbolTable().count(stmt->instr.operand) > 0);
            irStmt.instr.label = stmt->label;
        } else if (stmt->type == AssemblerParser::Statement::Type::DIRECTIVE) {
            irStmt.type = AsmIR::Statement::Type::DIRECTIVE;
            irStmt.dir.name = stmt->dir.name;
            irStmt.dir.type = mapDirectiveType(stmt->dir.name);
            irStmt.dir.arguments = stmt->dir.arguments;
            irStmt.label = stmt->label;
        } else {
            continue;
        }

        if (irStmt.source_line > 0 && irStmt.source_line <= (int)sourceLines.size()) {
            irStmt.source_file = parser.getLineMap().empty() ? "" : parser.getLineMap()[0].file;
        }

        module.statements.push_back(irStmt);
    }
}
