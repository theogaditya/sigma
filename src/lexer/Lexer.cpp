#include "Lexer.h"
#include "../utils/Error.h"
#include <iostream>

namespace sigma {

// Initialize keyword map with all Sigma language keywords
const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"fr",      TokenType::FR},
    {"say",     TokenType::SAY},
    {"lowkey",  TokenType::LOWKEY},
    {"highkey", TokenType::HIGHKEY},
    {"goon",    TokenType::GOON},
    {"vibe",    TokenType::VIBE},
    {"send",    TokenType::SEND},
    {"ongod",   TokenType::ONGOD},
    {"cap",     TokenType::CAP},
    {"nah",     TokenType::NAH},
    {"skip",    TokenType::SKIP},
    {"mog",     TokenType::MOG},
    {"edge",    TokenType::EDGE}
};

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::scanTokens() {
    while (!isAtEnd()) {
        // We are at the beginning of the next lexeme
        start_ = current_;
        scanToken();
    }

    // Add EOF token at the end
    tokens_.emplace_back(TokenType::END_OF_FILE, "", line_);
    return tokens_;
}

void Lexer::scanToken() {
    char c = advance();

    switch (c) {
        // Single-character tokens
        case '(': addToken(TokenType::LPAREN); break;
        case ')': addToken(TokenType::RPAREN); break;
        case '{': addToken(TokenType::LBRACE); break;
        case '}': addToken(TokenType::RBRACE); break;
        case ',': addToken(TokenType::COMMA); break;
        case '+': addToken(TokenType::PLUS); break;
        case '-': addToken(TokenType::MINUS); break;
        case '*': addToken(TokenType::STAR); break;
        case '/': addToken(TokenType::SLASH); break;

        // One or two character tokens
        case '!':
            addToken(match('=') ? TokenType::NEQ : TokenType::NOT);
            break;
        case '=':
            addToken(match('=') ? TokenType::EQ : TokenType::ASSIGN);
            break;
        case '<':
            addToken(match('=') ? TokenType::LEQ : TokenType::LT);
            break;
        case '>':
            addToken(match('=') ? TokenType::GEQ : TokenType::GT);
            break;

        // Two-character tokens (logical operators)
        case '&':
            if (match('&')) {
                addToken(TokenType::AND);
            } else {
                error("Expected '&&' for logical AND");
            }
            break;
        case '|':
            if (match('|')) {
                addToken(TokenType::OR);
            } else {
                error("Expected '||' for logical OR");
            }
            break;

        // Comments start with #
        case '#':
            skipComment();
            break;

        // Whitespace - ignore
        case ' ':
        case '\r':
        case '\t':
            break;

        // Newlines - track line number
        case '\n':
            line_++;
            break;

        // String literals
        case '"':
            scanString();
            break;

        default:
            if (isDigit(c)) {
                scanNumber();
            } else if (isAlpha(c)) {
                scanIdentifier();
            } else {
                error("Unexpected character: " + std::string(1, c));
            }
            break;
    }
}

// ============================================================================
// Character reading helpers
// ============================================================================

bool Lexer::isAtEnd() const {
    return current_ >= source_.length();
}

char Lexer::advance() {
    return source_[current_++];
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char Lexer::peekNext() const {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

bool Lexer::match(char expected) {
    if (isAtEnd()) return false;
    if (source_[current_] != expected) return false;
    
    current_++;
    return true;
}

// ============================================================================
// Character type helpers
// ============================================================================

bool Lexer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

bool Lexer::isAlpha(char c) const {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

bool Lexer::isAlphaNumeric(char c) const {
    return isAlpha(c) || isDigit(c);
}

// ============================================================================
// Token creation helpers
// ============================================================================

void Lexer::addToken(TokenType type) {
    addToken(type, std::monostate{});
}

void Lexer::addToken(TokenType type, LiteralValue literal) {
    std::string text = source_.substr(start_, current_ - start_);
    tokens_.emplace_back(type, text, literal, line_);
}

// ============================================================================
// Specific token scanners
// ============================================================================

void Lexer::scanString() {
    // Consume characters until closing quote or end of file
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line_++;  // Support multi-line strings
        advance();
    }

    if (isAtEnd()) {
        error("Unterminated string");
        return;
    }

    // Consume the closing "
    advance();

    // Extract string value (without quotes)
    std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
    addToken(TokenType::STRING, value);
}

void Lexer::scanNumber() {
    // Consume all digits
    while (isDigit(peek())) advance();

    // Look for decimal part
    if (peek() == '.' && isDigit(peekNext())) {
        // Consume the '.'
        advance();

        // Consume decimal digits
        while (isDigit(peek())) advance();
    }

    // Parse the number
    std::string numStr = source_.substr(start_, current_ - start_);
    double value = std::stod(numStr);
    addToken(TokenType::NUMBER, value);
}

void Lexer::scanIdentifier() {
    // Consume alphanumeric characters
    while (isAlphaNumeric(peek())) advance();

    // Check if it's a keyword
    std::string text = source_.substr(start_, current_ - start_);
    
    auto it = keywords_.find(text);
    if (it != keywords_.end()) {
        addToken(it->second);
    } else {
        addToken(TokenType::IDENTIFIER);
    }
}

void Lexer::skipComment() {
    // Comments go until end of line
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
}

// ============================================================================
// Error reporting
// ============================================================================

void Lexer::error(const std::string& message) {
    ErrorReporter::lexerError(line_, message);
    hadError_ = true;
}

} // namespace sigma
