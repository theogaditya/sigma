#ifndef SIGMA_LEXER_H
#define SIGMA_LEXER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "Token.h"

namespace sigma {

class Lexer {
public:
    // Constructor takes the source code string
    explicit Lexer(std::string source);

    // Tokenize the entire source and return list of tokens
    std::vector<Token> scanTokens();

    // Check if any errors occurred during lexing
    bool hasError() const { return hadError_; }

private:
    std::string source_;
    std::vector<Token> tokens_;
    
    // Position tracking
    size_t start_ = 0;      // Start of current lexeme
    size_t current_ = 0;    // Current position in source
    int line_ = 1;          // Current line number
    
    // Error flag
    bool hadError_ = false;

    // Keyword lookup table
    static const std::unordered_map<std::string, TokenType> keywords_;

    // Core scanning methods
    void scanToken();
    
    // Character reading helpers
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);
    
    // Character type helpers
    bool isDigit(char c) const;
    bool isAlpha(char c) const;
    bool isAlphaNumeric(char c) const;
    
    // Token creation helpers
    void addToken(TokenType type);
    void addToken(TokenType type, LiteralValue literal);
    
    // Specific token scanners
    void scanString();
    void scanNumber();
    void scanIdentifier();
    void skipComment();
    
    // Error reporting
    void error(const std::string& message);
};

} // namespace sigma

#endif // SIGMA_LEXER_H
