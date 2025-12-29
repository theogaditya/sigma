#ifndef SIGMA_TOKEN_H
#define SIGMA_TOKEN_H

#include <string>
#include <variant>
#include <ostream>
#include <cstdint>

namespace sigma {

// All token types in the Sigma language
enum class TokenType {
    // Keywords (Gen-Z brainrot themed)
    FR,         // variable declaration
    SAY,        // print
    LOWKEY,     // if
    MIDKEY,     // else if
    HIGHKEY,    // else
    GOON,       // while loop
    VIBE,       // function definition
    SEND,       // return
    ONGOD,      // true
    CAP,        // false
    NAH,        // null
    SKIP,       // continue
    MOG,        // break
    EDGE,       // for loop
    SIMP,       // switch
    STAN,       // case
    GHOST,      // default
    YEET,       // try
    CAUGHT,     // catch
    COLON,      // :

    // Literals
    NUMBER,     // integer or float
    STRING,     // string literal
    INTERP_STRING, // interpolated string like "hello {x}"
    IDENTIFIER, // variable/function names

    // Arithmetic operators
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /
    PERCENT,    // % (modulo)

    // Compound assignment operators
    PLUS_EQ,    // +=
    MINUS_EQ,   // -=
    STAR_EQ,    // *=
    SLASH_EQ,   // /=
    PERCENT_EQ, // %=

    // Increment/Decrement
    PLUS_PLUS,   // ++
    MINUS_MINUS, // --

    // Comparison operators
    EQ,         // ==
    NEQ,        // !=
    LT,         // <
    GT,         // >
    LEQ,        // <=
    GEQ,        // >=

    // Logical operators
    AND,        // &&
    OR,         // ||
    NOT,        // !

    // Bitwise operators
    BIT_AND,    // &
    BIT_OR,     // |
    BIT_XOR,    // ^
    BIT_NOT,    // ~
    LSHIFT,     // <<
    RSHIFT,     // >>

    // Assignment
    ASSIGN,     // =

    // Punctuation
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
    LBRACKET,   // [
    RBRACKET,   // ]
    COMMA,      // ,

    // Special
    END_OF_FILE,
    INVALID
};

// Convert TokenType to string for debugging
inline std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::FR: return "FR";
        case TokenType::SAY: return "SAY";
        case TokenType::LOWKEY: return "LOWKEY";
        case TokenType::MIDKEY: return "MIDKEY";
        case TokenType::HIGHKEY: return "HIGHKEY";
        case TokenType::GOON: return "GOON";
        case TokenType::VIBE: return "VIBE";
        case TokenType::SEND: return "SEND";
        case TokenType::ONGOD: return "ONGOD";
        case TokenType::CAP: return "CAP";
        case TokenType::NAH: return "NAH";
        case TokenType::SKIP: return "SKIP";
        case TokenType::MOG: return "MOG";
        case TokenType::EDGE: return "EDGE";
        case TokenType::SIMP: return "SIMP";
        case TokenType::STAN: return "STAN";
        case TokenType::GHOST: return "GHOST";
        case TokenType::YEET: return "YEET";
        case TokenType::CAUGHT: return "CAUGHT";
        case TokenType::COLON: return "COLON";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::INTERP_STRING: return "INTERP_STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::PLUS_EQ: return "PLUS_EQ";
        case TokenType::MINUS_EQ: return "MINUS_EQ";
        case TokenType::STAR_EQ: return "STAR_EQ";
        case TokenType::SLASH_EQ: return "SLASH_EQ";
        case TokenType::PERCENT_EQ: return "PERCENT_EQ";
        case TokenType::PLUS_PLUS: return "PLUS_PLUS";
        case TokenType::MINUS_MINUS: return "MINUS_MINUS";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LEQ: return "LEQ";
        case TokenType::GEQ: return "GEQ";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::BIT_AND: return "BIT_AND";
        case TokenType::BIT_OR: return "BIT_OR";
        case TokenType::BIT_XOR: return "BIT_XOR";
        case TokenType::BIT_NOT: return "BIT_NOT";
        case TokenType::LSHIFT: return "LSHIFT";
        case TokenType::RSHIFT: return "RSHIFT";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::COMMA: return "COMMA";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::INVALID: return "INVALID";
        default: return "UNKNOWN";
    }
}

// Literal value can be an integer, float, or string
using LiteralValue = std::variant<std::monostate, int64_t, double, std::string>;

// Token structure
struct Token {
    TokenType type;
    std::string lexeme;     // The actual text in source code
    LiteralValue literal;   // Parsed literal value (if applicable)
    int line;               // Line number for error reporting

    Token(TokenType type, std::string lexeme, LiteralValue literal, int line)
        : type(type), lexeme(std::move(lexeme)), literal(std::move(literal)), line(line) {}

    // Convenience constructor for tokens without literal values
    Token(TokenType type, std::string lexeme, int line)
        : type(type), lexeme(std::move(lexeme)), literal(std::monostate{}), line(line) {}
};

// Stream output for Token (useful for debugging)
inline std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << "[" << tokenTypeToString(token.type) << " '" << token.lexeme << "' L" << token.line;
    
    // Print literal value if present
    if (std::holds_alternative<int64_t>(token.literal)) {
        os << " = " << std::get<int64_t>(token.literal) << "i";
    } else if (std::holds_alternative<double>(token.literal)) {
        os << " = " << std::get<double>(token.literal);
    } else if (std::holds_alternative<std::string>(token.literal)) {
        os << " = \"" << std::get<std::string>(token.literal) << "\"";
    }
    
    os << "]";
    return os;
}

} // namespace sigma

#endif // SIGMA_TOKEN_H
