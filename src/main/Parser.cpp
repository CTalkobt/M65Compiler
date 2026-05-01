#include "Parser.hpp"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

const Token& Parser::peek() const {
    if (pos >= tokens.size()) return tokens.back();
    return tokens[pos];
}

const Token& Parser::advance() {
    if (pos < tokens.size()) pos++;
    return tokens[pos - 1];
}

bool Parser::match(TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::expect(TokenType type, const std::string& message) {
    if (peek().type == type) {
        return advance();
    }
    std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
    throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": " + message + ". Found '" + foundStr + "' (" + peek().typeToString() + ") instead.");
}

std::unique_ptr<TranslationUnit> Parser::parse() {
    auto unit = setPos(std::make_unique<TranslationUnit>(), tokens[0]);
    while (peek().type != TokenType::END_OF_FILE) {
        if (match(TokenType::TYPEDEF)) {
            parseTypedef();
            continue;
        }
        if (peek().type == TokenType::ASM) {
            advance();
            expect(TokenType::OPEN_PAREN, "Expected '(' after asm");
            std::string code = expect(TokenType::STRING_LITERAL, "Expected string literal for asm code").value;
            expect(TokenType::CLOSE_PAREN, "Expected ')' after asm code");
            expect(TokenType::SEMICOLON, "Expected ';'");
            flushPending(*unit);
            unit->topLevelDecls.push_back(setPos(std::make_unique<AsmStatement>(code), tokens[pos-5]));
            continue;
        }
        if (peek().type == TokenType::STATIC_ASSERT) {
            auto decl = parseStaticAssert();
            flushPending(*unit);
            unit->topLevelDecls.push_back(std::unique_ptr<Statement>(std::move(decl)));
            continue;
        }

        if (peek().type == TokenType::STRUCT || peek().type == TokenType::UNION || peek().type == TokenType::ENUM) {
            bool isUnion = peek().type == TokenType::UNION;
            bool isEnum = peek().type == TokenType::ENUM;
            // Check if it's a definition: struct/union/enum name { ... };
            if (isEnum) {
                advance(); // enum
                auto def = parseEnumDefinition();
                expect(TokenType::SEMICOLON, "Expected ';' after enum definition");
                unit->topLevelDecls.push_back(std::move(def));
                continue;
            }
            if (pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::IDENTIFIER &&
                pos + 2 < tokens.size() && tokens[pos+2].type == TokenType::OPEN_BRACE) {
                advance(); // struct/union
                auto def = parseStructDefinition(isUnion);
                expect(TokenType::SEMICOLON, "Expected ';' after struct/union definition");
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(def));
                continue;
            }
        }

        // Look ahead to distinguish function from global variable
        size_t look = pos;
        bool isVol = false;
        bool isConst = false;
        bool isNR = false;
        bool isExtern = false;
        bool isStatic = false;

        if (tokens[look].type == TokenType::EXTERN) {
            isExtern = true;
            look++;
        } else if (tokens[look].type == TokenType::STATIC) {
            isStatic = true;
            look++;
        }

        if (tokens[look].type == TokenType::NORETURN) {
            isNR = true;
            look++;
        }

        if (tokens[look].type == TokenType::ALIGNAS) {
            look++;
            if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                look++;
                int parens = 1;
                while (look < tokens.size() && parens > 0) {
                    if (tokens[look].type == TokenType::OPEN_PAREN) parens++;
                    else if (tokens[look].type == TokenType::CLOSE_PAREN) parens--;
                    look++;
                }
            }
        }
        
        bool isSig = false;

        while (tokens[look].type == TokenType::VOLATILE || tokens[look].type == TokenType::CONST || tokens[look].type == TokenType::RESTRICT || tokens[look].type == TokenType::AUTO || tokens[look].type == TokenType::REGISTER || tokens[look].type == TokenType::INLINE ||
               tokens[look].type == TokenType::SIGNED || tokens[look].type == TokenType::UNSIGNED) {
            if (tokens[look].type == TokenType::VOLATILE) isVol = true;
            else if (tokens[look].type == TokenType::CONST) isConst = true;
            else if (tokens[look].type == TokenType::SIGNED) isSig = true;
            look++;
        }

        if (look < tokens.size() && (tokens[look].type == TokenType::INT ||
                                     tokens[look].type == TokenType::CHAR ||
                                     tokens[look].type == TokenType::BOOL ||
                                     tokens[look].type == TokenType::UNSIGNED ||
                                     tokens[look].type == TokenType::SIGNED ||
                                     tokens[look].type == TokenType::VOID ||
                                     tokens[look].type == TokenType::STRUCT ||
                                     tokens[look].type == TokenType::UNION ||
                                     tokens[look].type == TokenType::ENUM ||
                                     (tokens[look].type == TokenType::IDENTIFIER && isTypedef(tokens[look].value)))) {
            if (tokens[look].type == TokenType::UNSIGNED || tokens[look].type == TokenType::SIGNED) {
                look++;
                if (look < tokens.size() && (tokens[look].type == TokenType::INT || tokens[look].type == TokenType::CHAR)) {
                    look++;
                }
            } else if (tokens[look].type == TokenType::STRUCT || tokens[look].type == TokenType::UNION || tokens[look].type == TokenType::ENUM) {
                look++; // skip struct/union/enum
                if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER) {
                    look++; // skip struct/union name
                } else if (look < tokens.size() && tokens[look].type == TokenType::OPEN_BRACE) {
                    int braceLevel = 1;
                    look++;
                    while (look < tokens.size() && braceLevel > 0) {
                        if (tokens[look].type == TokenType::OPEN_BRACE) braceLevel++;
                        else if (tokens[look].type == TokenType::CLOSE_BRACE) braceLevel--;
                        look++;
                    }
                } else {
                    auto decl = parseFunctionDeclaration();
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                    continue;
                }
            } else {
                look++; // skip int/char/void
            }
            
            // Check for function pointer: type (*name)(params) — treat as variable
            if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN &&
                look + 1 < tokens.size() && tokens[look + 1].type == TokenType::STAR) {
                if (isExtern) match(TokenType::EXTERN);
                if (isStatic) match(TokenType::STATIC);
                if (isNR) match(TokenType::NORETURN);
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED));
                auto decl = parseVariableDeclaration(isVol, isConst, isStatic);
                if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                    vd->isGlobal = true;
                    vd->isExtern = isExtern;
                }
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
                continue;
            }

            while (look < tokens.size() && tokens[look].type == TokenType::STAR) look++;

            if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER) {
                look++;
                if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                    if (isExtern) match(TokenType::EXTERN);
                    if (isStatic) match(TokenType::STATIC);
                    if (isNR) match(TokenType::NORETURN);
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED));
                    auto decl = parseFunctionDeclaration();
                    decl->isNoreturn = isNR;
                    decl->isStatic = isStatic;
                    // extern functions are always prototypes (no body)
                    if (isExtern) decl->isPrototype = true;
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                } else {
                    if (isExtern) match(TokenType::EXTERN);
                    if (isStatic) match(TokenType::STATIC);
                    if (isNR) match(TokenType::NORETURN);
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED));
                    auto decl = parseVariableDeclaration(isVol, isConst);
                    if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                        vd->isGlobal = true;
                        vd->isSigned = isSig;
                        vd->isExtern = isExtern;
                        vd->isStatic = isStatic;
                    }
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                }
            } else {
                if (isNR) match(TokenType::NORETURN);
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED));
                auto decl = parseFunctionDeclaration();
                decl->isNoreturn = isNR;
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
            }
        } else {
            if (isNR) match(TokenType::NORETURN);
            while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED));
            auto decl = parseFunctionDeclaration();
            decl->isNoreturn = isNR;
            flushPending(*unit);
            unit->topLevelDecls.push_back(std::unique_ptr<Statement>(std::move(decl)));
        }
    }
    return unit;
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    const Token& startToken = peek();
    bool isNR = match(TokenType::NORETURN);
    std::string returnType;
    bool isSigned = false;
    int basePtrLevel = 0;

    if (match(TokenType::SIGNED)) {
        isSigned = true;
        if (match(TokenType::INT)) returnType = "int";
        else if (match(TokenType::CHAR)) returnType = "char";
        else returnType = "int";
    }
    else if (match(TokenType::UNSIGNED)) {
        if (match(TokenType::INT)) returnType = "int";
        else if (match(TokenType::CHAR)) returnType = "char";
        else returnType = "int"; // bare 'unsigned' is 'unsigned int'
    }
    else if (match(TokenType::INT)) returnType = "int";
    else if (match(TokenType::CHAR)) returnType = "char";
    else if (match(TokenType::BOOL)) returnType = "_Bool";
    else if (match(TokenType::VOID)) returnType = "void";
    else if (match(TokenType::STRUCT) || match(TokenType::UNION) || match(TokenType::ENUM)) {
        bool isU = tokens[pos-1].type == TokenType::UNION;
        bool isE = tokens[pos-1].type == TokenType::ENUM;
        if (isE) returnType = "enum " + expect(TokenType::IDENTIFIER, "Expected enum name").value;
        else returnType = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
    }
    else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
        std::string alias = advance().value;
        returnType = typedefs[alias].baseType;
        isSigned = typedefs[alias].isSigned;
        basePtrLevel = typedefs[alias].pointerLevel;
    }
    else {
        std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
        throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": Expected return type (int, char, void, struct, union) for function declaration. Found '" + foundStr + "' instead.");
    }

    int returnPtrLevel = basePtrLevel;
    while (match(TokenType::STAR)) returnPtrLevel++;

    std::string name = expect(TokenType::IDENTIFIER, "Expected function name").value;
    
    expect(TokenType::OPEN_PAREN, "Expected '('");
    std::vector<Parameter> params;
    // Handle (void) as empty parameter list
    if (peek().type == TokenType::VOID && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::CLOSE_PAREN) {
        advance(); // consume 'void'
    }
    else if (peek().type != TokenType::CLOSE_PAREN) {
        do {
            bool pIsVolatile = false;
            bool pIsConst = false;
            while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT)) {
                if (tokens[pos-1].type == TokenType::VOLATILE) pIsVolatile = true;
                else if (tokens[pos-1].type == TokenType::CONST) pIsConst = true;
            }
            std::string pType;
            bool pIsSigned = false;
            int pBasePtrLevel = 0;

            if (match(TokenType::SIGNED)) {
                pIsSigned = true;
                if (match(TokenType::INT)) pType = "int";
                else if (match(TokenType::CHAR)) pType = "char";
                else pType = "int";
            }
            else if (match(TokenType::UNSIGNED)) {
                if (match(TokenType::INT)) pType = "int";
                else if (match(TokenType::CHAR)) pType = "char";
                else pType = "int";
            }
            else if (match(TokenType::INT)) pType = "int";
            else if (match(TokenType::CHAR)) pType = "char";
            else if (match(TokenType::BOOL)) pType = "_Bool";
            else if (match(TokenType::VOID)) pType = "void";
            else if (match(TokenType::STRUCT) || match(TokenType::UNION) || match(TokenType::ENUM)) {
                bool isU = tokens[pos-1].type == TokenType::UNION;
                bool isE = tokens[pos-1].type == TokenType::ENUM;
                if (isE) pType = "enum " + expect(TokenType::IDENTIFIER, "Expected enum name").value;
                else pType = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
            }
            else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
                std::string pAlias = advance().value;
                auto& ta = typedefs[pAlias];
                pType = ta.baseType;
                pIsSigned = ta.isSigned;
                pBasePtrLevel = ta.pointerLevel;
                if (ta.isFunctionPointer) {
                    // Typedef'd function pointer parameter
                    std::string pName = expect(TokenType::IDENTIFIER, "Expected parameter name").value;
                    Parameter p;
                    p.type = "void";
                    p.pointerLevel = 1;
                    p.isSigned = false;
                    p.name = pName;
                    p.isVolatile = pIsVolatile;
                    p.isConst = pIsConst;
                    p.isFunctionPointer = true;
                    p.funcPtrSig = ta.funcPtrSig;
                    params.push_back(p);
                    continue;
                }
            }
            else {
                std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
                throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": Expected parameter type (int, char, struct, union). Found '" + foundStr + "' instead.");
            }

            int pPtrLevel = pBasePtrLevel;

            // Check for function pointer parameter: type (*name)(params)
            if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::STAR) {
                advance(); // consume '('
                advance(); // consume '*'
                std::string pName = expect(TokenType::IDENTIFIER, "Expected function pointer parameter name").value;
                expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer name");
                auto sig = parseFuncPtrParams(pType, pPtrLevel, pIsSigned);
                Parameter p;
                p.type = "void";
                p.pointerLevel = 1;
                p.isSigned = false;
                p.name = pName;
                p.isVolatile = pIsVolatile;
                p.isConst = pIsConst;
                p.isFunctionPointer = true;
                p.funcPtrSig = sig;
                params.push_back(p);
                continue;
            }

            while (match(TokenType::STAR)) pPtrLevel++;

            // Handle const/volatile after * (qualifies the pointer itself)
            bool pIsPointerConst = false;
            while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
                if (match(TokenType::CONST)) pIsPointerConst = true;
                else if (!match(TokenType::VOLATILE)) match(TokenType::RESTRICT);
            }

            std::string pName = expect(TokenType::IDENTIFIER, "Expected parameter name").value;
            params.push_back({pType, pPtrLevel, pIsSigned, pName, pIsVolatile, pIsConst, pIsPointerConst});
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::CLOSE_PAREN, "Expected ')'");

    // Forward declaration (prototype): ends with ';' instead of '{'
    if (match(TokenType::SEMICOLON)) {
        // Create a function declaration with an empty body
        auto func = setPos(std::make_unique<FunctionDeclaration>(name, returnType), startToken);
        func->isSigned = isSigned;
        func->parameters = std::move(params);
        func->body = std::make_unique<CompoundStatement>();
        func->isNoreturn = isNR;
        func->isPrototype = true;
        return func;
    }

    std::string savedFunctionName = currentFunctionName;
    currentFunctionName = name;
    auto body = parseCompoundStatement();
    currentFunctionName = savedFunctionName;
    auto func = setPos(std::make_unique<FunctionDeclaration>(name, returnType), startToken);
    func->isSigned = isSigned;
    func->parameters = std::move(params);
    func->body = std::move(body);
    func->isNoreturn = isNR;
    return func;
}

std::unique_ptr<CompoundStatement> Parser::parseCompoundStatement() {
    const Token& startToken = expect(TokenType::OPEN_BRACE, "Expected '{'");
    auto compound = setPos(std::make_unique<CompoundStatement>(), startToken);
    while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE) {
        compound->statements.push_back(parseStatement());
    }
    expect(TokenType::CLOSE_BRACE, "Expected '}'");
    return compound;
}

std::unique_ptr<Statement> Parser::parseStatement() {
    if (match(TokenType::SEMICOLON)) {
        return setPos(std::make_unique<CompoundStatement>(), tokens[pos-1]); // Null statement
    }

    if (peek().type == TokenType::IDENTIFIER && pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::COLON) {
        std::string label = advance().value;
        advance(); // :
        return setPos(std::make_unique<LabelledStatement>(label, parseStatement()), tokens[pos-2]);
    }

    if (match(TokenType::TYPEDEF)) {
        parseTypedef();
        return setPos(std::make_unique<CompoundStatement>(), tokens[pos-1]); // Dummy empty statement
    }

    if (match(TokenType::GOTO)) {
        const Token& startToken = tokens[pos-1];
        std::string label = expect(TokenType::IDENTIFIER, "Expected label name after 'goto'").value;
        expect(TokenType::SEMICOLON, "Expected ';' after goto label");
        return setPos(std::make_unique<GotoStatement>(label), startToken);
    }

    bool isVolatile = false;
    bool isConst = false;
    bool isAuto = false;
    bool isStatic = false;
    bool isRegister = false;
    while (true) {
        if (match(TokenType::VOLATILE)) {
            isVolatile = true;
        } else if (match(TokenType::CONST)) {
            isConst = true;
        } else if (match(TokenType::AUTO)) {
            isAuto = true;
        } else if (match(TokenType::STATIC)) {
            isStatic = true;
        } else if (match(TokenType::REGISTER)) {
            isRegister = true;
        } else if (match(TokenType::RESTRICT)) {
            // consumed; restrict is a hint only
        } else if (match(TokenType::INLINE)) {
            // consumed; inline is a hint only (no-op for now)
        } else {
            break;
        }
    }

    if (peek().type == TokenType::STRUCT || peek().type == TokenType::UNION || peek().type == TokenType::ENUM) {
        bool isUnion = peek().type == TokenType::UNION;
        bool isEnum = peek().type == TokenType::ENUM;
        
        if (isEnum) {
            // Check if it's a bare definition: enum [name] { ... };
            // vs a variable declaration: enum [name] [{...}] var;
            bool isDef = false;
            int look = pos + 1;
            if (look < tokens.size() && tokens[look].type == TokenType::OPEN_BRACE) isDef = true;
            else if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER) {
                look++;
                if (look < tokens.size() && tokens[look].type == TokenType::OPEN_BRACE) isDef = true;
            }
            
            if (isDef) {
                // Peek ahead for a semicolon after the closing brace
                int braceLevel = 0;
                bool foundBrace = false;
                for (size_t i = pos; i < tokens.size(); ++i) {
                    if (tokens[i].type == TokenType::OPEN_BRACE) { braceLevel++; foundBrace = true; }
                    else if (tokens[i].type == TokenType::CLOSE_BRACE) {
                        braceLevel--;
                        if (foundBrace && braceLevel == 0) {
                            if (i + 1 < tokens.size() && tokens[i+1].type == TokenType::SEMICOLON) {
                                advance(); // enum
                                auto def = parseEnumDefinition();
                                expect(TokenType::SEMICOLON, "Expected ';' after enum definition");
                                return def;
                            }
                            break;
                        }
                    }
                }
            }
        } else if (pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::IDENTIFIER && pos + 2 < tokens.size() && tokens[pos+2].type == TokenType::OPEN_BRACE) {
            advance(); // struct/union
            auto def = parseStructDefinition(isUnion);
            expect(TokenType::SEMICOLON, "Expected ';' after struct/union definition");
            return def;
        }
        return parseVariableDeclaration(isVolatile, isConst, isStatic, isRegister);
    }

    if (peek().type == TokenType::INT || peek().type == TokenType::CHAR || peek().type == TokenType::BOOL ||
        peek().type == TokenType::UNSIGNED || peek().type == TokenType::SIGNED ||
        (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value))) {
        return parseVariableDeclaration(isVolatile, isConst, isStatic, isRegister);
    }

    if (match(TokenType::RETURN)) {
        const Token& startToken = tokens[pos-1];
        std::unique_ptr<Expression> expr = nullptr;
        if (!match(TokenType::SEMICOLON)) {
            expr = parseExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after return expression");
        }
        return setPos(std::make_unique<ReturnStatement>(std::move(expr)), startToken);
    }

    if (match(TokenType::BREAK)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::SEMICOLON, "Expected ';' after break");
        return setPos(std::make_unique<BreakStatement>(), startToken);
    }

    if (match(TokenType::CONTINUE)) {
        const Token& startToken = tokens[pos-1];
        if (peek().type == TokenType::SEMICOLON) {
            advance();
            return setPos(std::make_unique<ContinueStatement>(), startToken);
        }
        // Handle continue <value> or continue default
        if (match(TokenType::DEFAULT)) {
            expect(TokenType::SEMICOLON, "Expected ';' after continue default");
            return setPos(std::make_unique<SwitchContinueStatement>(nullptr), startToken);
        }
        auto target = parseExpression();
        expect(TokenType::SEMICOLON, "Expected ';' after continue expression");
        return setPos(std::make_unique<SwitchContinueStatement>(std::move(target)), startToken);
    }

    if (match(TokenType::IF)) {

        const Token& startToken = tokens[pos-1];
        expect(TokenType::OPEN_PAREN, "Expected '(' after 'if'");
        auto condition = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')' after if condition");
        auto thenBranch = parseStatement();
        std::unique_ptr<Statement> elseBranch = nullptr;
        if (match(TokenType::ELSE)) {
            elseBranch = parseStatement();
        }
        return setPos(std::make_unique<IfStatement>(std::move(condition), std::move(thenBranch), std::move(elseBranch)), startToken);
    }

    if (match(TokenType::WHILE)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::OPEN_PAREN, "Expected '(' after 'while'");
        auto condition = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')' after while condition");
        auto body = parseStatement();
        return setPos(std::make_unique<WhileStatement>(std::move(condition), std::move(body)), startToken);
    }

    if (match(TokenType::DO)) {
        const Token& startToken = tokens[pos-1];
        auto body = parseStatement();
        expect(TokenType::WHILE, "Expected 'while' after 'do' body");
        expect(TokenType::OPEN_PAREN, "Expected '(' after 'while'");
        auto condition = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')' after while condition");
        expect(TokenType::SEMICOLON, "Expected ';' after do-while");
        return setPos(std::make_unique<DoWhileStatement>(std::move(body), std::move(condition)), startToken);
    }

    if (match(TokenType::FOR)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::OPEN_PAREN, "Expected '(' after 'for'");
        std::unique_ptr<Statement> initializer = nullptr;
        if (!match(TokenType::SEMICOLON)) {
            // Check if it's a declaration
            bool isDecl = false;
            if (peek().type == TokenType::INT || peek().type == TokenType::CHAR ||
                peek().type == TokenType::BOOL ||
                peek().type == TokenType::UNSIGNED || peek().type == TokenType::SIGNED ||
                peek().type == TokenType::STRUCT || peek().type == TokenType::UNION ||
                peek().type == TokenType::VOLATILE || peek().type == TokenType::CONST || peek().type == TokenType::RESTRICT || peek().type == TokenType::AUTO || peek().type == TokenType::REGISTER || peek().type == TokenType::INLINE) {
                isDecl = true;
            }

            if (isDecl) {
                bool isVolatile = false;
                bool isConst = false;
                bool isRegister = false;
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::REGISTER) || match(TokenType::INLINE)) {
                    if (tokens[pos-1].type == TokenType::VOLATILE) isVolatile = true;
                    else if (tokens[pos-1].type == TokenType::CONST) isConst = true;
                    else if (tokens[pos-1].type == TokenType::REGISTER) isRegister = true;
                }
                match(TokenType::AUTO); // consume auto if present
                initializer = parseVariableDeclaration(isVolatile, isConst, false, isRegister);
                // parseVariableDeclaration expects and consumes a semicolon
            } else {
                auto expr = parseExpression();
                expect(TokenType::SEMICOLON, "Expected ';' after for initializer expression");
                initializer = setPos(std::make_unique<ExpressionStatement>(std::move(expr)), startToken);
            }
        }
        std::unique_ptr<Expression> condition = nullptr;
        if (!match(TokenType::SEMICOLON)) {
            condition = parseExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after for condition");
        }
        std::unique_ptr<Expression> increment = nullptr;
        if (!match(TokenType::CLOSE_PAREN)) {
            increment = parseExpression();
            expect(TokenType::CLOSE_PAREN, "Expected ')' after for increment");
        }
        auto body = parseStatement();
        return setPos(std::make_unique<ForStatement>(std::move(initializer), std::move(condition), std::move(increment), std::move(body)), startToken);
    }

    if (match(TokenType::SWITCH)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::OPEN_PAREN, "Expected '(' after 'switch'");
        auto expression = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')' after switch expression");
        auto body = parseStatement();
        return setPos(std::make_unique<SwitchStatement>(std::move(expression), std::move(body)), startToken);
    }

    if (match(TokenType::CASE)) {
        const Token& startToken = tokens[pos-1];
        auto value = parseExpression();
        expect(TokenType::COLON, "Expected ':' after case value");
        return setPos(std::make_unique<CaseStatement>(std::move(value)), startToken);
    }

    if (match(TokenType::DEFAULT)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::COLON, "Expected ':' after default");
        return setPos(std::make_unique<DefaultStatement>(), startToken);
    }

    if (match(TokenType::ASM)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::OPEN_PAREN, "Expected '(' after asm");
        std::string code = expect(TokenType::STRING_LITERAL, "Expected string literal for asm code").value;
        expect(TokenType::CLOSE_PAREN, "Expected ')' after asm code");
        expect(TokenType::SEMICOLON, "Expected ';'");
        return setPos(std::make_unique<AsmStatement>(code), startToken);
    }

    if (peek().type == TokenType::STATIC_ASSERT) {
        return parseStaticAssert();
    }

    if (peek().type == TokenType::OPEN_BRACE) {
        return parseCompoundStatement();
    }
    
    const Token& startToken = peek();
    auto expr = parseExpression();
    expect(TokenType::SEMICOLON, "Expected ';'");
    return setPos(std::make_unique<ExpressionStatement>(std::move(expr)), startToken);
}

std::unique_ptr<Statement> Parser::parseVariableDeclaration(bool isVolatile, bool isConst, bool isStatic, bool isRegister) {
    std::unique_ptr<Expression> alignmentExpr = nullptr;
    if (match(TokenType::ALIGNAS)) {
        expect(TokenType::OPEN_PAREN, "Expected '(' after '_Alignas'");
        if (peek().type == TokenType::INT || peek().type == TokenType::CHAR || peek().type == TokenType::BOOL || peek().type == TokenType::STRUCT || peek().type == TokenType::VOID) {
            std::string aType;
            if (match(TokenType::INT)) aType = "int";
            else if (match(TokenType::CHAR)) aType = "char";
            else if (match(TokenType::VOID)) aType = "void";
            else if (match(TokenType::STRUCT)) aType = "struct " + expect(TokenType::IDENTIFIER, "Expected struct name").value;
            int aPtr = 0;
            while (match(TokenType::STAR)) aPtr++;
            alignmentExpr = std::make_unique<AlignofExpression>(aType, aPtr);
        } else {
            alignmentExpr = parseExpression();
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after alignment expression");
    }

    const Token& typeToken = peek();
    std::string type;
    bool isSigned = false;
    int basePtrLevel = 0;
    if (match(TokenType::SIGNED)) {
        isSigned = true;
        if (match(TokenType::INT)) type = "int";
        else if (match(TokenType::CHAR)) type = "char";
        else type = "int";
    }
    else if (match(TokenType::UNSIGNED)) {
        if (match(TokenType::INT)) type = "int";
        else if (match(TokenType::CHAR)) type = "char";
        else type = "int";
    }
    else if (match(TokenType::INT)) type = "int";
    else if (match(TokenType::CHAR)) type = "char";
    else if (match(TokenType::BOOL)) type = "_Bool";
    else if (match(TokenType::STRUCT) || match(TokenType::UNION) || match(TokenType::ENUM)) {
        bool isU = tokens[pos-1].type == TokenType::UNION;
        bool isE = tokens[pos-1].type == TokenType::ENUM;
        if (isE) {
            if (peek().type == TokenType::IDENTIFIER) {
                type = "enum " + advance().value;
            } else {
                auto def = parseEnumDefinition();
                type = "enum " + def->name;
            }
        } else {
            type = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
        }
    }
    else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
        std::string alias = advance().value;
        auto& ta = typedefs[alias];
        type = ta.baseType;
        isSigned = ta.isSigned;
        basePtrLevel = ta.pointerLevel;
        if (ta.isFunctionPointer) {
            // Typedef is a function pointer type — create decl directly
            std::string fpName = expect(TokenType::IDENTIFIER, "Expected variable name").value;
            auto decl = setPos(std::make_unique<VariableDeclaration>("void", fpName, 1), typeToken);
            decl->isSigned = false;
            decl->isVolatile = isVolatile;
            decl->isConst = isConst;
            decl->isStatic = isStatic;
            decl->isRegister = isRegister;
            decl->isFunctionPointer = true;
            decl->funcPtrSig = ta.funcPtrSig;
            decl->alignmentExpr = std::move(alignmentExpr);
            if (match(TokenType::EQUALS)) {
                decl->initializer = parseExpression();
            }
            expect(TokenType::SEMICOLON, "Expected ';'");
            return decl;
        }
    } else {
        throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": Expected type for variable declaration. Found '" + peek().typeToString() + "' instead.");
    }

    int ptrLevel = basePtrLevel;

    // Check for function pointer declaration: type (*name)(params)
    if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::STAR) {
        advance(); // consume '('
        advance(); // consume '*'
        std::string fpName = expect(TokenType::IDENTIFIER, "Expected function pointer name").value;
        expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer name");
        auto fpSig = parseFuncPtrParams(type, ptrLevel, isSigned);
        auto decl = setPos(std::make_unique<VariableDeclaration>("void", fpName, 1), typeToken);
        decl->isSigned = false;
        decl->isVolatile = isVolatile;
        decl->isConst = isConst;
        decl->isStatic = isStatic;
        decl->isRegister = isRegister;
        decl->isFunctionPointer = true;
        decl->funcPtrSig = fpSig;
        decl->alignmentExpr = std::move(alignmentExpr);
        if (match(TokenType::EQUALS)) {
            decl->initializer = parseExpression();
        }
        expect(TokenType::SEMICOLON, "Expected ';'");
        return decl;
    }

    while (match(TokenType::STAR)) ptrLevel++;

    // Handle const/volatile after * (qualifies the pointer itself)
    bool isPointerConst = false;
    while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
        if (match(TokenType::CONST)) isPointerConst = true;
        else if (!match(TokenType::VOLATILE)) match(TokenType::RESTRICT);
    }

    std::string name = expect(TokenType::IDENTIFIER, "Expected variable name").value;
    std::vector<int> arrayDims;
    while (match(TokenType::OPEN_SQUARE)) {
        const Token& sizeToken = expect(TokenType::INTEGER_LITERAL, "Expected integer literal for array size");
        arrayDims.push_back(std::stoi(sizeToken.value));
        expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
    }
    auto decl = setPos(std::make_unique<VariableDeclaration>(type, name, ptrLevel), typeToken);
    decl->isSigned = isSigned;
    decl->isVolatile = isVolatile;
    decl->isConst = isConst;
    decl->isStatic = isStatic;
    decl->isRegister = isRegister;
    decl->isPointerConst = isPointerConst;
    decl->alignmentExpr = std::move(alignmentExpr);
    decl->arrayDims = arrayDims;

    if (match(TokenType::EQUALS)) {
        if (peek().type == TokenType::OPEN_BRACE) {
            decl->initializer = parseInitializerList();
        } else {
            decl->initializer = parseExpression();
        }
    }
    expect(TokenType::SEMICOLON, "Expected ';'");
    return decl;
}

std::unique_ptr<StaticAssert> Parser::parseStaticAssert() {
    const Token& startToken = expect(TokenType::STATIC_ASSERT, "Expected '_Static_assert'");
    expect(TokenType::OPEN_PAREN, "Expected '(' after '_Static_assert'");
    auto condition = parseExpression();
    expect(TokenType::COMMA, "Expected ',' after condition in '_Static_assert'");
    std::string message = expect(TokenType::STRING_LITERAL, "Expected string literal for '_Static_assert' message").value;
    expect(TokenType::CLOSE_PAREN, "Expected ')' after '_Static_assert'");
    expect(TokenType::SEMICOLON, "Expected ';'");
    return setPos(std::make_unique<StaticAssert>(std::move(condition), message), startToken);
}

std::unique_ptr<StructDefinition> Parser::parseStructDefinition(bool isUnion) {
    const Token& startToken = tokens[pos-1]; // 'struct' or 'union'
    std::string name;
    if (peek().type == TokenType::IDENTIFIER) {
        name = advance().value;
    } else {
        name = (isUnion ? "<anon_union_" : "<anon_struct_") + std::to_string(anonymousAggregateCount++) + ">";
    }

    expect(TokenType::OPEN_BRACE, "Expected '{'");
    auto def = setPos(std::make_unique<StructDefinition>(name, isUnion), startToken);
    while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE) {
        std::unique_ptr<Expression> mAlignmentExpr = nullptr;
        if (match(TokenType::ALIGNAS)) {
            expect(TokenType::OPEN_PAREN, "Expected '(' after '_Alignas'");
            if (peek().type == TokenType::INT || peek().type == TokenType::CHAR || peek().type == TokenType::STRUCT || peek().type == TokenType::UNION || peek().type == TokenType::VOID) {
                std::string aType;
                if (match(TokenType::INT)) aType = "int";
                else if (match(TokenType::CHAR)) aType = "char";
                else if (match(TokenType::VOID)) aType = "void";
                else if (match(TokenType::STRUCT) || match(TokenType::UNION)) {
                    bool isU = tokens[pos-1].type == TokenType::UNION;
                    aType = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
                }
                int aPtr = 0;
                while (match(TokenType::STAR)) aPtr++;
                mAlignmentExpr = std::make_unique<AlignofExpression>(aType, aPtr);
            } else {
                mAlignmentExpr = parseExpression();
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')' after alignment expression");
        }

        if (peek().type == TokenType::STRUCT || peek().type == TokenType::UNION) {
            bool isNestedUnion = peek().type == TokenType::UNION;
            if (pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::OPEN_BRACE) {
                // Anonymous nested struct/union
                advance(); // struct/union
                auto nestedDef = parseStructDefinition(isNestedUnion);
                std::string nestedTypeName = (isNestedUnion ? "union " : "struct ") + nestedDef->name;
                { StructMember sm; sm.type = nestedTypeName; sm.isAnonymous = true; def->members.push_back(std::move(sm)); }
                pendingDefinitions.push_back(std::move(nestedDef));
                match(TokenType::SEMICOLON); // consume optional semicolon
                continue; 
            }
        }

        bool mIsConst = false;
        while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT)) {
            if (tokens[pos-1].type == TokenType::CONST) mIsConst = true;
        }

        std::string type;
        bool mIsSigned = false;
        if (match(TokenType::SIGNED)) {
            mIsSigned = true;
            if (match(TokenType::INT)) type = "int";
            else if (match(TokenType::CHAR)) type = "char";
            else type = "int";
        }
        else if (match(TokenType::UNSIGNED)) {
            if (match(TokenType::INT)) type = "int";
            else if (match(TokenType::CHAR)) type = "char";
            else type = "int";
        }
        else if (match(TokenType::INT)) type = "int";
        else if (match(TokenType::CHAR)) type = "char";
        else if (match(TokenType::BOOL)) type = "_Bool";
        else if (match(TokenType::STRUCT) || match(TokenType::UNION)) {
            bool isU = tokens[pos-1].type == TokenType::UNION;
            if (peek().type == TokenType::OPEN_BRACE) {
                // Inline definition: struct { ... } var;
                auto nestedDef = parseStructDefinition(isU);
                type = (isU ? "union " : "struct ") + nestedDef->name;
                pendingDefinitions.push_back(std::move(nestedDef));
            } else {
                type = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
            }
        } else {
            throw std::runtime_error("Expected member type");
        }
        int ptrLevel = 0;
        while (match(TokenType::STAR)) ptrLevel++;
        std::string memberName = expect(TokenType::IDENTIFIER, "Expected member name").value;
        std::vector<int> memberArrayDims;
        while (match(TokenType::OPEN_SQUARE)) {
            const Token& sizeToken = expect(TokenType::INTEGER_LITERAL, "Expected integer literal for array size");
            memberArrayDims.push_back(std::stoi(sizeToken.value));
            expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
        }
        StructMember sm;
        sm.type = type; sm.pointerLevel = ptrLevel; sm.isSigned = mIsSigned;
        sm.name = memberName; sm.isConst = mIsConst; sm.alignment = 0;
        sm.alignmentExpr = std::move(mAlignmentExpr); sm.isAnonymous = false;
        sm.arrayDims = memberArrayDims;
        def->members.push_back(std::move(sm));
        expect(TokenType::SEMICOLON, "Expected ';'");
    }
    expect(TokenType::CLOSE_BRACE, "Expected '}'");
    return def;
}

std::unique_ptr<Expression> Parser::parseExpression() {
    auto expr = parseConditional();
    if (match(TokenType::EQUALS) || match(TokenType::PLUS_EQUALS) || match(TokenType::MINUS_EQUALS) ||
        match(TokenType::STAR_EQUALS) || match(TokenType::SLASH_EQUALS) || match(TokenType::PERCENT_EQUALS) ||
        match(TokenType::AMPERSAND_EQUALS) || match(TokenType::PIPE_EQUALS) || match(TokenType::CARET_EQUALS) ||
        match(TokenType::LSHIFT_EQUALS) || match(TokenType::RSHIFT_EQUALS)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        return setPos(std::make_unique<Assignment>(std::move(expr), parseExpression(), op), opToken);
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parseConditional() {
    auto expr = parseLogicalOr();
    if (match(TokenType::QUESTION_MARK)) {
        const Token& opToken = tokens[pos-1];
        auto thenExpr = parseExpression();
        expect(TokenType::COLON, "Expected ':' in conditional expression");
        auto elseExpr = parseConditional();
        return setPos(std::make_unique<ConditionalExpression>(std::move(expr), std::move(thenExpr), std::move(elseExpr)), opToken);
    }
    return expr;
}

std::unique_ptr<Expression> Parser::parseLogicalOr() {
    auto left = parseLogicalAnd();
    while (match(TokenType::OR)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseLogicalAnd();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseLogicalAnd() {
    auto left = parseBitwiseOr();
    while (match(TokenType::AND)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseBitwiseOr();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseBitwiseOr() {
    auto left = parseBitwiseXor();
    while (match(TokenType::PIPE)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseBitwiseXor();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseBitwiseXor() {
    auto left = parseBitwiseAnd();
    while (match(TokenType::CARET)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseBitwiseAnd();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseBitwiseAnd() {
    auto left = parseEquality();
    while (match(TokenType::AMPERSAND)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseEquality();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseEquality() {
    auto left = parseRelational();
    while (match(TokenType::EQUALS_EQUALS) || match(TokenType::NOT_EQUALS)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseRelational();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseRelational() {
    auto left = parseShift();
    while (match(TokenType::LESS_THAN) || match(TokenType::GREATER_THAN) ||
           match(TokenType::LESS_EQUAL) || match(TokenType::GREATER_EQUAL)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseShift();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseShift() {
    auto left = parseAdditive();
    while (match(TokenType::LSHIFT) || match(TokenType::RSHIFT)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseAdditive();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseMultiplicative();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto right = parseUnary();
        left = setPos(std::make_unique<BinaryOperation>(op, std::move(left), std::move(right)), opToken);
    }
    return left;
}

std::unique_ptr<Expression> Parser::parseUnary() {
    if (match(TokenType::SIZEOF)) {
        const Token& startToken = tokens[pos-1];
        if (match(TokenType::OPEN_PAREN)) {
            // Check if it's a type or an expression
            if (peek().type == TokenType::INT || peek().type == TokenType::CHAR ||
                peek().type == TokenType::BOOL ||
                peek().type == TokenType::STRUCT || peek().type == TokenType::UNION || peek().type == TokenType::ENUM ||
                peek().type == TokenType::SIGNED || peek().type == TokenType::UNSIGNED ||
                (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value))) {

                std::string type;
                bool sIsSigned = false;
                int sBasePtrLevel = 0;
                if (match(TokenType::SIGNED) || match(TokenType::UNSIGNED)) {
                    if (tokens[pos-1].type == TokenType::SIGNED) sIsSigned = true;
                    if (match(TokenType::INT)) type = "int";
                    else if (match(TokenType::CHAR)) type = "char";
                    else type = "int";
                }
                else if (match(TokenType::INT)) type = "int";
                else if (match(TokenType::CHAR)) type = "char";
                else if (match(TokenType::BOOL)) type = "_Bool";
                else if (match(TokenType::STRUCT) || match(TokenType::UNION) || match(TokenType::ENUM)) {
                    bool isU = tokens[pos-1].type == TokenType::UNION;
                    bool isE = tokens[pos-1].type == TokenType::ENUM;
                    if (isE) {
                        type = "enum " + expect(TokenType::IDENTIFIER, "Expected enum name").value;
                    } else {
                        type = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
                    }
                }

                else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
                    std::string sAlias = advance().value;
                    type = typedefs[sAlias].baseType;
                    sIsSigned = typedefs[sAlias].isSigned;
                    sBasePtrLevel = typedefs[sAlias].pointerLevel;
                }

                int ptrLevel = sBasePtrLevel;
                while (match(TokenType::STAR)) ptrLevel++;
                expect(TokenType::CLOSE_PAREN, "Expected ')' after type in sizeof");
                auto node = std::make_unique<SizeofExpression>(type, ptrLevel);
                // SizeofExpression doesn't currently store isSigned, but it's used for getTypeSize
                return setPos(std::move(node), startToken);
            }
 else {
                auto expr = parseExpression();
                expect(TokenType::CLOSE_PAREN, "Expected ')' after expression in sizeof");
                return setPos(std::make_unique<SizeofExpression>(std::move(expr)), startToken);
            }
        } else {
            return setPos(std::make_unique<SizeofExpression>(parseUnary()), startToken);
        }
    }

    // Explicit cast: (type)expr
    if (peek().type == TokenType::OPEN_PAREN) {
        size_t savedPos = pos;
        const Token& startToken = tokens[pos];
        advance(); // consume '('
        bool isCast = false;
        std::string castType;
        bool castSigned = false;
        int castPtrLevel = 0;

        if (peek().type == TokenType::INT || peek().type == TokenType::CHAR || peek().type == TokenType::BOOL ||
            peek().type == TokenType::VOID ||
            peek().type == TokenType::STRUCT || peek().type == TokenType::UNION || peek().type == TokenType::ENUM ||
            peek().type == TokenType::SIGNED || peek().type == TokenType::UNSIGNED ||
            (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value))) {

            if (match(TokenType::SIGNED) || match(TokenType::UNSIGNED)) {
                castSigned = (tokens[pos-1].type == TokenType::SIGNED);
                if (match(TokenType::INT)) castType = "int";
                else if (match(TokenType::CHAR)) castType = "char";
                else castType = "int";
            }
            else if (match(TokenType::INT)) castType = "int";
            else if (match(TokenType::CHAR)) castType = "char";
            else if (match(TokenType::BOOL)) castType = "_Bool";
            else if (match(TokenType::VOID)) castType = "void";
            else if (match(TokenType::STRUCT) || match(TokenType::UNION) || match(TokenType::ENUM)) {
                bool isU = tokens[pos-1].type == TokenType::UNION;
                bool isE = tokens[pos-1].type == TokenType::ENUM;
                if (isE) castType = "enum " + expect(TokenType::IDENTIFIER, "Expected enum name").value;
                else castType = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
            }
            else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
                std::string alias = advance().value;
                castType = typedefs[alias].baseType;
                castSigned = typedefs[alias].isSigned;
                castPtrLevel = typedefs[alias].pointerLevel;
            }

            while (match(TokenType::STAR)) castPtrLevel++;

            if (peek().type == TokenType::CLOSE_PAREN) {
                advance(); // consume ')'
                isCast = true;
            }
        }

        if (isCast) {
            auto operand = parseUnary();
            return setPos(std::make_unique<CastExpression>(castType, castPtrLevel, castSigned, std::move(operand)), startToken);
        } else {
            pos = savedPos; // backtrack, let parsePrimary handle it as parenthesized expr
        }
    }

    if (match(TokenType::BANG) || match(TokenType::TILDE) || match(TokenType::MINUS) || match(TokenType::STAR) || match(TokenType::AMPERSAND) ||
        match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        return setPos(std::make_unique<UnaryOperation>(op, parseUnary()), opToken);
    }
    return parsePrimary();
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    std::unique_ptr<Expression> expr;
    if (match(TokenType::GENERIC)) {
        return parseGenericSelection();
    }
    if (match(TokenType::ALIGNOF)) {
        expect(TokenType::OPEN_PAREN, "Expected '(' after '_Alignof'");
        std::string typeName;
        if (match(TokenType::INT)) typeName = "int";
        else if (match(TokenType::CHAR)) typeName = "char";
        else if (match(TokenType::VOID)) typeName = "void";
        else if (match(TokenType::STRUCT)) typeName = "struct " + expect(TokenType::IDENTIFIER, "Expected struct name").value;
        else throw std::runtime_error("Expected type name in _Alignof");
        
        int aPtrLevel = 0;
        while (match(TokenType::STAR)) aPtrLevel++;
        
        expect(TokenType::CLOSE_PAREN, "Expected ')' after _Alignof type");
        return setPos(std::make_unique<AlignofExpression>(typeName, aPtrLevel), tokens[pos-1]);
    }

    if (peek().type == TokenType::IDENTIFIER && (peek().value == "__func__" || peek().value == "__FUNCTION__")) {
        const Token& funcToken = advance();
        if (currentFunctionName.empty()) {
            throw std::runtime_error("Error at " + std::to_string(funcToken.line) + ":" + std::to_string(funcToken.column) + ": __func__ used outside of a function");
        }
        expr = setPos(std::make_unique<StringLiteral>(currentFunctionName), funcToken);
    } else if (peek().type == TokenType::IDENTIFIER) {
        const Token& nameToken = advance();
        std::string name = nameToken.value;

        auto enumIt = enumConstants.find(name);
        if (enumIt != enumConstants.end()) {
            return setPos(std::make_unique<IntegerLiteral>(enumIt->second), nameToken);
        }

        if (match(TokenType::OPEN_PAREN)) {
            auto call = setPos(std::make_unique<FunctionCall>(name), nameToken);
            if (peek().type != TokenType::CLOSE_PAREN) {
                do {
                    call->arguments.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')'");
            expr = std::move(call);
        } else {
            expr = setPos(std::make_unique<VariableReference>(name), nameToken);
        }
    } else if (peek().type == TokenType::INTEGER_LITERAL) {
        const Token& litToken = advance();
        expr = setPos(std::make_unique<IntegerLiteral>(std::stoi(litToken.value)), litToken);
    } else if (peek().type == TokenType::STRING_LITERAL) {
        const Token& litToken = advance();
        expr = setPos(std::make_unique<StringLiteral>(litToken.value), litToken);
    } else if (match(TokenType::OPEN_PAREN)) {
        expr = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
    } else {
        std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
        throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": Expected expression. Found '" + foundStr + "' (" + peek().typeToString() + ") instead.");
    }

    while (match(TokenType::DOT) || match(TokenType::ARROW) || match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS) || match(TokenType::OPEN_SQUARE) || match(TokenType::OPEN_PAREN)) {
        const Token& opToken = tokens[pos-1];
        if (opToken.type == TokenType::PLUS_PLUS || opToken.type == TokenType::MINUS_MINUS) {
            expr = setPos(std::make_unique<UnaryOperation>(opToken.value + "_POST", std::move(expr)), opToken);
        } else if (opToken.type == TokenType::OPEN_SQUARE) {
            auto indexExpr = parseExpression();
            expect(TokenType::CLOSE_SQUARE, "Expected ']' after array index");
            expr = setPos(std::make_unique<ArrayAccess>(std::move(expr), std::move(indexExpr)), opToken);
        } else if (opToken.type == TokenType::OPEN_PAREN) {
            // Indirect function call: expr(args) — e.g. (*fp)(args), fp_array[i](args)
            auto call = setPos(std::make_unique<FunctionCall>(""), opToken);
            call->callExpr = std::move(expr);
            if (peek().type != TokenType::CLOSE_PAREN) {
                do {
                    call->arguments.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')'");
            expr = std::move(call);
        } else {
            bool isArrow = (opToken.type == TokenType::ARROW);
            std::string memberName = expect(TokenType::IDENTIFIER, "Expected member name").value;
            expr = setPos(std::make_unique<MemberAccess>(std::move(expr), memberName, isArrow), opToken);
        }
    }
    return expr;
}

void Parser::flushPending(TranslationUnit& unit) {
    for (auto& def : pendingDefinitions) {
        if (def) unit.topLevelDecls.push_back(std::move(def));
    }
    pendingDefinitions.clear();
}

std::unique_ptr<Expression> Parser::parseGenericSelection() {
    const Token& startToken = tokens[pos-1];
    expect(TokenType::OPEN_PAREN, "Expected '(' after '_Generic'");
    auto control = parseExpression();
    expect(TokenType::COMMA, "Expected ',' after controlling expression in _Generic");

    auto node = setPos(std::make_unique<GenericSelection>(std::move(control)), startToken);

    do {
        GenericAssociation assoc;
        if (match(TokenType::DEFAULT)) {
            assoc.isDefault = true;
        } else {
            if (match(TokenType::INT)) assoc.typeName = "int";
            else if (match(TokenType::CHAR)) assoc.typeName = "char";
            else if (match(TokenType::VOID)) assoc.typeName = "void";
            else if (match(TokenType::STRUCT) || match(TokenType::UNION)) {
                assoc.typeName = (tokens[pos-1].type == TokenType::STRUCT ? "struct " : "union ") + expect(TokenType::IDENTIFIER, "Expected aggregate name").value;
            } else {
                throw std::runtime_error("Expected type name in _Generic association");
            }
            while (match(TokenType::STAR)) assoc.pointerLevel++;
        }

        expect(TokenType::COLON, "Expected ':' in _Generic association");
        assoc.result = parseExpression();
        node->associations.push_back(std::move(assoc));

    } while (match(TokenType::COMMA));

    expect(TokenType::CLOSE_PAREN, "Expected ')' at end of _Generic");
    return node;
}

std::unique_ptr<Expression> Parser::parseInitializerList() {
    const Token& startToken = expect(TokenType::OPEN_BRACE, "Expected '{'");
    auto initList = setPos(std::make_unique<InitializerList>(), startToken);
    if (peek().type != TokenType::CLOSE_BRACE) {
        do {
            if (peek().type == TokenType::OPEN_BRACE) {
                initList->elements.push_back(parseInitializerList());
            } else {
                initList->elements.push_back(parseConditional());
            }
        } while (match(TokenType::COMMA) && peek().type != TokenType::CLOSE_BRACE);
    }
    expect(TokenType::CLOSE_BRACE, "Expected '}'");
    return initList;
}

void Parser::parseTypedef() {
    std::string baseType;
    bool isSigned = false;
    int basePtrLevel = 0;

    if (match(TokenType::SIGNED)) {
        isSigned = true;
        if (match(TokenType::INT)) baseType = "int";
        else if (match(TokenType::CHAR)) baseType = "char";
        else baseType = "int";
    }
    else if (match(TokenType::UNSIGNED)) {
        if (match(TokenType::INT)) baseType = "int";
        else if (match(TokenType::CHAR)) baseType = "char";
        else baseType = "int";
    }
    else if (match(TokenType::INT)) baseType = "int";
    else if (match(TokenType::CHAR)) baseType = "char";
    else if (match(TokenType::BOOL)) baseType = "_Bool";
    else if (match(TokenType::VOID)) baseType = "void";
    else if (match(TokenType::STRUCT) || match(TokenType::UNION)) {
        bool isU = tokens[pos-1].type == TokenType::UNION;
        if (peek().type == TokenType::OPEN_BRACE) {
            auto def = parseStructDefinition(isU);
            baseType = (isU ? "union " : "struct ") + def->name;
            pendingDefinitions.push_back(std::move(def));
            // parseStructDefinition might have consumed a semicolon if it was struct { ... };
            // but for typedef struct { ... } Name; it shouldn't have.
            // If it did, we might need to backtrack or adjust.
            // Currently parseStructDefinition has match(TokenType::SEMICOLON) which is optional.
        } else {
            baseType = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
        }
    }
    else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
        std::string alias = advance().value;
        baseType = typedefs[alias].baseType;
        isSigned = typedefs[alias].isSigned;
        basePtrLevel = typedefs[alias].pointerLevel;
    }
    else {
        throw std::runtime_error("Expected type in typedef");
    }

    int ptrLevel = basePtrLevel;

    // Check for function pointer typedef: typedef int (*name)(int, char);
    if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::STAR) {
        advance(); // consume '('
        advance(); // consume '*'
        std::string newName = expect(TokenType::IDENTIFIER, "Expected alias name in function pointer typedef").value;
        expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer name");
        auto sig = parseFuncPtrParams(baseType, ptrLevel, isSigned);
        expect(TokenType::SEMICOLON, "Expected ';' after typedef");
        TypeAlias alias;
        alias.baseType = "void"; // function pointers are pointer-sized
        alias.pointerLevel = 1;
        alias.isSigned = false;
        alias.isFunctionPointer = true;
        alias.funcPtrSig = sig;
        typedefs[newName] = alias;
        return;
    }

    while (match(TokenType::STAR)) ptrLevel++;

    std::string newName = expect(TokenType::IDENTIFIER, "Expected alias name in typedef").value;
    expect(TokenType::SEMICOLON, "Expected ';' after typedef");

    typedefs[newName] = {baseType, ptrLevel, isSigned};
}

bool Parser::isTypedef(const std::string& name) const {
    return typedefs.find(name) != typedefs.end();
}

std::shared_ptr<FuncPtrSignature> Parser::parseFuncPtrParams(const std::string& returnType, int returnPtrLevel, bool returnIsSigned) {
    auto sig = std::make_shared<FuncPtrSignature>();
    sig->returnType = returnType;
    sig->returnPointerLevel = returnPtrLevel;
    sig->returnIsSigned = returnIsSigned;
    // Current token should be '(' — the opening paren of the parameter list
    expect(TokenType::OPEN_PAREN, "Expected '(' for function pointer parameter list");
    if (peek().type == TokenType::VOID && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::CLOSE_PAREN) {
        advance(); // consume 'void'
    } else if (peek().type != TokenType::CLOSE_PAREN) {
        do {
            FuncPtrSignature::Param fp;
            while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT)) {}
            if (match(TokenType::SIGNED)) {
                fp.isSigned = true;
                if (match(TokenType::INT)) fp.type = "int";
                else if (match(TokenType::CHAR)) fp.type = "char";
                else fp.type = "int";
            } else if (match(TokenType::UNSIGNED)) {
                if (match(TokenType::INT)) fp.type = "int";
                else if (match(TokenType::CHAR)) fp.type = "char";
                else fp.type = "int";
            } else if (match(TokenType::INT)) fp.type = "int";
            else if (match(TokenType::CHAR)) fp.type = "char";
            else if (match(TokenType::BOOL)) fp.type = "_Bool";
            else if (match(TokenType::VOID)) fp.type = "void";
            else if (match(TokenType::STRUCT) || match(TokenType::UNION) || match(TokenType::ENUM)) {
                bool isU = tokens[pos-1].type == TokenType::UNION;
                bool isE = tokens[pos-1].type == TokenType::ENUM;
                if (isE) fp.type = "enum " + expect(TokenType::IDENTIFIER, "Expected enum name").value;
                else fp.type = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct/union name").value;
            } else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
                std::string alias = advance().value;
                fp.type = typedefs[alias].baseType;
                fp.isSigned = typedefs[alias].isSigned;
                fp.pointerLevel = typedefs[alias].pointerLevel;
            } else {
                throw std::runtime_error("Expected type in function pointer parameter list");
            }
            while (match(TokenType::STAR)) fp.pointerLevel++;
            // Skip optional parameter name
            if (peek().type == TokenType::IDENTIFIER && !isTypedef(peek().value)) advance();
            sig->params.push_back(fp);
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer parameter list");
    return sig;
}

std::unique_ptr<EnumDefinition> Parser::parseEnumDefinition() {
    const Token& startToken = tokens[pos-1]; // 'enum'
    std::string name;
    if (peek().type == TokenType::IDENTIFIER) {
        name = advance().value;
        enumNames.insert(name);
    }

    if (match(TokenType::OPEN_BRACE)) {
        auto def = setPos(std::make_unique<EnumDefinition>(name), startToken);
        int currentValue = 0;
        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE) {
            std::string enumName = expect(TokenType::IDENTIFIER, "Expected enumerator name").value;
            if (match(TokenType::EQUALS)) {
                // For simplicity, only integer literals for now. 
                // Full expression evaluation would be better but this covers most cases.
                currentValue = std::stoi(expect(TokenType::INTEGER_LITERAL, "Expected integer literal for enum value").value);
            }
            enumConstants[enumName] = currentValue;
            def->enumerators.push_back({enumName, currentValue});
            currentValue++;
            if (!match(TokenType::COMMA)) break;
        }
        expect(TokenType::CLOSE_BRACE, "Expected '}' after enum members");
        return def;
    }
    return setPos(std::make_unique<EnumDefinition>(name), startToken);
}
