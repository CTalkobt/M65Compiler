#include "Parser.hpp"
#include <stdexcept>
#include <iostream>

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {
    // Built-in type aliases for wide integers
    typedefs["__int128"] = {"struct __int128", 0, false, false, {}, {}};
    typedefs["__int128_t"] = {"struct __int128", 0, false, false, {}, {}};
    typedefs["__uint128_t"] = {"struct __int128", 0, false, false, {}, {}};
    typedefs["__int64_t"] = {"struct __int64", 0, false, false, {}, {}};
    typedefs["__uint64_t"] = {"struct __int64", 0, false, false, {}, {}};
}

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
        // Skip __extension__ and __attribute__ at top level (GCC compatibility)
        while (match(TokenType::EXTENSION) || tryParseAttribute()) {}
        // Skip stray semicolons at top level (e.g., }; after function)
        if (match(TokenType::SEMICOLON)) continue;
        if (match(TokenType::TYPEDEF)) {
            parseTypedef();
            continue;
        }
        if (peek().type == TokenType::ASM) {
            advance();
            match(TokenType::VOLATILE); // optional volatile
            if (peek().type == TokenType::IDENTIFIER && peek().value == "goto") advance();
            expect(TokenType::OPEN_PAREN, "Expected '(' after asm");
            std::string code = expect(TokenType::STRING_LITERAL, "Expected string literal for asm code").value;
            // Skip extended asm constraint sections
            for (int section = 0; section < 3 && match(TokenType::COLON); section++) {
                if (peek().type == TokenType::CLOSE_PAREN || peek().type == TokenType::COLON) continue;
                do {
                    if (peek().type == TokenType::OPEN_SQUARE) { advance(); while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE) advance(); match(TokenType::CLOSE_SQUARE); }
                    if (peek().type == TokenType::STRING_LITERAL) {
                        advance();
                        if (match(TokenType::OPEN_PAREN)) { int d=1; while (d>0 && peek().type!=TokenType::END_OF_FILE) { if (match(TokenType::OPEN_PAREN)) d++; else if (peek().type==TokenType::CLOSE_PAREN) { d--; if(d>0) advance(); } else advance(); } match(TokenType::CLOSE_PAREN); }
                    } else break;
                } while (match(TokenType::COMMA));
            }
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
                // Check if this is enum as function return type: enum E foo(...)
                // vs enum definition: enum E { ... };
                size_t eLook = pos + 1; // after 'enum'
                if (eLook < tokens.size() && tokens[eLook].type == TokenType::IDENTIFIER) {
                    eLook++; // after enum name
                    if (eLook < tokens.size() && tokens[eLook].type == TokenType::IDENTIFIER) {
                        // enum Name FuncName — function with enum return
                        auto decl = parseFunctionDeclaration();
                        flushPending(*unit);
                        unit->topLevelDecls.push_back(std::move(decl));
                        continue;
                    }
                }
                advance(); // enum
                auto def = parseEnumDefinition();
                while (tryParseAttribute()) {}
                // enum E; or enum E { ... };
                // Could also be enum E var; — check for identifier before ;
                if (peek().type == TokenType::IDENTIFIER) {
                    // enum E var; — variable with enum type
                    std::string eType = "enum " + def->name;
                    std::string eName = advance().value;
                    auto vDecl = std::make_unique<VariableDeclaration>(eType, eName, 0);
                    vDecl->isGlobal = true;
                    if (match(TokenType::EQUALS)) vDecl->initializer = parseExpression();
                    expect(TokenType::SEMICOLON, "Expected ';'");
                    unit->topLevelDecls.push_back(std::move(def));
                    unit->topLevelDecls.push_back(std::move(vDecl));
                    continue;
                }
                expect(TokenType::SEMICOLON, "Expected ';' after enum definition");
                unit->topLevelDecls.push_back(std::move(def));
                continue;
            }
            {
                // Check for struct/union forward declaration: struct Name;
                size_t sLook = pos + 1;
                while (sLook < tokens.size()) {
                    if (tokens[sLook].type == TokenType::UNPACKED) {
                        sLook++;
                    } else if (tokens[sLook].type == TokenType::ATTRIBUTE) {
                        sLook++;
                        if (sLook < tokens.size() && tokens[sLook].type == TokenType::OPEN_PAREN) {
                            sLook++;
                            int parens = 1;
                            while (sLook < tokens.size() && parens > 0) {
                                if (tokens[sLook].type == TokenType::OPEN_PAREN) parens++;
                                else if (tokens[sLook].type == TokenType::CLOSE_PAREN) parens--;
                                sLook++;
                            }
                        }
                    } else {
                        break;
                    }
                }
                if (sLook < tokens.size() && tokens[sLook].type == TokenType::IDENTIFIER &&
                    sLook + 1 < tokens.size() && tokens[sLook+1].type == TokenType::SEMICOLON) {
                    advance(); // struct/union
                    std::string fwdName = advance().value; // name
                    advance(); // semicolon
                    // Forward declaration — just register the name, no definition needed
                    continue;
                }
                // Check for struct/union definition: struct [__unpacked] name { ... };
                if (sLook < tokens.size() && tokens[sLook].type == TokenType::IDENTIFIER &&
                    sLook + 1 < tokens.size() && (tokens[sLook+1].type == TokenType::OPEN_BRACE ||
                    tokens[sLook+1].type == TokenType::COLON ||
                    (tokens[sLook+1].type == TokenType::IDENTIFIER && tokens[sLook+1].value == "final"))) {
                    advance(); // struct/union
                    auto def = parseStructDefinition(isUnion);
                    // Skip __attribute__ after struct/union definition
                    while (tryParseAttribute()) {}
                    if (peek().type == TokenType::SEMICOLON) {
                        advance(); // consume ';'
                        flushPending(*unit);
                        // Methods stay in def->methods — IRBuilder::visit(StructDefinition&)
                        // will emit them as top-level functions and generate vtable data.
                        unit->topLevelDecls.push_back(std::move(def));
                    } else {
                        // struct Name { ... } var; — inline definition + variable
                        std::string sType = (isUnion ? "union " : "struct ") + def->name;
                        unit->topLevelDecls.push_back(std::move(def));
                        // Parse the variable declaration using the struct type
                        int ptrLevel = 0;
                        while (tryParseAttribute()) {} // skip attributes before pointer/name
                        while (match(TokenType::STAR)) ptrLevel++;
                        std::string vName = expect(TokenType::IDENTIFIER, "Expected variable name after struct definition").value;
                        auto vDecl = std::make_unique<VariableDeclaration>(sType, vName, ptrLevel);
                        vDecl->isGlobal = true;
                        // Array dimensions: struct S { } arr[2];
                        while (match(TokenType::OPEN_SQUARE)) {
                            if (match(TokenType::CLOSE_SQUARE)) {
                                vDecl->arrayDims.push_back(0);
                            } else {
                                auto szExpr = parseExpression();
                                int sz = 1;
                                if (auto* lit = dynamic_cast<IntegerLiteral*>(szExpr.get())) sz = (int)lit->value;
                                vDecl->arrayDims.push_back(sz);
                                expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                            }
                        }
                        while (tryParseAttribute()) {} // skip __attribute__ after var name
                        if (match(TokenType::EQUALS)) {
                            if (peek().type == TokenType::OPEN_BRACE)
                                vDecl->initializer = parseInitializerList();
                            else
                                vDecl->initializer = parseExpression();
                        }
                        flushPending(*unit);
                        unit->topLevelDecls.push_back(std::move(vDecl));
                        // Multi-variable: struct S { } a, b;
                        while (match(TokenType::COMMA)) {
                            while (tryParseAttribute()) {}
                            int ep = 0;
                            while (match(TokenType::STAR)) ep++;
                            std::string en = expect(TokenType::IDENTIFIER, "Expected variable name").value;
                            auto ev = std::make_unique<VariableDeclaration>(sType, en, ep);
                            ev->isGlobal = true;
                            while (match(TokenType::OPEN_SQUARE)) {
                                if (match(TokenType::CLOSE_SQUARE)) ev->arrayDims.push_back(0);
                                else {
                                    auto se = parseExpression();
                                    int ss = 1;
                                    if (auto* lit = dynamic_cast<IntegerLiteral*>(se.get())) ss = (int)lit->value;
                                    ev->arrayDims.push_back(ss);
                                    expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                                }
                            }
                            if (match(TokenType::EQUALS)) {
                                if (peek().type == TokenType::OPEN_BRACE) ev->initializer = parseInitializerList();
                                else ev->initializer = parseExpression();
                            }
                            unit->topLevelDecls.push_back(std::move(ev));
                        }
                        expect(TokenType::SEMICOLON, "Expected ';'");
                    }
                    continue;
                }
            }
        }

        // Look ahead to distinguish function from global variable
        size_t look = pos;
        bool isVol = false;
        bool isConst = false;
        bool isNR = false;
        bool isExtern = false;
        bool isStatic = false;
        bool isFC = false;
        bool isInterrupt = false;
        bool isNaked = false;
        bool isRegparm = false;
        bool isInlineFunc = false;

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
        if (tokens[look].type == TokenType::FASTCALL) {
            isFC = true;
            look++;
        }
        if (tokens[look].type == TokenType::INTERRUPT) {
            isInterrupt = true;
            look++;
        }
        if (tokens[look].type == TokenType::NAKED) {
            isNaked = true;
            look++;
        }
        if (tokens[look].type == TokenType::REGPARM) {
            isRegparm = true;
            look++;
        }
        if (tokens[look].type == TokenType::INLINE) {
            isInlineFunc = true;
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

        // Skip __attribute__((...)) in lookahead
        while (look < tokens.size() && (tokens[look].type == TokenType::ATTRIBUTE || tokens[look].type == TokenType::EXTENSION)) {
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
        bool isUnsig = false;

        while (look < tokens.size() && (tokens[look].type == TokenType::VOLATILE || tokens[look].type == TokenType::CONST || tokens[look].type == TokenType::RESTRICT || tokens[look].type == TokenType::AUTO || tokens[look].type == TokenType::REGISTER || tokens[look].type == TokenType::INLINE || tokens[look].type == TokenType::FASTCALL ||
               tokens[look].type == TokenType::SIGNED || tokens[look].type == TokenType::UNSIGNED || tokens[look].type == TokenType::ATTRIBUTE || tokens[look].type == TokenType::EXTENSION)) {
            if (tokens[look].type == TokenType::ATTRIBUTE) {
                look++; // skip __attribute__
                if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                    look++; int p = 1;
                    while (look < tokens.size() && p > 0) {
                        if (tokens[look].type == TokenType::OPEN_PAREN) p++;
                        else if (tokens[look].type == TokenType::CLOSE_PAREN) p--;
                        look++;
                    }
                }
                continue;
            }
            if (tokens[look].type == TokenType::EXTENSION) {
                look++;
                continue;
            }
            if (tokens[look].type == TokenType::VOLATILE) isVol = true;
            else if (tokens[look].type == TokenType::CONST) isConst = true;
            else if (tokens[look].type == TokenType::SIGNED) {
                if (isUnsig) throw std::runtime_error("Syntax Error at " + std::to_string(tokens[look].line) + ":" + std::to_string(tokens[look].column) + ": both 'signed' and 'unsigned' in declaration");
                isSig = true;
            }
            else if (tokens[look].type == TokenType::UNSIGNED) {
                if (isSig) throw std::runtime_error("Syntax Error at " + std::to_string(tokens[look].line) + ":" + std::to_string(tokens[look].column) + ": both 'signed' and 'unsigned' in declaration");
                isUnsig = true;
            }
            look++;
        }

        if (look < tokens.size() && (tokens[look].type == TokenType::INT ||
                                     tokens[look].type == TokenType::SHORT ||
                                     tokens[look].type == TokenType::LONG ||
                                     tokens[look].type == TokenType::CHAR ||
                                     tokens[look].type == TokenType::BOOL ||
                                     tokens[look].type == TokenType::UNSIGNED ||
                                     tokens[look].type == TokenType::SIGNED ||
                                     tokens[look].type == TokenType::VOID ||
                                     tokens[look].type == TokenType::FLOAT ||
                                     tokens[look].type == TokenType::DOUBLE ||
                                     tokens[look].type == TokenType::COMPLEX ||
                                     tokens[look].type == TokenType::INT_N ||
                                     tokens[look].type == TokenType::UINT_N ||
                                     tokens[look].type == TokenType::STRUCT ||
                                     tokens[look].type == TokenType::UNION ||
                                     tokens[look].type == TokenType::ENUM ||
                                     (tokens[look].type == TokenType::IDENTIFIER && isTypedef(tokens[look].value)))) {
            if (tokens[look].type == TokenType::COMPLEX) {
                look++; // skip _Complex
                // Skip base type: float, double, int, long double, etc.
                if (look < tokens.size() && (tokens[look].type == TokenType::FLOAT || tokens[look].type == TokenType::DOUBLE ||
                    tokens[look].type == TokenType::INT || tokens[look].type == TokenType::LONG ||
                    tokens[look].type == TokenType::SHORT || tokens[look].type == TokenType::UNSIGNED ||
                    tokens[look].type == TokenType::SIGNED)) {
                    look++;
                    if (look < tokens.size() && tokens[look].type == TokenType::DOUBLE) look++;
                }
            } else if (tokens[look].type == TokenType::INT_N || tokens[look].type == TokenType::UINT_N) {
                look++; // skip __int/__uint
                // Skip (N) if present
                if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                    look++; // (
                    if (look < tokens.size()) look++; // N
                    if (look < tokens.size() && tokens[look].type == TokenType::CLOSE_PAREN) look++; // )
                }
            } else if (tokens[look].type == TokenType::UNSIGNED || tokens[look].type == TokenType::SIGNED) {
                look++;
                if (look < tokens.size() && (tokens[look].type == TokenType::INT || tokens[look].type == TokenType::SHORT || tokens[look].type == TokenType::LONG || tokens[look].type == TokenType::CHAR)) {
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
                look++; // skip int/char/void/float/double
                // Handle long long, long int, etc.
                if (look < tokens.size() && tokens[look].type == TokenType::LONG) look++;
                if (look < tokens.size() && tokens[look].type == TokenType::INT) look++;
                // Handle type followed by __complex__ (reverse order: float __complex__)
                if (look < tokens.size() && tokens[look].type == TokenType::COMPLEX) look++;
            }

            // Skip qualifiers that may appear after the type keyword (e.g., int volatile x)
            while (look < tokens.size() && (tokens[look].type == TokenType::VOLATILE || tokens[look].type == TokenType::CONST ||
                   tokens[look].type == TokenType::RESTRICT || tokens[look].type == TokenType::SIGNED || tokens[look].type == TokenType::UNSIGNED ||
                   tokens[look].type == TokenType::ATTRIBUTE || tokens[look].type == TokenType::EXTENSION)) {
                if (tokens[look].type == TokenType::ATTRIBUTE) {
                    look++;
                    if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                        look++; int p = 1;
                        while (look < tokens.size() && p > 0) {
                            if (tokens[look].type == TokenType::OPEN_PAREN) p++;
                            else if (tokens[look].type == TokenType::CLOSE_PAREN) p--;
                            look++;
                        }
                    }
                    continue;
                }
                if (tokens[look].type == TokenType::EXTENSION) {
                    look++;
                    continue;
                }
                if (tokens[look].type == TokenType::VOLATILE) isVol = true;
                else if (tokens[look].type == TokenType::CONST) isConst = true;
                else if (tokens[look].type == TokenType::SIGNED) isSig = true;
                look++;
            }

            // Skip pointers, qualifiers, and attributes interleaved
            while (look < tokens.size()) {
                if (tokens[look].type == TokenType::STAR) {
                    look++;
                } else if (tokens[look].type == TokenType::CONST || tokens[look].type == TokenType::VOLATILE ||
                           tokens[look].type == TokenType::RESTRICT || tokens[look].type == TokenType::EXTENSION) {
                    look++;
                } else if (tokens[look].type == TokenType::ATTRIBUTE) {
                    look++;
                    if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                        look++; int p = 1;
                        while (look < tokens.size() && p > 0) {
                            if (tokens[look].type == TokenType::OPEN_PAREN) p++;
                            else if (tokens[look].type == TokenType::CLOSE_PAREN) p--;
                            look++;
                        }
                    }
                } else break;
            }

            // Check for parenthesized declarator (e.g., function pointer: type (*name)(params)) — treat as variable
            if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                if (isExtern) match(TokenType::EXTERN);
                if (isStatic) match(TokenType::STATIC);
                if (isNR) match(TokenType::NORETURN);
                if (isFC) match(TokenType::FASTCALL);
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                auto decl = parseVariableDeclaration(isVol, isConst, isStatic);
                if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                    vd->isGlobal = true;
                    vd->isExtern = isExtern;
                    vd->isSigned = !isUnsig;
                } else if (auto* cs = dynamic_cast<CompoundStatement*>(decl.get())) {
                    for (auto& s : cs->statements) {
                        if (auto* vd2 = dynamic_cast<VariableDeclaration*>(s.get())) {
                            vd2->isGlobal = true;
                            vd2->isExtern = isExtern;
                            vd2->isStatic = isStatic;
                        }
                    }
                }
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
                continue;
            }

            if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER) {
                look++;
                if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                    if (isExtern) match(TokenType::EXTERN);
                    if (isStatic) match(TokenType::STATIC);
                    if (isNR) match(TokenType::NORETURN);
                    if (isFC) match(TokenType::FASTCALL);
                    if (isInterrupt) match(TokenType::INTERRUPT);
                    if (isNaked) match(TokenType::NAKED);
                    if (isRegparm) match(TokenType::REGPARM);
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) || match(TokenType::REGPARM) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                    auto decl = parseFunctionDeclaration();
                    decl->isNoreturn = isNR;
                    decl->isFastcall = isFC;
                    decl->isInterrupt = isInterrupt;
                    decl->isNaked = isNaked;
                    decl->isRegparm = isRegparm;
                    decl->isInline = isInlineFunc;
                    decl->isStatic = isStatic;
                    // extern functions are always prototypes (no body)
                    if (isExtern) decl->isPrototype = true;
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                } else {
                    if (isExtern) match(TokenType::EXTERN);
                    if (isStatic) match(TokenType::STATIC);
                    if (isNR) match(TokenType::NORETURN);
                    if (isFC) match(TokenType::FASTCALL);
                    if (isInterrupt) match(TokenType::INTERRUPT);
                    if (isNaked) match(TokenType::NAKED);
                    if (isRegparm) match(TokenType::REGPARM);
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) || match(TokenType::REGPARM) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                    auto decl = parseVariableDeclaration(isVol, isConst);
                    if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                        vd->isGlobal = true;
                        vd->isSigned = !isUnsig;
                        vd->isExtern = isExtern;
                        vd->isStatic = isStatic;
                    } else if (auto* cs = dynamic_cast<CompoundStatement*>(decl.get())) {
                        for (auto& s : cs->statements) {
                            if (auto* vd2 = dynamic_cast<VariableDeclaration*>(s.get())) {
                                vd2->isGlobal = true;
                                vd2->isSigned = isSig;
                                vd2->isExtern = isExtern;
                                vd2->isStatic = isStatic;
                            }
                        }
                    }
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                }
            } else {
                if (isNR) match(TokenType::NORETURN);
                if (isFC) match(TokenType::FASTCALL);
                if (isInterrupt) match(TokenType::INTERRUPT);
                if (isNaked) match(TokenType::NAKED);
                    if (isRegparm) match(TokenType::REGPARM);
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                auto decl = parseFunctionDeclaration();
                decl->isNoreturn = isNR;
                decl->isFastcall = isFC;
                decl->isInterrupt = isInterrupt;
                decl->isNaked = isNaked;
                    decl->isRegparm = isRegparm;
                    decl->isInline = isInlineFunc;
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
            }
        } else {
            // Implicit int: if we have qualifiers but no type keyword, and next
            // is IDENTIFIER followed by ; or = or , or [ → treat as "int" declaration
            if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER &&
                !isTypedef(tokens[look].value) &&
                look + 1 < tokens.size() &&
                (tokens[look+1].type == TokenType::SEMICOLON ||
                 tokens[look+1].type == TokenType::EQUALS ||
                 tokens[look+1].type == TokenType::COMMA ||
                 tokens[look+1].type == TokenType::OPEN_SQUARE ||
                 tokens[look+1].type == TokenType::OPEN_PAREN)) { // implicit int function
                // Check if it's a function (IDENTIFIER followed by '(') or variable
                if (tokens[look+1].type == TokenType::OPEN_PAREN) {
                    // Implicit int function: static inline f(...) → static inline int f(...)
                    if (isExtern) match(TokenType::EXTERN);
                    if (isStatic) match(TokenType::STATIC);
                    if (isNR) match(TokenType::NORETURN);
                    if (isFC) match(TokenType::FASTCALL);
                    if (isInterrupt) match(TokenType::INTERRUPT);
                    if (isNaked) match(TokenType::NAKED);
                    if (isRegparm) match(TokenType::REGPARM);
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                    auto decl = parseFunctionDeclaration();
                    decl->isStatic = isStatic;
                    decl->isInline = isInlineFunc;
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                    continue;
                }
                if (isExtern) match(TokenType::EXTERN);
                if (isStatic) match(TokenType::STATIC);
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                auto decl = parseVariableDeclaration(isVol, isConst, isStatic);
                if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                    vd->isGlobal = true;
                    vd->isExtern = isExtern;
                    vd->isSigned = !isUnsig; // implicit int: signed unless 'unsigned' was explicit
                }
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
                continue;
            }
            if (isExtern) match(TokenType::EXTERN);
            if (isStatic) match(TokenType::STATIC);
            if (isNR) match(TokenType::NORETURN);
            if (isFC) match(TokenType::FASTCALL);
            if (isInterrupt) match(TokenType::INTERRUPT);
            if (isNaked) match(TokenType::NAKED);
            if (isRegparm) match(TokenType::REGPARM);
            if (isInlineFunc) match(TokenType::INLINE);
            while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
            auto decl = parseFunctionDeclaration();
            decl->isNoreturn = isNR;
            decl->isFastcall = isFC;
            decl->isInterrupt = isInterrupt;
            decl->isNaked = isNaked;
            decl->isRegparm = isRegparm;
            decl->isInline = isInlineFunc;
            decl->isStatic = isStatic;
            flushPending(*unit);
            unit->topLevelDecls.push_back(std::unique_ptr<Statement>(std::move(decl)));
        }
    }
    return unit;
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    const Token& startToken = peek();
    bool isNR = match(TokenType::NORETURN);
    bool isFC = match(TokenType::FASTCALL);
    bool isInterrupt = match(TokenType::INTERRUPT);
    bool isNaked = match(TokenType::NAKED);
    bool isRegparm = match(TokenType::REGPARM);
    bool isInlineFunc = match(TokenType::INLINE);
    bool isStatic = match(TokenType::STATIC);
    std::string returnType;
    bool isSigned = false;
    int basePtrLevel = 0;

    // Skip any extra qualifiers that might precede the return type (e.g., inline static before type)
    while (match(TokenType::INLINE) || match(TokenType::STATIC)) {
        if (tokens[pos-1].type == TokenType::INLINE) isInlineFunc = true;
        if (tokens[pos-1].type == TokenType::STATIC) isStatic = true;
    }
    {
        auto ts = parseTypeSpecifier();
        if (ts.valid) {
            returnType = ts.name;
            isSigned = ts.isSigned;
            basePtrLevel = ts.pointerLevel;
        } else if (peek().type == TokenType::IDENTIFIER) {
            // C89 implicit int: function declared without return type defaults to int
            returnType = "int";
        } else {
            std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
            throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": Expected return type (int, char, void, struct, union) for function declaration. Found '" + foundStr + "' instead.");
        }
    }
    // Skip qualifiers after the type keyword (e.g., int volatile func())
    while (match(TokenType::INLINE) || match(TokenType::STATIC)) {}

    int returnPtrLevel = basePtrLevel;
    while (true) {
        if (tryParseAttribute() || match(TokenType::EXTENSION)) {
            continue;
        }
        if (match(TokenType::STAR)) {
            returnPtrLevel++;
            while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
                match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT);
            }
            continue;
        }
        break;
    }

    std::string name;
    if (peek().type == TokenType::IDENTIFIER && peek().value == "operator") {
        advance(); // consume 'operator'
        // Consume operator symbol(s) to form the mangled name
        const Token& opTok = advance();
        std::string opName = opTok.value;
        // Handle two-character operators: ++, --, +=, ==, !=, <=, >=, <<=, >>=, etc.
        if (peek().type != TokenType::OPEN_PAREN) {
            opName += advance().value;
        }
        // Map operator symbol to safe identifier suffix
        static const std::map<std::string, std::string> opMap = {
            {"+", "add"}, {"-", "sub"}, {"*", "mul"}, {"/", "div"}, {"%", "mod"},
            {"==", "eq"}, {"!=", "ne"}, {"<", "lt"}, {">", "gt"}, {"<=", "le"}, {">=", "ge"},
            {"<<", "shl"}, {">>", "shr"}, {"&", "band"}, {"|", "bor"}, {"^", "bxor"},
            {"~", "bnot"}, {"!", "lnot"}, {"++", "inc"}, {"--", "dec"},
            {"+=", "add_assign"}, {"-=", "sub_assign"}, {"*=", "mul_assign"},
            {"/=", "div_assign"}, {"%=", "mod_assign"}, {"&=", "band_assign"},
            {"|=", "bor_assign"}, {"^=", "bxor_assign"}, {"<<=", "shl_assign"}, {">>=", "shr_assign"},
        };
        // Defer final name until we know param count (unary vs binary)
        auto it = opMap.find(opName);
        std::string opSuffix = it != opMap.end() ? it->second : opName;
        // Check if it's a unary operator (no params before ')')
        if (opName == "-" || opName == "+" || opName == "*" || opName == "&") {
            // These can be both unary and binary — peek at params
            // If next tokens are '(' ')' or '(' 'void' ')', it's unary
            size_t peekPos = pos; // pos is at '('
            if (peekPos + 1 < tokens.size() && tokens[peekPos].type == TokenType::OPEN_PAREN &&
                tokens[peekPos + 1].type == TokenType::CLOSE_PAREN) {
                // No params → unary
                static const std::map<std::string, std::string> unaryMap = {
                    {"-", "neg"}, {"+", "pos"}, {"*", "deref"}, {"&", "addr"},
                };
                auto uit = unaryMap.find(opName);
                if (uit != unaryMap.end()) opSuffix = uit->second;
            }
        }
        name = "operator_" + opSuffix;
    } else {
        name = expect(TokenType::IDENTIFIER, "Expected function name").value;
    }

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
            std::string pType;
            bool pIsSigned = false;
            int pBasePtrLevel = 0;

            {
                auto ts = parseTypeSpecifier();
                if (ts.valid) {
                    pType = ts.name;
                    pIsSigned = ts.isSigned;
                    pBasePtrLevel = ts.pointerLevel;
                    if (ts.isVolatile) pIsVolatile = true;
                    if (ts.isConst) pIsConst = true;
                    if (!ts.arrayDims.empty()) {
                        pBasePtrLevel++; // array dims decay to pointer
                    }
                    if (ts.isFunctionPointer) {
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
                        p.funcPtrSig = ts.funcPtrSig;
                        params.push_back(p);
                        continue;
                    }
                }
                else if (peek().type == TokenType::IDENTIFIER) {
                    // K&R style: parameter is just a name, type declared after ')'
                    std::string pName = advance().value;
                    params.push_back({"int", 0, false, pName, pIsVolatile, pIsConst, false, false, nullptr});
                    continue; // skip the pointer/name parsing below
                }
                else {
                    std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
                    throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": Expected parameter type (int, char, struct, union). Found '" + foundStr + "' instead.");
                }
            }

            int pPtrLevel = pBasePtrLevel;

            // Check for function pointer parameter: type (*name)(params)
            if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::STAR) {
                advance(); // consume '('
                advance(); // consume '*'
                std::string pName;
                if (peek().type == TokenType::IDENTIFIER) {
                    pName = advance().value;
                } else {
                    pName = "__unnamed_fp_" + std::to_string(pos);
                }
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

            std::string pName;
            if (peek().type == TokenType::IDENTIFIER) {
                pName = advance().value;
            } else {
                // Unnamed parameter (valid in prototypes): void foo(int, char *)
                pName = "__unnamed_" + std::to_string(pos);
            }

            // Skip __attribute__ after parameter name
            while (tryParseAttribute()) {}

            // Array parameters: int a[], int a[N], int a[][M], int a[expr]
            // These decay to pointers per C semantics
            if (peek().type == TokenType::OPEN_SQUARE) {
                pPtrLevel++; // first [] decays to pointer
                advance(); // consume '['
                // Skip the dimension expression (may be empty for [])
                if (peek().type != TokenType::CLOSE_SQUARE) {
                    int depth = 1;
                    while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                        if (peek().type == TokenType::OPEN_SQUARE) depth++;
                        else if (peek().type == TokenType::CLOSE_SQUARE) { depth--; if (depth > 0) advance(); else break; }
                        else advance();
                    }
                }
                expect(TokenType::CLOSE_SQUARE, "Expected ']' in array parameter");
                // Additional dimensions: int a[][M], int a[2][3]
                while (peek().type == TokenType::OPEN_SQUARE) {
                    advance(); // consume '['
                    if (peek().type != TokenType::CLOSE_SQUARE) {
                        int depth2 = 1;
                        while (depth2 > 0 && peek().type != TokenType::END_OF_FILE) {
                            if (peek().type == TokenType::OPEN_SQUARE) depth2++;
                            else if (peek().type == TokenType::CLOSE_SQUARE) { depth2--; if (depth2 > 0) advance(); else break; }
                            else advance();
                        }
                    }
                    expect(TokenType::CLOSE_SQUARE, "Expected ']' in array parameter");
                }
            }

            // Skip __attribute__ after array params
            while (tryParseAttribute()) {}

            params.push_back({pType, pPtrLevel, pIsSigned, pName, pIsVolatile, pIsConst, pIsPointerConst, false, nullptr});
        } while (match(TokenType::COMMA) && peek().type != TokenType::ELLIPSIS);
    }
    bool isVariadic = match(TokenType::ELLIPSIS);
    expect(TokenType::CLOSE_PAREN, "Expected ')'");

    // --- K&R parameter type declarations after ')' ---
    // Pattern: int foo(a, b) int a; char *b; { ... }
    // If next token is a type keyword (not '{' or ';'), parse K&R declarations
    while (isTypeStartToken() || peek().type == TokenType::REGISTER || peek().type == TokenType::AUTO) {
        // Parse type specifier
        std::string krType;
        bool krSigned = false;
        bool krIsConst = false, krIsVol = false;
        // Consume storage class specifiers that parseTypeSpecifier doesn't handle
        while (match(TokenType::REGISTER) || match(TokenType::AUTO)) {}
        {
            // Save pos before parseTypeSpecifier so we can rewind past any consumed stars
            // (K&R declarations have per-declarator pointer levels: int *a, *b;)
            size_t preStarPos = pos;
            auto ts = parseTypeSpecifier();
            if (!ts.valid) break;
            krType = ts.name;
            krSigned = ts.isSigned;
            krIsConst = ts.isConst;
            krIsVol = ts.isVolatile;
            // Rewind past stars that parseTypeSpecifier consumed — stars belong to declarators in K&R
            if (ts.pointerLevel > 0) {
                // Walk backwards from current pos to find where stars began
                size_t p = pos;
                int starsToRewind = ts.pointerLevel;
                while (starsToRewind > 0 && p > preStarPos) {
                    p--;
                    if (tokens[p].type == TokenType::STAR) starsToRewind--;
                    else if (tokens[p].type == TokenType::CONST || tokens[p].type == TokenType::VOLATILE || tokens[p].type == TokenType::RESTRICT) continue;
                    else { p++; break; }
                }
                pos = p;
            }
        }

        // Parse declarator(s) for this type: int a; or int a, b;
        do {
            int krPtr = 0;
            while (match(TokenType::STAR)) krPtr++;
            std::string krName = expect(TokenType::IDENTIFIER, "Expected parameter name in K&R declaration").value;
            // Update matching parameter
            for (auto& p : params) {
                if (p.name == krName) {
                    p.type = krType;
                    p.pointerLevel = krPtr;
                    p.isSigned = krSigned;
                    p.isConst = krIsConst;
                    p.isVolatile = krIsVol;
                    break;
                }
            }
        } while (match(TokenType::COMMA));
        expect(TokenType::SEMICOLON, "Expected ';' after K&R parameter declaration");
    }

    // Skip any __attribute__((...)) after parameter list / K&R declarations
    while (tryParseAttribute()) {}

    // Check for prototype
    bool isInline = isInlineFunc;
    if (match(TokenType::SEMICOLON)) {
        auto func = setPos(std::make_unique<FunctionDeclaration>(name, returnType), startToken);
        func->returnPointerLevel = returnPtrLevel;
        func->isSigned = isSigned;
        func->parameters = std::move(params);
        func->isNoreturn = isNR;
        func->isFastcall = isFC;
        func->isInterrupt = isInterrupt;
        func->isNaked = isNaked;
        func->isRegparm = isRegparm;
        func->isPrototype = true;
        func->isVariadic = isVariadic;
        return func;
    }

    std::string fullName = name;
    if (currentFunction) {
        fullName = currentFunctionName + "_" + name;
        nestedFunctionMap[name] = fullName;
    }

    auto func = setPos(std::make_unique<FunctionDeclaration>(fullName, returnType), startToken);
    func->returnPointerLevel = returnPtrLevel;
    func->isSigned = isSigned;
    func->parameters = std::move(params);
    func->isNoreturn = isNR;
    func->isFastcall = isFC;
    func->isInterrupt = isInterrupt;
    func->isNaked = isNaked;
    func->isRegparm = isRegparm;
    func->isVariadic = isVariadic;
    func->isInline = isInline;
    func->isStatic = isStatic;

    if (currentFunction) {
        func->isNested = true;
        func->parentFunc = currentFunction;
    }

    std::string savedFunctionName = currentFunctionName;
    FunctionDeclaration* savedFunction = currentFunction;
    auto savedNestedMap = nestedFunctionMap;
    
    // 'final' keyword on methods: int speak() final { ... }
    if (peek().type == TokenType::IDENTIFIER && peek().value == "final") {
        advance();
        func->isFinal = true;
    }

    currentFunctionName = fullName;
    currentFunction = func.get();

    func->body = parseCompoundStatement();

    currentFunctionName = savedFunctionName;
    currentFunction = savedFunction;
    nestedFunctionMap = savedNestedMap;
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
    // Skip __attribute__ at statement level (e.g., before nested functions)
    while (tryParseAttribute()) {}

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

    // GCC __label__ local label declaration: __label__ lbl1, lbl2, ...;
    if (peek().type == TokenType::IDENTIFIER && peek().value == "__label__") {
        const Token& startToken = advance();
        do {
            expect(TokenType::IDENTIFIER, "Expected label name after __label__");
        } while (match(TokenType::COMMA));
        expect(TokenType::SEMICOLON, "Expected ';' after __label__ declaration");
        return setPos(std::make_unique<CompoundStatement>(), startToken); // no-op
    }

    if (match(TokenType::GOTO)) {
        const Token& startToken = tokens[pos-1];
        if (match(TokenType::STAR)) {
            auto target = parseExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after computed goto");
            return setPos(std::make_unique<GotoStatement>("", std::move(target)), startToken);
        }
        std::string label = expect(TokenType::IDENTIFIER, "Expected label name after goto").value;
        expect(TokenType::SEMICOLON, "Expected ';' after goto");
        return setPos(std::make_unique<GotoStatement>(label), startToken);
    }

    bool isVolatile = false;
    bool isConst = false;
    bool isAuto = false;
    bool isStatic = false;
    bool isRegister = false;
    bool isInline = false;
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
            isInline = true;
        } else if (match(TokenType::FASTCALL)) {
            // consumed; handled at function declaration level
        } else {
            break;
        }
    }

    if (peek().type == TokenType::STRUCT || peek().type == TokenType::UNION || peek().type == TokenType::ENUM) {
        // Check for nested function with struct/enum return type: struct A foo() { ... }
        if (isFunctionDeclaration()) {
            return parseFunctionDeclaration();
        }
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
            while (tryParseAttribute()) {} // skip __attribute__ after struct def
            if (peek().type == TokenType::SEMICOLON) {
                advance();
                return def;
            }
            // struct Name { ... } var[N] = {...}; — inline definition + variable in local scope
            std::string sType = (isUnion ? "union " : "struct ") + def->name;
            pendingDefinitions.push_back(std::move(def));
            while (tryParseAttribute()) {}
            int ptrLevel = 0;
            while (match(TokenType::STAR)) ptrLevel++;
            std::string varName = expect(TokenType::IDENTIFIER, "Expected variable name after struct definition").value;
            auto vDecl = std::make_unique<VariableDeclaration>(sType, varName, ptrLevel);
            vDecl->isVolatile = isVolatile;
            vDecl->isConst = isConst;
            vDecl->isStatic = isStatic;
            // Array dimensions
            while (match(TokenType::OPEN_SQUARE)) {
                if (match(TokenType::CLOSE_SQUARE)) {
                    vDecl->arrayDims.push_back(0);
                } else {
                    auto szExpr = parseExpression();
                    int sz = 1;
                    if (auto* lit = dynamic_cast<IntegerLiteral*>(szExpr.get())) sz = (int)lit->value;
                    vDecl->arrayDims.push_back(sz);
                    expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                }
            }
            // Skip __attribute__ after variable name
            while (tryParseAttribute()) {}
            // Initializer for first variable
            if (match(TokenType::EQUALS)) {
                if (peek().type == TokenType::OPEN_BRACE) vDecl->initializer = parseInitializerList();
                else vDecl->initializer = parseExpression();
            }
            // Multi-variable: struct S { } a, b;
            std::vector<std::unique_ptr<Statement>> extraVars;
            while (match(TokenType::COMMA)) {
                while (tryParseAttribute()) {}
                int ep = 0;
                while (match(TokenType::STAR)) ep++;
                std::string en = expect(TokenType::IDENTIFIER, "Expected variable name").value;
                auto ev = std::make_unique<VariableDeclaration>(sType, en, ep);
                ev->isVolatile = isVolatile;
                ev->isConst = isConst;
                ev->isStatic = isStatic;
                while (match(TokenType::OPEN_SQUARE)) {
                    if (match(TokenType::CLOSE_SQUARE)) ev->arrayDims.push_back(0);
                    else {
                        auto se = parseExpression();
                        int ss = 1;
                        if (auto* lit = dynamic_cast<IntegerLiteral*>(se.get())) ss = (int)lit->value;
                        ev->arrayDims.push_back(ss);
                        expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                    }
                }
                if (match(TokenType::EQUALS)) {
                    if (peek().type == TokenType::OPEN_BRACE) ev->initializer = parseInitializerList();
                    else ev->initializer = parseExpression();
                }
                extraVars.push_back(std::move(ev));
            }
            expect(TokenType::SEMICOLON, "Expected ';'");
            if (extraVars.empty()) {
                return vDecl;
            }
            auto compound = std::make_unique<CompoundStatement>();
            compound->statements.push_back(std::move(vDecl));
            for (auto& ev : extraVars) compound->statements.push_back(std::move(ev));
            return compound;
        }
        return parseVariableDeclaration(isVolatile, isConst, isStatic, isRegister);
    }

    if (peek().type == TokenType::ALIGNAS || peek().type == TokenType::INT || peek().type == TokenType::SHORT || peek().type == TokenType::LONG || peek().type == TokenType::CHAR || peek().type == TokenType::BOOL ||
        peek().type == TokenType::VOID || peek().type == TokenType::TYPEOF ||
        peek().type == TokenType::FLOAT || peek().type == TokenType::DOUBLE || peek().type == TokenType::COMPLEX ||
        peek().type == TokenType::INT_N || peek().type == TokenType::UINT_N ||
        peek().type == TokenType::UNSIGNED || peek().type == TokenType::SIGNED ||
        (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value))) {
        if (isFunctionDeclaration()) {
            return parseFunctionDeclaration();
        }
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
            if (peek().type == TokenType::INT || peek().type == TokenType::SHORT || peek().type == TokenType::LONG || peek().type == TokenType::CHAR ||
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
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL)) {
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

    if (match(TokenType::REPEAT)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::OPEN_PAREN, "Expected '(' after 'repeat'");
        // Check for optional variable declaration: repeat(type var, count)
        // vs simple: repeat(count)
        std::string varType, varName;
        bool varSigned = false;
        int count = 0;
        // Lookahead: if next tokens are type + identifier + comma, it's the var form
        bool hasVar = false;
        {
            size_t saved = pos;
            if (peek().type == TokenType::UNSIGNED || peek().type == TokenType::SIGNED ||
                peek().type == TokenType::CHAR || peek().type == TokenType::INT ||
                peek().type == TokenType::SHORT || peek().type == TokenType::LONG) {
                // Could be a type — check if followed by identifier then comma
                size_t look = pos;
                if (tokens[look].type == TokenType::UNSIGNED || tokens[look].type == TokenType::SIGNED) look++;
                if (tokens[look].type == TokenType::CHAR || tokens[look].type == TokenType::INT ||
                    tokens[look].type == TokenType::SHORT || tokens[look].type == TokenType::LONG) look++;
                if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER &&
                    look + 1 < tokens.size() && tokens[look + 1].type == TokenType::COMMA) {
                    hasVar = true;
                }
            }
            pos = saved;
        }
        if (hasVar) {
            auto ts = parseTypeSpecifier();
            if (ts.valid) {
                varType = ts.name;
                varSigned = ts.isSigned;
            } else {
                varType = "int";
            }
            varName = expect(TokenType::IDENTIFIER, "Expected variable name in repeat").value;
            expect(TokenType::COMMA, "Expected ',' after repeat variable");
        }
        auto countExpr = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')' after repeat count");
        // Count must be a constant integer literal
        if (auto* lit = dynamic_cast<IntegerLiteral*>(countExpr.get())) {
            count = (int)lit->value;
        } else {
            throw std::runtime_error("repeat count must be a compile-time constant");
        }
        if (count < 0 || count > 1024) {
            throw std::runtime_error("repeat count must be between 0 and 1024");
        }
        auto body = parseStatement();
        auto rep = setPos(std::make_unique<RepeatStatement>(count, std::move(body)), startToken);
        rep->varName = varName;
        rep->varType = varType;
        rep->varSigned = varSigned;
        return rep;
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
        std::unique_ptr<Expression> rangeEnd;
        if (match(TokenType::ELLIPSIS)) {
            rangeEnd = parseExpression();
        }
        expect(TokenType::COLON, "Expected ':' after case value");
        return setPos(std::make_unique<CaseStatement>(std::move(value), std::move(rangeEnd)), startToken);
    }

    if (match(TokenType::DEFAULT)) {
        const Token& startToken = tokens[pos-1];
        expect(TokenType::COLON, "Expected ':' after default");
        return setPos(std::make_unique<DefaultStatement>(), startToken);
    }

    if (match(TokenType::ASM)) {
        const Token& startToken = tokens[pos-1];
        // Skip optional volatile/goto qualifiers
        match(TokenType::VOLATILE);
        if (peek().type == TokenType::IDENTIFIER && peek().value == "goto") advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after asm");
        std::string code = expect(TokenType::STRING_LITERAL, "Expected string literal for asm code").value;
        // Extended asm: "template" : outputs : inputs : clobbers
        // Parse and discard constraint sections (template is still emitted)
        for (int section = 0; section < 3 && match(TokenType::COLON); section++) {
            // Each section is comma-separated: "constraint" (expr) or just "string"
            if (peek().type == TokenType::CLOSE_PAREN || peek().type == TokenType::COLON)
                continue; // empty section
            do {
                // Skip optional constraint name [name]
                if (peek().type == TokenType::OPEN_SQUARE) {
                    advance();
                    while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE)
                        advance();
                    match(TokenType::CLOSE_SQUARE);
                }
                if (peek().type == TokenType::STRING_LITERAL) {
                    advance(); // constraint string
                    if (match(TokenType::OPEN_PAREN)) {
                        // Parse and discard the operand expression
                        int depth = 1;
                        while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                            if (match(TokenType::OPEN_PAREN)) depth++;
                            else if (peek().type == TokenType::CLOSE_PAREN) { depth--; if (depth > 0) advance(); }
                            else advance();
                        }
                        match(TokenType::CLOSE_PAREN);
                    }
                } else {
                    break; // not a constraint
                }
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after asm");
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
    // Handle comma operator in expression statements: i++, x--;
    while (match(TokenType::COMMA)) {
        const Token& commaToken = tokens[pos-1];
        auto right = parseExpression();
        expr = setPos(std::make_unique<BinaryOperation>(",", std::move(expr), std::move(right)), commaToken);
    }
    expect(TokenType::SEMICOLON, "Expected ';'");
    return setPos(std::make_unique<ExpressionStatement>(std::move(expr)), startToken);
}

std::unique_ptr<Statement> Parser::parseVariableDeclaration(bool isVolatile, bool isConst, bool isStatic, bool isRegister) {
    std::unique_ptr<Expression> alignmentExpr = nullptr;
    if (match(TokenType::ALIGNAS)) {
        expect(TokenType::OPEN_PAREN, "Expected '(' after '_Alignas'");
        if (isTypeStartToken()) {
            auto ts = parseTypeSpecifier();
            alignmentExpr = std::make_unique<AlignofExpression>(ts.name, ts.pointerLevel);
        } else {
            alignmentExpr = parseExpression();
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after alignment expression");
    }

    const Token& typeToken = peek();
    std::string type;
    bool isSigned = false;
    int basePtrLevel = 0;
    std::vector<int> typedefArrayDims;
    if (match(TokenType::TYPEOF)) {
        // typeof(expr) or typeof(type) — resolve to type name
        expect(TokenType::OPEN_PAREN, "Expected '(' after typeof");
        if (isTypeStartToken()) {
            auto ts = parseTypeSpecifier();
            type = ts.name;
            isSigned = ts.isSigned;
            // skip pointer levels in typeof (already consumed by parseTypeSpecifier)
        } else {
            // Parse as expression, infer type (default to int)
            auto expr = parseExpression();
            type = "int"; // conservative: most expressions are int-width
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after typeof");
    } else {
        // Save pos so we can rewind past any stars that parseTypeSpecifier consumes
        // (stars and pointer-const are handled separately below for variable declarations)
        size_t preTypePos = pos;
        auto ts = parseTypeSpecifier();
        if (ts.valid) {
            type = ts.name;
            isSigned = ts.isSigned;
            typedefArrayDims = ts.arrayDims;
            // Rewind past any stars (and their trailing qualifiers) consumed by
            // parseTypeSpecifier, so the existing star/pointer-const handling code
            // below can re-process them correctly. This is needed because
            // parseTypeSpecifier lumps post-star const into ts.isConst, but
            // parseVariableDeclaration needs to distinguish isConst (type qualifier)
            // from isPointerConst (pointer qualifier).
            {
                size_t p = pos;
                bool foundStar = false;
                while (p > preTypePos) {
                    TokenType tt = tokens[p - 1].type;
                    if (tt == TokenType::STAR || tt == TokenType::CONST ||
                        tt == TokenType::VOLATILE || tt == TokenType::RESTRICT) {
                        if (tt == TokenType::STAR) foundStar = true;
                        p--;
                    } else {
                        break;
                    }
                }
                if (foundStar) {
                    pos = p;
                    // ts.isConst/isVolatile may include post-star qualifiers that
                    // we rewound. Only apply leading/trailing qualifiers that were
                    // consumed BEFORE the stars (those are in the token range
                    // preTypePos..p and already consumed; we re-scan to check).
                    // For simplicity: set isConst/isVolatile only if they were
                    // explicitly set on the type itself (not from stars).
                    // Re-check: was const/volatile present before the star region?
                    for (size_t i = preTypePos; i < p; i++) {
                        if (tokens[i].type == TokenType::CONST) isConst = true;
                        if (tokens[i].type == TokenType::VOLATILE) isVolatile = true;
                    }
                    basePtrLevel = 0; // stars will be re-counted below
                } else {
                    // No stars consumed; safe to apply qualifiers directly
                    if (ts.isConst) isConst = true;
                    if (ts.isVolatile) isVolatile = true;
                    basePtrLevel = ts.pointerLevel; // from typedef only
                }
            }
            if (ts.isFunctionPointer) {
                // Typedef is a function pointer type — create decl directly
                std::string fpName = expect(TokenType::IDENTIFIER, "Expected variable name").value;
                auto decl = setPos(std::make_unique<VariableDeclaration>("void", fpName, 1), typeToken);
                decl->isSigned = false;
                decl->isVolatile = isVolatile;
                decl->isConst = isConst;
                decl->isStatic = isStatic;
                decl->isRegister = isRegister;
                decl->isFunctionPointer = true;
                decl->funcPtrSig = ts.funcPtrSig;
                decl->alignmentExpr = std::move(alignmentExpr);
                // Handle array of function pointers: frob f[] = {abort};
                while (match(TokenType::OPEN_SQUARE)) {
                    if (match(TokenType::CLOSE_SQUARE)) {
                        decl->arrayDims.push_back(0);
                    } else {
                        auto szExpr = parseExpression();
                        int sz = 1;
                        if (auto* lit = dynamic_cast<IntegerLiteral*>(szExpr.get())) sz = (int)lit->value;
                        decl->arrayDims.push_back(sz);
                        expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                    }
                }
                if (match(TokenType::EQUALS)) {
                    if (peek().type == TokenType::OPEN_BRACE)
                        decl->initializer = parseInitializerList();
                    else
                        decl->initializer = parseExpression();
                }
                expect(TokenType::SEMICOLON, "Expected ';'");
                return decl;
            }
        } else if (peek().type == TokenType::IDENTIFIER && !isTypedef(peek().value)) {
            // Implicit int: "static max;", "unsigned d;" (where unsigned was consumed as qualifier)
            type = "int";
        } else {
            throw std::runtime_error("Syntax Error at " + std::to_string(peek().line) + ":" + std::to_string(peek().column) + ": Expected type for variable declaration. Found '" + peek().typeToString() + "' instead.");
        }
    }

    int ptrLevel = basePtrLevel;

    // Consume any qualifiers that appear after the type keyword (e.g., int volatile x)
    while (peek().type == TokenType::VOLATILE || peek().type == TokenType::CONST ||
           peek().type == TokenType::RESTRICT || peek().type == TokenType::SIGNED ||
           peek().type == TokenType::UNSIGNED || peek().type == TokenType::ATTRIBUTE ||
           peek().type == TokenType::EXTENSION) {
        if (tryParseAttribute() || match(TokenType::EXTENSION)) continue;
        if (match(TokenType::VOLATILE)) isVolatile = true;
        else if (match(TokenType::CONST)) isConst = true;
        else if (match(TokenType::SIGNED)) isSigned = true;
        else if (match(TokenType::UNSIGNED)) { /* already unsigned by type match */ }
        else match(TokenType::RESTRICT);
    }

    bool isPointerConst = false;
    while (true) {
        if (tryParseAttribute() || match(TokenType::EXTENSION)) {
            continue;
        }
        if (match(TokenType::STAR)) {
            ptrLevel++;
            while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
                if (match(TokenType::CONST)) isPointerConst = true;
                else if (!match(TokenType::VOLATILE)) match(TokenType::RESTRICT);
            }
            continue;
        }
        break;
    }

    // Check for function pointer declaration: type (*name)(params) or type (*name[size])(params)
    if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::STAR) {
        advance(); // consume '('
        advance(); // consume '*'
        while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT)) {}
        std::string fpName = expect(TokenType::IDENTIFIER, "Expected function pointer name").value;
        std::vector<int> fpArrayDims;
        while (match(TokenType::OPEN_SQUARE)) {
            if (peek().type == TokenType::CLOSE_SQUARE) {
                fpArrayDims.push_back(0);
                advance(); // consume ']'
            } else {
                auto szExpr = parseExpression();
                int arrSize = 1;
                if (auto* lit = dynamic_cast<IntegerLiteral*>(szExpr.get())) arrSize = (int)lit->value;
                fpArrayDims.push_back(arrSize);
                expect(TokenType::CLOSE_SQUARE, "Expected ']'");
            }
        }
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
        decl->arrayDims = fpArrayDims;
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

    std::string name = expect(TokenType::IDENTIFIER, "Expected variable name").value;
    std::vector<int> arrayDims;
    bool unsizedArray = false;
    while (match(TokenType::OPEN_SQUARE)) {
        if (peek().type == TokenType::CLOSE_SQUARE) {
            // Empty brackets: size inferred from initializer
            arrayDims.push_back(0);
            unsizedArray = true;
        } else {
            auto sizeExpr = parseExpression();
            // Evaluate constant array size: integer literals, sizeof, constant expressions
            int arrSize = 0;
            if (auto* lit = dynamic_cast<IntegerLiteral*>(sizeExpr.get())) arrSize = (int)lit->value;
            else if (auto* szExpr = dynamic_cast<SizeofExpression*>(sizeExpr.get())) {
                if (szExpr->typeName == "int" || szExpr->typeName == "short") arrSize = 2;
                else if (szExpr->typeName == "long") arrSize = 4;
                else if (szExpr->typeName == "char") arrSize = 1;
                else arrSize = 2; // default
                for (int _p = 0; _p < szExpr->pointerLevel; _p++) arrSize = 2;
            }
            else arrSize = 1; // fallback for non-constant expressions
            arrayDims.push_back(arrSize);
        }
        expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
    }
    // Append typedef array dimensions
    arrayDims.insert(arrayDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
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

    // Infer array size from initializer for unsized arrays (e.g., char x[] = "str")
    if (unsizedArray && decl->initializer && !decl->arrayDims.empty() && decl->arrayDims[0] == 0) {
        if (auto* strLit = dynamic_cast<StringLiteral*>(decl->initializer.get())) {
            decl->arrayDims[0] = (int)strLit->value.length() + 1; // +1 for NUL
        } else if (auto* initList = dynamic_cast<InitializerList*>(decl->initializer.get())) {
            decl->arrayDims[0] = (int)initList->elements.size();
        }
    }

    // Skip __attribute__ after variable declaration: int x __attribute__((unused));
    while (tryParseAttribute()) {}

    // --- Multi-variable declarations: int a, b = 3, *c, d[4]; ---
    if (peek().type == TokenType::COMMA) {
        auto compound = setPos(std::make_unique<CompoundStatement>(), typeToken);
        compound->statements.push_back(std::move(decl));

        while (match(TokenType::COMMA)) {
            // Parse additional declarators reusing the same base type
            int extraPtr = basePtrLevel;
            while (match(TokenType::STAR)) extraPtr++;
            // Handle const/volatile after * for additional declarators
            bool extraPtrConst = false;
            while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
                if (match(TokenType::CONST)) extraPtrConst = true;
                else if (!match(TokenType::VOLATILE)) match(TokenType::RESTRICT);
            }
            std::string extraName = expect(TokenType::IDENTIFIER, "Expected variable name").value;
            std::vector<int> extraDims;
            bool extraUnsized = false;
            while (match(TokenType::OPEN_SQUARE)) {
                if (peek().type == TokenType::CLOSE_SQUARE) {
                    extraDims.push_back(0);
                    extraUnsized = true;
                } else {
                    auto stExpr = parseExpression();
                    int estSize = 0;
                    if (auto* lit = dynamic_cast<IntegerLiteral*>(stExpr.get())) estSize = (int)lit->value;
                    else estSize = 1;
                    extraDims.push_back(estSize);
                }
                expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
            }
            // Append typedef array dimensions
            extraDims.insert(extraDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
            auto extraDecl = setPos(std::make_unique<VariableDeclaration>(type, extraName, extraPtr), typeToken);
            extraDecl->isSigned = isSigned;
            extraDecl->isVolatile = isVolatile;
            extraDecl->isConst = isConst;
            extraDecl->isStatic = isStatic;
            extraDecl->isRegister = isRegister;
            extraDecl->isPointerConst = extraPtrConst;
            extraDecl->arrayDims = extraDims;
            if (match(TokenType::EQUALS)) {
                if (peek().type == TokenType::OPEN_BRACE) {
                    extraDecl->initializer = parseInitializerList();
                } else {
                    extraDecl->initializer = parseExpression();
                }
            }
            if (extraUnsized && extraDecl->initializer && !extraDecl->arrayDims.empty() && extraDecl->arrayDims[0] == 0) {
                if (auto* sl = dynamic_cast<StringLiteral*>(extraDecl->initializer.get()))
                    extraDecl->arrayDims[0] = (int)sl->value.length() + 1;
                else if (auto* il = dynamic_cast<InitializerList*>(extraDecl->initializer.get()))
                    extraDecl->arrayDims[0] = (int)il->elements.size();
            }
            compound->statements.push_back(std::move(extraDecl));
        }

        expect(TokenType::SEMICOLON, "Expected ';'");
        return compound;
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
    bool isUnpacked = match(TokenType::UNPACKED);
    // Skip __attribute__ after struct/union keyword: struct __attribute__((packed)) S { }
    while (tryParseAttribute()) {}
    std::string name;
    if (peek().type == TokenType::IDENTIFIER) {
        name = advance().value;
    } else {
        name = (isUnion ? "<anon_union_" : "<anon_struct_") + std::to_string(anonymousAggregateCount++) + ">";
    }

    // 'final' keyword: struct Leaf final : Component { } or struct Leaf final { }
    bool structIsFinal = false;
    if (peek().type == TokenType::IDENTIFIER && peek().value == "final") {
        advance();
        structIsFinal = true;
    }

    // Phase 3: Single inheritance — struct Dog : Animal { ... }
    std::string parentStruct;
    if (match(TokenType::COLON)) {
        // Expect parent struct name
        if (match(TokenType::STRUCT) || match(TokenType::IDENTIFIER)) {
            // Could be "struct Name" or just "Name"
            if (tokens[pos-1].type == TokenType::STRUCT) {
                parentStruct = expect(TokenType::IDENTIFIER, "Expected parent struct name").value;
            } else {
                parentStruct = tokens[pos-1].value;
            }
        }
    }

    expect(TokenType::OPEN_BRACE, "Expected '{'");
    auto def = setPos(std::make_unique<StructDefinition>(name, isUnion), startToken);
    def->isUnpacked = isUnpacked;
    def->isFinal = structIsFinal;
    def->parentStruct = parentStruct;

    // Validate: cannot inherit from a final struct
    if (!parentStruct.empty() && structs.count(parentStruct) && structs[parentStruct]->isFinal) {
        throw std::runtime_error("Error: cannot inherit from final struct '" + parentStruct + "'");
    }

    while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE) {
        // Check for struct method: type name(params) { body }
        // Also handle 'virtual' keyword
        bool methodIsVirtual = false;
        if (peek().type == TokenType::IDENTIFIER && peek().value == "virtual") {
            advance(); // consume 'virtual'
            methodIsVirtual = true;
            def->hasVirtual = true;
        }

        if (!isUnion && (methodIsVirtual || isFunctionDeclaration())) {
            auto method = parseFunctionDeclaration();
            method->isVirtual = methodIsVirtual;
            // Mangle name: StructName__methodName
            std::string origName = method->name;
            method->name = name + "__" + origName;
            method->isMethod = true;
            method->methodStructName = name;
            // Add hidden 'this' parameter as first param
            Parameter thisParam;
            thisParam.type = "struct " + name;
            thisParam.pointerLevel = 1;
            thisParam.isSigned = false;
            thisParam.name = "__this";
            method->parameters.insert(method->parameters.begin(), thisParam);
            def->methods.push_back(std::move(method));
            continue;
        }

        std::unique_ptr<Expression> mAlignmentExpr = nullptr;
        if (match(TokenType::ALIGNAS)) {
            expect(TokenType::OPEN_PAREN, "Expected '(' after '_Alignas'");
            if (isTypeStartToken()) {
                auto ts = parseTypeSpecifier();
                mAlignmentExpr = std::make_unique<AlignofExpression>(ts.name, ts.pointerLevel);
            } else {
                mAlignmentExpr = parseExpression();
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')' after alignment expression");
        }

        if (peek().type == TokenType::STRUCT || peek().type == TokenType::UNION) {
            bool isNestedUnion = peek().type == TokenType::UNION;
            if (pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::OPEN_BRACE) {
                // Determine if this is an anonymous nested struct (no member name)
                size_t scan = pos + 2;
                int braceCount = 1;
                while (scan < tokens.size() && braceCount > 0) {
                    if (tokens[scan].type == TokenType::OPEN_BRACE) braceCount++;
                    else if (tokens[scan].type == TokenType::CLOSE_BRACE) braceCount--;
                    scan++;
                }
                while (scan < tokens.size()) {
                    if (tokens[scan].type == TokenType::ATTRIBUTE) {
                        scan++;
                        if (scan < tokens.size() && tokens[scan].type == TokenType::OPEN_PAREN) {
                            scan++;
                            int parens = 1;
                            while (scan < tokens.size() && parens > 0) {
                                if (tokens[scan].type == TokenType::OPEN_PAREN) parens++;
                                else if (tokens[scan].type == TokenType::CLOSE_PAREN) parens--;
                                scan++;
                            }
                        }
                    } else {
                        break;
                    }
                }
                if (scan < tokens.size() && tokens[scan].type == TokenType::SEMICOLON) {
                    advance(); // struct/union
                    auto nestedDef = parseStructDefinition(isNestedUnion);
                    std::string nestedTypeName = (isNestedUnion ? "union " : "struct ") + nestedDef->name;
                    { StructMember sm; sm.type = nestedTypeName; sm.pointerLevel = 0; sm.isAnonymous = true; def->members.push_back(std::move(sm)); }
                    pendingDefinitions.push_back(std::move(nestedDef));
                    while (tryParseAttribute()) {}
                    match(TokenType::SEMICOLON); // consume optional semicolon
                    continue; 
                }
            }
        }

        bool mIsConst = false;
        while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT)) {
            if (tokens[pos-1].type == TokenType::CONST) mIsConst = true;
        }

        std::string type;
        bool mIsSigned = false;
        int basePtrLevel = 0;
        std::vector<int> typedefArrayDims;
        if (peek().type == TokenType::ATTRIBUTE) {
            // __attribute__ as member type prefix — skip and retry
            while (tryParseAttribute()) {}
            continue; // re-enter the member parsing loop
        }
        {
            auto ts = parseTypeSpecifier();
            if (!ts.valid) {
                throw std::runtime_error("Expected member type");
            }
            type = ts.name;
            mIsSigned = ts.isSigned;
            basePtrLevel = ts.pointerLevel;
            typedefArrayDims = ts.arrayDims;
            if (ts.isConst) mIsConst = true;
        }
        // Skip __attribute__ after type, before pointer/name
        while (tryParseAttribute()) {}
        int ptrLevel = basePtrLevel;
        while (match(TokenType::STAR)) {
            ptrLevel++;
            while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT) || tryParseAttribute() || match(TokenType::EXTENSION));
        }
        // Unnamed bitfield: int : 6;  (no member name, colon follows type)
        if (peek().type == TokenType::COLON) {
            advance(); // consume ':'
            const Token& bwTok = expect(TokenType::INTEGER_LITERAL, "Expected integer literal for bitfield width");
            int bw = std::stoi(bwTok.value);
            StructMember sm;
            sm.type = type; sm.pointerLevel = ptrLevel; sm.isSigned = mIsSigned;
            sm.name = "__unnamed_bf_" + std::to_string(def->members.size());
            sm.bitWidth = bw;
            while (tryParseAttribute()) {}
            def->members.push_back(std::move(sm));
            expect(TokenType::SEMICOLON, "Expected ';'");
            continue;
        }

        // Function pointer member: type (*name)(params);
        if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::STAR) {
            advance(); // (
            advance(); // *
            std::string fpName = expect(TokenType::IDENTIFIER, "Expected function pointer member name").value;
            // Skip array dims on fp name: void (*p[N])()
            while (match(TokenType::OPEN_SQUARE)) {
                int depth = 1;
                while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                    if (match(TokenType::OPEN_SQUARE)) depth++;
                    else if (peek().type == TokenType::CLOSE_SQUARE) { depth--; if (depth > 0) advance(); }
                    else advance();
                }
                expect(TokenType::CLOSE_SQUARE, "Expected ']'");
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer name");
            // Skip parameter list
            if (match(TokenType::OPEN_PAREN)) {
                int depth = 1;
                while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                    if (match(TokenType::OPEN_PAREN)) depth++;
                    else if (peek().type == TokenType::CLOSE_PAREN) { depth--; if (depth > 0) advance(); }
                    else advance();
                }
                expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer params");
            }
            while (tryParseAttribute()) {}
            StructMember sm;
            sm.type = "void"; sm.pointerLevel = 1; sm.isSigned = false;
            sm.name = fpName;
            def->members.push_back(std::move(sm));
            expect(TokenType::SEMICOLON, "Expected ';'");
            continue;
        }

        std::string memberName = expect(TokenType::IDENTIFIER, "Expected member name").value;
        std::vector<int> memberArrayDims;
        bool isFlexArray = false;
        while (match(TokenType::OPEN_SQUARE)) {
            if (match(TokenType::CLOSE_SQUARE)) {
                // Flexible array member: type name[];
                isFlexArray = true;
                memberArrayDims.push_back(0);
            } else {
                auto mSizeExpr = parseExpression();
                int mArrSize = 0;
                if (auto* lit = dynamic_cast<IntegerLiteral*>(mSizeExpr.get())) mArrSize = (int)lit->value;
                else mArrSize = 1;
                memberArrayDims.push_back(mArrSize);
                expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
            }
        }
        // Append typedef array dimensions
        memberArrayDims.insert(memberArrayDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
        if (isFlexArray && memberArrayDims.size() > 1)
            throw std::runtime_error("Flexible array member '" + memberName + "' cannot be multi-dimensional");
        int memberBitWidth = 0;
        if (match(TokenType::COLON)) {
            if (!memberArrayDims.empty())
                throw std::runtime_error("Bitfield member '" + memberName + "' cannot be an array");
            if (ptrLevel > 0)
                throw std::runtime_error("Bitfield member '" + memberName + "' cannot be a pointer");
            const Token& bwTok = expect(TokenType::INTEGER_LITERAL, "Expected integer literal for bitfield width");
            memberBitWidth = std::stoi(bwTok.value);
            // Max bitfield width: char=8, int=16, long=32
            // Use long for bitfields >16 bits (with __int(N) for arbitrary widths)
            int maxBits = (type == "long") ? 32 : (type == "char") ? 8 : 16;
            if (memberBitWidth < 1 || memberBitWidth > maxBits)
                throw std::runtime_error("Bitfield width " + std::to_string(memberBitWidth) + " out of range for type '" + type + "'");
        }
        while (tryParseAttribute()) {}
        StructMember sm;
        sm.type = type; sm.pointerLevel = ptrLevel; sm.isSigned = mIsSigned;
        sm.name = memberName; sm.isConst = mIsConst; sm.alignment = 0;
        sm.alignmentExpr = std::move(mAlignmentExpr); sm.isAnonymous = false;
        sm.arrayDims = memberArrayDims; sm.bitWidth = memberBitWidth;
        def->members.push_back(std::move(sm));

        // Multi-member declarations: long p_x, p_y;
        while (match(TokenType::COMMA)) {
            int extraPtr = basePtrLevel;
            while (match(TokenType::STAR)) extraPtr++;
            std::string extraName = expect(TokenType::IDENTIFIER, "Expected member name").value;
            std::vector<int> extraDims;
            while (match(TokenType::OPEN_SQUARE)) {
                if (match(TokenType::CLOSE_SQUARE)) { extraDims.push_back(0); }
                else {
                    extraDims.push_back(std::stoi(expect(TokenType::INTEGER_LITERAL, "Expected array size").value));
                    expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                }
            }
            // Append typedef array dimensions
            extraDims.insert(extraDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
            int extraBitWidth = 0;
            if (match(TokenType::COLON)) {
                extraBitWidth = std::stoi(expect(TokenType::INTEGER_LITERAL, "Expected bitfield width").value);
            }
            while (tryParseAttribute()) {}
            StructMember esm;
            esm.type = type; esm.pointerLevel = extraPtr; esm.isSigned = mIsSigned;
            esm.name = extraName; esm.isConst = mIsConst;
            esm.arrayDims = extraDims; esm.bitWidth = extraBitWidth;
            def->members.push_back(std::move(esm));
        }
        while (tryParseAttribute()) {}
        expect(TokenType::SEMICOLON, "Expected ';'");
    }
    expect(TokenType::CLOSE_BRACE, "Expected '}'");

    // Phase 3: If this struct has virtual methods, insert __vt pointer as first member
    if (def->hasVirtual || (!def->parentStruct.empty() && structs.count(def->parentStruct) && structs[def->parentStruct]->hasVirtual)) {
        // Check if __vt already exists (inherited from parent)
        bool hasVt = false;
        for (const auto& m : def->members) { if (m.name == "__vt") { hasVt = true; break; } }
        if (!hasVt) {
            StructMember vtMember;
            vtMember.type = "int"; // pointer to vtable (2 bytes)
            vtMember.pointerLevel = 1;
            vtMember.name = "__vt";
            def->members.insert(def->members.begin(), std::move(vtMember));
        }
        def->hasVirtual = true;
    }

    // Assign vtable slots to virtual methods
    if (def->hasVirtual) {
        int slot = 0;
        // Inherit parent's vtable slot assignments
        if (!def->parentStruct.empty() && structs.count(def->parentStruct)) {
            auto* parent = structs[def->parentStruct];
            for (auto& pm : parent->methods) {
                if (pm->isVirtual) slot = std::max(slot, pm->vtableSlot + 1);
            }
        }
        for (auto& method : def->methods) {
            if (method->isVirtual) {
                // Check if this overrides a parent method (same name)
                bool isOverride = false;
                if (!def->parentStruct.empty() && structs.count(def->parentStruct)) {
                    for (auto& pm : structs[def->parentStruct]->methods) {
                        if (pm->isVirtual && pm->name.substr(pm->name.rfind("__") + 2) == method->name.substr(method->name.rfind("__") + 2)) {
                            // Validate: cannot override a final method
                            if (pm->isFinal) {
                                throw std::runtime_error("Error: cannot override final method '" + pm->name.substr(pm->name.rfind("__") + 2) + "' in struct '" + name + "'");
                            }
                            method->vtableSlot = pm->vtableSlot;
                            isOverride = true;
                            break;
                        }
                    }
                }
                if (!isOverride) {
                    method->vtableSlot = slot++;
                }
            }
        }
    }

    // Register for inheritance lookup
    structs[name] = def.get();

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
        std::unique_ptr<Expression> thenExpr;
        if (peek().type == TokenType::COLON) {
            // GCC Elvis operator: expr ?: default — thenExpr is null (use condition value)
            thenExpr = nullptr;
        } else {
            thenExpr = parseExpression();
        }
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
    // __real__ / __imag__: unary operators that access .real / .imag
    // Support both __real__ x (no parens) and __real__(x) (with parens)
    if (match(TokenType::REAL_PART) || match(TokenType::IMAG_PART)) {
        const Token& opToken = tokens[pos-1];
        std::string memberName = (opToken.type == TokenType::REAL_PART) ? "real" : "imag";
        std::unique_ptr<Expression> operand;
        if (match(TokenType::OPEN_PAREN)) {
            operand = parseExpression();
            expect(TokenType::CLOSE_PAREN, "Expected ')' after __real__/__imag__");
        } else {
            operand = parseUnary();
        }
        auto ma = std::make_unique<MemberAccess>(std::move(operand), memberName, false);
        return setPos(std::move(ma), opToken);
    }
    if (match(TokenType::SIZEOF)) {
        const Token& startToken = tokens[pos-1];
        if (match(TokenType::OPEN_PAREN)) {
            // Check if it's a type or an expression
            if (isTypeStartToken()) {
                auto ts = parseTypeSpecifier();
                expect(TokenType::CLOSE_PAREN, "Expected ')' after type in sizeof");
                auto node = std::make_unique<SizeofExpression>(ts.name, ts.pointerLevel);
                node->arrayDims = ts.arrayDims;
                return setPos(std::move(node), startToken);
            } else {
                auto expr = parseExpression();
                expect(TokenType::CLOSE_PAREN, "Expected ')' after expression in sizeof");
                return setPos(std::make_unique<SizeofExpression>(std::move(expr)), startToken);
            }
        } else {
            return setPos(std::make_unique<SizeofExpression>(parseUnary()), startToken);
        }
    }

    // Statement expression: ({ stmt; stmt; expr; })
    if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::OPEN_BRACE) {
        const Token& startToken = tokens[pos];
        advance(); // consume '('
        advance(); // consume '{'
        // Parse statements until we hit '}'
        // The last expression before '}' is the result value
        std::unique_ptr<Expression> resultExpr;
        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE) {
            // Try to parse as a statement
            auto stmt = parseStatement();
            // If it was an ExpressionStatement and the next token is '}', use the expression as result
            if (peek().type == TokenType::CLOSE_BRACE) {
                if (auto* exprStmt = dynamic_cast<ExpressionStatement*>(stmt.get())) {
                    // The last statement's expression is the result
                    // We already consumed the semicolon in parseStatement, so this is fine
                }
            }
        }
        expect(TokenType::CLOSE_BRACE, "Expected '}' in statement expression");
        expect(TokenType::CLOSE_PAREN, "Expected ')' after statement expression");
        // Statement expressions yield their last value; for now, treat as 0
        std::unique_ptr<Expression> result = setPos(std::make_unique<IntegerLiteral>(0), startToken);
        // Allow postfix operators: ({...}).member, ({...})[i]
        while (peek().type == TokenType::DOT || peek().type == TokenType::ARROW ||
               peek().type == TokenType::OPEN_SQUARE) {
            if (match(TokenType::DOT)) {
                std::string member = expect(TokenType::IDENTIFIER, "Expected member name").value;
                result = setPos(std::make_unique<MemberAccess>(std::move(result), member, false), startToken);
            } else if (match(TokenType::ARROW)) {
                std::string member = expect(TokenType::IDENTIFIER, "Expected member name").value;
                result = setPos(std::make_unique<MemberAccess>(std::move(result), member, true), startToken);
            } else if (match(TokenType::OPEN_SQUARE)) {
                auto index = parseExpression();
                expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                result = setPos(std::make_unique<ArrayAccess>(std::move(result), std::move(index)), startToken);
            }
        }
        return result;
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
        bool castVolatile = false, castConst = false;

        if (isTypeStartToken()) {
            auto ts = parseTypeSpecifier();
            castType = ts.name;
            castSigned = ts.isSigned;
            castPtrLevel = ts.pointerLevel;
            castVolatile = ts.isVolatile;
            castConst = ts.isConst;

            // Array cast: (int []) or (int [N]) — array compound literal
            while (peek().type == TokenType::OPEN_SQUARE) {
                advance(); // [
                if (peek().type != TokenType::CLOSE_SQUARE) {
                    // Skip size expression
                    while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE) advance();
                }
                expect(TokenType::CLOSE_SQUARE, "Expected ']' in array cast");
                castPtrLevel++; // array decays to pointer
            }

            // Function pointer cast: (return_type (*)(params))
            if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::STAR) {
                // Skip (*) and parameter list
                advance(); // (
                advance(); // *
                expect(TokenType::CLOSE_PAREN, "Expected ')' in function pointer cast");
                // Skip parameter list: (type, type, ...)
                if (match(TokenType::OPEN_PAREN)) {
                    int depth = 1;
                    while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                        if (match(TokenType::OPEN_PAREN)) depth++;
                        else if (peek().type == TokenType::CLOSE_PAREN) { depth--; if (depth > 0) advance(); }
                        else advance();
                    }
                    expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer cast params");
                }
                castPtrLevel = 1; // function pointer is a pointer
            }

            if (peek().type == TokenType::CLOSE_PAREN) {
                advance(); // consume ')'
                isCast = true;
            }
        } else if (castVolatile || castConst) {
            // Had qualifiers but no type — backtrack
            pos = savedPos;
        }

        if (isCast) {
            // Compound literal: (type){...}
            if (peek().type == TokenType::OPEN_BRACE) {
                auto initList = parseInitializerList();
                std::unique_ptr<InitializerList> initListPtr(static_cast<InitializerList*>(initList.release()));
                auto cl = std::make_unique<CompoundLiteral>(castType, castPtrLevel, castSigned, std::move(initListPtr));
                // For array compound literals like (int[]){1,2,3}, infer dimension from element count
                if (castPtrLevel == 0 && !castType.empty() && castType.substr(0, 7) != "struct " && castType.substr(0, 6) != "union ") {
                    if (cl->initializer && cl->initializer->elements.size() > 1) {
                        cl->arrayDims.push_back((int)cl->initializer->elements.size());
                    }
                }
                std::unique_ptr<Expression> result = setPos(std::move(cl), startToken);
                // Allow postfix operators on compound literals: (int[]){1,2}[0]
                while (peek().type == TokenType::OPEN_SQUARE || peek().type == TokenType::DOT ||
                       peek().type == TokenType::ARROW) {
                    if (match(TokenType::OPEN_SQUARE)) {
                        auto index = parseExpression();
                        expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                        result = setPos(std::make_unique<ArrayAccess>(std::move(result), std::move(index)), startToken);
                    } else if (match(TokenType::DOT)) {
                        std::string member = expect(TokenType::IDENTIFIER, "Expected member name").value;
                        result = setPos(std::make_unique<MemberAccess>(std::move(result), member, false), startToken);
                    } else if (match(TokenType::ARROW)) {
                        std::string member = expect(TokenType::IDENTIFIER, "Expected member name").value;
                        result = setPos(std::make_unique<MemberAccess>(std::move(result), member, true), startToken);
                    }
                }
                return result;
            }
            auto operand = parseUnary();
            return setPos(std::make_unique<CastExpression>(castType, castPtrLevel, castSigned, std::move(operand)), startToken);
        } else {
            pos = savedPos; // backtrack, let parsePrimary handle it as parenthesized expr
        }
    }

    if (match(TokenType::AND)) {
        // GCC Extension: &&label (address of label)
        std::string label = expect(TokenType::IDENTIFIER, "Expected label name after &&").value;
        return setPos(std::make_unique<LabelAddressExpression>(label), tokens[pos-2]);
    }
    if (match(TokenType::AMPERSAND)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto operand = parseUnary();
        if (dynamic_cast<CpuRegisterAccess*>(operand.get())) {
            throw std::runtime_error("Error at " + std::to_string(opToken.line) + ":" + std::to_string(opToken.column) + ": Cannot take address of CPU register");
        }
        if (dynamic_cast<CpuFlagAccess*>(operand.get())) {
            throw std::runtime_error("Error at " + std::to_string(opToken.line) + ":" + std::to_string(opToken.column) + ": Cannot take address of CPU flag");
        }
        return setPos(std::make_unique<UnaryOperation>(op, std::move(operand)), opToken);
    }

    if (match(TokenType::BANG) || match(TokenType::TILDE) || match(TokenType::MINUS) || match(TokenType::STAR) ||
        match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS)) {
        const Token& opToken = tokens[pos-1];
        std::string op = opToken.value;
        auto operand = parseUnary();
        return setPos(std::make_unique<UnaryOperation>(op, std::move(operand)), opToken);
    }
    return parsePrimary();
}

std::unique_ptr<Expression> Parser::parsePrimary() {
    // __extension__ is a GCC marker with no semantic effect — skip it
    while (match(TokenType::EXTENSION)) {}

    std::unique_ptr<Expression> expr;
    if (match(TokenType::GENERIC)) {
        return parseGenericSelection();
    }
    if (match(TokenType::ALIGNOF)) {
        expect(TokenType::OPEN_PAREN, "Expected '(' after '_Alignof'");
        if (isTypeStartToken()) {
            auto ts = parseTypeSpecifier();
            expect(TokenType::CLOSE_PAREN, "Expected ')' after _Alignof type");
            return setPos(std::make_unique<AlignofExpression>(ts.name, ts.pointerLevel), tokens[pos-1]);
        } else {
            // Fall back: treat as expression (sizeof-style)
            auto expr = parseExpression();
            expect(TokenType::CLOSE_PAREN, "Expected ')' after _Alignof");
            // Use a default alignment of 1 for expressions
            return setPos(std::make_unique<IntegerLiteral>(1), tokens[pos-1]);
        }
    }

    if (peek().type == TokenType::IDENTIFIER && (peek().value == "__func__" || peek().value == "__FUNCTION__")) {
        const Token& funcToken = advance();
        if (currentFunctionName.empty()) {
            throw std::runtime_error("Error at " + std::to_string(funcToken.line) + ":" + std::to_string(funcToken.column) + ": __func__ used outside of a function");
        }
        expr = setPos(std::make_unique<StringLiteral>(currentFunctionName), funcToken);
    } else if (peek().type == TokenType::IDENTIFIER && (peek().value == "__cpu" || peek().value == "__flags")) {
        const Token& baseToken = advance();
        std::string base = baseToken.value;
        if (!match(TokenType::DOT)) {
            throw std::runtime_error("Error at " + std::to_string(baseToken.line) + ":" + std::to_string(baseToken.column) + ": '" + base + "' must be followed by '.' and a member name");
        }
        const Token& memberToken = expect(TokenType::IDENTIFIER, "Expected member name after " + base + ".");
        std::string member = memberToken.value;
        if (base == "__cpu") {
            expr = setPos(std::make_unique<CpuRegisterAccess>(member), baseToken);
        } else {
            expr = setPos(std::make_unique<CpuFlagAccess>(member), baseToken);
        }
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_constant_p") {
        // __builtin_constant_p(x) → 1 if x is a compile-time constant, 0 otherwise
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_constant_p");
        auto arg = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        // Check if the argument is a compile-time constant
        bool isConst = false;
        if (dynamic_cast<IntegerLiteral*>(arg.get())) isConst = true;
        else if (dynamic_cast<StringLiteral*>(arg.get())) isConst = true;
        else if (auto* unary = dynamic_cast<UnaryOperation*>(arg.get())) {
            if (dynamic_cast<IntegerLiteral*>(unary->operand.get())) isConst = true;
        } else if (auto* binop = dynamic_cast<BinaryOperation*>(arg.get())) {
            if (dynamic_cast<IntegerLiteral*>(binop->left.get()) &&
                dynamic_cast<IntegerLiteral*>(binop->right.get())) isConst = true;
        }
        expr = setPos(std::make_unique<IntegerLiteral>(isConst ? 1 : 0), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_offsetof") {
        // __builtin_offsetof(type, member) → compile-time offset
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_offsetof");
        // Parse type (struct/union name)
        std::string ofType;
        if (match(TokenType::STRUCT) || match(TokenType::UNION)) {
            bool isU = tokens[pos-1].type == TokenType::UNION;
            ofType = (isU ? "union " : "struct ") + expect(TokenType::IDENTIFIER, "Expected struct name").value;
        } else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
            ofType = typedefs[advance().value].baseType;
        } else {
            ofType = expect(TokenType::IDENTIFIER, "Expected type name").value;
        }
        expect(TokenType::COMMA, "Expected ',' in __builtin_offsetof");
        // Parse member (may include array subscripts: member[i][j])
        std::string memberName = expect(TokenType::IDENTIFIER, "Expected member name").value;
        // Skip array subscripts if present
        while (match(TokenType::OPEN_SQUARE)) {
            while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE) advance();
            expect(TokenType::CLOSE_SQUARE, "Expected ']'");
        }
        // Skip nested member access (.submember)
        while (match(TokenType::DOT)) {
            expect(TokenType::IDENTIFIER, "Expected member name");
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after __builtin_offsetof");
        // For now, emit 0 as placeholder — the actual offset would need struct layout info
        // which the parser doesn't have. The IRBuilder will resolve it.
        expr = setPos(std::make_unique<IntegerLiteral>(0), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_expect") {
        // __builtin_expect(x, v) → evaluates to x (branch prediction hint, no-op)
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_expect");
        expr = parseExpression();
        expect(TokenType::COMMA, "Expected ',' in __builtin_expect");
        parseExpression(); // parse and discard expected value
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_trap") {
        // __builtin_trap() → BRK instruction
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_trap");
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        // Emit as inline asm BRK — wrap in a comma expression that returns 0
        expr = setPos(std::make_unique<IntegerLiteral>(0), bToken);
        // The actual BRK will be emitted if this is used in a statement context
        // For now, it's a no-op in expression context
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_unreachable") {
        // __builtin_unreachable() → no-op (undefined behavior if reached)
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '('");
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        expr = setPos(std::make_unique<IntegerLiteral>(0), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_va_start") {
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_va_start");
        auto apExpr = parseExpression();
        expect(TokenType::COMMA, "Expected ',' in __builtin_va_start");
        std::string lastParam = expect(TokenType::IDENTIFIER, "Expected last named parameter").value;
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        expr = setPos(std::make_unique<BuiltinVaStart>(std::move(apExpr), lastParam), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_va_arg") {
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_va_arg");
        auto apExpr = parseExpression();
        expect(TokenType::COMMA, "Expected ',' in __builtin_va_arg");
        // Parse type specifier
        std::string vaType;
        bool vaSigned = false;
        int vaPtrLevel = 0;
        if (match(TokenType::TYPEOF)) {
            // typeof(expr) in va_arg — resolve to int (best effort)
            expect(TokenType::OPEN_PAREN, "Expected '(' after typeof");
            int depth = 1;
            while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                if (match(TokenType::OPEN_PAREN)) depth++;
                else if (peek().type == TokenType::CLOSE_PAREN) { depth--; if (depth > 0) advance(); }
                else advance();
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')' after typeof");
            vaType = "int"; // best effort: typeof in va_arg usually resolves to the arg type
        } else {
            auto ts = parseTypeSpecifier();
            if (!ts.valid) {
                throw std::runtime_error("Expected type in __builtin_va_arg");
            }
            vaType = ts.name;
            vaSigned = ts.isSigned;
            vaPtrLevel = ts.pointerLevel;
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        expr = setPos(std::make_unique<BuiltinVaArg>(std::move(apExpr), vaType, vaPtrLevel, vaSigned), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_va_end") {
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_va_end");
        auto apExpr = parseExpression(); // consume but ignore
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        // va_end is a no-op; emit 0
        expr = setPos(std::make_unique<IntegerLiteral>(0), bToken);
    } else if (peek().type == TokenType::IDENTIFIER) {
        const Token& nameToken = advance();
        std::string name = nameToken.value;

        // Resolve nested function name
        std::string actualName = name;
        if (nestedFunctionMap.count(name)) {
            actualName = nestedFunctionMap.at(name);
        }

        // 'this' keyword in struct methods → hidden '__this' parameter
        if (actualName == "this") {
            actualName = "__this";
        }

        // Map __builtin_X → X for known stdlib-equivalent builtins
        if (actualName.rfind("__builtin_", 0) == 0) {
            static const std::map<std::string, std::string> builtinAliases = {
                {"__builtin_abort", "abort"},
                {"__builtin_printf", "printf"},
                {"__builtin_sprintf", "sprintf"},
                {"__builtin_puts", "puts"},
                {"__builtin_putchar", "putchar"},
                {"__builtin_malloc", "malloc"},
                {"__builtin_free", "free"},
                {"__builtin_calloc", "calloc"},
                {"__builtin_strlen", "strlen"},
                {"__builtin_strcmp", "strcmp"},
                {"__builtin_strcpy", "strcpy"},
                {"__builtin_strncpy", "strncpy"},
                {"__builtin_strcat", "strcat"},
                {"__builtin_strchr", "strchr"},
                {"__builtin_strstr", "strstr"},
                {"__builtin_memcpy", "memcpy"},
                {"__builtin_memset", "memset"},
                {"__builtin_memmove", "memmove"},
                {"__builtin_memcmp", "memcmp"},
                {"__builtin_abs", "abs"},
                {"__builtin_labs", "labs"},
                {"__builtin_exit", "exit"},
            };
            auto it = builtinAliases.find(actualName);
            if (it != builtinAliases.end()) {
                actualName = it->second;
            }
        }

        auto enumIt = enumConstants.find(name);
        if (enumIt != enumConstants.end()) {
            return setPos(std::make_unique<IntegerLiteral>(enumIt->second), nameToken);
        }

        if (match(TokenType::OPEN_PAREN)) {
            auto call = setPos(std::make_unique<FunctionCall>(actualName), nameToken);
            if (peek().type != TokenType::CLOSE_PAREN) {
                do {
                    call->arguments.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')'");
            expr = std::move(call);
        } else {
            expr = setPos(std::make_unique<VariableReference>(actualName), nameToken);
        }
    } else if (peek().type == TokenType::INTEGER_LITERAL) {
        const Token& litToken = advance();
        std::string valStr = litToken.value;
        bool isLong = false;
        bool isUnsigned = false;
        if (!valStr.empty() && (valStr.back() == 'L' || valStr.back() == 'l')) {
            isLong = true;
            valStr.pop_back();
        }
        if (!valStr.empty() && (valStr.back() == 'U' || valStr.back() == 'u')) {
            isUnsigned = true;
            valStr.pop_back();
        }
        int64_t val = (int64_t)std::stoll(valStr, nullptr, 0);
        auto lit = std::make_unique<IntegerLiteral>(val);
        if (isLong) {
            lit->castType = "long";
            lit->castIsSigned = !isUnsigned;
        } else if (isUnsigned) {
            lit->castType = "unsigned int";
            lit->castIsSigned = false;
        }
        expr = setPos(std::move(lit), litToken);
    } else if (peek().type == TokenType::FLOAT_LITERAL) {
        const Token& litToken = advance();
        double val = std::stod(litToken.value);
        expr = setPos(std::make_unique<FloatLiteral>(val), litToken);
    } else if (peek().type == TokenType::IMAGINARY_LITERAL) {
        // 1.0fi / 1.0i → (struct _Complex_float){0.0, VALUE}
        const Token& litToken = advance();
        double val = std::stod(litToken.value);
        auto initList = std::make_unique<InitializerList>();
        initList->elements.push_back(std::make_unique<FloatLiteral>(0.0));
        initList->elements.push_back(std::make_unique<FloatLiteral>(val));
        auto cl = std::make_unique<CompoundLiteral>("struct _Complex_float", 0, false, std::move(initList));
        expr = setPos(std::move(cl), litToken);
    } else if (peek().type == TokenType::STRING_LITERAL || peek().type == TokenType::ASCII_STRING_LITERAL) {
        bool isAscii = (peek().type == TokenType::ASCII_STRING_LITERAL);
        const Token& litToken = advance();
        std::string concatStr = litToken.value;
        // C string literal concatenation: "hello" " world" → "hello world"
        while (peek().type == TokenType::STRING_LITERAL || peek().type == TokenType::ASCII_STRING_LITERAL) {
            concatStr += advance().value;
        }
        expr = setPos(std::make_unique<StringLiteral>(concatStr, isAscii), litToken);
    } else if (match(TokenType::OPEN_PAREN)) {
        const Token& parenToken = tokens[pos-1];
        expr = parseExpression();
        // Handle comma operator inside parenthesized expressions: (expr, expr, ...)
        while (match(TokenType::COMMA)) {
            const Token& commaToken = tokens[pos-1];
            auto right = parseExpression();
            expr = setPos(std::make_unique<BinaryOperation>(",", std::move(expr), std::move(right)), commaToken);
        }
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
            if (match(TokenType::LONG)) { assoc.typeName = "long"; match(TokenType::INT); } else if (match(TokenType::SHORT)) { assoc.typeName = "int"; match(TokenType::INT); } else if (match(TokenType::INT)) assoc.typeName = "int";
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
    bool hasDesignators = false;
    if (peek().type != TokenType::CLOSE_BRACE) {
        do {
            InitializerList::Designator desig;
            // Check for designators: .member = , .member.sub = , or [index] =
            if (peek().type == TokenType::DOT && pos + 1 < tokens.size()
                && tokens[pos + 1].type == TokenType::IDENTIFIER) {
                // Consume .member chain (e.g., .f.f9)
                std::string chain;
                while (peek().type == TokenType::DOT && pos + 1 < tokens.size()
                       && tokens[pos + 1].type == TokenType::IDENTIFIER) {
                    advance(); // consume '.'
                    if (!chain.empty()) chain += ".";
                    chain += advance().value; // consume identifier
                }
                if (match(TokenType::EQUALS)) {
                    desig.memberName = chain;
                    hasDesignators = true;
                } else {
                    // Not a designator — backtrack would be needed, but for simplicity
                    // treat as designator without '=' (some C99 code uses this)
                    desig.memberName = chain;
                    hasDesignators = true;
                }
            } else if (peek().type == TokenType::OPEN_SQUARE && pos + 3 < tokens.size()
                && tokens[pos + 1].type == TokenType::INTEGER_LITERAL
                && tokens[pos + 2].type == TokenType::CLOSE_SQUARE
                && tokens[pos + 3].type == TokenType::EQUALS) {
                advance(); // consume '['
                desig.arrayIndex = std::stoi(advance().value); // consume integer
                advance(); // consume ']'
                advance(); // consume '='
                hasDesignators = true;
            }
            initList->designators.push_back(desig);
            if (peek().type == TokenType::OPEN_BRACE) {
                initList->elements.push_back(parseInitializerList());
            } else {
                initList->elements.push_back(parseConditional());
            }
        } while (match(TokenType::COMMA) && peek().type != TokenType::CLOSE_BRACE);
    }
    // Strip empty designators if none were used
    if (!hasDesignators) initList->designators.clear();
    expect(TokenType::CLOSE_BRACE, "Expected '}'");
    return initList;
}

// Parse __attribute__((...)) — returns true if an attribute was consumed.
// ===========================================================================
// Unified type specifier parsing
// ===========================================================================

bool Parser::isTypeStartToken() const {
    return isTypeStartAt(pos);
}

bool Parser::isTypeStartAt(size_t look) const {
    if (look >= tokens.size()) return false;
    TokenType t = tokens[look].type;
    return t == TokenType::INT || t == TokenType::CHAR || t == TokenType::LONG ||
           t == TokenType::SHORT || t == TokenType::VOID || t == TokenType::BOOL ||
           t == TokenType::FLOAT || t == TokenType::DOUBLE || t == TokenType::COMPLEX ||
           t == TokenType::SIGNED || t == TokenType::UNSIGNED ||
           t == TokenType::STRUCT || t == TokenType::UNION || t == TokenType::ENUM ||
           t == TokenType::CONST || t == TokenType::VOLATILE || t == TokenType::RESTRICT ||
           t == TokenType::INT_N || t == TokenType::UINT_N ||
           (t == TokenType::IDENTIFIER && isTypedef(tokens[look].value));
}

Parser::TypeSpec Parser::parseTypeSpecifier() {
    TypeSpec ts;

    // Consume leading qualifiers
    while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT) ||
           tryParseAttribute() || match(TokenType::EXTENSION)) {
        if (tokens[pos-1].type == TokenType::CONST) ts.isConst = true;
        if (tokens[pos-1].type == TokenType::VOLATILE) ts.isVolatile = true;
    }

    // Parse base type
    if (match(TokenType::SIGNED)) {
        ts.isSigned = true;
        // Skip qualifiers between signed and type
        while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT)) {
            if (tokens[pos-1].type == TokenType::CONST) ts.isConst = true;
            if (tokens[pos-1].type == TokenType::VOLATILE) ts.isVolatile = true;
        }
        if (match(TokenType::LONG)) {
            if (match(TokenType::LONG)) ts.name = "struct __int64";
            else if (match(TokenType::DOUBLE)) { ts.name = match(TokenType::COMPLEX) ? "struct _Complex_float" : "float"; }
            else { ts.name = "long"; match(TokenType::INT); }
        } else if (match(TokenType::SHORT)) { ts.name = "int"; match(TokenType::INT); }
        else if (match(TokenType::INT)) ts.name = "int";
        else if (match(TokenType::CHAR)) ts.name = "char";
        else ts.name = "int"; // bare 'signed'
    }
    else if (match(TokenType::UNSIGNED)) {
        ts.isUnsigned = true;
        while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT)) {
            if (tokens[pos-1].type == TokenType::CONST) ts.isConst = true;
            if (tokens[pos-1].type == TokenType::VOLATILE) ts.isVolatile = true;
        }
        if (match(TokenType::LONG)) {
            if (match(TokenType::LONG)) ts.name = "struct __int64";
            else if (match(TokenType::DOUBLE)) { ts.name = match(TokenType::COMPLEX) ? "struct _Complex_float" : "float"; }
            else { ts.name = "long"; match(TokenType::INT); }
        } else if (match(TokenType::SHORT)) { ts.name = "int"; match(TokenType::INT); }
        else if (match(TokenType::INT)) ts.name = "int";
        else if (match(TokenType::CHAR)) ts.name = "char";
        else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
            ts.name = typedefs[advance().value].baseType;
        }
        else ts.name = "int"; // bare 'unsigned'
    }
    else if (match(TokenType::LONG)) {
        if (match(TokenType::DOUBLE)) { ts.name = match(TokenType::COMPLEX) ? "struct _Complex_float" : "float"; }
        else {
            if (match(TokenType::LONG)) ts.name = "struct __int64";
            else ts.name = "long";
            if (match(TokenType::UNSIGNED)) { ts.isUnsigned = true; }
            else if (match(TokenType::SIGNED)) { ts.isSigned = true; }
            match(TokenType::INT);
        }
    }
    else if (match(TokenType::SHORT)) {
        ts.name = "int";
        if (match(TokenType::UNSIGNED)) { ts.isUnsigned = true; }
        else if (match(TokenType::SIGNED)) { ts.isSigned = true; }
        match(TokenType::INT);
    }
    else if (match(TokenType::INT)) ts.name = "int";
    else if (match(TokenType::CHAR)) ts.name = "char";
    else if (match(TokenType::BOOL)) ts.name = "_Bool";
    else if (match(TokenType::VOID)) ts.name = "void";
    else if (match(TokenType::FLOAT)) { ts.name = match(TokenType::COMPLEX) ? "struct _Complex_float" : "float"; }
    else if (match(TokenType::DOUBLE)) { ts.name = match(TokenType::COMPLEX) ? "struct _Complex_float" : "float"; }
    else if (match(TokenType::COMPLEX)) ts.name = resolveComplexType();
    else if (match(TokenType::INT_N)) { ts.name = resolveIntNType(true); }
    else if (match(TokenType::UINT_N)) { ts.name = resolveIntNType(false); }
    else if (match(TokenType::STRUCT) || match(TokenType::UNION)) {
        bool isU = tokens[pos-1].type == TokenType::UNION;
        if (peek().type == TokenType::OPEN_BRACE) {
            auto def = parseStructDefinition(isU);
            ts.name = (isU ? "union " : "struct ") + def->name;
            pendingDefinitions.push_back(std::move(def));
        } else if (peek().type == TokenType::IDENTIFIER && (pos+1>=tokens.size() || tokens[pos+1].type != TokenType::OPEN_BRACE)) {
            ts.name = (isU ? "union " : "struct ") + advance().value;
        } else {
            auto def = parseStructDefinition(isU);
            ts.name = (isU ? "union " : "struct ") + def->name;
            pendingDefinitions.push_back(std::move(def));
        }
    }
    else if (match(TokenType::ENUM)) {
        if (peek().type == TokenType::IDENTIFIER && (pos+1>=tokens.size() || tokens[pos+1].type != TokenType::OPEN_BRACE)) {
            ts.name = "enum " + advance().value;
        } else {
            auto def = parseEnumDefinition();
            ts.name = "enum " + def->name;
        }
    }
    else if (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value)) {
        std::string alias = advance().value;
        auto& ta = typedefs[alias];
        ts.name = ta.baseType;
        ts.isSigned = ta.isSigned;
        ts.pointerLevel = ta.pointerLevel;
        ts.arrayDims = ta.arrayDims;
        ts.isFunctionPointer = ta.isFunctionPointer;
        ts.funcPtrSig = ta.funcPtrSig;
    }
    else {
        // No type found
        return ts; // ts.valid = false, ts.name = ""
    }

    ts.valid = true;

    // Consume trailing qualifiers
    while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT) ||
           tryParseAttribute() || match(TokenType::EXTENSION)) {
        if (tokens[pos-1].type == TokenType::CONST) ts.isConst = true;
        if (tokens[pos-1].type == TokenType::VOLATILE) ts.isVolatile = true;
    }

    // If type is followed by __complex__ (reverse order: float __complex__)
    if (match(TokenType::COMPLEX)) {
        if (ts.name == "float" || ts.name == "double") ts.name = "struct _Complex_float";
        else ts.name = "struct _Complex_int";
    }

    // Consume pointer stars
    while (match(TokenType::STAR)) {
        ts.pointerLevel++;
        while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT)) {
            if (tokens[pos-1].type == TokenType::CONST) ts.isConst = true;
        }
    }

    return ts;
}

// ===========================================================================
// Existing helper functions
// ===========================================================================

// Recognized attributes: noinline, noclone, packed (silently accepted).
// Unrecognized attributes: warned and skipped.
std::string Parser::resolveComplexType() {
    // Called after matching COMPLEX token. Consume the following base type
    // and return the appropriate struct name.
    if (match(TokenType::FLOAT)) return "struct _Complex_float";
    if (match(TokenType::DOUBLE)) return "struct _Complex_float";
    if (match(TokenType::LONG)) {
        if (match(TokenType::DOUBLE)) return "struct _Complex_float";  // _Complex long double
        match(TokenType::INT);  // _Complex long int → integer complex
        return "struct _Complex_int";
    }
    if (match(TokenType::INT)) return "struct _Complex_int";
    if (match(TokenType::SHORT)) { match(TokenType::INT); return "struct _Complex_int"; }
    if (match(TokenType::UNSIGNED)) { match(TokenType::LONG) || match(TokenType::SHORT) || match(TokenType::INT) || match(TokenType::CHAR); match(TokenType::INT); return "struct _Complex_int"; }
    if (match(TokenType::SIGNED)) { match(TokenType::LONG) || match(TokenType::SHORT) || match(TokenType::INT) || match(TokenType::CHAR); match(TokenType::INT); return "struct _Complex_int"; }
    if (match(TokenType::CHAR)) return "struct _Complex_int";
    // Bare _Complex with no type → default to double → float
    return "struct _Complex_float";
}

std::string Parser::resolveIntNType(bool isSigned) {
    // Called after matching INT_N or UINT_N token.
    // If the token value already encodes the width (e.g. "__int128"), extract it.
    // Otherwise consume (N) to get the width.
    const std::string& tokVal = tokens[pos-1].value;
    int bits = 0;
    if (tokVal == "__int128" || tokVal == "__int128_t") {
        bits = 128;
    } else if (tokVal == "__uint128_t") {
        bits = 128;
    } else if (match(TokenType::OPEN_PAREN)) {
        if (peek().type == TokenType::INTEGER_LITERAL) {
            bits = std::stoi(advance().value);
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after __int/__uint width");
    }
    // Map to nearest supported type
    if (bits <= 0) bits = 32; // default
    std::string prefix = isSigned ? "__int" : "__uint";
    if (bits <= 8) return isSigned ? "char" : "unsigned char";
    if (bits <= 16) return isSigned ? "int" : "unsigned int";
    if (bits <= 32) return isSigned ? "long" : "unsigned long";
    // >32 bits: return canonical name for future I64 support
    return prefix + std::to_string(bits);
}

bool Parser::tryParseAttribute() {
    if (!match(TokenType::ATTRIBUTE)) return false;

    // Expect (( ... ))
    if (!match(TokenType::OPEN_PAREN)) return true;
    if (!match(TokenType::OPEN_PAREN)) return true;

    // Parse comma-separated attribute list until ))
    while (peek().type != TokenType::CLOSE_PAREN && peek().type != TokenType::END_OF_FILE) {
        if (peek().type == TokenType::IDENTIFIER) {
            std::string attr = advance().value;
            // Strip leading/trailing underscores for canonical form
            std::string canonical = attr;
            if (canonical.size() > 4 && canonical.substr(0, 2) == "__" &&
                canonical.substr(canonical.size() - 2) == "__")
                canonical = canonical.substr(2, canonical.size() - 4);

            // Recognized — silently accept
            if (canonical == "noinline" || canonical == "noclone" || canonical == "packed" ||
                canonical == "always_inline" || canonical == "gnu_inline" ||
                canonical == "unused" || canonical == "used" || canonical == "weak" ||
                canonical == "const" || canonical == "pure" || canonical == "flatten" ||
                canonical == "cold" || canonical == "hot" || canonical == "nothrow" ||
                canonical == "returns_twice" || canonical == "warn_unused_result" ||
                canonical == "deprecated" || canonical == "visibility" ||
                canonical == "section" || canonical == "nonnull" || canonical == "sentinel" ||
                canonical == "format" || canonical == "format_arg" || canonical == "malloc" ||
                canonical == "alloc_size" || canonical == "constructor" || canonical == "destructor") {
                // Skip optional parenthesized argument
                if (peek().type == TokenType::OPEN_PAREN) {
                    int depth = 1; advance();
                    while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                        if (match(TokenType::OPEN_PAREN)) depth++;
                        else if (match(TokenType::CLOSE_PAREN)) depth--;
                        else advance();
                    }
                }
            }
            // Warn-and-skip (#110-#114)
            else if (canonical == "noipa" || canonical == "aligned" ||
                     canonical == "mode" || canonical == "vector_size" ||
                     canonical == "may_alias") {
                std::cerr << tokens[pos-1].line << ": warning: __attribute__(("
                          << attr << ")) ignored\n";
                // Skip parenthesized argument if present
                if (peek().type == TokenType::OPEN_PAREN) {
                    int depth = 1; advance();
                    while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                        if (match(TokenType::OPEN_PAREN)) depth++;
                        else if (match(TokenType::CLOSE_PAREN)) depth--;
                        else advance();
                    }
                }
            }
            // Unknown — warn and skip
            else {
                std::cerr << tokens[pos-1].line << ": warning: unknown __attribute__(("
                          << attr << ")) ignored\n";
                if (peek().type == TokenType::OPEN_PAREN) {
                    int depth = 1; advance();
                    while (depth > 0 && peek().type != TokenType::END_OF_FILE) {
                        if (match(TokenType::OPEN_PAREN)) depth++;
                        else if (match(TokenType::CLOSE_PAREN)) depth--;
                        else advance();
                    }
                }
            }
        } else {
            advance(); // skip unexpected token
        }
        match(TokenType::COMMA); // optional comma between attributes
    }

    match(TokenType::CLOSE_PAREN); // inner )
    match(TokenType::CLOSE_PAREN); // outer )
    return true;
}

void Parser::parseTypedef() {
    std::string baseType;
    bool isSigned = false;
    int basePtrLevel = 0;
    std::vector<int> typedefArrayDims;

    {
        auto ts = parseTypeSpecifier();
        if (!ts.valid) {
            throw std::runtime_error("Expected type in typedef");
        }
        baseType = ts.name;
        isSigned = ts.isSigned;
        basePtrLevel = ts.pointerLevel;
        typedefArrayDims = ts.arrayDims;
    }
    // Skip __attribute__ after type: typedef short __attribute__((...)) T;
    while (tryParseAttribute()) {}

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

    // Skip __attribute__ before alias name: typedef int __attribute__((aligned)) T;
    while (tryParseAttribute()) {}

    std::string newName = expect(TokenType::IDENTIFIER, "Expected alias name in typedef").value;
    std::vector<int> arrayDims = typedefArrayDims;
    while (match(TokenType::OPEN_SQUARE)) {
        if (match(TokenType::CLOSE_SQUARE)) {
            arrayDims.push_back(0);
        } else {
            auto szExpr = parseExpression();
            int arrSize = 1;
            if (auto* lit = dynamic_cast<IntegerLiteral*>(szExpr.get())) arrSize = (int)lit->value;
            arrayDims.push_back(arrSize);
            expect(TokenType::CLOSE_SQUARE, "Expected ']'");
        }
    }
    // Skip __attribute__ after alias: typedef int T __attribute__((unused));
    while (tryParseAttribute()) {}
    expect(TokenType::SEMICOLON, "Expected ';' after typedef");

    typedefs[newName] = {baseType, ptrLevel, isSigned, false, nullptr, arrayDims};
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
            {
                auto ts = parseTypeSpecifier();
                if (!ts.valid) {
                    throw std::runtime_error("Expected type in function pointer parameter list");
                }
                fp.type = ts.name;
                fp.isSigned = ts.isSigned;
                fp.pointerLevel = ts.pointerLevel;
            }
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

bool Parser::isFunctionDeclaration() {
    size_t look = pos;
    // Skip common leading keywords/qualifiers
    while (look < tokens.size()) {
        TokenType t = tokens[look].type;
        if (t == TokenType::STATIC || t == TokenType::INLINE || t == TokenType::VOLATILE ||
            t == TokenType::CONST || t == TokenType::EXTERN || t == TokenType::NORETURN ||
            t == TokenType::FASTCALL || t == TokenType::INTERRUPT || t == TokenType::NAKED ||
            t == TokenType::REGPARM || t == TokenType::RESTRICT || t == TokenType::AUTO ||
            t == TokenType::REGISTER || t == TokenType::EXTENSION) {
            look++;
        } else if (t == TokenType::ATTRIBUTE) {
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
        } else break;
    }

    if (look >= tokens.size()) return false;
    TokenType t = tokens[look].type;
    if (t == TokenType::INT || t == TokenType::CHAR || t == TokenType::LONG ||
        t == TokenType::SHORT || t == TokenType::VOID || t == TokenType::BOOL ||
        t == TokenType::FLOAT || t == TokenType::DOUBLE || t == TokenType::COMPLEX || t == TokenType::INT_N || t == TokenType::UINT_N ||
        t == TokenType::STRUCT || t == TokenType::UNION || t == TokenType::ENUM ||
        (t == TokenType::IDENTIFIER && isTypedef(tokens[look].value))) {

        look++;
        // Skip _Complex base type (float, double, int, long double)
        if (t == TokenType::COMPLEX && look < tokens.size()) {
            TokenType ct = tokens[look].type;
            if (ct == TokenType::FLOAT || ct == TokenType::DOUBLE || ct == TokenType::INT ||
                ct == TokenType::LONG || ct == TokenType::SHORT || ct == TokenType::UNSIGNED ||
                ct == TokenType::SIGNED) {
                look++;
                // long double after _Complex
                if (ct == TokenType::LONG && look < tokens.size() && tokens[look].type == TokenType::DOUBLE) look++;
            }
        }
        // Skip __int(N) / __uint(N) parameter
        if ((t == TokenType::INT_N || t == TokenType::UINT_N) && look < tokens.size()) {
            if (tokens[look].type == TokenType::OPEN_PAREN) {
                look++; // (
                if (look < tokens.size()) look++; // N
                if (look < tokens.size() && tokens[look].type == TokenType::CLOSE_PAREN) look++; // )
            }
        }
        // Skip struct/union/enum name
        if (t == TokenType::STRUCT || t == TokenType::UNION || t == TokenType::ENUM) {
            if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER) look++;
        }
        
        // Skip any pointers, qualifiers, and attributes interleaved
        while (look < tokens.size()) {
            if (tokens[look].type == TokenType::STAR) {
                look++;
            } else if (tokens[look].type == TokenType::CONST || tokens[look].type == TokenType::VOLATILE ||
                       tokens[look].type == TokenType::RESTRICT || tokens[look].type == TokenType::EXTENSION) {
                look++;
            } else if (tokens[look].type == TokenType::ATTRIBUTE) {
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
            } else break;
        }
        
        // Check for identifier followed by (
        if (look + 1 < tokens.size() && tokens[look].type == TokenType::IDENTIFIER &&
            tokens[look+1].type == TokenType::OPEN_PAREN) {
            return true;
        }
        // Check for operator overloading: operator+ ( , operator== ( , etc.
        if (look + 2 < tokens.size() && tokens[look].type == TokenType::IDENTIFIER &&
            tokens[look].value == "operator" && tokens[look+2].type == TokenType::OPEN_PAREN) {
            return true;
        }
        // Unary operators: operator++, operator-- (two-char tokens + paren)
        if (look + 2 < tokens.size() && tokens[look].type == TokenType::IDENTIFIER &&
            tokens[look].value == "operator") {
            // Skip the operator symbol(s) and check for (
            size_t opLook = look + 1;
            // Skip one or two operator tokens (++, --, etc.)
            if (opLook < tokens.size() && tokens[opLook].type != TokenType::OPEN_PAREN) opLook++;
            if (opLook < tokens.size() && tokens[opLook].type != TokenType::OPEN_PAREN) opLook++;
            if (opLook < tokens.size() && tokens[opLook].type == TokenType::OPEN_PAREN) return true;
        }
    }
    return false;
}
