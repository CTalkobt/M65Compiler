#pragma once
#include <string>

enum class TokenType {
    // Keywords
    INT,
    CHAR,
    AUTO,
    UNSIGNED,
    SIGNED,
    RETURN,
    VOID,
    CONST,
    IF,
    ELSE,
    WHILE,
    FOR,
    ASM,
    DO,
    STRUCT,
    UNION,
    VOLATILE,
    STATIC_ASSERT,
    ALIGNAS,
    ALIGNOF,
    NORETURN,
    ATTRIBUTE,  // __attribute__
    EXTENSION,  // __extension__ (GCC, skip silently)
    TYPEOF,     // typeof / __typeof__ (GCC)
    BREAK,
    CONTINUE,
    SWITCH,
    CASE,
    DEFAULT,
    GOTO,
    SIZEOF,
    GENERIC,
    TYPEDEF,
    ENUM,
    EXTERN,
    STATIC,
    BOOL,  // _Bool (C99)
    RESTRICT, // restrict (C99)
    SHORT,    // short (alias for int on this target)
    LONG,     // long (32-bit on this target)
    REGISTER, // register (storage class)
    INLINE,   // inline (function specifier, currently no-op)
    FASTCALL, // __fastcall__ (per-function ZP calling convention)
    INTERRUPT, // __interrupt (ISR function attribute)
    NAKED,     // __naked (no prologue/epilogue)
    UNPACKED,  // __unpacked (opt-in alignment padding for structs)
    REPEAT,    // repeat (compile-time loop unrolling)
    REGPARM,   // __regparm (first param in A/AX register)
    FLOAT,     // float (CBM 40-bit)
    DOUBLE,    // double (downgraded to float)
    COMPLEX,   // _Complex / __complex__
    REAL_PART, // __real__ (complex real part)
    IMAG_PART, // __imag__ (complex imaginary part)
    INT_N,     // __int (parameterized width integer)
    UINT_N,    // __uint (parameterized width unsigned integer)
    LONG_LONG, // long long (64-bit integer)

    // Variadic
    ELLIPSIS,  // ...

    // Literals
    IDENTIFIER,
    INTEGER_LITERAL,
    FLOAT_LITERAL,  // 1.5, 3.14f
    IMAGINARY_LITERAL,  // 1.5i, 3.14fi (complex imaginary)
    STRING_LITERAL,
    ASCII_STRING_LITERAL,  // @"..." — raw ASCII, no PETSCII conversion
    
    // Punctuators
    OPEN_PAREN,    // (
    CLOSE_PAREN,   // )
    OPEN_BRACE,    // {
    CLOSE_BRACE,   // }
    SEMICOLON,     // ;
    COLON,         // :
    COMMA,         // ,
    EQUALS,        // =
    PLUS,          // +
    MINUS,         // -
    STAR,          // *
    SLASH,         // /
    PERCENT,       // %
    LESS_THAN,     // <
    GREATER_THAN,  // >
    LESS_EQUAL,    // <=
    GREATER_EQUAL, // >=
    EQUALS_EQUALS, // ==
    NOT_EQUALS,    // !=
    BANG,          // !
    AMPERSAND,     // &
    PIPE,          // |
    CARET,         // ^
    TILDE,         // ~
    LSHIFT,        // <<
    RSHIFT,        // >>
    AND,           // &&
    OR,            // ||
    DOT,           // .
    ARROW,         // ->
    PLUS_PLUS,     // ++
    MINUS_MINUS,   // --
    PLUS_EQUALS,   // +=
    MINUS_EQUALS,  // -=
    STAR_EQUALS,   // *=
    SLASH_EQUALS,  // /=
    PERCENT_EQUALS,// %=
    AMPERSAND_EQUALS, // &=
    PIPE_EQUALS,   // |=
    CARET_EQUALS,  // ^=
    LSHIFT_EQUALS, // <<=
    RSHIFT_EQUALS, // >>=
    QUESTION_MARK, // ?
    OPEN_SQUARE,   // [
    CLOSE_SQUARE,  // ]
    
    // Special
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    std::string sourceFile;  // Filename from #line directive, if any

    std::string typeToString() const {
        switch (type) {
            case TokenType::INT: return "INT";
            case TokenType::CHAR: return "CHAR";
            case TokenType::AUTO: return "AUTO";
            case TokenType::UNSIGNED: return "UNSIGNED";
            case TokenType::SIGNED: return "SIGNED";
            case TokenType::RETURN: return "RETURN";
            case TokenType::VOID: return "VOID";
            case TokenType::CONST: return "CONST";
            case TokenType::IF: return "IF";
            case TokenType::ELSE: return "ELSE";
            case TokenType::WHILE: return "WHILE";
            case TokenType::FOR: return "FOR";
            case TokenType::ASM: return "ASM";
            case TokenType::DO: return "DO";
            case TokenType::STRUCT: return "STRUCT";
            case TokenType::UNION: return "UNION";
            case TokenType::VOLATILE: return "VOLATILE";
            case TokenType::STATIC_ASSERT: return "STATIC_ASSERT";
            case TokenType::ALIGNAS: return "ALIGNAS";
            case TokenType::ALIGNOF: return "ALIGNOF";
            case TokenType::NORETURN: return "NORETURN";
            case TokenType::ATTRIBUTE: return "ATTRIBUTE";
            case TokenType::TYPEOF: return "TYPEOF";
            case TokenType::EXTENSION: return "EXTENSION";
            case TokenType::BREAK: return "BREAK";
            case TokenType::CONTINUE: return "CONTINUE";
            case TokenType::SWITCH: return "SWITCH";
            case TokenType::CASE: return "CASE";
            case TokenType::DEFAULT: return "DEFAULT";
            case TokenType::GOTO: return "GOTO";
            case TokenType::SIZEOF: return "SIZEOF";
            case TokenType::GENERIC: return "GENERIC";
            case TokenType::TYPEDEF: return "TYPEDEF";
            case TokenType::ENUM: return "ENUM";
            case TokenType::EXTERN: return "EXTERN";
            case TokenType::STATIC: return "STATIC";
            case TokenType::BOOL: return "_Bool";
            case TokenType::RESTRICT: return "RESTRICT";
            case TokenType::SHORT: return "SHORT";
            case TokenType::LONG: return "LONG";
            case TokenType::REGISTER: return "REGISTER";
            case TokenType::INLINE: return "INLINE";
            case TokenType::FASTCALL: return "FASTCALL";
            case TokenType::INTERRUPT: return "INTERRUPT";
            case TokenType::NAKED: return "NAKED";
            case TokenType::UNPACKED: return "UNPACKED";
            case TokenType::REPEAT: return "REPEAT";
            case TokenType::REGPARM: return "REGPARM";
            case TokenType::FLOAT: return "FLOAT";
            case TokenType::DOUBLE: return "DOUBLE";
            case TokenType::COMPLEX: return "COMPLEX";
            case TokenType::REAL_PART: return "REAL_PART";
            case TokenType::IMAG_PART: return "IMAG_PART";
            case TokenType::INT_N: return "INT_N";
            case TokenType::UINT_N: return "UINT_N";
            case TokenType::LONG_LONG: return "LONG_LONG";
            case TokenType::ELLIPSIS: return "ELLIPSIS";
            case TokenType::IDENTIFIER: return "IDENTIFIER";
            case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
            case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
            case TokenType::STRING_LITERAL: return "STRING_LITERAL";
            case TokenType::ASCII_STRING_LITERAL: return "ASCII_STRING_LITERAL";
            case TokenType::OPEN_PAREN: return "OPEN_PAREN";
            case TokenType::CLOSE_PAREN: return "CLOSE_PAREN";
            case TokenType::OPEN_BRACE: return "OPEN_BRACE";
            case TokenType::CLOSE_BRACE: return "CLOSE_BRACE";
            case TokenType::OPEN_SQUARE: return "OPEN_SQUARE";
            case TokenType::CLOSE_SQUARE: return "CLOSE_SQUARE";
            case TokenType::SEMICOLON: return "SEMICOLON";
            case TokenType::COLON: return "COLON";
            case TokenType::QUESTION_MARK: return "QUESTION_MARK";
            case TokenType::COMMA: return "COMMA";
            case TokenType::EQUALS: return "EQUALS";
            case TokenType::PLUS: return "PLUS";
            case TokenType::MINUS: return "MINUS";
            case TokenType::STAR: return "STAR";
            case TokenType::SLASH: return "SLASH";
            case TokenType::PERCENT: return "PERCENT";
            case TokenType::LESS_THAN: return "LESS_THAN";
            case TokenType::GREATER_THAN: return "GREATER_THAN";
            case TokenType::LESS_EQUAL: return "LESS_EQUAL";
            case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
            case TokenType::EQUALS_EQUALS: return "EQUALS_EQUALS";
            case TokenType::NOT_EQUALS: return "NOT_EQUALS";
            case TokenType::BANG: return "BANG";
            case TokenType::AMPERSAND: return "AMPERSAND";
            case TokenType::PIPE: return "PIPE";
            case TokenType::CARET: return "CARET";
            case TokenType::TILDE: return "TILDE";
            case TokenType::LSHIFT: return "LSHIFT";
            case TokenType::RSHIFT: return "RSHIFT";
            case TokenType::AND: return "AND";
            case TokenType::OR: return "OR";
            case TokenType::DOT: return "DOT";
            case TokenType::ARROW: return "ARROW";
            case TokenType::PLUS_PLUS: return "PLUS_PLUS";
            case TokenType::MINUS_MINUS: return "MINUS_MINUS";
            case TokenType::PLUS_EQUALS: return "PLUS_EQUALS";
            case TokenType::MINUS_EQUALS: return "MINUS_EQUALS";
            case TokenType::STAR_EQUALS: return "STAR_EQUALS";
            case TokenType::SLASH_EQUALS: return "SLASH_EQUALS";
            case TokenType::PERCENT_EQUALS: return "PERCENT_EQUALS";
            case TokenType::AMPERSAND_EQUALS: return "AMPERSAND_EQUALS";
            case TokenType::PIPE_EQUALS: return "PIPE_EQUALS";
            case TokenType::CARET_EQUALS: return "CARET_EQUALS";
            case TokenType::LSHIFT_EQUALS: return "LSHIFT_EQUALS";
            case TokenType::RSHIFT_EQUALS: return "RSHIFT_EQUALS";
            case TokenType::END_OF_FILE: return "EOF";
            default: return "UNKNOWN";
        }
    }
};
