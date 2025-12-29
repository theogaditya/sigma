#include "Lexer.h"
#include "../utils/Error.h"
#include <iostream>

namespace sigma {

// Initialize keyword map with all Sigma language keywords
const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"fr",      TokenType::FR},
    {"say",     TokenType::SAY},
    {"lowkey",  TokenType::LOWKEY},
    {"midkey",  TokenType::MIDKEY},
    {"highkey", TokenType::HIGHKEY},
    {"goon",    TokenType::GOON},
    {"vibe",    TokenType::VIBE},
    {"send",    TokenType::SEND},
    {"ongod",   TokenType::ONGOD},
    {"cap",     TokenType::CAP},
    {"nah",     TokenType::NAH},
    {"skip",    TokenType::SKIP},
    {"mog",     TokenType::MOG},
    {"edge",    TokenType::EDGE},
    {"simp",    TokenType::SIMP},
    {"stan",    TokenType::STAN},
    {"ghost",   TokenType::GHOST},
    {"yeet",    TokenType::YEET},
    {"caught",  TokenType::CAUGHT}
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
        case '[': addToken(TokenType::LBRACKET); break;
        case ']': addToken(TokenType::RBRACKET); break;
        case ',': addToken(TokenType::COMMA); break;
        case ':': addToken(TokenType::COLON); break;
        case '~': addToken(TokenType::BIT_NOT); break;
        case '^': addToken(TokenType::BIT_XOR); break;

        // Multi-character operators
        case '+':
            if (match('+')) addToken(TokenType::PLUS_PLUS);
            else if (match('=')) addToken(TokenType::PLUS_EQ);
            else addToken(TokenType::PLUS);
            break;
        case '-':
            if (match('-')) addToken(TokenType::MINUS_MINUS);
            else if (match('=')) addToken(TokenType::MINUS_EQ);
            else addToken(TokenType::MINUS);
            break;
        case '*':
            addToken(match('=') ? TokenType::STAR_EQ : TokenType::STAR);
            break;
        case '/':
            addToken(match('=') ? TokenType::SLASH_EQ : TokenType::SLASH);
            break;
        case '%':
            addToken(match('=') ? TokenType::PERCENT_EQ : TokenType::PERCENT);
            break;

        // Comparison and assignment
        case '!':
            addToken(match('=') ? TokenType::NEQ : TokenType::NOT);
            break;
        case '=':
            addToken(match('=') ? TokenType::EQ : TokenType::ASSIGN);
            break;
        case '<':
            if (match('<')) addToken(TokenType::LSHIFT);
            else if (match('=')) addToken(TokenType::LEQ);
            else addToken(TokenType::LT);
            break;
        case '>':
            if (match('>')) addToken(TokenType::RSHIFT);
            else if (match('=')) addToken(TokenType::GEQ);
            else addToken(TokenType::GT);
            break;

        // Logical and bitwise operators
        case '&':
            if (match('&')) addToken(TokenType::AND);
            else addToken(TokenType::BIT_AND);
            break;
        case '|':
            if (match('|')) addToken(TokenType::OR);
            else addToken(TokenType::BIT_OR);
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
    
    // Check if it's an interpolated string (contains {})
    bool hasInterpolation = false;
    for (size_t i = 0; i < value.length(); i++) {
        if (value[i] == '{') {
            // Make sure there's a closing }
            size_t end = value.find('}', i);
            if (end != std::string::npos) {
                hasInterpolation = true;
                break;
            }
        }
    }
    
    if (hasInterpolation) {
        addToken(TokenType::INTERP_STRING, value);
    } else {
        addToken(TokenType::STRING, value);
    }
}

void Lexer::scanNumber() {
    // Consume all digits
    while (isDigit(peek())) advance();

    bool isFloat = false;
    
    // Look for decimal part
    if (peek() == '.' && isDigit(peekNext())) {
        isFloat = true;
        // Consume the '.'
        advance();

        // Consume decimal digits
        while (isDigit(peek())) advance();
    }

    // Parse the number
    std::string numStr = source_.substr(start_, current_ - start_);
    
    if (isFloat) {
        double value = std::stod(numStr);
        addToken(TokenType::NUMBER, value);
    } else {
        int64_t value = std::stoll(numStr);
        addToken(TokenType::NUMBER, value);
    }
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
