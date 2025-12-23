#ifndef SIGMA_TOKEN_H
#define SIGMA_TOKEN_H

#include <string>
#include <variant>
#include <ostream>

namespace sigma {

// All token types in the Sigma language
enum class TokenType {
    // Keywords (Gen-Z brainrot themed)
    FR,         // variable declaration
    SAY,        // print
    LOWKEY,     // if
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

    // Literals
    NUMBER,     // integer or float
    STRING,     // string literal
    IDENTIFIER, // variable/function names

    // Arithmetic operators
    PLUS,       // +
    MINUS,      // -
    STAR,       // *
    SLASH,      // /

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

    // Assignment
    ASSIGN,     // =

    // Punctuation
    LPAREN,     // (
    RPAREN,     // )
    LBRACE,     // {
    RBRACE,     // }
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
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::EQ: return "EQ";
        case TokenType::NEQ: return "NEQ";
        case TokenType::LT: return "LT";
        case TokenType::GT: return "GT";
        case TokenType::LEQ: return "LEQ";
        case TokenType::GEQ: return "GEQ";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::COMMA: return "COMMA";
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::INVALID: return "INVALID";
        default: return "UNKNOWN";
    }
}

// Literal value can be a number (double) or string
using LiteralValue = std::variant<std::monostate, double, std::string>;

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
    if (std::holds_alternative<double>(token.literal)) {
        os << " = " << std::get<double>(token.literal);
    } else if (std::holds_alternative<std::string>(token.literal)) {
        os << " = \"" << std::get<std::string>(token.literal) << "\"";
    }
    
    os << "]";
    return os;
}

} // namespace sigma

#endif // SIGMA_TOKEN_H
