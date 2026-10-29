#include "FarPointer.hpp"
#include "FixedType.hpp"
#include "StructLayout.hpp"
#include <cstring>
#include <algorithm>
#include <map>
#include <functional>
#include "Parser.hpp"
#include "Diagnostic.hpp"
#include <stdexcept>
#include <iostream>
#include <cctype>

// C11 lets an alignment specifier appear anywhere among the declaration
// specifiers (_Alignas(8) static int x; static int _Alignas(8) x;). The
// declaration parsers accept it right after the storage class: move each
// _Alignas(...) group there, within its run of specifier tokens.
static void normalizeAlignas(std::vector<Token>& toks) {
    auto isSpec = [](TokenType t) {
        switch (t) {
            case TokenType::STATIC: case TokenType::EXTERN: case TokenType::REGISTER: case TokenType::AUTO:
            case TokenType::CONST: case TokenType::VOLATILE: case TokenType::RESTRICT:
            case TokenType::SIGNED: case TokenType::UNSIGNED: case TokenType::INT: case TokenType::CHAR:
            case TokenType::SHORT: case TokenType::LONG: case TokenType::FLOAT: case TokenType::DOUBLE:
            case TokenType::BOOL: case TokenType::VOID: case TokenType::INLINE: case TokenType::NORETURN:
                return true;
            default: return false;
        }
    };
    auto isStorage = [](TokenType t) {
        return t == TokenType::STATIC || t == TokenType::EXTERN || t == TokenType::REGISTER || t == TokenType::AUTO;
    };
    for (size_t i = 0; i < toks.size(); i++) {
        if (toks[i].type != TokenType::ALIGNAS) continue;
        // the group: _Alignas ( ... )
        size_t end = i + 1;
        if (end >= toks.size() || toks[end].type != TokenType::OPEN_PAREN) continue;
        int depth = 0;
        for (; end < toks.size(); end++) {
            if (toks[end].type == TokenType::OPEN_PAREN) depth++;
            else if (toks[end].type == TokenType::CLOSE_PAREN && --depth == 0) break;
        }
        if (end >= toks.size()) break;
        end++;   // one past ')'
        // the specifiers before it (struct/union/enum NAME counts as one)
        size_t start = i;
        while (start > 0) {
            TokenType t = toks[start - 1].type;
            if (isSpec(t)) { start--; continue; }
            if (t == TokenType::IDENTIFIER && start >= 2 &&
                (toks[start - 2].type == TokenType::STRUCT || toks[start - 2].type == TokenType::UNION ||
                 toks[start - 2].type == TokenType::ENUM)) { start -= 2; continue; }
            break;
        }
        size_t target = start;
        while (target < i && isStorage(toks[target].type)) target++;
        if (target == i) {
            // storage classes after it (_Alignas(8) static int): it goes after them
            size_t after = end;
            while (after < toks.size() && isStorage(toks[after].type)) after++;
            if (after > end) std::rotate(toks.begin() + i, toks.begin() + end, toks.begin() + after);
            i = after - 1;
            continue;
        }
        std::rotate(toks.begin() + target, toks.begin() + i, toks.begin() + end);
        i = end - 1;
    }
}

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {
    normalizeAlignas(this->tokens);
    // __bit is a type (#73: a _Bool; one at file scope is packed in a byte
    // with others by IRBuilder) and, followed by '(', the builtin __bit(v, n)
    // (#204): that one is renamed so it is not taken for the type
    typedefs["__bit"] = TypeAlias{"__bit", 0, false};
    // fixed point (#72): __fixed(I, F) is the type __fixedI_F (I + F bits,
    // 16 or 32, signed; IRBuilder scales constants and products);
    // __fixed8_8 and __fixed16_16 are two of them
    typedefs["__fixed8_8"] = TypeAlias{"__fixed8_8", 0, true};
    typedefs["__fixed16_16"] = TypeAlias{"__fixed16_16", 0, true};
    for (size_t i = 0; i + 5 < this->tokens.size(); i++) {
        auto& t = this->tokens;
        if (t[i].type != TokenType::IDENTIFIER || t[i].value != "__fixed" || t[i + 1].type != TokenType::OPEN_PAREN)
            continue;
        long ib = -1, fb = -1;
        if (t[i + 2].type == TokenType::INTEGER_LITERAL && t[i + 3].type == TokenType::COMMA &&
            t[i + 4].type == TokenType::INTEGER_LITERAL && t[i + 5].type == TokenType::CLOSE_PAREN) {
            try { ib = std::stol(t[i + 2].value, nullptr, 0); fb = std::stol(t[i + 4].value, nullptr, 0); } catch (...) {}
        }
        if (ib < 1 || fb < 1 || (ib + fb != 16 && ib + fb != 32))
            throw std::runtime_error(formatDiagnostic(t[i].sourceFile, t[i].line, t[i].column, Severity::Error,
                "__fixed(I, F): I integer and F fraction bits, both at least 1, I + F 16 or 32 (__fixed(8, 8), "
                "__fixed(4, 28))"));
        std::string name = "__fixed" + std::to_string(ib) + "_" + std::to_string(fb);
        typedefs[name] = TypeAlias{name, 0, true};
        t[i].value = name;
        t.erase(t.begin() + (long)i + 1, t.begin() + (long)i + 6);
    }
    for (size_t i = 0; i + 1 < this->tokens.size(); i++)
        if (this->tokens[i].type == TokenType::IDENTIFIER && this->tokens[i].value == "__bit" &&
            this->tokens[i + 1].type == TokenType::OPEN_PAREN)
            this->tokens[i].value = BIT_BUILTIN;
    // struct __unpacked Name outside a definition names the struct: drop the
    // keyword (the struct's layout comes from its definition)
    for (size_t i = 1; i + 2 < this->tokens.size(); ) {
        auto& t = this->tokens;
        if (t[i].type == TokenType::UNPACKED &&
            (t[i-1].type == TokenType::STRUCT || t[i-1].type == TokenType::UNION) &&
            t[i+1].type == TokenType::IDENTIFIER && t[i+2].type != TokenType::OPEN_BRACE &&
            t[i+2].type != TokenType::COLON && !(t[i+2].type == TokenType::IDENTIFIER && t[i+2].value == "final"))
            t.erase(t.begin() + i);
        else
            i++;
    }
    this->tokens.reserve(tokens.size() * 2 + 1024);  // room for insertTokens
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
    std::string msg = message + ". Found '" + foundStr + "' (" + peek().typeToString() + ") instead.";
    throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, msg));
}

std::unique_ptr<TranslationUnit> Parser::parse() {
    auto unit = setPos(std::make_unique<TranslationUnit>(), tokens[0]);
    while (peek().type != TokenType::END_OF_FILE) {
        pendingAlwaysInline_ = pendingNoInline_ = false;
        // Skip __extension__ and __attribute__ at top level (GCC compatibility)
        while (match(TokenType::EXTENSION) || tryParseAttribute()) {}
        // Skip stray semicolons at top level (e.g., }; after function)
        if (match(TokenType::SEMICOLON)) continue;
        if (match(TokenType::TYPEDEF)) {
            parseTypedef(); pendingAlign_ = 0; takePendingSection(false);  // aligned on a typedef is not supported
            continue;
        }
        if (peek().type == TokenType::ASM) {
            size_t asmTok = pos;
            advance();
            match(TokenType::VOLATILE); // optional volatile
            if (peek().type == TokenType::IDENTIFIER && peek().value == "goto") advance();
            expect(TokenType::OPEN_PAREN, "Expected '(' after asm");
            std::string code = expect(TokenType::STRING_LITERAL, "Expected string literal for asm code").value;
            while (peek().type == TokenType::STRING_LITERAL) code += advance().value;  // "a" "b"
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
            if (code == ".signed_default" || code == ".unsigned_default") {
                signedDefault = (code == ".signed_default");  // #pragma cc45 signed_default / unsigned_default
                continue;
            }
            if (code == ".noescape_strings" || code == ".escape_strings") {
                noescapeStrings_ = code == ".noescape_strings";   // #pragma cc45 noescape_strings / escape_strings
                continue;
            }
            if (code.rfind(".cc45_section ", 0) == 0) {
                // #pragma section("NAME"[, address]): IRBuilder records the address
                std::istringstream sec(code.substr(14));
                std::string n;
                sec >> n;
                sections_.insert(n);
            }
            flushPending(*unit);
            unit->topLevelDecls.push_back(setPos(std::make_unique<AsmStatement>(code), tokens[asmTok]));
            continue;
        }
        if (peek().type == TokenType::STATIC_ASSERT) {
            auto decl = parseStaticAssert();
            flushPending(*unit);
            unit->topLevelDecls.push_back(std::unique_ptr<Statement>(std::move(decl)));
            continue;
        }

        // static / extern / const / volatile before a tagged definition
        // (static struct S { ... } x;): taken here, they apply to the
        // variables declared with it
        bool defStatic = false, defExtern = false, defConst = false, defVolatile = false;
        {
            size_t q = pos;
            bool st = false, ex = false, co = false, vo = false;
            for (; q < tokens.size(); q++) {
                TokenType t = tokens[q].type;
                if (t == TokenType::STATIC) st = true;
                else if (t == TokenType::EXTERN) ex = true;
                else if (t == TokenType::CONST) co = true;
                else if (t == TokenType::VOLATILE) vo = true;
                else break;
            }
            if (q > pos && q + 2 < tokens.size() &&
                (tokens[q].type == TokenType::STRUCT || tokens[q].type == TokenType::UNION ||
                 tokens[q].type == TokenType::ENUM) &&
                tokens[q + 1].type == TokenType::IDENTIFIER && tokens[q + 2].type == TokenType::OPEN_BRACE) {
                pos = q;
                defStatic = st; defExtern = ex; defConst = co; defVolatile = vo;
            }
        }
        auto defQuals = [&](VariableDeclaration& vd) {
            if (defStatic) vd.isStatic = true;
            if (defExtern) vd.isExtern = true;
            if (defConst) vd.isConst = true;
            if (defVolatile) vd.isVolatile = true;
        };
        if (peek().type == TokenType::STRUCT || peek().type == TokenType::UNION || peek().type == TokenType::ENUM) {
            bool isUnion = peek().type == TokenType::UNION;
            bool isEnum = peek().type == TokenType::ENUM;
            // Check if it's a definition: struct/union/enum name { ... };
            // an enum's definition (enum [E] { ... } [var];) or declaration
            // (enum E;); anything else is a declaration of that type, a
            // variable or a function (enum E shade; enum E *p; enum E f(void);),
            // which the general path below parses (#337)
            bool enumDef = false;
            if (isEnum) {
                size_t eLook = pos + 1; // after 'enum'
                if (eLook < tokens.size() && tokens[eLook].type == TokenType::IDENTIFIER) eLook++;
                enumDef = eLook < tokens.size() &&
                          (tokens[eLook].type == TokenType::OPEN_BRACE || tokens[eLook].type == TokenType::SEMICOLON);
            }
            if (enumDef) {
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
                    defQuals(*vDecl);
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
            if (!isEnum) {
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
                        flushPending(*unit);  // nested definitions come first
                        unit->topLevelDecls.push_back(std::move(def));
                        // Parse the variable declaration using the struct type
                        int ptrLevel = 0;
                        while (tryParseAttribute()) {} // skip attributes before pointer/name
                        while (match(TokenType::STAR)) ptrLevel++;
                        std::string vName = expect(TokenType::IDENTIFIER, "Expected variable name after struct definition").value;
                        auto vDecl = std::make_unique<VariableDeclaration>(sType, vName, ptrLevel);
                        declTypes_[vName] = {sType, ptrLevel, false};
                        vDecl->isGlobal = true;
                        defQuals(*vDecl);
                        // Array dimensions: struct S { } arr[2];
                        while (match(TokenType::OPEN_SQUARE)) {
                            if (match(TokenType::CLOSE_SQUARE)) {
                                vDecl->arrayDims.push_back(0);
                            } else {
                                auto szExpr = parseExpression();
                                int sz = 1;
                                { long cv; if (evalConstInt(szExpr.get(), cv)) sz = (int)cv; }
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
                declTypes_[en] = {sType, ep, false};
                            declTypes_[en] = {sType, ep, false};
                            ev->isGlobal = true;
                            defQuals(*ev);
                            while (match(TokenType::OPEN_SQUARE)) {
                                if (match(TokenType::CLOSE_SQUARE)) ev->arrayDims.push_back(0);
                                else {
                                    auto se = parseExpression();
                                    int ss = 1;
                                    { long cv; if (evalConstInt(se.get(), cv)) ss = (int)cv; }
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
        bool isFarSpec = false;     // __far (a function: kept in far memory)

        // the storage class and function specifiers, in any order
        // (__naked static, static __naked)
        for (bool more = true; more && look < tokens.size(); ) {
            switch (tokens[look].type) {
            case TokenType::EXTERN:    isExtern = true; break;
            case TokenType::STATIC:    isStatic = true; break;
            case TokenType::NORETURN:  isNR = true; break;
            case TokenType::FASTCALL:  isFC = true; break;
            case TokenType::INTERRUPT: isInterrupt = true; break;
            case TokenType::NAKED:     isNaked = true; break;
            case TokenType::REGPARM:   isRegparm = true; break;
            case TokenType::INLINE:    isInlineFunc = true; break;
            case TokenType::FAR:       isFarSpec = true; break;
            default: more = false; continue;
            }
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
        bool isStriped = false;  // Phase 92: Track __striped keyword

        while (look < tokens.size() && (tokens[look].type == TokenType::VOLATILE || tokens[look].type == TokenType::CONST || tokens[look].type == TokenType::RESTRICT || tokens[look].type == TokenType::AUTO || tokens[look].type == TokenType::REGISTER || tokens[look].type == TokenType::INLINE || tokens[look].type == TokenType::FASTCALL ||
               tokens[look].type == TokenType::SIGNED || tokens[look].type == TokenType::UNSIGNED || tokens[look].type == TokenType::ATTRIBUTE || tokens[look].type == TokenType::EXTENSION || tokens[look].type == TokenType::STRIPED || tokens[look].type == TokenType::ZP || tokens[look].type == TokenType::ABS || tokens[look].type == TokenType::FAR || tokens[look].type == TokenType::NEAR)) {
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
            else if (tokens[look].type == TokenType::STRIPED) isStriped = true;  // Phase 92
            else if (tokens[look].type == TokenType::SIGNED) {
                if (isUnsig) throw std::runtime_error(formatDiagnostic(tokens[look].sourceFile, tokens[look].line, tokens[look].column, Severity::Error, "both 'signed' and 'unsigned' in declaration"));
                isSig = true;
            }
            else if (tokens[look].type == TokenType::UNSIGNED) {
                if (isSig) throw std::runtime_error(formatDiagnostic(tokens[look].sourceFile, tokens[look].line, tokens[look].column, Severity::Error, "both 'signed' and 'unsigned' in declaration"));
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
                   tokens[look].type == TokenType::ZP || tokens[look].type == TokenType::ABS || tokens[look].type == TokenType::FAR || tokens[look].type == TokenType::NEAR ||
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
                while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                       match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                       match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || match(TokenType::ZP) || match(TokenType::ABS) || match(TokenType::FAR) || match(TokenType::NEAR) || tryParseAttribute() || match(TokenType::EXTENSION));
                auto decl = parseVariableDeclaration(isVol, isConst, isStatic);
                if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                    vd->isGlobal = true;
                    vd->isExtern = isExtern;
                    vd->isSigned = isUnsig ? false : (vd->isSigned || isSig);  // cc45: unsigned unless declared signed
                    vd->isPlainUnsigned = vd->isPlainUnsigned && !isSig && !isUnsig && !vd->isSigned;
                    // (signed/unsigned were consumed above: a function pointer's result is what they say)
                    if (vd->funcPtrSig) vd->funcPtrSig->returnIsSigned = isUnsig ? false : (vd->funcPtrSig->returnIsSigned || isSig);
                } else if (auto* cs = dynamic_cast<CompoundStatement*>(decl.get())) {
                    for (auto& s : cs->statements) {
                        if (auto* vd2 = dynamic_cast<VariableDeclaration*>(s.get())) {
                            vd2->isGlobal = true;
                            vd2->isExtern = isExtern;
                            vd2->isStatic = isStatic;
                            vd2->isSigned = isUnsig ? false : (vd2->isSigned || isSig);
                            vd2->isPlainUnsigned = vd2->isPlainUnsigned && !isSig && !isUnsig && !vd2->isSigned;
                            if (vd2->funcPtrSig)
                                vd2->funcPtrSig->returnIsSigned = isUnsig ? false : (vd2->funcPtrSig->returnIsSigned || isSig);
                        }
                    }
                }
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
                continue;
            }

            // Check for complex declarators: (*name) or (*name(params)) → function declaration
            if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN &&
                look + 1 < tokens.size() && tokens[look + 1].type == TokenType::STAR) {
                while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                       match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                       match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) || match(TokenType::REGPARM) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                auto decl = parseFunctionDeclaration();
                if (isSig) decl->isSigned = true;  // 'signed'/'unsigned' were consumed with the qualifiers above
                    if (isUnsig) decl->isSigned = false;
                decl->isNoreturn = isNR;
                decl->isFastcall = isFC;
                decl->isInterrupt = isInterrupt;
                decl->isNaked = isNaked;
                decl->isFar = decl->isFar || isFarSpec;
                decl->isRegparm = isRegparm;
                decl->isInline = isInlineFunc;
                decl->isStatic = isStatic;
                if (isExtern) decl->isPrototype = true;
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
            }
            else if (look < tokens.size() && tokens[look].type == TokenType::IDENTIFIER) {
                look++;
                if (look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                    while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                           match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                           match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) || match(TokenType::REGPARM) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || match(TokenType::ZP) || match(TokenType::ABS) || match(TokenType::FAR) || match(TokenType::NEAR) || tryParseAttribute() || match(TokenType::EXTENSION));
                    auto decl = parseFunctionDeclaration();
                    if (isSig) decl->isSigned = true;  // 'signed'/'unsigned' were consumed with the qualifiers above
                    if (isUnsig) decl->isSigned = false;
                    decl->isNoreturn = isNR;
                    decl->isFastcall = isFC;
                    decl->isInterrupt = isInterrupt;
                    decl->isNaked = isNaked;
                    decl->isFar = decl->isFar || isFarSpec;
                    decl->isRegparm = isRegparm;
                    decl->isInline = isInlineFunc;
                    decl->isStatic = isStatic;
                    // extern functions are always prototypes (no body)
                    if (isExtern) decl->isPrototype = true;
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                } else {
                    while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                           match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                           match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
                    // NOTE: ZP/ABS/FAR are handled in parseTypeSpecifier, not here
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) || match(TokenType::REGPARM) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || match(TokenType::STRIPED) || tryParseAttribute() || match(TokenType::EXTENSION));
                    auto decl = parseVariableDeclaration(isVol, isConst, isStatic, false, isStriped);
                    if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                        vd->isGlobal = true;
                        vd->isSigned = isUnsig ? false : (vd->isSigned || isSig);
                        vd->isPlainUnsigned = vd->isPlainUnsigned && !isSig && !isUnsig && !vd->isSigned;
                        vd->isExtern = isExtern;
                        vd->isStatic = isStatic;
                    } else if (auto* cs = dynamic_cast<CompoundStatement*>(decl.get())) {
                        for (auto& s : cs->statements) {
                            if (auto* vd2 = dynamic_cast<VariableDeclaration*>(s.get())) {
                                vd2->isGlobal = true;
                                vd2->isSigned = isUnsig ? false : (vd2->isSigned || isSig);
                                vd2->isPlainUnsigned = vd2->isPlainUnsigned && !isSig && !isUnsig && !vd2->isSigned;
                                vd2->isExtern = isExtern;
                                vd2->isStatic = isStatic;
                            }
                        }
                    }
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                }
            } else {
                while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                       match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                       match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                auto decl = parseFunctionDeclaration();
                if (isSig) decl->isSigned = true;  // 'signed'/'unsigned' were consumed with the qualifiers above
                    if (isUnsig) decl->isSigned = false;
                decl->isNoreturn = isNR;
                decl->isFastcall = isFC;
                decl->isInterrupt = isInterrupt;
                decl->isNaked = isNaked;
                decl->isFar = decl->isFar || isFarSpec;
                    decl->isRegparm = isRegparm;
                    decl->isInline = isInlineFunc;
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
            }
        } else {
            // 'signed' / 'unsigned' alone before a parenthesized declarator
            // (unsigned (*fp)(long), unsigned (*t[2])(int)): 'int' is meant
            // (C17 6.7.2p2); parse it again with the keyword in place
            if ((isSig || isUnsig) && look < tokens.size() && tokens[look].type == TokenType::OPEN_PAREN) {
                Token intTok = tokens[look];
                intTok.type = TokenType::INT;
                intTok.value = "int";
                insertTokens(look, {intTok});
                continue;
            }
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
                    while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                           match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                           match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
                    while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::INLINE) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
                    auto decl = parseFunctionDeclaration();
                    if (isSig) decl->isSigned = true;  // 'signed'/'unsigned' were consumed with the qualifiers above
                    if (isUnsig) decl->isSigned = false;
                    decl->isStatic = isStatic;
                    decl->isInline = isInlineFunc;
                    flushPending(*unit);
                    unit->topLevelDecls.push_back(std::move(decl));
                    continue;
                }
                while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                       match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                       match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
                while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::REGISTER) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || match(TokenType::STRIPED) || tryParseAttribute() || match(TokenType::EXTENSION));
                auto decl = parseVariableDeclaration(isVol, isConst, isStatic, false, isStriped);
                if (auto* vd = dynamic_cast<VariableDeclaration*>(decl.get())) {
                    vd->isGlobal = true;
                    vd->isExtern = isExtern;
                    vd->isSigned = isUnsig ? false : (vd->isSigned || isSig || signedDefault); // implicit int follows the active default
                    vd->isPlainUnsigned = vd->isPlainUnsigned && !isSig && !isUnsig && !vd->isSigned;
                }
                flushPending(*unit);
                unit->topLevelDecls.push_back(std::move(decl));
                continue;
            }
            while (match(TokenType::EXTERN) || match(TokenType::STATIC) || match(TokenType::NORETURN) ||
                   match(TokenType::FASTCALL) || match(TokenType::INTERRUPT) || match(TokenType::NAKED) ||
                   match(TokenType::REGPARM) || match(TokenType::INLINE) || match(TokenType::FAR) || match(TokenType::NEAR));
            while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT) || match(TokenType::AUTO) || match(TokenType::SIGNED) || match(TokenType::UNSIGNED) || tryParseAttribute() || match(TokenType::EXTENSION));
            auto decl = parseFunctionDeclaration();
            if (isSig) decl->isSigned = true;  // 'signed'/'unsigned' were consumed with the qualifiers above
                    if (isUnsig) decl->isSigned = false;
            decl->isNoreturn = isNR;
            decl->isFastcall = isFC;
            decl->isInterrupt = isInterrupt;
            decl->isNaked = isNaked;
            decl->isFar = decl->isFar || isFarSpec;
            decl->isRegparm = isRegparm;
            decl->isInline = isInlineFunc;
            decl->isStatic = isStatic;
            flushPending(*unit);
            unit->topLevelDecls.push_back(std::unique_ptr<Statement>(std::move(decl)));
        }
    }
    if (usesJsr_) {
        // __jsr's target functions: declared, and not defined in this unit
        // (cc45's own call graph does not see a jsr in assembly; ln45 sees
        // one into another object)
        for (const auto& [name, tok] : jsrTargets_) {
            bool declared = false, defined = false;
            for (const auto& d : unit->topLevelDecls)
                if (auto* f = dynamic_cast<FunctionDeclaration*>(d.get()); f && f->name == name) {
                    declared = true;
                    if (f->body) defined = true;
                }
            if (!declared || defined)
                throw std::runtime_error(formatDiagnostic(tok.sourceFile, tok.line, tok.column, Severity::Error,
                    defined ? "'" + name + "' is a C function of this file: call it (" + name + "()), not __jsr"
                            : "the target of " + tok.value + " is an address or a function declared extern ('" + name + "' is neither)"));
        }
        // the register bytes: extern volatile, at the unit's start
        static const std::pair<const char*, const char*> regs[] = {
            {"__jsr_a", "char"}, {"__jsr_x", "char"}, {"__jsr_y", "char"}, {"__jsr_z", "char"},
            {"__jsr_c", "char"}, {"__jsr_p", "char"}, {"__jsr_ax", "int"}, {"__jsr_ay", "int"},
            {"__jsr_az", "int"}, {"__jsr_xy", "int"}, {"__jsr_q", "long"}};
        std::vector<std::unique_ptr<Statement>> decls;
        for (const auto& [n, t] : regs) {
            auto d = setPos(std::make_unique<VariableDeclaration>(t, n), tokens[0]);
            d->isGlobal = true;
            d->isExtern = true;
            d->isVolatile = true;
            decls.push_back(std::move(d));
        }
        unit->topLevelDecls.insert(unit->topLevelDecls.begin(), std::make_move_iterator(decls.begin()),
                                   std::make_move_iterator(decls.end()));
    }
    return unit;
}

// __jsr(target[, .R = value]...[, .R -> lvalue]...)[.R], __jsr_raw(...),
// __jmp(target[, .R = value]...): a call of an address with the CPU's
// registers. The values go to the register bytes (or straight into the
// assembly when constant), one inline assembly block loads the registers,
// calls (an address in the ROM, $C000 and up, through __rom_calli: the ROM's
// base page and stack) and saves the registers, and the results are read
// from the bytes:
//   ({ __jsr_a = v; asm("lda ___jsr_a\n jsr $FFD2\n sta ___jsr_a"); __jsr_a; })
std::unique_ptr<Expression> Parser::parseJsr() {
    const Token& kw = advance();
    std::string what = kw.value;
    bool isJmp = what == "__jmp", raw = what == "__jsr_raw";
    expect(TokenType::OPEN_PAREN, "Expected '(' after " + what);
    usesJsr_ = true;
    auto err = [&](const Token& t, const std::string& m) {
        return std::runtime_error(formatDiagnostic(t.sourceFile, t.line, t.column, Severity::Error, what + ": " + m));
    };
    // the target: a constant address or a function declared extern
    std::string target;
    bool rom = false;
    const Token& targetTok = peek();
    {
        auto t = parseConditional();
        long a;
        if (evalConstInt(t.get(), a)) {
            if (a < 0 || a > 0xFFFF) throw err(targetTok, "the address is not 16 bits");
            char b[8];
            snprintf(b, sizeof b, "$%04lX", a);
            target = b;
            rom = !raw && a >= 0xC000;
        } else if (auto* v = dynamic_cast<VariableReference*>(t.get())) {
            target = "_" + v->name;
            jsrTargets_.push_back({v->name, kw});
        } else {
            throw err(targetTok, "the target is not a constant address or a function name");
        }
    }
    // the registers: storage, its registers byte by byte
    struct Reg { const char* store; const char* regs; };
    static const std::map<std::string, Reg> regTable = {
        {"A", {"__jsr_a", "a"}}, {"X", {"__jsr_x", "x"}}, {"Y", {"__jsr_y", "y"}}, {"Z", {"__jsr_z", "z"}},
        {"AX", {"__jsr_ax", "ax"}}, {"AY", {"__jsr_ay", "ay"}}, {"AZ", {"__jsr_az", "az"}}, {"XY", {"__jsr_xy", "xy"}},
        {"Q", {"__jsr_q", "axyz"}}, {"AXYZ", {"__jsr_q", "axyz"}}};
    static const std::map<std::string, int> flagBits = {{"Carry", 0}, {"Zero", 1}, {"Overflow", 6}, {"Negative", 7}};
    auto body = setPos(std::make_unique<CompoundStatement>(), kw);
    std::string loads, carryIn;
    std::set<char> loaded;
    struct Out { std::string reg; std::unique_ptr<Expression> lvalue; };
    std::vector<Out> outs;
    auto readReg = [&](const std::string& r, const Token& at) -> std::unique_ptr<Expression> {
        if (auto it = regTable.find(r); it != regTable.end())
            return setPos(std::make_unique<VariableReference>(it->second.store), at);
        auto fb = flagBits.find(r);
        auto p = setPos(std::make_unique<VariableReference>("__jsr_p"), at);
        std::unique_ptr<Expression> e = std::move(p);
        if (fb->second) e = setPos(std::make_unique<BinaryOperation>(">>", std::move(e), setPos(std::make_unique<IntegerLiteral>(fb->second), at)), at);
        return setPos(std::make_unique<BinaryOperation>("&", std::move(e), setPos(std::make_unique<IntegerLiteral>(1), at)), at);
    };
    std::set<std::string> outRegs;
    // the values set before the block, by register (a lone 8-bit one may be
    // given straight to its register: __cpu.R = v, #344)
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> regValues;
    bool carryUsesA = false;
    while (match(TokenType::COMMA)) {
        expect(TokenType::DOT, "Expected '.' and a register (.A = value, .X -> variable)");
        const Token& rt = expect(TokenType::IDENTIFIER, "Expected a register after '.'");
        std::string r = rt.value;
        bool isReg = regTable.count(r), isFlag = flagBits.count(r);
        if (!isReg && !isFlag) throw err(rt, "'" + r + "' is not a register (A X Y Z AX AY AZ XY Q) or flag (Carry Zero Overflow Negative)");
        if (match(TokenType::ARROW)) {
            if (isJmp) throw err(rt, "__jmp does not return: no results");
            outs.push_back({r, parseConditional()});
            outRegs.insert(r);
            continue;
        }
        expect(TokenType::EQUALS, "Expected '=' (a value) or '->' (a result) after ." + r);
        auto v = parseConditional();
        long c;
        bool isConst = evalConstInt(v.get(), c);
        if (isFlag) {
            if (r != "Carry") throw err(rt, "only the Carry flag can be given a value");
            if (!carryIn.empty()) throw err(rt, ".Carry given twice");
            if (isConst) carryIn = c ? " sec\n" : " clc\n";
            else {
                body->statements.push_back(setPos(std::make_unique<ExpressionStatement>(setPos(std::make_unique<Assignment>(
                    setPos(std::make_unique<VariableReference>("__jsr_c"), rt), std::move(v)), rt)), rt));
                carryIn = " lda ___jsr_c\n cmp #1\n";   // (C = the value is not 0)
                carryUsesA = true;
            }
            continue;
        }
        const Reg& g = regTable.at(r);
        std::string regs = g.regs;
        for (char ch : regs)
            if (!loaded.insert(ch).second) throw err(rt, std::string("register ") + (char)toupper(ch) + " is given twice");
        if (!isConst) {
            regValues.push_back({r, std::move(v)});
            continue;
        }
        for (size_t i = 0; i < regs.size(); i++) {
            std::string ld = std::string(" ld") + regs[i];
            if (isConst) {
                char b[16];
                snprintf(b, sizeof b, " #$%02lX", (c >> (8 * i)) & 0xFF);
                loads += ld + b + "\n";
            } else {
                loads += ld + " ___" + std::string(g.store).substr(2) + (i ? "+" + std::to_string(i) : "") + "\n";
            }
        }
    }
    expect(TokenType::CLOSE_PAREN, "Expected ')' after " + what);
    // the values computed before the block: through the register bytes, or
    // one 8-bit register (nothing else loaded through A) directly
    bool direct = regValues.size() == 1 && regValues[0].first.size() == 1 && !carryUsesA;
    for (auto& [r, v] : regValues) {
        const Reg& g = regTable.at(r);
        if (direct) {
            body->statements.push_back(setPos(std::make_unique<ExpressionStatement>(setPos(std::make_unique<Assignment>(
                setPos(std::make_unique<CpuRegisterAccess>(r), kw), std::move(v)), kw)), kw));
            continue;
        }
        body->statements.push_back(setPos(std::make_unique<ExpressionStatement>(setPos(std::make_unique<Assignment>(
            setPos(std::make_unique<VariableReference>(g.store), kw), std::move(v)), kw)), kw));
        std::string regs = g.regs;
        for (size_t i = 0; i < regs.size(); i++)
            loads += std::string(" ld") + regs[i] + " ___" + std::string(g.store).substr(2) + (i ? "+" + std::to_string(i) : "") + "\n";
    }
    // .R after the call: its value
    std::string resultReg;
    Token resultTok = kw;
    if (!isJmp && peek().type == TokenType::DOT && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::IDENTIFIER &&
        (regTable.count(tokens[pos + 1].value) || flagBits.count(tokens[pos + 1].value))) {
        advance();
        resultTok = advance();
        resultReg = resultTok.value;
    }
    // the registers saved after the call: those asked for
    std::set<std::string> saves = outRegs;
    if (!resultReg.empty()) saves.insert(resultReg);
    std::string stores;
    bool flags = false;
    std::set<std::string> stored;
    for (const auto& r : saves) {
        if (flagBits.count(r)) { flags = true; continue; }
        const Reg& g = regTable.at(r);
        if (!stored.insert(g.store).second) continue;
        std::string regs = g.regs;
        for (size_t i = 0; i < regs.size(); i++)
            stores += std::string(" st") + regs[i] + " ___" + std::string(g.store).substr(2) + (i ? "+" + std::to_string(i) : "") + "\n";
    }
    if (flags) stores = " php\n" + stores + " pla\n sta ___jsr_p\n";
    // the call
    std::string code;
    if (rom && !isJmp) {
        // (the routine's address in the word after the JSR)
        code = carryIn + loads + " jsr __rom_calli\n .word " + target + "\n" + stores;
    } else {
        std::string ext = target[0] == '_' ? " .extern " + target + "\n" : "";
        code = ext + carryIn + loads + (isJmp && rom ? " jsr __bp_rom\n" : "") + (isJmp ? " jmp " : " jsr ") + target + "\n" + stores;
    }
    std::string text = code;
    while (!text.empty() && text.back() == '\n') text.pop_back();
    body->statements.push_back(setPos(std::make_unique<AsmStatement>(text), kw));
    for (auto& o : outs)
        body->statements.push_back(setPos(std::make_unique<ExpressionStatement>(setPos(std::make_unique<Assignment>(
            std::move(o.lvalue), readReg(o.reg, kw)), kw)), kw));
    auto se = setPos(std::make_unique<StatementExpression>(), kw);
    se->body = std::move(body);
    if (!resultReg.empty()) se->result = readReg(resultReg, resultTok);
    return se;
}

void Parser::insertTokens(size_t at, const std::vector<Token>& v) {
    if (tokens.size() + v.size() > tokens.capacity())
        throw std::runtime_error(formatDiagnostic(tokens[at].sourceFile, tokens[at].line, tokens[at].column, Severity::Error,
            "too many declarations with several declarators in this file; declare these functions separately"));
    tokens.insert(tokens.begin() + at, v.begin(), v.end());
}

std::unique_ptr<FunctionDeclaration> Parser::parseFunctionDeclaration() {
    const Token& startToken = peek();
    // Start of the declaration's specifiers: the caller has consumed the
    // storage class and qualifiers already (static, extern, signed, ...)
    size_t declStart = pos;
    while (declStart > 0) {
        TokenType t = tokens[declStart - 1].type;
        if (t != TokenType::STATIC && t != TokenType::EXTERN && t != TokenType::INLINE &&
            t != TokenType::SIGNED && t != TokenType::UNSIGNED && t != TokenType::CONST &&
            t != TokenType::VOLATILE && t != TokenType::RESTRICT && t != TokenType::NORETURN &&
            t != TokenType::FASTCALL && t != TokenType::INTERRUPT && t != TokenType::NAKED &&
            t != TokenType::REGPARM && t != TokenType::EXTENSION && t != TokenType::REGISTER &&
            t != TokenType::AUTO && t != TokenType::ZP && t != TokenType::ABS && t != TokenType::FAR && t != TokenType::NEAR) break;
        declStart--;
    }
    bool isNR = match(TokenType::NORETURN);
    bool isFC = match(TokenType::FASTCALL);
    bool isInterrupt = match(TokenType::INTERRUPT);
    bool isNaked = match(TokenType::NAKED);
    bool isRegparm = match(TokenType::REGPARM);
    bool isInlineFunc = match(TokenType::INLINE);
    bool isStatic = match(TokenType::STATIC);
    bool isFar = false;
    bool farReturn = false;   // T __far *f(): returns a far pointer (#68)
    bool nearReturn = false;  // T __near *f(): returns a near pointer
    std::string returnType;
    bool isSigned = false;
    int basePtrLevel = 0;

    // Skip any extra qualifiers that might precede the return type (e.g., inline static before type)
    while (match(TokenType::INLINE) || match(TokenType::STATIC) || match(TokenType::FAR)) {
        if (tokens[pos-1].type == TokenType::INLINE) isInlineFunc = true;
        if (tokens[pos-1].type == TokenType::STATIC) isStatic = true;
        if (tokens[pos-1].type == TokenType::FAR) isFar = true;
    }
    {
        auto ts = parseTypeSpecifier();
        // (__far among the type's qualifiers: of the function; after the
        // type of a pointer it returns, T __far *f(): a far pointer)
        if (ts.valid && ts.addressSpace == 3 && !ts.farAfterType) isFar = true;
        if (ts.valid && ts.farAfterType) farReturn = true;   // (int __far f(): a far function)
        if (ts.valid && ts.addressSpace == 4) nearReturn = true;
        if (ts.valid) {
            returnType = ts.name;
            isSigned = ts.isSigned;
            basePtrLevel = ts.pointerLevel;
        } else if (peek().type == TokenType::IDENTIFIER) {
            // C89 implicit int: function declared without return type defaults to int
            returnType = "int";
        } else {
            std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
            throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected return type (int, char, void, struct, union) for function declaration. Found '" + foundStr + "' instead."));
        }
    }
    // Skip qualifiers after the type keyword (e.g., int volatile func())
    while (match(TokenType::INLINE) || match(TokenType::STATIC)) {}
    // The specifiers shared by every declarator of the declaration
    size_t specEnd = pos;
    while (specEnd > declStart && tokens[specEnd - 1].type == TokenType::STAR) specEnd--;

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
    bool isComplexDeclarator = false;

    // Handle complex declarators: (*name)(params) or (*name(params))(result)
    // Pattern: type (*func_name(actual_params))(returned_fp_params)
    // The (actual_params) are the function's own parameters
    // The (returned_fp_params) describe what the function returns
    if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::STAR) {
        isComplexDeclarator = true;
        advance(); // consume '('
        advance(); // consume '*'
        while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT)) {}
        name = expect(TokenType::IDENTIFIER, "Expected function name").value;

        // If next is '(', parse the actual function parameters (e.g., (void) in (*make_fp(void)))
        std::vector<Parameter> actualParams;
        if (peek().type == TokenType::OPEN_PAREN) {
            advance(); // consume '('
            if (peek().type == TokenType::VOID && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::CLOSE_PAREN) {
                advance(); // consume 'void'
            } else if (peek().type != TokenType::CLOSE_PAREN) {
                // Parse actual parameters
                do {
                    std::string pType = "int";  // fallback
                    std::string pName = "__param";
                    Parameter p = {pType, 0, false, pName, false, false, false, false, nullptr};
                    actualParams.push_back(p);
                } while (match(TokenType::COMMA) && peek().type != TokenType::CLOSE_PAREN);
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')' after function parameters");
        }

        expect(TokenType::CLOSE_PAREN, "Expected ')' after function name");
    }
    else if (peek().type == TokenType::IDENTIFIER && peek().value == "operator") {
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

    std::vector<Parameter> params;

    // For complex declarators, parameters were already parsed above
    // The next parens (e.g., (int, int)) are the signature of returned function pointer
    if (!isComplexDeclarator) {
        expect(TokenType::OPEN_PAREN, "Expected '('");
        // Handle (void) as empty parameter list
        if (peek().type == TokenType::VOID && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::CLOSE_PAREN) {
            advance(); // consume 'void'
        }
        else if (peek().type != TokenType::CLOSE_PAREN) {
        auto markParamPos = [](Parameter& p, const Token& t) {
            p.sourceFile = t.sourceFile; p.line = t.line; p.column = t.column;
        };
        do {
            bool pIsVolatile = false;
            bool pIsConst = false;
            std::string pType;
            bool pIsSigned = false;
            int pBasePtrLevel = 0;
            bool pBaseRestrict = false;   // T *restrict (the type's last star)
            int pSpace = 0;
            std::vector<int> pTypedefDims;   // a typedef'd array type (typedef T pair[2])
            int pTypeStars = 0;              // the '*'s parseTypeSpecifier took

            {
                auto ts = parseTypeSpecifier();
                if (ts.valid) {
                    pSpace = ts.addressSpace;
                    pType = ts.name;
                    pIsSigned = ts.isSigned;
                    pBasePtrLevel = ts.pointerLevel;
                    pBaseRestrict = ts.lastStarRestrict && ts.starsParsed > 0;
                    if (ts.isVolatile) pIsVolatile = true;
                    if (ts.isConst) pIsConst = true;
                    if (!ts.arrayDims.empty()) {
                        pBasePtrLevel++; // array dims decay to pointer
                        pTypedefDims = ts.arrayDims;
                        pTypeStars = ts.starsParsed;
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
                        markParamPos(p, tokens[pos - 1]);
                        params.push_back(p);
                        continue;
                    }
                }
                else if (peek().type == TokenType::IDENTIFIER) {
                    // K&R style: parameter is just a name, type declared after ')'
                    std::string pName = advance().value;
                    params.push_back({"int", 0, false, pName, pIsVolatile, pIsConst, false, false, nullptr});
                    markParamPos(params.back(), tokens[pos - 1]);
                    continue; // skip the pointer/name parsing below
                }
                else {
                    std::string foundStr = peek().value.empty() ? peek().typeToString() : peek().value;
                    throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected parameter type (int, char, struct, union). Found '" + foundStr + "' instead."));
                }
            }

            int pPtrLevel = pBasePtrLevel;

            // A pointer to an array: T (*p)[N]..., T (__far *p)[N] (#347): p[i]
            // is a row of N
            if (peek().type == TokenType::OPEN_PAREN && pos + 2 < tokens.size() &&
                (tokens[pos + 1].type == TokenType::STAR ||
                 ((tokens[pos + 1].type == TokenType::FAR || tokens[pos + 1].type == TokenType::NEAR) &&
                  tokens[pos + 2].type == TokenType::STAR))) {
                size_t save = pos;
                advance();   // '('
                int space = pSpace;
                if (match(TokenType::FAR)) space = 3;
                else if (match(TokenType::NEAR)) space = 4;
                advance();   // '*'
                bool ptrConst = false;
                while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT)
                    if (advance().type == TokenType::CONST) ptrConst = true;
                const Token& nameTok = peek();
                std::string pName = peek().type == TokenType::IDENTIFIER ? advance().value : "__unnamed_" + std::to_string(pos);
                if (match(TokenType::CLOSE_PAREN) && peek().type == TokenType::OPEN_SQUARE) {
                    std::vector<int> dims;
                    while (match(TokenType::OPEN_SQUARE)) {
                        auto szExpr = parseExpression();
                        long cv = 1;
                        if (!evalConstInt(szExpr.get(), cv) || cv <= 0)
                            throw std::runtime_error(formatDiagnostic(nameTok.sourceFile, nameTok.line, nameTok.column,
                                Severity::Error, "a pointer to an array needs constant dimensions"));
                        dims.push_back((int)cv);
                        expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                    }
                    std::string t = pType;
                    int lvl = pPtrLevel - (pTypedefDims.empty() ? 0 : 1) + 1;
                    dims.insert(dims.end(), pTypedefDims.begin(), pTypedefDims.end());
                    Parameter p;
                    bool far = (space == 3 || space == 4) && lvl == 1;
                    if (far) { t = farArrayType(t, dims); farPointer(t, lvl, space, pIsSigned); }
                    p.type = t;
                    p.pointerLevel = lvl;
                    p.isSigned = pIsSigned;
                    p.name = pName;
                    p.isVolatile = pIsVolatile;
                    p.isConst = far ? ptrConst : pIsConst;
                    p.isPointerConst = far ? false : ptrConst;
                    if (!far) p.innerArrayDims = dims;
                    markParamPos(p, nameTok);
                    params.push_back(p);
                    continue;
                }
                pos = save;   // (a function pointer)
            }

            // Check for function pointer parameter: type (*name)(params), also
            // with the declarator in extra parentheses: type ((*name)(params))
            size_t fpParens = 0;
            while (pos + fpParens < tokens.size() && tokens[pos + fpParens].type == TokenType::OPEN_PAREN) fpParens++;
            if (fpParens > 0 && pos + fpParens < tokens.size() && tokens[pos + fpParens].type == TokenType::STAR) {
                for (size_t k = 0; k < fpParens; k++) advance(); // the '('s
                advance(); // consume '*'
                std::string pName;
                const Token& fpNameTok = peek();
                if (peek().type == TokenType::IDENTIFIER) {
                    pName = advance().value;
                } else {
                    pName = "__unnamed_fp_" + std::to_string(pos);
                }
                expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer name");
                auto sig = parseFuncPtrParams(pType, pPtrLevel, pIsSigned);
                for (size_t k = 1; k < fpParens; k++)
                    expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer declarator");
                Parameter p;
                p.type = "void";
                p.pointerLevel = 1;
                p.isSigned = false;
                p.name = pName;
                p.isVolatile = pIsVolatile;
                p.isConst = pIsConst;
                p.isFunctionPointer = true;
                p.funcPtrSig = sig;
                markParamPos(p, fpNameTok);
                params.push_back(p);
                continue;
            }

            while (match(TokenType::STAR)) pPtrLevel++;
            // pair *p (typedef T pair[N]): a pointer to the array, not the
            // decayed T ** (#347); without '*' the array parameter decays to a
            // pointer to its rows
            std::vector<int> pRowDims;
            if (!pTypedefDims.empty()) {
                if (pPtrLevel - pBasePtrLevel + pTypeStars > 0) {
                    pPtrLevel--;
                    if ((pSpace == 3 || pSpace == 4) && pPtrLevel == 1) pType = farArrayType(pType, pTypedefDims);
                    else if (pPtrLevel == 1) pRowDims = pTypedefDims;
                } else {
                    pRowDims.assign(pTypedefDims.begin() + 1, pTypedefDims.end());
                }
            }

            // Handle const/volatile after * (qualifies the pointer itself)
            bool pIsPointerConst = false, pPtrVolatile = false, pRestrict = false;
            while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
                if (match(TokenType::CONST)) pIsPointerConst = true;
                else if (match(TokenType::VOLATILE)) pPtrVolatile = true;
                else if (match(TokenType::RESTRICT)) pRestrict = true;
            }
            // volatile T *p: what p points to is volatile, not p
            if (pPtrLevel > pBasePtrLevel) pIsVolatile = pPtrVolatile;

            std::string pName;
            std::shared_ptr<Expression> pVlaSize;  // int a[n++]
            std::vector<int> pInnerDims;           // int a[][M]
            std::vector<std::shared_ptr<Expression>> pInnerDimExprs;
            const Token& pNameTok = peek();
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
                // The dimension (may be empty for []): only a non-constant one
                // matters, for its side effects (int a[n++])
                // int a[static 10], int a[restrict], int a[const restrict 4]
                while (match(TokenType::STATIC) || match(TokenType::CONST) || match(TokenType::VOLATILE) ||
                       match(TokenType::RESTRICT))
                    if (tokens[pos - 1].type == TokenType::RESTRICT) pRestrict = true;
                if (peek().type != TokenType::CLOSE_SQUARE) {
                    auto dimExpr = parseExpression();
                    long cv;
                    if (dimExpr && !evalConstInt(dimExpr.get(), cv)) pVlaSize = std::move(dimExpr);
                }
                expect(TokenType::CLOSE_SQUARE, "Expected ']' in array parameter");
                // Additional dimensions: int a[][M], int a[2][3] — kept when
                // constant (the parameter points to an array of them)
                bool knownDims = true;
                while (peek().type == TokenType::OPEN_SQUARE) {
                    advance(); // consume '['
                    if (peek().type != TokenType::CLOSE_SQUARE) {
                        auto dimExpr = parseExpression();
                        long cv;
                        if (dimExpr && evalConstInt(dimExpr.get(), cv) && cv > 0) {
                            pInnerDims.push_back((int)cv);
                            pInnerDimExprs.push_back(nullptr);
                        } else if (dimExpr) {
                            pInnerDims.push_back(0);  // run-time dimension
                            pInnerDimExprs.push_back(std::shared_ptr<Expression>(std::move(dimExpr)));
                        } else {
                            knownDims = false;
                        }
                    } else {
                        knownDims = false;
                    }
                    expect(TokenType::CLOSE_SQUARE, "Expected ']' in array parameter");
                }
                if (!knownDims) { pInnerDims.clear(); pInnerDimExprs.clear(); }
            }

            // Skip __attribute__ after array params
            while (tryParseAttribute()) {}

            if (farPointer(pType, pPtrLevel, pSpace, pIsSigned) && pPtrLevel == 0) {
                pIsConst = pIsPointerConst;   // (T __far *const p)
                pIsVolatile = pIsPointerConst = false;
            }
            params.push_back({pType, pPtrLevel, pIsSigned, pName, pIsVolatile, pIsConst, pIsPointerConst, false, nullptr});
            params.back().isRestrict = (pRestrict || (pBaseRestrict && pPtrLevel == pBasePtrLevel)) && pPtrLevel > 0;
            params.back().vlaSizeExpr = std::move(pVlaSize);
            params.back().innerArrayDims = pInnerDims;
            params.back().innerDimExprs = pInnerDimExprs;
            if (pInnerDims.empty() && !pRowDims.empty() && params.back().pointerLevel == 1)
                params.back().innerArrayDims = pRowDims;
            markParamPos(params.back(), pNameTok);
        } while (match(TokenType::COMMA) && peek().type != TokenType::ELLIPSIS);
        }
    }
    bool isVariadic = match(TokenType::ELLIPSIS);
    if (!isComplexDeclarator) {
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
    }
    // For complex declarators, the return type params come next (e.g., (int, int))

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
    // int f(void), g(void); — a prototype followed by more declarators
    bool moreDeclarators = peek().type == TokenType::COMMA;
    if (moreDeclarators || match(TokenType::SEMICOLON)) {
        if (moreDeclarators) advance();
        // In a function: the forward declaration of a nested function that
        // the same function defines further on (GCC writes auto int h(int);)
        std::string protoName = name;
        if (currentFunction) {
            int depth = 0;
            for (size_t i = pos; i + 1 < tokens.size(); i++) {
                TokenType tt = tokens[i].type;
                if (tt == TokenType::OPEN_BRACE) depth++;
                else if (tt == TokenType::CLOSE_BRACE) { if (--depth < 0) break; }
                else if (depth == 0 && tt == TokenType::IDENTIFIER && tokens[i].value == name &&
                         tokens[i + 1].type == TokenType::OPEN_PAREN) {
                    // name ( ... ) { : its definition
                    size_t j = i + 2;
                    int paren = 1;
                    while (j < tokens.size() && paren > 0) {
                        if (tokens[j].type == TokenType::OPEN_PAREN) paren++;
                        else if (tokens[j].type == TokenType::CLOSE_PAREN) paren--;
                        j++;
                    }
                    if (j < tokens.size() && tokens[j].type == TokenType::OPEN_BRACE) {
                        protoName = currentFunctionName + "_" + name;
                        nestedFunctionMap[name] = protoName;
                        break;
                    }
                }
            }
        }
        if (farReturn && returnPtrLevel == 0) isFar = true;
        else if (farReturn) { int sp = 3; farPointer(returnType, returnPtrLevel, sp, isSigned); }
        if (nearReturn) { int sp = 4; farPointer(returnType, returnPtrLevel, sp, isSigned); }
        auto func = setPos(std::make_unique<FunctionDeclaration>(protoName, returnType), startToken);
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
        func->isAlwaysInline = pendingAlwaysInline_;
        func->alignment = takePendingAlign();
        func->purity = pendingPurity_; pendingPurity_ = 0;
        pendingNoescape_ = false;   // (on a function: no effect)
        applyFunctionSection(*func);
        applyAlias(func->name);
        func->isNoInline = pendingNoInline_ || !func->section.empty();
        pendingAlwaysInline_ = pendingNoInline_ = false;
        // The next declarator is parsed as a declaration of its own: repeat
        // the specifiers in front of it (startToken is not used after this)
        if (moreDeclarators) {
            std::vector<Token> spec(tokens.begin() + declStart, tokens.begin() + specEnd);
            insertTokens(pos, spec);
        }
        return func;
    }

    std::string fullName = name;
    if (currentFunction) {
        fullName = currentFunctionName + "_" + name;
        nestedFunctionMap[name] = fullName;
    }

    if (farReturn && returnPtrLevel == 0) isFar = true;
    else if (farReturn) { int sp = 3; farPointer(returnType, returnPtrLevel, sp, isSigned); }
    if (nearReturn) { int sp = 4; farPointer(returnType, returnPtrLevel, sp, isSigned); }
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
    func->isAlwaysInline = pendingAlwaysInline_;
    func->alignment = takePendingAlign();
    func->purity = pendingPurity_; pendingPurity_ = 0;
    pendingNoescape_ = false;   // (on a function: no effect)
    applyFunctionSection(*func);
    applyAlias(func->name);
    func->isNoInline = pendingNoInline_ || !func->section.empty();
    pendingAlwaysInline_ = pendingNoInline_ = false;
    func->isStatic = isStatic;
    func->isFar = isFar || func->bank >= 0;

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

    {
        EnumScope paramScope(*this);   // the parameters hide enum constants
        for (const auto& prm : func->parameters) hideEnumConstant(prm.name);
        func->body = parseCompoundStatement();
    }

    currentFunctionName = savedFunctionName;
    currentFunction = savedFunction;
    nestedFunctionMap = savedNestedMap;
    return func;
}

std::unique_ptr<CompoundStatement> Parser::parseCompoundStatement() {
    const Token& startToken = expect(TokenType::OPEN_BRACE, "Expected '{'");
    auto compound = setPos(std::make_unique<CompoundStatement>(), startToken);
    EnumScope enumScope(*this);
    while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE) {
        size_t pendingBefore = pendingDefinitions.size();
        auto stmt = parseStatement();
        // A struct with run-time member sizes (char x[n]) defined by this
        // statement stays here, before it: its sizes are evaluated where it is
        for (size_t i = pendingBefore; i < pendingDefinitions.size();) {
            auto* sd = dynamic_cast<StructDefinition*>(pendingDefinitions[i].get());
            bool vm = false;
            if (sd) for (const auto& m : sd->members) for (const auto& d : m.vlaDimExprs) if (d) vm = true;
            if (vm) {
                compound->statements.push_back(std::unique_ptr<Statement>(static_cast<Statement*>(pendingDefinitions[i].release())));
                pendingDefinitions.erase(pendingDefinitions.begin() + i);
            } else {
                i++;
            }
        }
        compound->statements.push_back(std::move(stmt));
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

    // extern in a block: the declaration, of a file-scope name (a function's
    // is a prototype as without it)
    if (match(TokenType::EXTERN)) {
        auto st = parseStatement();
        std::function<void(Statement*)> mark = [&](Statement* s) {
            if (auto* vd = dynamic_cast<VariableDeclaration*>(s)) vd->isExtern = true;
            else if (auto* cs = dynamic_cast<CompoundStatement*>(s); cs && cs->isDeclGroup)
                for (auto& x : cs->statements) mark(x.get());
        };
        mark(st.get());
        return st;
    }

    if (peek().type == TokenType::IDENTIFIER && pos + 1 < tokens.size() && tokens[pos+1].type == TokenType::COLON) {
        std::string label = advance().value;
        advance(); // :
        return setPos(std::make_unique<LabelledStatement>(label, parseStatement()), tokens[pos-2]);
    }

    if (match(TokenType::TYPEDEF)) {
        parseTypedef(); pendingAlign_ = 0; takePendingSection(false);  // aligned on a typedef is not supported
        auto group = setPos(std::make_unique<CompoundStatement>(), tokens[pos-1]);
        group->isDeclGroup = true;
        // typedef int T[n]: n evaluated here
        for (auto& d : typedefVlaDecls_) group->statements.push_back(std::move(d));
        typedefVlaDecls_.clear();
        return group;
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
    bool isStriped = false;         // Phase 92: Striped array support
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
        } else if (match(TokenType::STRIPED)) {
            isStriped = true;
        } else if (match(TokenType::FASTCALL)) {
            // consumed; handled at function declaration level
        } else if ((isStatic || isConst || isVolatile) && tryParseAttribute()) {
            // static __attribute__((...)) / __section("X") char x; (the
            // declaration takes it)
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
            declTypes_[varName] = {sType, ptrLevel, false};
            hideEnumConstant(varName);
            vDecl->isVolatile = isVolatile;
            vDecl->isConst = isConst;
            vDecl->isStatic = isStatic;
            vDecl->isStriped = isStriped;
            // Array dimensions
            while (match(TokenType::OPEN_SQUARE)) {
                if (match(TokenType::CLOSE_SQUARE)) {
                    vDecl->arrayDims.push_back(0);
                } else {
                    auto szExpr = parseExpression();
                    int sz = 1;
                    { long cv; if (evalConstInt(szExpr.get(), cv)) sz = (int)cv; }
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
                hideEnumConstant(en);
                ev->isVolatile = isVolatile;
                ev->isConst = isConst;
                ev->isStatic = isStatic;
                while (match(TokenType::OPEN_SQUARE)) {
                    if (match(TokenType::CLOSE_SQUARE)) ev->arrayDims.push_back(0);
                    else {
                        auto se = parseExpression();
                        int ss = 1;
                        { long cv; if (evalConstInt(se.get(), cv)) ss = (int)cv; }
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
            compound->isDeclGroup = true;
            compound->statements.push_back(std::move(vDecl));
            for (auto& ev : extraVars) compound->statements.push_back(std::move(ev));
            return compound;
        }
        return parseVariableDeclaration(isVolatile, isConst, isStatic, isRegister, isStriped);
    }

    if (peek().type == TokenType::ALIGNAS || peek().type == TokenType::INT || peek().type == TokenType::SHORT || peek().type == TokenType::LONG || peek().type == TokenType::CHAR || peek().type == TokenType::BOOL ||
        peek().type == TokenType::VOID || peek().type == TokenType::TYPEOF ||
        peek().type == TokenType::FLOAT || peek().type == TokenType::DOUBLE || peek().type == TokenType::COMPLEX ||
        peek().type == TokenType::INT_N || peek().type == TokenType::UINT_N ||
        peek().type == TokenType::UNSIGNED || peek().type == TokenType::SIGNED || peek().type == TokenType::ZP ||
        peek().type == TokenType::FAR || peek().type == TokenType::NEAR ||
        (peek().type == TokenType::IDENTIFIER && isTypedef(peek().value))) {
        if (isFunctionDeclaration()) {
            return parseFunctionDeclaration();
        }
        return parseVariableDeclaration(isVolatile, isConst, isStatic, isRegister, isStriped);
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
        // break N: leave N enclosing loops/switches (break 1 is break)
        int levels = 1;
        if (peek().type == TokenType::INTEGER_LITERAL) {
            const Token& n = advance();
            unsigned long long v = 0;
            try { v = std::stoull(n.value, nullptr, 0); } catch (...) { v = 0; }
            if (v < 1 || v > 255)
                throw std::runtime_error(formatDiagnostic(n.sourceFile, n.line, n.column, Severity::Error,
                    "the level of 'break " + n.value + "' must be from 1 to 255"));
            levels = (int)v;
        }
        expect(TokenType::SEMICOLON, "Expected ';' after break");
        return setPos(std::make_unique<BreakStatement>(levels), startToken);
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
        // the condition's hint (!__likely(x): expected false)
        Expression* hinted = condition.get();
        int nots = 0;
        while (!hinted->branchHint) {
            auto* u = dynamic_cast<UnaryOperation*>(hinted);
            if (!u || u->op != "!" || !u->operand) break;
            hinted = u->operand.get();
            nots++;
        }
        int hint = (nots & 1) ? -hinted->branchHint : hinted->branchHint;
        auto thenBranch = parseStatement();
        std::unique_ptr<Statement> elseBranch = nullptr;
        if (match(TokenType::ELSE)) {
            elseBranch = parseStatement();
        }
        auto ifs = std::make_unique<IfStatement>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
        ifs->hint = hint;
        return setPos(std::move(ifs), startToken);
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
        EnumScope forScope(*this);   // a declaration in the initializer ends with the loop
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
                auto expr = parseCommaExpression();
                expect(TokenType::SEMICOLON, "Expected ';' after for initializer expression");
                initializer = setPos(std::make_unique<ExpressionStatement>(std::move(expr)), startToken);
            }
        }
        std::unique_ptr<Expression> condition = nullptr;
        if (!match(TokenType::SEMICOLON)) {
            condition = parseCommaExpression();
            expect(TokenType::SEMICOLON, "Expected ';' after for condition");
        }
        std::unique_ptr<Expression> increment = nullptr;
        if (!match(TokenType::CLOSE_PAREN)) {
            increment = parseCommaExpression();   // for (...; ...; i++, j++)
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
            throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "repeat count must be a compile-time constant"));
        }
        if (count < 0 || count > 1024) {
            throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "repeat count must be between 0 and 1024"));
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
        while (peek().type == TokenType::STRING_LITERAL) code += advance().value;  // "a" "b"
        // Extended asm: "template" : outputs : inputs : clobbers [: labels]
        // The template is emitted as is (no %N substitution). The operands
        // keep their meaning around it: inputs are evaluated, an output tied
        // to an input ("0") receives the input's value, and a "+" output's
        // lvalue is evaluated once
        struct AsmOperand { std::string constraint; std::unique_ptr<Expression> expr; };
        std::vector<AsmOperand> asmOps[2];   // outputs, inputs
        bool pendingColon = false;           // the second half of a '::' token
        auto takeColon = [&]() {
            if (pendingColon) { pendingColon = false; return true; }
            if (match(TokenType::COLON)) return true;
            if (match(TokenType::SCOPE_RESOLUTION)) { pendingColon = true; return true; }
            return false;
        };
        for (int section = 0; section < 4 && takeColon(); section++) {
            if (pendingColon || peek().type == TokenType::CLOSE_PAREN || peek().type == TokenType::COLON ||
                peek().type == TokenType::SCOPE_RESOLUTION)
                continue; // empty section
            do {
                // Optional symbolic name [name]
                if (match(TokenType::OPEN_SQUARE)) {
                    while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE)
                        advance();
                    match(TokenType::CLOSE_SQUARE);
                }
                if (section == 3) {
                    // asm goto labels
                    expect(TokenType::IDENTIFIER, "Expected label in asm goto");
                } else if (peek().type == TokenType::STRING_LITERAL) {
                    std::string constraint = advance().value;
                    while (peek().type == TokenType::STRING_LITERAL) constraint += advance().value;
                    if (section < 2) {
                        expect(TokenType::OPEN_PAREN, "Expected '(' before asm operand");
                        auto e = parseExpression();
                        expect(TokenType::CLOSE_PAREN, "Expected ')' after asm operand");
                        asmOps[section].push_back({constraint, std::move(e)});
                    }
                } else {
                    break;
                }
            } while (match(TokenType::COMMA));
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after asm");
        expect(TokenType::SEMICOLON, "Expected ';'");
        if (asmOps[0].empty() && asmOps[1].empty() && (code == ".noescape_strings" || code == ".escape_strings")) {
            noescapeStrings_ = code == ".noescape_strings";   // (the pragma inside a function)
            auto none = setPos(std::make_unique<CompoundStatement>(), startToken);
            none->isDeclGroup = true;
            return none;
        }
        if (asmOps[0].empty() && asmOps[1].empty())
            return setPos(std::make_unique<AsmStatement>(code), startToken);

        auto group = setPos(std::make_unique<CompoundStatement>(), startToken);
        group->isDeclGroup = true;  // not a scope
        auto isPlain = [](Expression* e) {
            return dynamic_cast<VariableReference*>(e) || dynamic_cast<IntegerLiteral*>(e) ||
                   dynamic_cast<StringLiteral*>(e);
        };
        // Inputs tied to an output: output index -> input
        std::map<size_t, std::unique_ptr<Expression>> tied;
        for (auto& in : asmOps[1]) {
            const std::string& c = in.constraint;
            if (!c.empty() && std::all_of(c.begin(), c.end(), ::isdigit)) {
                size_t n = (size_t)std::stoul(c);
                if (n < asmOps[0].size()) { tied[n] = std::move(in.expr); continue; }
            }
            if (!isPlain(in.expr.get()))
                group->statements.push_back(setPos(std::make_unique<ExpressionStatement>(std::move(in.expr)), startToken));
        }
        group->statements.push_back(setPos(std::make_unique<AsmStatement>(code), startToken));
        for (size_t i = 0; i < asmOps[0].size(); i++) {
            auto& out = asmOps[0][i];
            if (tied.count(i)) {
                // The output register held the input's value
                group->statements.push_back(setPos(std::make_unique<ExpressionStatement>(
                    setPos(std::make_unique<Assignment>(std::move(out.expr), std::move(tied[i])), startToken)), startToken));
            } else if (!isPlain(out.expr.get())) {
                // Evaluate the lvalue for its side effects (the value is unchanged)
                group->statements.push_back(setPos(std::make_unique<ExpressionStatement>(
                    setPos(std::make_unique<UnaryOperation>("&", std::move(out.expr)), startToken)), startToken));
            }
        }
        return group;
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

// An expression with the comma operator (a, b: both evaluated, the value b's)
std::unique_ptr<Expression> Parser::parseCommaExpression() {
    auto expr = parseExpression();
    while (match(TokenType::COMMA)) {
        const Token& commaToken = tokens[pos - 1];
        auto right = parseExpression();
        expr = setPos(std::make_unique<BinaryOperation>(",", std::move(expr), std::move(right)), commaToken);
    }
    return expr;
}

std::unique_ptr<Statement> Parser::parseVariableDeclaration(bool isVolatile, bool isConst, bool isStatic, bool isRegister, bool isStriped) {
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
    size_t specStart = pos;
    std::string type;
    bool isSigned = false;
    bool plainInt = false;   // no signed/unsigned written, not a typedef
    int basePtrLevel = 0;
    int addressSpace = 0;  // Phase 97: Track address space qualifier
    std::vector<int> typedefArrayDims;
    if (match(TokenType::TYPEOF)) {
        // typeof(expr) or typeof(type) — resolve to type name
        expect(TokenType::OPEN_PAREN, "Expected '(' after typeof");
        if (isTypeStartToken()) {
            auto ts = parseTypeSpecifier();
            type = ts.name;
            isSigned = ts.isSigned;
            addressSpace = ts.addressSpace;  // Phase 97: Extract address space
            // skip pointer levels in typeof (already consumed by parseTypeSpecifier)
        } else {
            // typeof(expression): a variable's declared type, else int
            auto expr = parseExpression();
            DeclType dt = typeofExpr(expr.get());
            type = dt.type;
            isSigned = dt.isSigned;
            basePtrLevel = dt.ptrLevel;
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
            plainInt = !ts.fromTypedef && !ts.isUnsigned && !ts.isSigned &&
                       (ts.name == "char" || ts.name == "short" || ts.name == "int" || ts.name == "long");
            addressSpace = ts.addressSpace;  // Phase 97: Extract address space
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
                hideEnumConstant(fpName);
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
                        { long cv; if (evalConstInt(szExpr.get(), cv)) sz = (int)cv; }
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
            throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected type for variable declaration. Found '" + peek().typeToString() + "' instead."));
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
    // volatile T *p: what p points to is volatile, not p (char * volatile p)
    bool ptrVolatile = false;
    unsigned constStars = 0;    // bit j: const after the j-th '*' (from 1)
    int stars = 0;
    while (true) {
        if (tryParseAttribute() || match(TokenType::EXTENSION)) {
            continue;
        }
        if (match(TokenType::STAR)) {
            ptrLevel++;
            stars++;
            // (only the last '*''s qualifiers are the variable's: in T *const *p
            // the pointers p points to are const, not p, #334)
            isPointerConst = false;
            ptrVolatile = false;
            while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
                if (match(TokenType::CONST)) { isPointerConst = true; constStars |= 1u << stars; }
                else if (match(TokenType::VOLATILE)) ptrVolatile = true;
                else match(TokenType::RESTRICT);
            }
            continue;
        }
        break;
    }

    bool declVolatile = ptrLevel > basePtrLevel ? ptrVolatile : isVolatile;

    // Check for function pointer declaration: type (*name)(params) or type (*name[size])(params)
    // Also handles: type (*name(params))(result) — function returning function pointer
    // (T (__far *p)[N]: a far pointer to an array, #347)
    int declSpace = addressSpace;
    if (peek().type == TokenType::OPEN_PAREN && pos + 2 < tokens.size() &&
        (tokens[pos + 1].type == TokenType::FAR || tokens[pos + 1].type == TokenType::NEAR) &&
        tokens[pos + 2].type == TokenType::STAR) {
        size_t p = pos + 3;
        while (p < tokens.size() && (tokens[p].type == TokenType::VOLATILE || tokens[p].type == TokenType::CONST ||
                                     tokens[p].type == TokenType::RESTRICT)) p++;
        if (p + 2 < tokens.size() && tokens[p].type == TokenType::IDENTIFIER &&
            tokens[p + 1].type == TokenType::CLOSE_PAREN && tokens[p + 2].type == TokenType::OPEN_SQUARE) {
            declSpace = tokens[pos + 1].type == TokenType::FAR ? 3 : 4;
            tokens.erase(tokens.begin() + pos + 1);   // (then as T (*p)[N])
        }
    }
    if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::STAR) {
        advance(); // consume '('
        advance(); // consume '*'
        bool fpPtrConst = false;
        while (match(TokenType::VOLATILE) || match(TokenType::CONST) || match(TokenType::RESTRICT))
            if (tokens[pos - 1].type == TokenType::CONST) fpPtrConst = true;
        std::string fpName = expect(TokenType::IDENTIFIER, "Expected function pointer name").value;

        // Handle complex declarators: if next is '(' it's a function declarator like (*name(void))
        // Parse array dimensions and function parameters
        std::vector<int> fpArrayDims;
        while (match(TokenType::OPEN_SQUARE)) {
            if (peek().type == TokenType::CLOSE_SQUARE) {
                fpArrayDims.push_back(0);
                advance(); // consume ']'
            } else {
                auto szExpr = parseExpression();
                int arrSize = 1;
                { long cv; if (evalConstInt(szExpr.get(), cv)) arrSize = (int)cv; }
                fpArrayDims.push_back(arrSize);
                expect(TokenType::CLOSE_SQUARE, "Expected ']'");
            }
        }

        // If we see '(' here, it's a function declarator: (*name(params))(return_type)
        // Skip the function parameters for now; they'll be parsed as the final return type
        if (peek().type == TokenType::OPEN_PAREN) {
            advance(); // consume '('
            // Skip parameters until matching ')'
            int parenDepth = 1;
            while (parenDepth > 0 && peek().type != TokenType::END_OF_FILE) {
                if (match(TokenType::OPEN_PAREN)) parenDepth++;
                else if (match(TokenType::CLOSE_PAREN)) parenDepth--;
                else advance();
            }
        }

        expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer name");
        // A pointer to an array: T (*name)[N]...
        if (peek().type == TokenType::OPEN_SQUARE && fpArrayDims.empty()) {
            auto decl = setPos(std::make_unique<VariableDeclaration>(type, fpName, ptrLevel + 1), typeToken);
            hideEnumConstant(fpName);
            decl->isSigned = isSigned;
            decl->isVolatile = isVolatile;
            decl->isConst = isConst;
            decl->isStatic = isStatic;
            decl->isRegister = isRegister;
            decl->alignmentExpr = std::move(alignmentExpr);
            while (match(TokenType::OPEN_SQUARE)) {
                auto szExpr = parseExpression();
                int arrSize = 1;
                { long cv; if (evalConstInt(szExpr.get(), cv)) arrSize = (int)cv; }
                decl->ptrArrayDims.push_back(arrSize);
                expect(TokenType::CLOSE_SQUARE, "Expected ']'");
            }
            // (pair (*p)[3]: rows of the typedef's arrays)
            decl->ptrArrayDims.insert(decl->ptrArrayDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
            // T (__far *p)[N]: the far pointer's pointee is the row
            // ("__farp T[N]", #347)
            if ((declSpace == 3 || declSpace == 4) && decl->pointerLevel == 1) {
                std::string ft = farArrayType(type, decl->ptrArrayDims);
                int lvl = 1, sp = declSpace;
                bool sg = isSigned;
                farPointer(ft, lvl, sp, sg);
                decl->type = ft;
                decl->pointerLevel = 0;
                decl->isSigned = false;
                decl->ptrArrayDims.clear();
                decl->isConst = fpPtrConst;
            }
            while (tryParseAttribute()) {}
            if (match(TokenType::EQUALS)) {
                if (peek().type == TokenType::OPEN_BRACE) decl->initializer = parseInitializerList();
                else decl->initializer = parseExpression();
            }
            expect(TokenType::SEMICOLON, "Expected ';'");
            return decl;
        }
        auto fpSig = parseFuncPtrParams(type, ptrLevel, isSigned);
        auto decl = setPos(std::make_unique<VariableDeclaration>("void", fpName, 1), typeToken);
        hideEnumConstant(fpName);
        decl->isSigned = false;
        decl->isVolatile = isVolatile;
        decl->isConst = isConst;
        decl->isStatic = isStatic;
        decl->isRegister = isRegister;
        decl->isStriped = isStriped;
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
    std::vector<std::shared_ptr<Expression>> vlaDimExprs;
    bool isVla = false;
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
            long cv;
            bool constSize = evalConstInt(sizeExpr.get(), cv);
            if (constSize) arrSize = (int)cv;
            else if (auto* szExpr = dynamic_cast<SizeofExpression*>(sizeExpr.get())) {
                if (szExpr->typeName == "int" || szExpr->typeName == "short") arrSize = 2;
                else if (szExpr->typeName == "long") arrSize = 4;
                else if (szExpr->typeName == "char") arrSize = 1;
                else arrSize = 2; // default
                for (int _p = 0; _p < szExpr->pointerLevel; _p++) arrSize = 2;
            }
            else arrSize = 0; // a run-time size: a variable-length array
            if (!constSize && arrSize == 0 && !dynamic_cast<SizeofExpression*>(sizeExpr.get())) {
                isVla = true;
                vlaDimExprs.push_back(std::shared_ptr<Expression>(std::move(sizeExpr)));
            } else {
                vlaDimExprs.push_back(nullptr);
            }
            arrayDims.push_back(arrSize);
        }
        if (vlaDimExprs.size() < arrayDims.size()) vlaDimExprs.push_back(nullptr);
        expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
    }
    // pair *p (typedef T pair[N]): a pointer to the array (#347), else the
    // typedef's dimensions follow the declarator's
    std::vector<int> rowDims;
    std::string declType = type;
    if (!typedefArrayDims.empty() && ptrLevel == basePtrLevel + 1 && arrayDims.empty()) {
        if ((addressSpace == 3 || addressSpace == 4) && ptrLevel == 1) declType = farArrayType(type, typedefArrayDims);
        else rowDims = typedefArrayDims;
    } else {
        arrayDims.insert(arrayDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
    }
    // T __far *p: a far pointer (#68)
    bool declSigned = isSigned;
    bool farPtr = farPointer(declType, ptrLevel, addressSpace, declSigned);
    auto decl = setPos(std::make_unique<VariableDeclaration>(declType, name, ptrLevel), typeToken);
    hideEnumConstant(name);
    if (!rowDims.empty()) decl->ptrArrayDims = rowDims;
    if (isVla) {
        vlaDimExprs.resize(arrayDims.size());  // typedef dimensions are constant
        decl->vlaDimExprs = std::move(vlaDimExprs);
    }
    decl->isSigned = declSigned;
    decl->isPlainUnsigned = plainInt && !isSigned && ptrLevel == 0 && !farPtr;
    decl->isVolatile = declVolatile;
    decl->isConst = isConst;
    decl->isStatic = isStatic;
    decl->isRegister = isRegister;
    decl->isStriped = isStriped;
    decl->isPointerConst = isPointerConst;
    for (int j = 1; j < stars && j < 16; j++)
        if (constStars & (1u << j)) decl->innerConstMask |= 1u << (stars - j);
    if (farPtr && ptrLevel == 0) {
        // (the qualifiers of what it points to are not kept; T __far *const p is const)
        decl->isConst = isPointerConst;
        decl->isVolatile = false;
        decl->isPointerConst = false;
    }
    decl->addressSpace = addressSpace;  // Phase 97: Set address space qualifier
    decl->alignmentExpr = std::move(alignmentExpr);
    decl->arrayDims = arrayDims;
    // int x __attribute__((aligned(8))) = 5;
    while (tryParseAttribute()) {}

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
            decl->arrayDims[0] = initListLength(*initList);
        }
    }

    recordVarSize(name, decl->type, ptrLevel, decl->arrayDims);

    // Skip __attribute__ after variable declaration: int x __attribute__((unused));
    while (tryParseAttribute()) {}
    decl->alignment = std::max(decl->alignment, takePendingAlign());
    decl->isNoescape = takePendingNoescape();
    decl->section = takePendingSection(true);
    decl->bank = takePendingBank();
    pendingPurity_ = 0;   // (on a variable: no effect)
    applyAlias(decl->name);
    applyVectorMode(decl->type, decl->pointerLevel, decl->isSigned);
    declTypes_[decl->name] = {decl->type, decl->pointerLevel, decl->isSigned};

    // --- Multi-variable declarations: int a, b = 3, *c, d[4]; ---
    if (peek().type == TokenType::COMMA) {
        const bool declNoescape = decl->isNoescape;   // (decl moves into the group)
        const int declBank = decl->bank;
        const std::string declSection = decl->section;
        auto compound = setPos(std::make_unique<CompoundStatement>(), typeToken);
        compound->isDeclGroup = true;
        compound->statements.push_back(std::move(decl));

        while (peek().type == TokenType::COMMA) {
            // int x, f(void); — a function declarator ends this declaration:
            // the rest becomes a declaration of its own with the same specifiers
            {
                size_t j = pos + 1;
                while (j < tokens.size() && (tokens[j].type == TokenType::STAR || tokens[j].type == TokenType::CONST ||
                       tokens[j].type == TokenType::VOLATILE || tokens[j].type == TokenType::RESTRICT)) j++;
                if (j + 1 < tokens.size() && tokens[j].type == TokenType::IDENTIFIER &&
                    tokens[j + 1].type == TokenType::OPEN_PAREN) {
                    std::vector<Token> spec;
                    auto kw = [&](TokenType t, const char* v) { Token k = tokens[pos]; k.type = t; k.value = v; spec.push_back(k); };
                    if (isStatic) kw(TokenType::STATIC, "static");
                    if (isConst) kw(TokenType::CONST, "const");
                    if (isVolatile) kw(TokenType::VOLATILE, "volatile");
                    for (size_t k = specStart; k < tokens.size() && k < pos; k++) {
                        if (tokens[k].type == TokenType::STAR ||
                            (tokens[k].type == TokenType::IDENTIFIER && tokens[k].value == name)) break;
                        spec.push_back(tokens[k]);
                    }
                    tokens[pos].type = TokenType::SEMICOLON;
                    tokens[pos].value = ";";
                    insertTokens(pos + 1, spec);
                    break;
                }
            }
            advance();
            // Parse additional declarators reusing the same base type
            int extraPtr = basePtrLevel;
            // const/volatile after each '*': the last one's are the variable's
            bool extraPtrConst = false, extraPtrVolatile = false;
            unsigned extraConstStars = 0;
            int extraStars = 0;
            while (match(TokenType::STAR)) {
                extraPtr++;
                extraStars++;
                extraPtrConst = extraPtrVolatile = false;
                while (peek().type == TokenType::CONST || peek().type == TokenType::VOLATILE || peek().type == TokenType::RESTRICT) {
                    if (match(TokenType::CONST)) { extraPtrConst = true; extraConstStars |= 1u << extraStars; }
                    else if (match(TokenType::VOLATILE)) extraPtrVolatile = true;
                    else match(TokenType::RESTRICT);
                }
            }
            std::string extraName = expect(TokenType::IDENTIFIER, "Expected variable name").value;
            declTypes_[extraName] = {type, extraPtr, isSigned};
            std::vector<int> extraDims;
            bool extraUnsized = false;
            while (match(TokenType::OPEN_SQUARE)) {
                if (peek().type == TokenType::CLOSE_SQUARE) {
                    extraDims.push_back(0);
                    extraUnsized = true;
                } else {
                    const Token& dimTok = peek();
                    auto stExpr = parseExpression();
                    long cv;
                    if (!evalConstInt(stExpr.get(), cv))
                        throw std::runtime_error(formatDiagnostic(dimTok.sourceFile, dimTok.line, dimTok.column, Severity::Error,
                            "array size of '" + extraName + "' is not a constant expression"));
                    extraDims.push_back((int)cv);
                }
                expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
            }
            // Append typedef array dimensions
            extraDims.insert(extraDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
            int extraSpace = addressSpace;
            std::string extraType = type;
            bool extraSigned = isSigned;
            bool extraFar = farPointer(extraType, extraPtr, extraSpace, extraSigned);
            declTypes_[extraName] = {extraType, extraPtr, extraSigned};
            auto extraDecl = setPos(std::make_unique<VariableDeclaration>(extraType, extraName, extraPtr), typeToken);
            hideEnumConstant(extraName);
            extraDecl->isSigned = extraSigned;
            extraDecl->isPlainUnsigned = plainInt && !isSigned && extraPtr == 0 && !extraFar;
            extraDecl->isVolatile = extraPtr > basePtrLevel ? extraPtrVolatile : isVolatile;
            extraDecl->isConst = isConst;
            extraDecl->isStatic = isStatic;
            extraDecl->isRegister = isRegister;
            extraDecl->isStriped = isStriped;
            extraDecl->isPointerConst = extraPtrConst;
            for (int j = 1; j < extraStars && j < 16; j++)
                if (extraConstStars & (1u << j)) extraDecl->innerConstMask |= 1u << (extraStars - j);
            if (extraFar && extraPtr == 0) {
                extraDecl->isConst = extraPtrConst;
                extraDecl->isVolatile = false;
                extraDecl->isPointerConst = false;
            }
            extraDecl->addressSpace = extraSpace;  // Phase 97: Set address space qualifier
            extraDecl->isNoescape = declNoescape || takePendingNoescape();
            extraDecl->section = takePendingSection(true);
            extraDecl->bank = takePendingBank();
            if (extraDecl->bank < 0) extraDecl->bank = declBank;
            if (extraDecl->section.empty()) extraDecl->section = declSection;
            extraDecl->arrayDims = extraDims;
            recordVarSize(extraName, extraType, extraPtr, extraDims);
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
                    extraDecl->arrayDims[0] = initListLength(*il);
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
    // Evaluated here, where the declarations so far give sizeof x (the
    // folder, which checks it, knows no variable's size): its value as a
    // literal. One the parser cannot evaluate is left to the folder
    if (long cv; evalConstInt(condition.get(), cv)) condition = setPos(std::make_unique<IntegerLiteral>(cv != 0), startToken);
    return setPos(std::make_unique<StaticAssert>(std::move(condition), message), startToken);
}

std::unique_ptr<StructDefinition> Parser::parseStructDefinition(bool isUnion) {
    const Token& startToken = tokens[pos-1]; // 'struct' or 'union'
    bool isUnpacked = match(TokenType::UNPACKED);
    // Skip __attribute__ after struct/union keyword: struct __attribute__((packed)) S { }
    while (tryParseAttribute()) {}
    int structAlign = takePendingAlign();
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
    def->alignment = structAlign;

    // Validate: cannot inherit from a final struct
    if (!parentStruct.empty() && structs.count(parentStruct) && structs[parentStruct]->isFinal) {
        throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "cannot inherit from final struct '" + parentStruct + "'"));
    }

    while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE) {
        // Check for struct method: type name(params) { body }
        // Handle 'static', 'virtual' keywords
        bool methodIsVirtual = false;
        bool methodIsStatic = false;
        if (peek().type == TokenType::STATIC) {
            advance(); // consume 'static'
            methodIsStatic = true;
        }
        if (peek().type == TokenType::IDENTIFIER && peek().value == "virtual") {
            if (methodIsStatic) {
                throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "a method cannot be both 'static' and 'virtual'"));
            }
            advance(); // consume 'virtual'
            methodIsVirtual = true;
            def->hasVirtual = true;
        }

        if (!isUnion && (methodIsVirtual || methodIsStatic || isFunctionDeclaration())) {
            // 'this' in a non-static method body names the hidden __this parameter
            if (!methodIsStatic) inStructMethod_++;
            auto method = parseFunctionDeclaration();
            if (!methodIsStatic) inStructMethod_--;
            method->isVirtual = methodIsVirtual;
            method->isStatic = methodIsStatic;
            // Mangle name: StructName__methodName
            std::string origName = method->name;
            method->name = name + "__" + origName;
            method->isMethod = true;
            method->methodStructName = name;
            // Add hidden 'this' parameter for non-static methods
            if (!methodIsStatic) {
                Parameter thisParam;
                thisParam.type = "struct " + name;
                thisParam.pointerLevel = 1;
                thisParam.isSigned = false;
                thisParam.name = "__this";
                method->parameters.insert(method->parameters.begin(), thisParam);
            }
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
                    { StructMember sm; sm.type = nestedTypeName; sm.pointerLevel = 0; sm.isAnonymous = true; sm.name = "__anon_member_" + std::to_string(anonymousAggregateCount++); def->members.push_back(std::move(sm)); }
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
        int declStars = 0;   // the first declarator's '*'s, if the type specifier took them
        std::vector<int> typedefArrayDims;
        std::vector<std::string> memberTypedefVla;
        std::shared_ptr<FuncPtrSignature> memberFpSig;   // a typedef'd function pointer type
        int mSpace = 0;   // (3: __far, T __far *m a far pointer)
        // a member's type: T __far * a far pointer (#68)
        auto memberType = [&](StructMember& m, int level) {
            std::string t = type; int sp = mSpace; bool sg = mIsSigned;
            farPointer(t, level, sp, sg);
            m.type = t; m.pointerLevel = level; m.isSigned = sg;
        };
        if (peek().type == TokenType::ATTRIBUTE) {
            // __attribute__ as member type prefix — skip and retry
            while (tryParseAttribute()) {}
            continue; // re-enter the member parsing loop
        }
        {
            auto ts = parseTypeSpecifier();
            if (!ts.valid) {
                throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected member type"));
            }
            type = ts.name;
            mIsSigned = ts.isSigned;
            mSpace = ts.addressSpace;
            basePtrLevel = ts.pointerLevel;
            typedefArrayDims = ts.arrayDims;
            memberTypedefVla = ts.vlaDimVars;
            memberFpSig = ts.funcPtrSig;
            if (ts.isConst) mIsConst = true;
            // with '*'s in the declarator, only a const after the last one
            // makes the member itself const
            if (ts.starsParsed) mIsConst = ts.lastStarConst;
            declStars = ts.starsParsed;
        }
        // Skip __attribute__ after type, before pointer/name
        while (tryParseAttribute()) {}
        int ptrLevel = basePtrLevel;
        // const before the type qualifies what a declarator's '*' points to:
        // the member itself is const only with a const after its last '*'
        // (const char *p can be changed, char *const p cannot)
        const bool baseConst = mIsConst;
        auto parsePtrQuals = [&](int& level, bool& isConst) {
            while (match(TokenType::STAR)) {
                level++;
                isConst = false;
                for (;;) {
                    if (match(TokenType::CONST)) { isConst = true; continue; }
                    if (match(TokenType::VOLATILE) || match(TokenType::RESTRICT) || tryParseAttribute() ||
                        match(TokenType::EXTENSION)) continue;
                    break;
                }
            }
        };
        parsePtrQuals(ptrLevel, mIsConst);
        // Unnamed bitfield: int : 6;  (no member name, colon follows type)
        if (peek().type == TokenType::COLON) {
            advance(); // consume ':'
            int bw = parseBitfieldWidth(type, "", true);
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
            // the parameter list: the signature calls through the member use
            std::shared_ptr<FuncPtrSignature> fpSig;
            if (peek().type == TokenType::OPEN_PAREN) fpSig = parseFuncPtrParams(type, ptrLevel, mIsSigned);
            while (tryParseAttribute()) {}
            StructMember sm;
            sm.type = "void"; sm.pointerLevel = 1; sm.isSigned = false;
            sm.name = fpName;
            sm.funcPtrSig = fpSig;
            def->members.push_back(std::move(sm));
            expect(TokenType::SEMICOLON, "Expected ';'");
            continue;
        }

        std::string memberName = expect(TokenType::IDENTIFIER, "Expected member name").value;
        std::vector<int> memberArrayDims;
        std::vector<std::unique_ptr<Expression>> memberVlaDims;
        bool memberIsVla = false;
        bool isFlexArray = false;
        while (match(TokenType::OPEN_SQUARE)) {
            if (match(TokenType::CLOSE_SQUARE)) {
                // Flexible array member: type name[];
                isFlexArray = true;
                memberArrayDims.push_back(0);
                memberVlaDims.push_back(nullptr);
            } else {
                auto mSizeExpr = parseExpression();
                int mArrSize = 0;
                long cv;
                if (evalConstInt(mSizeExpr.get(), cv)) {
                    mArrSize = (int)cv;
                    memberVlaDims.push_back(nullptr);
                } else {
                    // A run-time size (a struct defined in a function)
                    mArrSize = 1;
                    memberVlaDims.push_back(std::move(mSizeExpr));
                    memberIsVla = true;
                }
                memberArrayDims.push_back(mArrSize);
                expect(TokenType::CLOSE_SQUARE, "Expected ']' after array size");
            }
        }
        // Append typedef array dimensions (run-time ones read their hidden variable)
        memberVlaDims.resize(memberArrayDims.size());
        for (size_t td = 0; td < typedefArrayDims.size(); td++) {
            memberArrayDims.push_back(typedefArrayDims[td]);
            if (td < memberTypedefVla.size() && !memberTypedefVla[td].empty()) {
                memberVlaDims.push_back(std::make_unique<VariableReference>(memberTypedefVla[td]));
                memberIsVla = true;
            } else {
                memberVlaDims.push_back(nullptr);
            }
        }
        if (isFlexArray && memberArrayDims.size() > 1)
            throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Flexible array member '" + memberName + "' cannot be multi-dimensional"));
        int memberBitWidth = 0;
        if (match(TokenType::COLON)) {
            if (!memberArrayDims.empty())
                throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Bitfield member '" + memberName + "' cannot be an array"));
            if (ptrLevel > 0)
                throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Bitfield member '" + memberName + "' cannot be a pointer"));
            memberBitWidth = parseBitfieldWidth(type, memberName, false);
        }
        while (tryParseAttribute()) {}
        StructMember sm;
        memberType(sm, ptrLevel);
        sm.name = memberName; sm.isConst = mIsConst; sm.alignment = takePendingAlign();
        takePendingSection(false);
        sm.alignmentExpr = std::move(mAlignmentExpr); sm.isAnonymous = false;
        sm.arrayDims = memberArrayDims; sm.bitWidth = memberBitWidth;
        // a __bit member: a 1-bit field (#73; consecutive ones share a byte)
        if (type == "__bit" && ptrLevel == 0 && memberArrayDims.empty() && sm.bitWidth == 0) sm.bitWidth = 1;
        sm.isFlexible = isFlexArray;
        if (ptrLevel == basePtrLevel && memberArrayDims.empty()) sm.funcPtrSig = memberFpSig;
        if (memberIsVla) {
            memberVlaDims.resize(memberArrayDims.size());
            sm.vlaDimExprs = std::move(memberVlaDims);
        }
        def->members.push_back(std::move(sm));

        // Multi-member declarations: long p_x, p_y;
        while (match(TokenType::COMMA)) {
            // (the type's own pointer level: not the first declarator's '*'s,
            // which the type specifier may have taken -- const char *a, *b
            // made b a char **, #333)
            int extraPtr = basePtrLevel - declStars;
            bool extraConst = baseConst;
            parsePtrQuals(extraPtr, extraConst);
            std::string extraName = expect(TokenType::IDENTIFIER, "Expected member name").value;
            std::vector<int> extraDims;
            bool extraFlex = false;
            while (match(TokenType::OPEN_SQUARE)) {
                if (match(TokenType::CLOSE_SQUARE)) { extraDims.push_back(0); extraFlex = true; }
                else {
                    extraDims.push_back(std::stoi(expect(TokenType::INTEGER_LITERAL, "Expected array size").value));
                    expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                }
            }
            // Append typedef array dimensions
            extraDims.insert(extraDims.end(), typedefArrayDims.begin(), typedefArrayDims.end());
            int extraBitWidth = 0;
            if (match(TokenType::COLON)) {
                if (!extraDims.empty())
                    throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Bitfield member '" + extraName + "' cannot be an array"));
                if (extraPtr > 0)
                    throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Bitfield member '" + extraName + "' cannot be a pointer"));
                extraBitWidth = parseBitfieldWidth(type, extraName, false);
            }
            while (tryParseAttribute()) {}
            StructMember esm;
            memberType(esm, extraPtr);
            esm.name = extraName; esm.isConst = extraConst;
            esm.arrayDims = extraDims; esm.bitWidth = extraBitWidth;
            if (type == "__bit" && extraPtr == 0 && extraDims.empty() && esm.bitWidth == 0) esm.bitWidth = 1;
            esm.isFlexible = extraFlex;
            esm.alignment = takePendingAlign();
            takePendingSection(false);
            def->members.push_back(std::move(esm));
        }
        while (tryParseAttribute()) {}
        expect(TokenType::SEMICOLON, "Expected ';'");
    }
    expect(TokenType::CLOSE_BRACE, "Expected '}'");
    // struct S { ... } __attribute__((aligned(N)))
    while (tryParseAttribute()) {}
    def->alignment = std::max(def->alignment, takePendingAlign());
    takePendingSection(false);

    // Phase 3: If this struct has virtual methods, insert __vt pointer as first member
    if (def->hasVirtual || (!def->parentStruct.empty() && structs.count(def->parentStruct) && structs[def->parentStruct]->hasVirtual)) {
        // Check if __vt already exists: a parent with virtual methods has it
        // at offset 0 (the parent's members are laid out first)
        bool hasVt = !def->parentStruct.empty() && structs.count(def->parentStruct) &&
                     structs[def->parentStruct]->hasVirtual;
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
                                throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "cannot override final method '" + pm->name.substr(pm->name.rfind("__") + 2) + "' in struct '" + name + "'"));
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

    // Validate flexible array members (C99)
    for (size_t i = 0; i < def->members.size(); i++) {
        auto& m = def->members[i];
        // name[0] (GNU zero-length array) may be anywhere, also alone or in a union
        bool isFAM = m.isFlexible && m.arrayDims.size() == 1;
        if (isFAM) {
            if (def->isUnion) {
                throw std::runtime_error(formatDiagnostic(startToken.sourceFile, startToken.line, startToken.column, Severity::Error, "Flexible array member '" + m.name + "' not allowed in union"));
            }
            if (i != def->members.size() - 1) {
                throw std::runtime_error(formatDiagnostic(startToken.sourceFile, startToken.line, startToken.column, Severity::Error, "Flexible array member '" + m.name + "' must be the last member of struct '" + def->name + "'"));
            }
            if (def->members.size() < 2) {
                throw std::runtime_error(formatDiagnostic(startToken.sourceFile, startToken.line, startToken.column, Severity::Error, "struct '" + def->name + "' with flexible array member must have at least one other member"));
            }
        }
    }

    // Validate member types (e.g. unknown nested structs/unions)
    for (auto& m : def->members) {
        if (m.pointerLevel == 0 && m.type.rfind("struct ", 0) == 0) {
            std::string sName = m.type.substr(7);
            if (!structs.count(sName)) {
                throw std::runtime_error(formatDiagnostic(startToken.sourceFile, startToken.line, startToken.column, Severity::Error, "Unknown struct/union type '" + sName + "'"));
            }
        } else if (m.pointerLevel == 0 && m.type.rfind("union ", 0) == 0) {
            std::string uName = m.type.substr(6);
            if (!structs.count(uName)) {
                throw std::runtime_error(formatDiagnostic(startToken.sourceFile, startToken.line, startToken.column, Severity::Error, "Unknown struct/union type '" + uName + "'"));
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
                // abstract declarators: (*)[N] is a pointer (to an array),
                // [N]... an array of the type (int[4], char *[4], T[2][3])
                std::vector<int> declDims;
                if (peek().type == TokenType::OPEN_PAREN && pos + 1 < tokens.size() &&
                    tokens[pos + 1].type == TokenType::STAR) {
                    advance();
                    while (match(TokenType::STAR)) ts.pointerLevel++;
                    expect(TokenType::CLOSE_PAREN, "Expected ')' in sizeof type");
                    ts.arrayDims.clear();
                    while (match(TokenType::OPEN_SQUARE)) {   // (the pointee's dimensions)
                        while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE) advance();
                        expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                    }
                    // (*)(params): a pointer to a function, the same size
                    if (peek().type == TokenType::OPEN_PAREN) {
                        int depth = 0;
                        do {
                            if (peek().type == TokenType::OPEN_PAREN) depth++;
                            else if (peek().type == TokenType::CLOSE_PAREN) depth--;
                            advance();
                        } while (depth > 0 && peek().type != TokenType::END_OF_FILE);
                    }
                }
                while (match(TokenType::OPEN_SQUARE)) {
                    auto szExpr = parseExpression();
                    long cv = 0;
                    if (!evalConstInt(szExpr.get(), cv)) cv = 1;
                    declDims.push_back((int)cv);
                    expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                }
                expect(TokenType::CLOSE_PAREN, "Expected ')' after type in sizeof");
                { int sp = ts.addressSpace; farPointer(ts.name, ts.pointerLevel, sp, ts.isSigned); }
                auto node = std::make_unique<SizeofExpression>(ts.name, ts.pointerLevel);
                node->arrayDims = ts.arrayDims;
                node->declDims = declDims;
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
        // Parse statements until '}'; a final expression statement is the value
        auto stmtExpr = std::make_unique<StatementExpression>();
        stmtExpr->body = std::make_unique<CompoundStatement>();
        while (peek().type != TokenType::CLOSE_BRACE && peek().type != TokenType::END_OF_FILE)
            stmtExpr->body->statements.push_back(parseStatement());
        expect(TokenType::CLOSE_BRACE, "Expected '}' in statement expression");
        expect(TokenType::CLOSE_PAREN, "Expected ')' after statement expression");
        auto& stmts = stmtExpr->body->statements;
        if (!stmts.empty()) {
            if (auto* exprStmt = dynamic_cast<ExpressionStatement*>(stmts.back().get())) {
                stmtExpr->result = std::move(exprStmt->expression);
                stmts.pop_back();
            }
        }
        std::unique_ptr<Expression> result = setPos(std::move(stmtExpr), startToken);
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
        std::vector<int> castArrayDims;
        bool castVolatile = false, castConst = false;

        if (isTypeStartToken()) {
            auto ts = parseTypeSpecifier();
            castType = ts.name;
            castSigned = ts.isSigned;
            castPtrLevel = ts.pointerLevel;
            castVolatile = ts.isVolatile;
            castConst = ts.isConst;
            // (pair __far *) with typedef T pair[N]: a far pointer to the
            // array (#347); (pair *): a pointer to it, cast as void * (a
            // cast keeps no row: assign it to a pair *p to index it)
            bool rowCast = false;
            if (!ts.arrayDims.empty() && ts.starsParsed == 1 && castPtrLevel == 1) {
                rowCast = true;
                if (ts.addressSpace == 3 || ts.addressSpace == 4) castType = farArrayType(castType, ts.arrayDims);
                else { castType = "void"; castSigned = false; }
            }
            { int sp = ts.addressSpace; farPointer(castType, castPtrLevel, sp, castSigned); }
            // (T (*)[N]), (T (__far *)[N]): a pointer to an array, likewise
            if (!rowCast && peek().type == TokenType::OPEN_PAREN && pos + 3 < tokens.size()) {
                size_t q = pos + 1;
                int sp = 0;
                if (tokens[q].type == TokenType::FAR) { sp = 3; q++; }
                else if (tokens[q].type == TokenType::NEAR) { sp = 4; q++; }
                if (q + 2 < tokens.size() && tokens[q].type == TokenType::STAR &&
                    tokens[q + 1].type == TokenType::CLOSE_PAREN && tokens[q + 2].type == TokenType::OPEN_SQUARE) {
                    pos = q + 2;
                    std::vector<int> dims;
                    while (match(TokenType::OPEN_SQUARE)) {
                        auto szExpr = parseExpression();
                        long cv = 1;
                        if (!evalConstInt(szExpr.get(), cv) || cv <= 0)
                            throw std::runtime_error(formatDiagnostic(startToken.sourceFile, startToken.line, startToken.column,
                                Severity::Error, "a pointer to an array needs constant dimensions"));
                        dims.push_back((int)cv);
                        expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                    }
                    dims.insert(dims.end(), ts.arrayDims.begin(), ts.arrayDims.end());
                    if ((sp == 3 || sp == 4) && castPtrLevel == 0) {
                        castType = farArrayType(castType, dims);
                        castPtrLevel = 1;
                        farPointer(castType, castPtrLevel, sp, castSigned);
                    } else {
                        castType = "void";
                        castSigned = false;
                        castPtrLevel = 1;
                    }
                }
            }

            // Array type: (int []) or (int [N]) — an array compound literal;
            // a 0 dimension is taken from the initializer
            while (peek().type == TokenType::OPEN_SQUARE) {
                advance(); // [
                int dim = 0;
                if (peek().type == TokenType::INTEGER_LITERAL && pos + 1 < tokens.size() &&
                    tokens[pos + 1].type == TokenType::CLOSE_SQUARE) {
                    std::string v = advance().value;
                    while (!v.empty() && std::strchr("uUlL", v.back())) v.pop_back();
                    dim = (int)std::stoll(v, nullptr, 0);
                } else {
                    // Skip a size expression (taken from the initializer)
                    while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE) advance();
                }
                expect(TokenType::CLOSE_SQUARE, "Expected ']' in array cast");
                castArrayDims.push_back(dim);
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
                // (T[]){...} / (T[N]){...}: an array (a 0 size comes from the initializer)
                cl->arrayDims = castArrayDims;
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
            castPtrLevel += (int)castArrayDims.size();  // a cast to an array type decays
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
            throw std::runtime_error(formatDiagnostic(opToken.sourceFile, opToken.line, opToken.column, Severity::Error, "Cannot take address of CPU register"));
        }
        if (dynamic_cast<CpuFlagAccess*>(operand.get())) {
            throw std::runtime_error(formatDiagnostic(opToken.sourceFile, opToken.line, opToken.column, Severity::Error, "Cannot take address of CPU flag"));
        }
        if (dynamic_cast<BitSelect*>(operand.get())) {
            throw std::runtime_error(formatDiagnostic(opToken.sourceFile, opToken.line, opToken.column, Severity::Error, "Cannot take address of __bit/__bits"));
        }
        return setPos(std::make_unique<UnaryOperation>(op, std::move(operand)), opToken);
    }

    if (match(TokenType::BANG) || match(TokenType::TILDE) || match(TokenType::MINUS) || match(TokenType::STAR) ||
        match(TokenType::PLUS) || match(TokenType::PLUS_PLUS) || match(TokenType::MINUS_MINUS)) {
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
        bool gnuSpelling = tokens[pos-1].value != "_Alignof";  // __alignof__ also takes an expression
        expect(TokenType::OPEN_PAREN, "Expected '(' after '_Alignof'");
        if (isTypeStartToken()) {
            auto ts = parseTypeSpecifier();
            expect(TokenType::CLOSE_PAREN, "Expected ')' after _Alignof type");
            return setPos(std::make_unique<AlignofExpression>(ts.name, ts.pointerLevel), tokens[pos-1]);
        } else if (!gnuSpelling) {
            throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected type name in _Alignof"));
        } else {
            // GCC __alignof__(expression): the alignment of its type, or of
            // the object or function it names
            const Token& at = peek();
            auto e = parseExpression();
            expect(TokenType::CLOSE_PAREN, "Expected ')' after _Alignof expression");
            auto ae = setPos(std::make_unique<AlignofExpression>("", 0), at);
            ae->expression = std::move(e);
            return ae;
        }
    }

    if (peek().type == TokenType::IDENTIFIER && (peek().value == "__func__" || peek().value == "__FUNCTION__")) {
        const Token& funcToken = advance();
        if (currentFunctionName.empty()) {
            throw std::runtime_error(formatDiagnostic(funcToken.sourceFile, funcToken.line, funcToken.column, Severity::Error, "__func__ used outside of a function"));
        }
        expr = setPos(std::make_unique<StringLiteral>(currentFunctionName), funcToken);
    } else if (peek().type == TokenType::IDENTIFIER && (peek().value == "__cpu" || peek().value == "__flags")) {
        const Token& baseToken = advance();
        std::string base = baseToken.value;
        if (!match(TokenType::DOT)) {
            throw std::runtime_error(formatDiagnostic(baseToken.sourceFile, baseToken.line, baseToken.column, Severity::Error, "'" + base + "' must be followed by '.' and a member name"));
        }
        const Token& memberToken = expect(TokenType::IDENTIFIER, "Expected member name after " + base + ".");
        std::string member = memberToken.value;
        if (base == "__cpu") {
            expr = setPos(std::make_unique<CpuRegisterAccess>(member), baseToken);
        } else {
            expr = setPos(std::make_unique<CpuFlagAccess>(member), baseToken);
        }
    } else if (peek().type == TokenType::IDENTIFIER &&
               (peek().value == "__jsr" || peek().value == "__jsr_raw" || peek().value == "__jmp") &&
               pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::OPEN_PAREN) {
        expr = parseJsr();
    } else if (peek().type == TokenType::IDENTIFIER && (peek().value == BIT_BUILTIN || peek().value == "__bits") &&
               pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::OPEN_PAREN) {
        // __bit(v, n) / __bits(v, lo, hi) (#204): bits of the integer lvalue v,
        // the positions constant (the operand's width is checked by IRBuilder)
        const Token& bToken = advance();
        std::string name = bToken.value == BIT_BUILTIN ? "__bit" : bToken.value;
        advance();   // (
        auto v = parseConditional();
        auto position = [&](const char* what) {
            expect(TokenType::COMMA, "Expected ',' in " + name);
            auto e = parseConditional();
            long c;
            if (!evalConstInt(e.get(), c))
                throw std::runtime_error(formatDiagnostic(e->sourceFile, e->line, e->column, Severity::Error,
                    std::string("the ") + what + " of " + name + " is not a constant"));
            return c;
        };
        long lo = position(name == "__bit" ? "bit number" : "low bit");
        long hi = name == "__bit" ? lo : position("high bit");
        expect(TokenType::CLOSE_PAREN, "Expected ')' after " + name);
        if (lo < 0 || hi > 31 || lo > hi)
            throw std::runtime_error(formatDiagnostic(bToken.sourceFile, bToken.line, bToken.column, Severity::Error,
                name + ": " + (lo > hi && lo >= 0 && hi <= 31 ? "the low bit " + std::to_string(lo) + " is above the high bit " + std::to_string(hi)
                                                               : "bit " + std::to_string(lo < 0 ? lo : hi) + " is out of range (0 to 31)")));
        expr = setPos(std::make_unique<BitSelect>(std::move(v), (int)lo, (int)hi), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_constant_p") {
        // __builtin_constant_p(x) → 1 if x is a compile-time constant, 0 otherwise
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_constant_p");
        auto arg = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        // Check if the argument is a compile-time constant
        bool isConst = false;
        long cv0;
        if (evalConstInt(arg.get(), cv0)) isConst = true;
        else if (dynamic_cast<IntegerLiteral*>(arg.get())) isConst = true;
        else if (dynamic_cast<StringLiteral*>(arg.get())) isConst = true;
        else if (auto* unary = dynamic_cast<UnaryOperation*>(arg.get())) {
            if (dynamic_cast<IntegerLiteral*>(unary->operand.get())) isConst = true;
        } else if (auto* binop = dynamic_cast<BinaryOperation*>(arg.get())) {
            if (dynamic_cast<IntegerLiteral*>(binop->left.get()) &&
                dynamic_cast<IntegerLiteral*>(binop->right.get())) isConst = true;
        }
        expr = setPos(std::make_unique<IntegerLiteral>(isConst ? 1 : 0), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_choose_expr") {
        // __builtin_choose_expr(c, a, b) → a if the constant c is nonzero, else b
        // (the other operand is not evaluated)
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_choose_expr");
        auto cond = parseConditional();
        expect(TokenType::COMMA, "Expected ',' in __builtin_choose_expr");
        auto a = parseConditional();
        expect(TokenType::COMMA, "Expected ',' in __builtin_choose_expr");
        auto b = parseConditional();
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        long cv;
        if (!evalConstInt(cond.get(), cv))
            throw std::runtime_error(formatDiagnostic(bToken.sourceFile, bToken.line, bToken.column, Severity::Error,
                "first argument to __builtin_choose_expr is not a constant"));
        expr = cv ? std::move(a) : std::move(b);
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
        // offsetof(T, m[i].n) is (unsigned)&((T *)0)->m[i].n
        std::unique_ptr<Expression> cur = setPos(std::make_unique<CastExpression>(ofType, 1, false,
            setPos(std::make_unique<IntegerLiteral>(0), bToken)), bToken);
        std::string memberName = expect(TokenType::IDENTIFIER, "Expected member name").value;
        cur = setPos(std::make_unique<MemberAccess>(std::move(cur), memberName, true), bToken);
        while (true) {
            if (match(TokenType::OPEN_SQUARE)) {
                auto idx = parseExpression();
                expect(TokenType::CLOSE_SQUARE, "Expected ']'");
                cur = setPos(std::make_unique<ArrayAccess>(std::move(cur), std::move(idx)), bToken);
            } else if (match(TokenType::DOT)) {
                std::string sub = expect(TokenType::IDENTIFIER, "Expected member name").value;
                cur = setPos(std::make_unique<MemberAccess>(std::move(cur), sub, false), bToken);
            } else {
                break;
            }
        }
        expect(TokenType::CLOSE_PAREN, "Expected ')' after __builtin_offsetof");
        expr = setPos(std::make_unique<CastExpression>("int", 0, false,
            setPos(std::make_unique<UnaryOperation>("&", std::move(cur)), bToken)), bToken);
    } else if (peek().type == TokenType::IDENTIFIER && peek().value == "__builtin_expect") {
        // __builtin_expect(x, v) → evaluates to x (branch prediction hint, no-op)
        const Token& bToken = advance();
        expect(TokenType::OPEN_PAREN, "Expected '(' after __builtin_expect");
        auto val = parseExpression();
        expect(TokenType::COMMA, "Expected ',' in __builtin_expect");
        auto hint = parseExpression();  // evaluated too (it may have side effects)
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        if (auto* lit = dynamic_cast<IntegerLiteral*>(hint.get())) {
            // a constant: val is expected to be it (0: false, else true)
            val->branchHint = lit->value ? 1 : -1;
            expr = std::move(val);
        } else {
            // (hint, val): the value of val, both evaluated
            expr = setPos(std::make_unique<BinaryOperation>(",", std::move(hint), std::move(val)), bToken);
        }
    } else if (peek().type == TokenType::IDENTIFIER && (peek().value == "__likely" || peek().value == "__unlikely")) {
        // __likely(x) / __unlikely(x): x, expected true / false (the branch of
        // an if that is not expected is placed at the function's end)
        bool likely = advance().value == "__likely";
        expect(TokenType::OPEN_PAREN, likely ? "Expected '(' after __likely" : "Expected '(' after __unlikely");
        expr = parseExpression();
        expect(TokenType::CLOSE_PAREN, "Expected ')'");
        expr->branchHint = likely ? 1 : -1;
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
            expect(TokenType::OPEN_PAREN, "Expected '(' after typeof");
            if (isTypeStartToken()) {
                auto ts = parseTypeSpecifier();
                vaType = ts.name;
                vaSigned = ts.isSigned;
                vaPtrLevel = ts.pointerLevel;
            } else {
                auto te = parseExpression();
                DeclType dt = typeofExpr(te.get());
                vaType = dt.type;
                vaSigned = dt.isSigned;
                vaPtrLevel = dt.ptrLevel;
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')' after typeof");
        } else {
            auto ts = parseTypeSpecifier();
            if (!ts.valid) {
                throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected type in __builtin_va_arg"));
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

        // 'this' in a struct method → the hidden '__this' parameter.
        // Elsewhere 'this' is an ordinary identifier (C allows it as a name).
        if (inStructMethod_ > 0 && actualName == "this") {
            actualName = "__this";
        }

        // Map __builtin_X → X for known stdlib-equivalent builtins
        if (actualName.rfind("__builtin_", 0) == 0) {
            static const std::map<std::string, std::string> builtinAliases = {
                {"__builtin_abort", "abort"},
                {"__builtin_printf", "printf"},
                {"__builtin_sprintf", "sprintf"},
                {"__builtin_puts", "puts"},
                {"__builtin_putstr", "putstr"},
                {"__builtin_putchar", "putchar"},
                {"__builtin_malloc", "malloc"},
                {"__builtin_free", "free"},
                {"__builtin_calloc", "calloc"},
                {"__builtin_realloc", "realloc"},
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
                {"__builtin_alloca", "alloca"},
                {"__builtin_fabs", "fabs"}, {"__builtin_fabsf", "fabsf"},
                {"__builtin_floor", "floor"}, {"__builtin_floorf", "floorf"},
                {"__builtin_ceil", "ceil"}, {"__builtin_ceilf", "ceilf"},
                {"__builtin_sqrt", "sqrt"}, {"__builtin_sqrtf", "sqrtf"},
                {"__builtin_trunc", "trunc"}, {"__builtin_truncf", "truncf"},
                {"__builtin_round", "round"}, {"__builtin_roundf", "roundf"},
                {"__builtin_fmod", "fmod"}, {"__builtin_fmodf", "fmodf"},
                {"__builtin_pow", "pow"}, {"__builtin_powf", "powf"},
                {"__builtin_sin", "sin"}, {"__builtin_sinf", "sinf"},
                {"__builtin_cos", "cos"}, {"__builtin_cosf", "cosf"},
                {"__builtin_tan", "tan"}, {"__builtin_tanf", "tanf"},
                {"__builtin_atan", "atan"}, {"__builtin_atanf", "atanf"},
                {"__builtin_atan2", "atan2"}, {"__builtin_atan2f", "atan2f"},
                {"__builtin_log", "log"}, {"__builtin_logf", "logf"},
                {"__builtin_log10", "log10"}, {"__builtin_log10f", "log10f"},
                {"__builtin_log2", "log2"}, {"__builtin_log2f", "log2f"},
                {"__builtin_exp", "exp"}, {"__builtin_expf", "expf"},
                {"__builtin_ldexp", "ldexp"}, {"__builtin_ldexpf", "ldexpf"},
                {"__builtin_frexp", "frexp"}, {"__builtin_frexpf", "frexpf"},
                {"__builtin_modf", "modf"}, {"__builtin_modff", "modff"},
                {"__builtin_copysign", "copysign"}, {"__builtin_copysignf", "copysignf"},
                {"__builtin_fmax", "fmax"}, {"__builtin_fmaxf", "fmaxf"},
                {"__builtin_fmin", "fmin"}, {"__builtin_fminf", "fminf"},
                {"__builtin_fdim", "fdim"}, {"__builtin_fdimf", "fdimf"},
                {"__builtin_signbit", "signbit"}, {"__builtin_signbitf", "signbitf"},
                {"__builtin_setjmp", "setjmp"},
                {"__builtin_longjmp", "longjmp"},
                {"__builtin_vprintf", "vprintf"},
            };
            auto it = builtinAliases.find(actualName);
            if (it != builtinAliases.end()) {
                actualName = it->second;
            }
        }
        if (auto al = aliasNames_.find(actualName); al != aliasNames_.end()) actualName = al->second;

        auto enumIt = enumConstants.find(name);
        if (enumIt != enumConstants.end()) {
            return setPos(std::make_unique<IntegerLiteral>(enumIt->second), nameToken);
        }

        // Static struct method call: StructName::method(args)
        if (match(TokenType::SCOPE_RESOLUTION)) {
            std::string methodName = expect(TokenType::IDENTIFIER, "Expected method name after '::'").value;
            std::string mangledName = actualName + "__" + methodName;
            expect(TokenType::OPEN_PAREN, "Expected '(' after '" + actualName + "::" + methodName + "'");
            auto call = setPos(std::make_unique<FunctionCall>(mangledName), nameToken);
            if (peek().type != TokenType::CLOSE_PAREN) {
                do {
                    call->arguments.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::CLOSE_PAREN, "Expected ')'");
            expr = std::move(call);
        } else if (match(TokenType::OPEN_PAREN)) {
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
        int longs = 0;
        // Suffixes in any order: L, LL, U, UL, LU, ULL, LLU (long long is long
        // here, but for a value a 32-bit long cannot hold: see wideKindOf)
        while (!valStr.empty() && std::strchr("LlUu", valStr.back())) {
            if (valStr.back() == 'U' || valStr.back() == 'u') isUnsigned = true;
            else { isLong = true; longs++; }
            valStr.pop_back();
        }
        int64_t val = (int64_t)std::stoull(valStr, nullptr, 0);
        // a character constant is the value of its byte as a char: negative
        // from $80 up when plain char is signed ('S' is $D3: -45)
        if (litToken.charConst && val >= 0x80 && val <= 0xFF && signedDefault && !unsignedChar) val -= 0x100;
        auto lit = std::make_unique<IntegerLiteral>(val);
        lit->isLongLong = longs >= 2;
        if (isUnsigned && !isLong && (uint64_t)val > 0xFFFF) isLong = true; // 0x80000001U
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
        std::string concatMask = litToken.escMask;
        bool wide = litToken.wide;
        int prefixEncoding = litToken.strEncoding;  // the first literal's encoding wins
        bool escaped = litToken.escaped;
        // C string literal concatenation: "hello" " world" → "hello world"
        while (peek().type == TokenType::STRING_LITERAL || peek().type == TokenType::ASCII_STRING_LITERAL) {
            const Token& next = advance();
            concatStr += next.value;
            concatMask += next.escMask;
            wide = wide || next.wide;
            escaped = escaped || next.escaped;
        }
        auto strLit = std::make_unique<StringLiteral>(concatStr, isAscii);
        strLit->escMask = concatMask;
        strLit->isWide = wide;
        strLit->prefixEncoding = isAscii ? -1 : prefixEncoding;
        strLit->noescape = noescapeStrings_ && !escaped;
        expr = setPos(std::move(strLit), litToken);
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
        throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected expression. Found '" + foundStr + "' (" + peek().typeToString() + ") instead."));
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
                throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected type name in _Generic association"));
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

// The length of an unsized array from its initializer: the highest element
// it sets + 1 ({ [2] = 5, [0] = 1 } has 3 elements)
int Parser::initListLength(const InitializerList& il) {
    int n = 0, at = 0;
    for (size_t i = 0; i < il.elements.size(); i++) {
        auto d = il.getDesignator(i);
        if (d.arrayIndex >= 0) at = d.arrayIndex;
        n = std::max(n, ++at);
    }
    return n;
}

std::unique_ptr<Expression> Parser::parseInitializerList() {
    const Token& startToken = expect(TokenType::OPEN_BRACE, "Expected '{'");
    auto initList = setPos(std::make_unique<InitializerList>(), startToken);
    bool hasDesignators = false;
    if (peek().type != TokenType::CLOSE_BRACE) {
        do {
            // Designator chain: .member, [index], or several (.a.x, [1].y, [2][0]);
            // .a.x = v is taken as .a = { .x = v }
            std::vector<InitializerList::Designator> parts;
            size_t save = pos;
            while (true) {
                if (peek().type == TokenType::DOT && pos + 1 < tokens.size() &&
                    tokens[pos + 1].type == TokenType::IDENTIFIER) {
                    advance();
                    InitializerList::Designator d;
                    d.memberName = advance().value;
                    parts.push_back(d);
                } else if (peek().type == TokenType::OPEN_SQUARE && pos + 2 < tokens.size() &&
                           tokens[pos + 1].type == TokenType::INTEGER_LITERAL &&
                           tokens[pos + 2].type == TokenType::CLOSE_SQUARE) {
                    advance();
                    InitializerList::Designator d;
                    std::string v = advance().value;
                    while (!v.empty() && std::strchr("uUlL", v.back())) v.pop_back();
                    d.arrayIndex = (int)std::stoll(v, nullptr, 0);
                    advance();
                    parts.push_back(d);
                } else if (peek().type == TokenType::OPEN_SQUARE) {
                    // [constant expression]: an enum constant, (2), 1 + 1
                    size_t at = pos;
                    advance();
                    long idx = 0;
                    bool ok = false;
                    try {
                        auto e = parseConditional();
                        ok = e && evalConstInt(e.get(), idx) && idx >= 0 && peek().type == TokenType::CLOSE_SQUARE;
                    } catch (const std::exception&) { ok = false; }
                    if (!ok) { pos = at; break; }
                    advance();   // ]
                    InitializerList::Designator d;
                    d.arrayIndex = (int)idx;
                    parts.push_back(d);
                } else {
                    break;
                }
            }
            if (!parts.empty() && !match(TokenType::EQUALS)) {
                // .member without '=' (GNU old style) is accepted; [n] needs '='
                if (parts[0].arrayIndex >= 0) { pos = save; parts.clear(); }
            }
            InitializerList::Designator desig;
            if (!parts.empty()) { desig = parts[0]; hasDesignators = true; }
            initList->designators.push_back(desig);
            std::unique_ptr<Expression> elem;
            if (peek().type == TokenType::OPEN_BRACE) elem = parseInitializerList();
            else elem = parseConditional();
            for (size_t k = parts.size(); k-- > 1;) {
                auto inner = setPos(std::make_unique<InitializerList>(), startToken);
                inner->designators.push_back(parts[k]);
                inner->elements.push_back(std::move(elem));
                elem = std::move(inner);
            }
            initList->elements.push_back(std::move(elem));
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
           t == TokenType::INT_N || t == TokenType::UINT_N || t == TokenType::ZP ||
           t == TokenType::FAR || t == TokenType::NEAR ||
           (t == TokenType::IDENTIFIER && isTypedef(tokens[look].value));
}

Parser::TypeSpec Parser::parseTypeSpecifier() {
    TypeSpec ts = parseTypeSpecifierRaw();
    // Plain integer types follow the active default (cc45: unsigned)
    if (ts.valid && !ts.fromTypedef && !ts.isSigned && !ts.isUnsigned &&
        (ts.name == "char" || ts.name == "int" || ts.name == "long"))
        ts.isSigned = signedDefault && !(unsignedChar && ts.name == "char");
    return ts;
}

// __far on a pointer type (T __far *p, __far T *p): the pointer the first
// '*' makes is a far pointer (#68), a 4-byte scalar cc45 names "__farp T";
// the other levels are plain pointers (T __far **pp: a pointer to one). A
// signed T is "__farp signed T"; the far pointer itself is unsigned.
// __near (addressSpace 4) likewise: "__nearp T", a base-page offset.
bool Parser::farPointer(std::string& type, int& ptrLevel, int& addressSpace, bool& isSigned) {
    if ((addressSpace != 3 && addressSpace != 4) || ptrLevel < 1 || isFarPointerType(type) || isNearPointerType(type))
        return false;
    type = std::string(addressSpace == 4 ? "__nearp " : "__farp ") + (isSigned ? "signed " : "") + type;
    isSigned = false;
    ptrLevel--;
    addressSpace = 0;
    return true;
}

Parser::TypeSpec Parser::parseTypeSpecifierRaw() {
    TypeSpec ts;

    // Consume leading qualifiers (Phase 97: include address space qualifiers)
    while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT) ||
           match(TokenType::ZP) || match(TokenType::ABS) || match(TokenType::FAR) || match(TokenType::NEAR) ||
           tryParseAttribute() || match(TokenType::EXTENSION)) {
        if (tokens[pos-1].type == TokenType::CONST) ts.isConst = true;
        if (tokens[pos-1].type == TokenType::VOLATILE) ts.isVolatile = true;
        // Phase 97: Address space qualifiers
        if (tokens[pos-1].type == TokenType::ZP) ts.addressSpace = 1;    // ZP
        if (tokens[pos-1].type == TokenType::ABS) ts.addressSpace = 2;   // ABS
        if (tokens[pos-1].type == TokenType::FAR) ts.addressSpace = 3;   // FAR
        if (tokens[pos-1].type == TokenType::NEAR) ts.addressSpace = 4;  // NEAR (a base-page pointer)
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
            ts.fromTypedef = true;
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
        ts.fromTypedef = true;
        ts.pointerLevel = ta.pointerLevel;
        ts.arrayDims = ta.arrayDims;
        ts.vlaDimVars = ta.vlaDimVars;
        ts.isFunctionPointer = ta.isFunctionPointer;
        ts.funcPtrSig = ta.funcPtrSig;
    }
    else {
        // No type found
        return ts; // ts.valid = false, ts.name = ""
    }

    ts.valid = true;

    // Consume trailing qualifiers (Phase 97: include address space qualifiers)
    while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT) ||
           match(TokenType::ZP) || match(TokenType::ABS) || match(TokenType::FAR) || match(TokenType::NEAR) ||
           tryParseAttribute() || match(TokenType::EXTENSION)) {
        if (tokens[pos-1].type == TokenType::CONST) ts.isConst = true;
        if (tokens[pos-1].type == TokenType::VOLATILE) ts.isVolatile = true;
        // Phase 97: Address space qualifiers
        if (tokens[pos-1].type == TokenType::ZP) ts.addressSpace = 1;    // ZP
        if (tokens[pos-1].type == TokenType::ABS) ts.addressSpace = 2;   // ABS
        if (tokens[pos-1].type == TokenType::FAR) { ts.addressSpace = 3; ts.farAfterType = true; }
        if (tokens[pos-1].type == TokenType::NEAR) ts.addressSpace = 4;
    }

    // If type is followed by __complex__ (reverse order: float __complex__)
    if (match(TokenType::COMPLEX)) {
        if (ts.name == "float" || ts.name == "double") ts.name = "struct _Complex_float";
        else ts.name = "struct _Complex_int";
    }

    // Consume pointer stars
    while (match(TokenType::STAR)) {
        ts.pointerLevel++;
        ts.starsParsed++;
        ts.lastStarConst = false;
        ts.lastStarRestrict = false;
        while (match(TokenType::CONST) || match(TokenType::VOLATILE) || match(TokenType::RESTRICT)) {
            if (tokens[pos-1].type == TokenType::CONST) { ts.isConst = true; ts.lastStarConst = true; }
            if (tokens[pos-1].type == TokenType::RESTRICT) ts.lastStarRestrict = true;
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

// (as written in messages: $800 for attic RAM's)
static std::string bankShown(int b) {
    char t[16];
    snprintf(t, sizeof t, b > 255 ? "$%X" : "%d", b);
    return t;
}

void Parser::applyFunctionSection(FunctionDeclaration& f) {
    std::string n = takePendingSection(true);
    if (n.empty()) {
        auto it = functionSections_.find(f.name);
        if (it != functionSections_.end()) n = it->second;
    } else {
        auto it = functionSections_.find(f.name);
        if (it != functionSections_.end() && it->second != n) {
            const Token& t = tokens[pendingSectionTok_ < tokens.size() ? pendingSectionTok_ : 0];
            throw std::runtime_error(formatDiagnostic(t.sourceFile, t.line, t.column, Severity::Error,
                "'" + f.name + "' is declared in section '" + it->second + "' and in section '" + n + "'"));
        }
        functionSections_[f.name] = n;
    }
    f.section = n;
    if (!n.empty()) f.isNoInline = true;   // (its code runs in the section)
    // __bank(n): a __far function kept in bank n (a prototype's bank too)
    int b = takePendingBank();
    if (b < 0) {
        auto it = functionBanks_.find(f.name);
        if (it != functionBanks_.end()) b = it->second;
    } else {
        if (b > 5) {
            const Token& t = tokens[pendingBankTok_ < tokens.size() ? pendingBankTok_ : 0];
            throw std::runtime_error(formatDiagnostic(t.sourceFile, t.line, t.column, Severity::Error,
                "__bank(" + bankShown(b) + ") on a function: code runs in chip RAM, banks 4 and 5 (attic RAM holds data)"));
        }
        functionBanks_[f.name] = b;
    }
    if (b >= 0) { f.bank = b; f.isFar = true; }
}

// The bank of the declaration __bank(n) is on: chip RAM banks 4 and 5, or
// attic RAM ($800-$87F, 64 KB banks of $8000000-$87FFFFF)
int Parser::takePendingBank() {
    int b = pendingBank_;
    pendingBank_ = -1;
    if (b < 0) return -1;
    if (b != 4 && b != 5 && !(b >= 0x800 && b <= 0x87F)) {
        const Token& t = tokens[pendingBankTok_ < tokens.size() ? pendingBankTok_ : 0];
        throw std::runtime_error(formatDiagnostic(t.sourceFile, t.line, t.column, Severity::Error,
            "__bank(" + bankShown(b) + "): the banks are 4 and 5 (chip RAM) and $800-$87F (attic RAM)"));
    }
    return b;
}

std::string Parser::takePendingSection(bool variable) {
    if (!variable && pendingBank_ >= 0) {
        const Token& t = tokens[pendingBankTok_ < tokens.size() ? pendingBankTok_ : 0];
        pendingBank_ = -1;
        throw std::runtime_error(formatDiagnostic(t.sourceFile, t.line, t.column, Severity::Error,
            "__bank(n) places variables and functions, not members or types"));
    }
    std::string n = pendingSection_;
    bool strict = pendingSectionStrict_;
    pendingSection_.clear();
    pendingSectionStrict_ = false;
    if (n.empty()) return n;
    const Token& t = tokens[pendingSectionTok_ < tokens.size() ? pendingSectionTok_ : 0];
    auto fail = [&](const std::string& msg) {
        throw std::runtime_error(formatDiagnostic(t.sourceFile, t.line, t.column, Severity::Error, msg));
    };
    if (!variable) {
        if (strict) fail("__section(\"" + n + "\") places variables and functions, not members or types");
        return "";
    }
    if (!sections_.count(n)) {
        if (strict) fail("section '" + n + "' is not declared: #pragma section(\"" + n + "\", address) before its variables");
        return "";   // (GCC's section("x"): ignored, as before)
    }
    return n;
}

bool Parser::tryParseAttribute() {
    if (!match(TokenType::ATTRIBUTE)) return false;

    // Expect (( ... ))
    if (!match(TokenType::OPEN_PAREN)) return true;
    if (!match(TokenType::OPEN_PAREN)) return true;

    // Parse comma-separated attribute list until ))
    while (peek().type != TokenType::CLOSE_PAREN && peek().type != TokenType::END_OF_FILE) {
        // const (__const__) is a keyword: the function attribute (Issue #76)
        if (peek().type == TokenType::CONST) {
            advance();
            pendingPurity_ = 2;
            continue;
        }
        if (peek().type == TokenType::IDENTIFIER) {
            std::string attr = advance().value;
            // Strip leading/trailing underscores for canonical form
            std::string canonical = attr;
            if (canonical.size() > 4 && canonical.substr(0, 2) == "__" &&
                canonical.substr(canonical.size() - 2) == "__")
                canonical = canonical.substr(2, canonical.size() - 4);

            if (canonical == "always_inline") pendingAlwaysInline_ = true;
            if (canonical == "noinline") pendingNoInline_ = true;
            // noescape on an object: keep it with its code (Issue #301); on
            // a parameter (Clang's meaning) it has no effect
            if (canonical == "noescape") pendingNoescape_ = true;
            // pure / const on a function (Issue #76): no side effects / the
            // result depends only on the arguments
            if (canonical == "pure") pendingPurity_ = std::max(pendingPurity_, 1);
            if (canonical == "const") pendingPurity_ = 2;

            // vector_size(N): a GCC vector of N bytes; mode(QI/HI/SI/...): an
            // integer of that width. Both apply to the declaration's type
            if (canonical == "vector_size") {
                expect(TokenType::OPEN_PAREN, "Expected '(' after vector_size");
                auto e = parseConditional();
                long v;
                if (evalConstInt(e.get(), v) && v > 0) pendingVectorSize_ = (int)v;
                expect(TokenType::CLOSE_PAREN, "Expected ')' after vector_size");
                match(TokenType::COMMA);
                continue;
            }
            if (canonical == "mode") {
                expect(TokenType::OPEN_PAREN, "Expected '(' after mode");
                std::string m = expect(TokenType::IDENTIFIER, "Expected machine mode").value;
                if (m.size() > 4 && m.compare(0, 2, "__") == 0) m = m.substr(2, m.size() - 4);
                pendingMode_ = m;
                expect(TokenType::CLOSE_PAREN, "Expected ')' after mode");
                match(TokenType::COMMA);
                continue;
            }

            // cc45_section("NAME") (__section) / section("NAME") (GCC): the
            // variable goes in the section NAME (Issue #70)
            // cc45_bank(n) (__bank): kept in bank n (#69)
            if (canonical == "cc45_bank") {
                expect(TokenType::OPEN_PAREN, "Expected '(' after __bank");
                pendingBankTok_ = pos;
                auto e = parseConditional();
                long v;
                if (!evalConstInt(e.get(), v))
                    throw std::runtime_error(formatDiagnostic(tokens[pendingBankTok_].sourceFile, tokens[pendingBankTok_].line,
                        tokens[pendingBankTok_].column, Severity::Error, "__bank(n): n must be a constant"));
                pendingBank_ = (int)v;
                expect(TokenType::CLOSE_PAREN, "Expected ')' after the bank");
                match(TokenType::COMMA);
                continue;
            }
            if (canonical == "cc45_section" || canonical == "section") {
                expect(TokenType::OPEN_PAREN, "Expected '(' after section");
                pendingSectionTok_ = pos;
                pendingSection_ = expect(TokenType::STRING_LITERAL, "Expected a section name (a string)").value;
                pendingSectionStrict_ = canonical == "cc45_section";
                expect(TokenType::CLOSE_PAREN, "Expected ')' after the section name");
                match(TokenType::COMMA);
                continue;
            }

            if (canonical == "alias") {
                expect(TokenType::OPEN_PAREN, "Expected '(' after alias");
                pendingAlias_ = expect(TokenType::STRING_LITERAL, "Expected alias target name").value;
                expect(TokenType::CLOSE_PAREN, "Expected ')' after alias target");
                match(TokenType::COMMA);
                continue;
            }

            if (canonical == "aligned") {
                // aligned(N), or plain aligned: the widest natural alignment (2)
                int a = 2;
                if (match(TokenType::OPEN_PAREN)) {
                    auto e = parseConditional();
                    long v;
                    if (evalConstInt(e.get(), v) && v > 0) a = (int)v;
                    else std::cerr << tokens[pos-1].line << ": warning: __attribute__((aligned)) needs a constant; using 2\n";
                    expect(TokenType::CLOSE_PAREN, "Expected ')' after aligned argument");
                }
                if (a & (a - 1))
                    std::cerr << tokens[pos-1].line << ": warning: alignment " << a << " is not a power of two\n";
                pendingAlign_ = std::max(pendingAlign_, a);
                match(TokenType::COMMA);
                continue;
            }

            // Recognized — silently accept
            if (canonical == "noinline" || canonical == "noclone" || canonical == "packed" || canonical == "noescape" ||
                canonical == "always_inline" || canonical == "gnu_inline" ||
                canonical == "unused" || canonical == "used" || canonical == "weak" ||
                canonical == "const" || canonical == "pure" || canonical == "flatten" ||
                canonical == "cold" || canonical == "hot" || canonical == "nothrow" ||
                canonical == "returns_twice" || canonical == "warn_unused_result" ||
                canonical == "deprecated" || canonical == "visibility" ||
                canonical == "nonnull" || canonical == "sentinel" ||
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
            else if (canonical == "noipa" ||
                     
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
    int tdSpace = 0;   // (3: __far)
    std::string baseType;
    bool isSigned = false;
    int basePtrLevel = 0;
    std::vector<int> typedefArrayDims;

    {
        auto ts = parseTypeSpecifier();
        if (!ts.valid) {
            throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected type in typedef"));
        }
        baseType = ts.name;
        isSigned = ts.isSigned;
        basePtrLevel = ts.pointerLevel;
        typedefArrayDims = ts.arrayDims;
        tdSpace = ts.addressSpace;
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
    std::vector<std::string> vlaDimVars;
    while (match(TokenType::OPEN_SQUARE)) {
        if (match(TokenType::CLOSE_SQUARE)) {
            arrayDims.push_back(0);
            vlaDimVars.push_back("");
        } else {
            auto szExpr = parseExpression();
            int arrSize = 1;
            long cv;
            if (evalConstInt(szExpr.get(), cv)) {
                arrSize = (int)cv;
                vlaDimVars.push_back("");
            } else if (!currentFunctionName.empty()) {
                // A run-time dimension: evaluated here, into a hidden variable
                std::string vn = "__tdv" + std::to_string(typedefVlaCounter_++) + "_" + newName;
                auto d = std::make_unique<VariableDeclaration>("int", vn, 0);
                d->initializer = std::move(szExpr);
                typedefVlaDecls_.push_back(std::move(d));
                vlaDimVars.push_back(vn);
            } else {
                vlaDimVars.push_back("");
            }
            arrayDims.push_back(arrSize);
            expect(TokenType::CLOSE_SQUARE, "Expected ']'");
        }
    }
    // Skip __attribute__ after alias: typedef int T __attribute__((unused));
    while (tryParseAttribute()) {}
    expect(TokenType::SEMICOLON, "Expected ';' after typedef");

    farPointer(baseType, ptrLevel, tdSpace, isSigned);   // typedef T __far *name
    applyVectorMode(baseType, ptrLevel, isSigned);
    TypeAlias ta{baseType, ptrLevel, isSigned, false, nullptr, arrayDims};
    bool anyVla = false;
    for (const auto& v : vlaDimVars) if (!v.empty()) anyVla = true;
    if (anyVla) {
        // the typedef's own dimensions come after any it was built from
        ta.vlaDimVars.assign(typedefArrayDims.size(), "");
        ta.vlaDimVars.insert(ta.vlaDimVars.end(), vlaDimVars.begin(), vlaDimVars.end());
    }
    typedefs[newName] = ta;
}

// mode(QI/HI/SI/word) picks the integer of that width; vector_size(N) makes
// the type a GCC vector: struct __vec_<elem>_<n> { T e[n]; } (one definition
// per element type and count, so equal vector types are the same struct)
void Parser::applyVectorMode(std::string& type, int ptrLevel, bool& isSigned) {
    if (!pendingMode_.empty()) {
        const std::string& m = pendingMode_;
        if (m == "QI") type = "char";
        else if (m == "HI" || m == "word" || m == "pointer") type = "int";
        else if (m == "SI") type = "long";
        pendingMode_.clear();
    }
    if (pendingVectorSize_ > 0 && ptrLevel == 0) {
        int total = pendingVectorSize_;
        pendingVectorSize_ = 0;
        int es = scalarTypeSize(type, 0);
        if (es <= 0) return;
        int n = total / es;
        if (n < 1) n = 1;
        std::string tag;
        for (char c : type) tag += std::isalnum((unsigned char)c) ? c : '_';
        std::string name = "__vec_" + tag + (isSigned ? "_s_" : "_u_") + std::to_string(n);
        if (!vectorTypes_.count(name)) {
            vectorTypes_.insert(name);
            auto def = std::make_unique<StructDefinition>(name, false);
            StructMember sm;
            sm.type = type;
            sm.pointerLevel = 0;
            sm.isSigned = isSigned;
            sm.name = "e";
            sm.arrayDims = {n};
            def->members.push_back(std::move(sm));
            structs[name] = def.get();
            pendingDefinitions.push_back(std::move(def));
        }
        type = "struct " + name;
        isSigned = false;
    }
    pendingVectorSize_ = 0;
}

Parser::DeclType Parser::typeofExpr(Expression* e) {
    if (auto* vr = dynamic_cast<VariableReference*>(e)) {
        auto it = declTypes_.find(vr->name);
        if (it != declTypes_.end()) return it->second;
    }
    return {"int", 0, true};
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
    // (noescape on a parameter, as in Clang, is not the declaration's)
    struct KeepNoescape { bool& f; bool v; ~KeepNoescape() { f = v; } } keepNoescape{pendingNoescape_, pendingNoescape_};
    if (peek().type == TokenType::VOID && pos + 1 < tokens.size() && tokens[pos + 1].type == TokenType::CLOSE_PAREN) {
        advance(); // consume 'void'
    } else if (peek().type == TokenType::CLOSE_PAREN) {
        sig->prototyped = false;
    } else {
        do {
            // ... ends a variadic parameter list
            if (match(TokenType::ELLIPSIS)) { sig->isVariadic = true; break; }
            FuncPtrSignature::Param fp;
            {
                auto ts = parseTypeSpecifier();
                if (!ts.valid) {
                    throw std::runtime_error(formatDiagnostic(peek().sourceFile, peek().line, peek().column, Severity::Error, "Expected type in function pointer parameter list"));
                }
                fp.type = ts.name;
                fp.isSigned = ts.isSigned;
                fp.pointerLevel = ts.pointerLevel;
            }
            // A function or function pointer parameter (void (), int (*)(int),
            // int (*name)(int)): a pointer; the declarator is skipped
            if (peek().type == TokenType::OPEN_PAREN) {
                while (peek().type == TokenType::OPEN_PAREN) {
                    int depth = 0;
                    do {
                        if (peek().type == TokenType::OPEN_PAREN) depth++;
                        else if (peek().type == TokenType::CLOSE_PAREN) depth--;
                        advance();
                    } while (depth > 0 && peek().type != TokenType::END_OF_FILE);
                }
                fp.type = "void";
                fp.pointerLevel = 1;
                fp.isSigned = false;
            }
            // Skip optional parameter name, then its attributes (int *p
            // __attribute__((noescape))) and an array suffix (int a[], a
            // pointer)
            if (peek().type == TokenType::IDENTIFIER && !isTypedef(peek().value)) advance();
            while (tryParseAttribute()) {}
            while (peek().type == TokenType::OPEN_SQUARE) {
                while (peek().type != TokenType::CLOSE_SQUARE && peek().type != TokenType::END_OF_FILE) advance();
                match(TokenType::CLOSE_SQUARE);
                fp.pointerLevel++;
            }
            while (tryParseAttribute()) {}
            sig->params.push_back(fp);
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::CLOSE_PAREN, "Expected ')' after function pointer parameter list");
    return sig;
}

int Parser::scalarTypeSize(const std::string& type, int ptrLevel) {
    if (ptrLevel > 0) return 2;
    std::string t = type;
    for (const char* q : {"unsigned ", "signed ", "const ", "volatile "})
        for (size_t at; (at = t.find(q)) != std::string::npos; ) t.erase(at, std::strlen(q));
    if (t == "char" || t == "_Bool" || t == "bool" || t == "__bit") return 1;
    if (t == "short" || t == "int" || t == "unsigned" || t == "signed" || t == "short int") return 2;
    if (int fb = fixedTypeBits(t)) return fb / 8;
    if (isFarPointerType(t)) return 4;
    if (isNearPointerType(t)) return 1;
    if (t == "long" || t == "long int") return 4;
    if (t == "long long" || t == "long long int") return 8;
    if (t == "float" || t == "double" || t == "long double") return 5;
    return -1;
}

// Max bitfield width: char 8, int 16, long 32 (a wider field: long, or
// __int(N)). A wider one would silently lose its top bits.
int Parser::parseBitfieldWidth(const std::string& type, const std::string& name, bool unnamed) {
    // (an integer constant expression: int b : 2 + 1, int c : sizeof(int) * 4)
    const Token& bwTok = peek();
    auto widthExpr = parseConditional();
    long cv;
    if (!evalConstInt(widthExpr.get(), cv))
        throw std::runtime_error(formatDiagnostic(bwTok.sourceFile, bwTok.line, bwTok.column, Severity::Error,
            "Expected an integer constant expression for the bitfield width" + (name.empty() ? std::string() : " of '" + name + "'")));
    int bw = (int)cv;
    if (type.rfind("struct ", 0) == 0 || type.rfind("union ", 0) == 0)   // long long: struct __int64
        throw std::runtime_error(formatDiagnostic(bwTok.sourceFile, bwTok.line, bwTok.column, Severity::Error,
            "Bitfield of type '" + type + "' not supported" + (name.empty() ? "" : " (member '" + name + "')") +
            "; use long (at most 32 bits)"));
    int maxBits = (type == "long") ? 32 : (type == "char") ? 8 : 16;
    if (bw < (unnamed ? 0 : 1) || bw > maxBits)
        throw std::runtime_error(formatDiagnostic(bwTok.sourceFile, bwTok.line, bwTok.column, Severity::Error,
            "Bitfield width " + std::to_string(bw) + " out of range for type '" + type + "'" +
            (name.empty() ? "" : " (member '" + name + "')") +
            (bw > maxBits ? " (at most " + std::to_string(maxBits) + " bits)" : "")));
    return bw;
}

int Parser::typeByteSize(const std::string& type, int ptrLevel, int* natAlign, int* declAlign, int depth) {
    int na = 1, da = 1, size = -1;
    if (depth > 32) return -1;
    std::string t = type;
    for (const char* q : {"const ", "volatile "})
        for (size_t at; (at = t.find(q)) != std::string::npos; ) t.erase(at, std::strlen(q));
    auto td = typedefs.find(t);
    if (ptrLevel > 0) {
        size = 2; na = 2;
    } else if (td != typedefs.end() && !td->second.isFunctionPointer) {
        const TypeAlias& a = td->second;
        for (const auto& v : a.vlaDimVars) if (!v.empty()) return -1;
        size = typeByteSize(a.baseType, a.pointerLevel, &na, &da, depth + 1);
        if (size < 0) return -1;
        for (int d : a.arrayDims) size *= d;
    } else if (td != typedefs.end()) {
        size = 2; na = 2;
    } else if (t.rfind("enum ", 0) == 0) {
        size = 2; na = 2;
    } else if (t.rfind("struct ", 0) == 0 || t.rfind("union ", 0) == 0) {
        auto it = structs.find(t.substr(t.find(' ') + 1));
        if (it == structs.end() || !it->second) return -1;
        StructDefinition* def = it->second;
        int startOffset = 0, startAlign = 1, startNatural = 1;
        if (!def->parentStruct.empty()) {
            int pna = 1, pda = 1;
            startOffset = typeByteSize("struct " + def->parentStruct, 0, &pna, &pda, depth + 1);
            if (startOffset < 0) return -1;
            startAlign = pda; startNatural = pna;
        }
        std::vector<StructLayout::Member> lm;
        for (const auto& m : def->members) {
            for (const auto& d : m.vlaDimExprs) if (d) return -1;
            StructLayout::Member l;
            int mna = 1, mda = 1;
            int es = typeByteSize(m.type, m.bitWidth > 0 ? 0 : m.pointerLevel, &mna, &mda, depth + 1);
            if (es < 0) return -1;
            if (m.bitWidth > 0) {
                l.unitType = m.type;
                l.size = es;
                l.bitWidth = m.bitWidth;
            } else {
                l.size = es;
                if (m.arraySize() >= 0) l.size *= m.arraySize();
                long ea = std::max(1, m.alignment);
                if (m.alignmentExpr) {
                    long cv;
                    if (!evalConstInt(m.alignmentExpr.get(), cv)) return -1;
                    ea = std::max(ea, cv);
                }
                l.explicitAlign = std::max((int)ea, m.pointerLevel > 0 ? 1 : mda);
            }
            l.naturalAlign = mna;
            lm.push_back(l);
        }
        StructLayout::Result r = StructLayout::layout(lm, def->isUnion, def->isUnpacked, def->alignment,
                                                      startOffset, startAlign, startNatural);
        size = r.size; na = r.naturalAlign; da = r.align;
    } else {
        size = scalarTypeSize(t, 0);
        if (size < 0) return -1;
        bool isFloat = t == "float" || t == "double" || t == "long double";
        na = (!isFloat && (size == 2 || size == 4)) ? size : 1;
    }
    if (natAlign) *natAlign = na;
    if (declAlign) *declAlign = da;
    return size;
}

void Parser::recordVarSize(const std::string& name, const std::string& type, int ptrLevel, const std::vector<int>& dims) {
    varShapes_[name] = {type, ptrLevel, dims};
    int es = typeByteSize(type, ptrLevel);
    if (es > 0) {
        long n = es;
        for (int d : dims) n *= d;
        varByteSizes[name] = n;
    } else {
        varByteSizes.erase(name);
    }
}

void Parser::hideEnumConstant(const std::string& name) {
    if (enumScopes_.empty()) return;   // file scope: a redeclaration
    auto it = enumConstants.find(name);
    if (it == enumConstants.end()) return;
    enumScopes_.back().push_back({name, it->second});
    enumConstants.erase(it);
}

void Parser::setEnumConstant(const std::string& name, int value) {
    if (!enumScopes_.empty()) {
        auto it = enumConstants.find(name);
        enumScopes_.back().push_back({name, it == enumConstants.end() ? std::nullopt : std::optional<int>(it->second)});
    }
    enumConstants[name] = value;
}

Parser::EnumScope::~EnumScope() {
    auto& saved = p.enumScopes_.back();
    for (auto it = saved.rbegin(); it != saved.rend(); ++it) {
        if (it->second) p.enumConstants[it->first] = *it->second;
        else p.enumConstants.erase(it->first);
    }
    p.enumScopes_.pop_back();
}

bool Parser::evalConstInt(Expression* e, long& out) {
    ConstValue v;
    if (!ConstEval::eval(e, *this, v) || !v.isInt()) return false;
    out = (long)v.intValue();
    return true;
}

bool Parser::identifier(VariableReference& node, ConstValue& out) {
    auto it = enumConstants.find(node.name);
    if (it == enumConstants.end()) return false;
    out = ConstValue::ofInt(it->second);
    return true;
}

bool Parser::sizeOf(SizeofExpression& sz, int64_t& out) {
    if (sz.isType) {
        int s = typeByteSize(sz.typeName, sz.pointerLevel);
        if (s < 0) return false;
        int64_t n = s;
        for (int d : sz.arrayDims) n *= d;
        out = n;
        return true;
    }
    if (auto* v = dynamic_cast<VariableReference*>(sz.expression.get())) {
        auto it = varByteSizes.find(v->name);
        if (it == varByteSizes.end()) return false;
        out = it->second;
        return true;
    }
    // sizeof x[i][j] / sizeof *p: each subscript or * drops an array
    // dimension, then a pointer level
    Expression* x = sz.expression.get();
    int depth = 0;
    for (;;) {
        if (auto* aa = dynamic_cast<ArrayAccess*>(x)) { x = aa->arrayExpr.get(); depth++; continue; }
        if (auto* un = dynamic_cast<UnaryOperation*>(x); un && un->op == "*") { x = un->operand.get(); depth++; continue; }
        break;
    }
    auto* v = dynamic_cast<VariableReference*>(x);
    if (!v || !depth) return false;
    auto it = varShapes_.find(v->name);
    if (it == varShapes_.end()) return false;
    const VarShape& sh = it->second;
    int ptr = sh.ptrLevel;
    size_t dimsLeft = sh.dims.size();
    for (int k = 0; k < depth; k++) {
        if (dimsLeft) dimsLeft--;
        else if (ptr > 0) ptr--;
        else return false;
    }
    int es = typeByteSize(sh.type, ptr);
    if (es < 0) return false;
    int64_t n = es;
    for (size_t k = sh.dims.size() - dimsLeft; k < sh.dims.size(); k++) n *= sh.dims[k];
    out = n;
    return true;
}

// _Alignof(type): 1 (cc45 packs everything) unless the type is declared
// aligned (_Alignas / aligned(N)); an expression's is IRBuilder's to say
bool Parser::alignOf(AlignofExpression& node, int64_t& out) {
    if (node.expression) return false;
    int na = 1, da = 1;
    if (typeByteSize(node.typeName, node.pointerLevel, &na, &da) < 0) return false;
    out = node.pointerLevel ? 1 : std::max(1, da);
    return true;
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
                auto valExpr = parseConditional();
                long cv;
                if (!evalConstInt(valExpr.get(), cv))
                    throw std::runtime_error("Expected an integer constant expression for enum value '" + enumName + "'");
                currentValue = (int)cv;
            }
            setEnumConstant(enumName, currentValue);
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
            t == TokenType::REGISTER || t == TokenType::EXTENSION || t == TokenType::STRIPED) {
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

    // signed / unsigned before the type (unsigned char f(), or unsigned f()
    // with an implicit int)
    bool sawSign = false;
    while (look < tokens.size() && (tokens[look].type == TokenType::SIGNED || tokens[look].type == TokenType::UNSIGNED)) {
        sawSign = true;
        look++;
    }
    if (look >= tokens.size()) return false;
    TokenType t = tokens[look].type;
    if (sawSign && t == TokenType::IDENTIFIER && !isTypedef(tokens[look].value))
        return look + 1 < tokens.size() && tokens[look + 1].type == TokenType::OPEN_PAREN;
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
        
        // Skip any pointers, qualifiers, and attributes interleaved (and the
        // rest of a multi-word type: unsigned long int, long long, long double)
        while (look < tokens.size()) {
            if (tokens[look].type == TokenType::STAR) {
                look++;
            } else if (t != TokenType::IDENTIFIER && t != TokenType::STRUCT && t != TokenType::UNION &&
                       t != TokenType::ENUM &&
                       (tokens[look].type == TokenType::INT || tokens[look].type == TokenType::LONG ||
                        tokens[look].type == TokenType::SHORT || tokens[look].type == TokenType::CHAR ||
                        tokens[look].type == TokenType::DOUBLE || tokens[look].type == TokenType::SIGNED ||
                        tokens[look].type == TokenType::UNSIGNED)) {
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
