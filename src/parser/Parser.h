#ifndef SIGMA_PARSER_H
#define SIGMA_PARSER_H

#include <vector>
#include <string>
#include <stdexcept>
#include "../lexer/Token.h"
#include "../ast/AST.h"

namespace sigma {

// Parser exception for syntax errors
class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

class Parser {
public:
    // Constructor takes the token list from the lexer
    explicit Parser(std::vector<Token> tokens);

    // Parse the entire program and return a list of statements
    Program parse();

    // Check if any errors occurred during parsing
    bool hasError() const { return hadError_; }

private:
    std::vector<Token> tokens_;
    size_t current_ = 0;
    bool hadError_ = false;

    // ========================================================================
    // Statement parsing methods (following grammar rules)
    // ========================================================================
    StmtPtr declaration();
    StmtPtr statement();
    StmtPtr varDeclaration();
    StmtPtr printStatement();
    StmtPtr ifStatement();
    StmtPtr whileStatement();
    StmtPtr forStatement();
    StmtPtr funcDefinition();
    StmtPtr returnStatement();
    StmtPtr breakStatement();
    StmtPtr continueStatement();
    StmtPtr expressionStatement();
    StmtPtr block();

    // ========================================================================
    // Expression parsing methods (precedence climbing)
    // ========================================================================
    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicalOr();
    ExprPtr logicalAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr call();
    ExprPtr primary();

    // Helper for parsing function call arguments
    ExprPtr finishCall(ExprPtr callee);

    // ========================================================================
    // Token navigation helpers
    // ========================================================================
    
    // Check if we've reached end of tokens
    bool isAtEnd() const;
    
    // Get current token without advancing
    const Token& peek() const;
    
    // Get previous token
    const Token& previous() const;
    
    // Advance to next token and return current
    const Token& advance();
    
    // Check if current token matches type
    bool check(TokenType type) const;
    
    // If current token matches any of the types, advance and return true
    template<typename... Types>
    bool match(Types... types);

    // Consume expected token or throw error
    const Token& consume(TokenType type, const std::string& message);

    // ========================================================================
    // Error handling
    // ========================================================================
    ParseError error(const Token& token, const std::string& message);
    void synchronize();
};

// Template implementation
template<typename... Types>
bool Parser::match(Types... types) {
    // Use fold expression to check each type
    bool matched = ((check(types) ? (advance(), true) : false) || ...);
    return matched;
}

} // namespace sigma

#endif // SIGMA_PARSER_H
