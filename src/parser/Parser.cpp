#include "Parser.h"
#include "../utils/Error.h"
#include <iostream>

namespace sigma {

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

// ============================================================================
// Main parse entry point
// ============================================================================

Program Parser::parse() {
    Program statements;

    while (!isAtEnd()) {
        try {
            StmtPtr stmt = declaration();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        } catch (const ParseError&) {
            synchronize();
        }
    }

    return statements;
}

// ============================================================================
// Statement Parsing
// ============================================================================

StmtPtr Parser::declaration() {
    // Check for variable declaration (fr)
    if (match(TokenType::FR)) {
        return varDeclaration();
    }

    // Check for function definition (vibe)
    if (match(TokenType::VIBE)) {
        return funcDefinition();
    }

    return statement();
}

StmtPtr Parser::statement() {
    if (match(TokenType::SAY)) return printStatement();
    if (match(TokenType::LOWKEY)) return ifStatement();
    if (match(TokenType::GOON)) return whileStatement();
    if (match(TokenType::EDGE)) return forStatement();
    if (match(TokenType::SEND)) return returnStatement();
    if (match(TokenType::MOG)) return breakStatement();
    if (match(TokenType::SKIP)) return continueStatement();
    if (match(TokenType::LBRACE)) return block();

    return expressionStatement();
}

// fr x = expression
StmtPtr Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name after 'fr'.");
    
    consume(TokenType::ASSIGN, "Expected '=' after variable name.");
    
    ExprPtr initializer = expression();
    
    return makeStmt<VarDeclStmt>(std::move(name), std::move(initializer));
}

// say expression
StmtPtr Parser::printStatement() {
    ExprPtr value = expression();
    return makeStmt<PrintStmt>(std::move(value));
}

// lowkey (condition) block [highkey block]
StmtPtr Parser::ifStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'lowkey'.");
    ExprPtr condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after condition.");

    consume(TokenType::LBRACE, "Expected '{' before 'lowkey' body.");
    StmtPtr thenBranch = block();

    StmtPtr elseBranch = nullptr;
    if (match(TokenType::HIGHKEY)) {
        consume(TokenType::LBRACE, "Expected '{' before 'highkey' body.");
        elseBranch = block();
    }

    return makeStmt<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}

// goon (condition) block
StmtPtr Parser::whileStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'goon'.");
    ExprPtr condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after condition.");

    consume(TokenType::LBRACE, "Expected '{' before 'goon' body.");
    StmtPtr body = block();

    return makeStmt<WhileStmt>(std::move(condition), std::move(body));
}

// edge (init; condition; increment) block
StmtPtr Parser::forStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'edge'.");

    // Initializer
    StmtPtr initializer = nullptr;
    if (match(TokenType::FR)) {
        initializer = varDeclaration();
    } else if (!check(TokenType::COMMA)) {
        initializer = expressionStatement();
    }
    consume(TokenType::COMMA, "Expected ',' after loop initializer.");

    // Condition
    ExprPtr condition = nullptr;
    if (!check(TokenType::COMMA)) {
        condition = expression();
    }
    consume(TokenType::COMMA, "Expected ',' after loop condition.");

    // Increment
    ExprPtr increment = nullptr;
    if (!check(TokenType::RPAREN)) {
        increment = expression();
    }
    consume(TokenType::RPAREN, "Expected ')' after 'edge' clauses.");

    consume(TokenType::LBRACE, "Expected '{' before 'edge' body.");
    StmtPtr body = block();

    return makeStmt<ForStmt>(
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

// vibe name(params) block
StmtPtr Parser::funcDefinition() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name after 'vibe'.");
    
    consume(TokenType::LPAREN, "Expected '(' after function name.");
    
    std::vector<Token> params;
    if (!check(TokenType::RPAREN)) {
        do {
            if (params.size() >= 255) {
                error(peek(), "Cannot have more than 255 parameters.");
            }
            params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name."));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after parameters.");

    consume(TokenType::LBRACE, "Expected '{' before function body.");
    
    // Parse function body statements
    std::vector<StmtPtr> body;
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        StmtPtr stmt = declaration();
        if (stmt) {
            body.push_back(std::move(stmt));
        }
    }
    consume(TokenType::RBRACE, "Expected '}' after function body.");

    return makeStmt<FuncDefStmt>(std::move(name), std::move(params), std::move(body));
}

// send expression
StmtPtr Parser::returnStatement() {
    Token keyword = previous();
    ExprPtr value = nullptr;
    
    // Check if there's a value to return
    if (!check(TokenType::RBRACE) && !isAtEnd()) {
        value = expression();
    }

    return makeStmt<ReturnStmt>(std::move(keyword), std::move(value));
}

// mog
StmtPtr Parser::breakStatement() {
    Token keyword = previous();
    return makeStmt<BreakStmt>(std::move(keyword));
}

// skip
StmtPtr Parser::continueStatement() {
    Token keyword = previous();
    return makeStmt<ContinueStmt>(std::move(keyword));
}

// { statements }
StmtPtr Parser::block() {
    std::vector<StmtPtr> statements;

    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        StmtPtr stmt = declaration();
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    }

    consume(TokenType::RBRACE, "Expected '}' after block.");
    return makeStmt<BlockStmt>(std::move(statements));
}

// expression as statement
StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    return makeStmt<ExprStmt>(std::move(expr));
}

// ============================================================================
// Expression Parsing (Precedence Climbing)
// ============================================================================

ExprPtr Parser::expression() {
    return assignment();
}

// Handle assignment: x = value
ExprPtr Parser::assignment() {
    ExprPtr expr = logicalOr();

    if (match(TokenType::ASSIGN)) {
        Token equals = previous();
        ExprPtr value = assignment();  // Right-associative

        // Check if left side is a valid assignment target
        if (auto* identExpr = std::get_if<IdentifierExpr>(&expr->node)) {
            Token name = identExpr->name;
            return makeExpr<AssignExpr>(std::move(name), std::move(value));
        }

        error(equals, "Invalid assignment target.");
    }

    return expr;
}

// Logical OR: a || b
ExprPtr Parser::logicalOr() {
    ExprPtr expr = logicalAnd();

    while (match(TokenType::OR)) {
        Token op = previous();
        ExprPtr right = logicalAnd();
        expr = makeExpr<LogicalExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Logical AND: a && b
ExprPtr Parser::logicalAnd() {
    ExprPtr expr = equality();

    while (match(TokenType::AND)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = makeExpr<LogicalExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Equality: a == b, a != b
ExprPtr Parser::equality() {
    ExprPtr expr = comparison();

    while (match(TokenType::EQ, TokenType::NEQ)) {
        Token op = previous();
        ExprPtr right = comparison();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Comparison: a < b, a > b, a <= b, a >= b
ExprPtr Parser::comparison() {
    ExprPtr expr = term();

    while (match(TokenType::LT, TokenType::GT, TokenType::LEQ, TokenType::GEQ)) {
        Token op = previous();
        ExprPtr right = term();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Term: a + b, a - b
ExprPtr Parser::term() {
    ExprPtr expr = factor();

    while (match(TokenType::PLUS, TokenType::MINUS)) {
        Token op = previous();
        ExprPtr right = factor();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Factor: a * b, a / b
ExprPtr Parser::factor() {
    ExprPtr expr = unary();

    while (match(TokenType::STAR, TokenType::SLASH)) {
        Token op = previous();
        ExprPtr right = unary();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Unary: -a, !a
ExprPtr Parser::unary() {
    if (match(TokenType::MINUS, TokenType::NOT)) {
        Token op = previous();
        ExprPtr right = unary();
        return makeExpr<UnaryExpr>(std::move(op), std::move(right));
    }

    return call();
}

// Function call: func(args)
ExprPtr Parser::call() {
    ExprPtr expr = primary();

    while (true) {
        if (match(TokenType::LPAREN)) {
            expr = finishCall(std::move(expr));
        } else {
            break;
        }
    }

    return expr;
}

ExprPtr Parser::finishCall(ExprPtr callee) {
    std::vector<ExprPtr> arguments;

    if (!check(TokenType::RPAREN)) {
        do {
            if (arguments.size() >= 255) {
                error(peek(), "Cannot have more than 255 arguments.");
            }
            arguments.push_back(expression());
        } while (match(TokenType::COMMA));
    }

    Token paren = consume(TokenType::RPAREN, "Expected ')' after arguments.");
    return makeExpr<CallExpr>(std::move(callee), std::move(paren), std::move(arguments));
}

// Primary expressions: literals, identifiers, grouping
ExprPtr Parser::primary() {
    // Boolean true: ongod
    if (match(TokenType::ONGOD)) {
        return makeExpr<LiteralExpr>(true);
    }

    // Boolean false: cap
    if (match(TokenType::CAP)) {
        return makeExpr<LiteralExpr>(false);
    }

    // Null: nah
    if (match(TokenType::NAH)) {
        return makeExpr<LiteralExpr>();
    }

    // Number literal
    if (match(TokenType::NUMBER)) {
        double value = std::get<double>(previous().literal);
        return makeExpr<LiteralExpr>(value);
    }

    // String literal
    if (match(TokenType::STRING)) {
        std::string value = std::get<std::string>(previous().literal);
        return makeExpr<LiteralExpr>(std::move(value));
    }

    // Identifier
    if (match(TokenType::IDENTIFIER)) {
        return makeExpr<IdentifierExpr>(previous());
    }

    // Grouped expression
    if (match(TokenType::LPAREN)) {
        ExprPtr expr = expression();
        consume(TokenType::RPAREN, "Expected ')' after expression.");
        return makeExpr<GroupingExpr>(std::move(expr));
    }

    throw error(peek(), "Expected expression.");
}

// ============================================================================
// Token Navigation Helpers
// ============================================================================

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    return tokens_[current_];
}

const Token& Parser::previous() const {
    return tokens_[current_ - 1];
}

const Token& Parser::advance() {
    if (!isAtEnd()) current_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

const Token& Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    throw error(peek(), message);
}

// ============================================================================
// Error Handling
// ============================================================================

ParseError Parser::error(const Token& token, const std::string& message) {
    hadError_ = true;
    
    std::string tokenStr = (token.type == TokenType::END_OF_FILE) ? "end of file" : token.lexeme;
    ErrorReporter::parserError(token.line, tokenStr, message);
    
    return ParseError(message);
}

// Panic mode recovery - skip tokens until we find a statement boundary
void Parser::synchronize() {
    advance();

    while (!isAtEnd()) {
        // Most statements start with these keywords
        switch (peek().type) {
            case TokenType::FR:
            case TokenType::VIBE:
            case TokenType::SAY:
            case TokenType::LOWKEY:
            case TokenType::GOON:
            case TokenType::EDGE:
            case TokenType::SEND:
            case TokenType::MOG:
            case TokenType::SKIP:
                return;
            default:
                advance();
        }
    }
}

} // namespace sigma
