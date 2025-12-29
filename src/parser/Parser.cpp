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
    if (match(TokenType::SIMP)) return switchStatement();
    if (match(TokenType::YEET)) return tryCatchStatement();
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

// lowkey (condition) block [midkey (condition) block]* [highkey block]
StmtPtr Parser::ifStatement() {
    consume(TokenType::LPAREN, "Expected '(' after 'lowkey'.");
    ExprPtr condition = expression();
    consume(TokenType::RPAREN, "Expected ')' after condition.");

    consume(TokenType::LBRACE, "Expected '{' before 'lowkey' body.");
    StmtPtr thenBranch = block();

    StmtPtr elseBranch = nullptr;
    
    // Handle midkey (else if) - creates a nested if
    while (match(TokenType::MIDKEY)) {
        consume(TokenType::LPAREN, "Expected '(' after 'midkey'.");
        ExprPtr midkeyCondition = expression();
        consume(TokenType::RPAREN, "Expected ')' after midkey condition.");
        
        consume(TokenType::LBRACE, "Expected '{' before 'midkey' body.");
        StmtPtr midkeyBranch = block();
        
        // Create nested if for this midkey
        StmtPtr nested = makeStmt<IfStmt>(std::move(midkeyCondition), std::move(midkeyBranch), nullptr);
        
        if (!elseBranch) {
            elseBranch = std::move(nested);
        } else {
            // Find the deepest else branch and attach there
            Stmt* current = elseBranch.get();
            while (auto* ifStmt = std::get_if<IfStmt>(&current->node)) {
                if (!ifStmt->elseBranch) {
                    ifStmt->elseBranch = std::move(nested);
                    break;
                }
                current = ifStmt->elseBranch.get();
            }
        }
    }
    
    // Handle highkey (else) - also supports highkey (condition) for backward compatibility
    if (match(TokenType::HIGHKEY)) {
        StmtPtr finalElse;
        
        if (check(TokenType::LPAREN)) {
            // highkey (condition) - treat as else-if for backward compatibility
            consume(TokenType::LPAREN, "Expected '(' after 'highkey'.");
            ExprPtr highkeyCondition = expression();
            consume(TokenType::RPAREN, "Expected ')' after condition.");
            
            consume(TokenType::LBRACE, "Expected '{' before 'highkey' body.");
            StmtPtr highkeyBranch = block();
            
            // Check for another highkey (final else)
            StmtPtr highkeyElse = nullptr;
            if (match(TokenType::HIGHKEY)) {
                consume(TokenType::LBRACE, "Expected '{' before 'highkey' body.");
                highkeyElse = block();
            }
            
            finalElse = makeStmt<IfStmt>(std::move(highkeyCondition), std::move(highkeyBranch), std::move(highkeyElse));
        } else {
            consume(TokenType::LBRACE, "Expected '{' before 'highkey' body.");
            finalElse = block();
        }
        
        // Attach final else to the deepest point
        if (!elseBranch) {
            elseBranch = std::move(finalElse);
        } else {
            Stmt* current = elseBranch.get();
            while (auto* ifStmt = std::get_if<IfStmt>(&current->node)) {
                if (!ifStmt->elseBranch) {
                    ifStmt->elseBranch = std::move(finalElse);
                    break;
                }
                current = ifStmt->elseBranch.get();
            }
        }
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

// simp (expr) { stan val: { ... } ghost: { ... } }
StmtPtr Parser::switchStatement() {
    Token keyword = previous();
    
    consume(TokenType::LPAREN, "Expected '(' after 'simp'.");
    ExprPtr expr = expression();
    consume(TokenType::RPAREN, "Expected ')' after switch expression.");
    
    consume(TokenType::LBRACE, "Expected '{' before switch body.");
    
    std::vector<SwitchCase> cases;
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (match(TokenType::STAN)) {
            // Case with value
            ExprPtr caseValue = expression();
            consume(TokenType::COLON, "Expected ':' after case value.");
            consume(TokenType::LBRACE, "Expected '{' before case body.");
            
            std::vector<StmtPtr> body;
            while (!check(TokenType::RBRACE) && !isAtEnd()) {
                StmtPtr stmt = declaration();
                if (stmt) body.push_back(std::move(stmt));
            }
            consume(TokenType::RBRACE, "Expected '}' after case body.");
            
            cases.push_back(SwitchCase(std::move(caseValue), std::move(body), false));
        } else if (match(TokenType::GHOST)) {
            // Default case
            consume(TokenType::COLON, "Expected ':' after 'ghost'.");
            consume(TokenType::LBRACE, "Expected '{' before default body.");
            
            std::vector<StmtPtr> body;
            while (!check(TokenType::RBRACE) && !isAtEnd()) {
                StmtPtr stmt = declaration();
                if (stmt) body.push_back(std::move(stmt));
            }
            consume(TokenType::RBRACE, "Expected '}' after default body.");
            
            cases.push_back(SwitchCase(nullptr, std::move(body), true));
        } else {
            throw error(peek(), "Expected 'stan' or 'ghost' in switch body.");
        }
    }
    
    consume(TokenType::RBRACE, "Expected '}' after switch body.");
    
    return makeStmt<SwitchStmt>(std::move(keyword), std::move(expr), std::move(cases));
}

// yeet { ... } caught { ... }
StmtPtr Parser::tryCatchStatement() {
    Token keyword = previous();
    
    consume(TokenType::LBRACE, "Expected '{' after 'yeet'.");
    StmtPtr tryBlock = block();
    
    consume(TokenType::CAUGHT, "Expected 'caught' after try block.");
    consume(TokenType::LBRACE, "Expected '{' after 'caught'.");
    StmtPtr catchBlock = block();
    
    return makeStmt<TryCatchStmt>(std::move(keyword), std::move(tryBlock), std::move(catchBlock));
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

// Handle assignment: x = value, arr[i] = value, x += value, etc.
ExprPtr Parser::assignment() {
    ExprPtr expr = logicalOr();

    // Regular assignment
    if (match(TokenType::ASSIGN)) {
        Token equals = previous();
        ExprPtr value = assignment();  // Right-associative

        // Check if left side is a valid assignment target
        if (auto* identExpr = std::get_if<IdentifierExpr>(&expr->node)) {
            Token name = identExpr->name;
            return makeExpr<AssignExpr>(std::move(name), std::move(value));
        }
        
        // Array index assignment: arr[i] = value
        if (auto* indexExpr = std::get_if<IndexExpr>(&expr->node)) {
            return makeExpr<IndexAssignExpr>(
                std::move(indexExpr->object),
                std::move(indexExpr->bracket),
                std::move(indexExpr->index),
                std::move(value)
            );
        }

        error(equals, "Invalid assignment target.");
    }
    
    // Compound assignment: +=, -=, *=, /=, %=
    if (match(TokenType::PLUS_EQ, TokenType::MINUS_EQ, TokenType::STAR_EQ, 
              TokenType::SLASH_EQ, TokenType::PERCENT_EQ)) {
        Token op = previous();
        ExprPtr value = assignment();
        
        if (auto* identExpr = std::get_if<IdentifierExpr>(&expr->node)) {
            Token name = identExpr->name;
            return makeExpr<CompoundAssignExpr>(std::move(name), std::move(op), std::move(value));
        }
        
        error(op, "Invalid compound assignment target.");
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
    ExprPtr expr = bitwiseOr();

    while (match(TokenType::AND)) {
        Token op = previous();
        ExprPtr right = bitwiseOr();
        expr = makeExpr<LogicalExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Bitwise OR: a | b
ExprPtr Parser::bitwiseOr() {
    ExprPtr expr = bitwiseXor();

    while (match(TokenType::BIT_OR)) {
        Token op = previous();
        ExprPtr right = bitwiseXor();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Bitwise XOR: a ^ b
ExprPtr Parser::bitwiseXor() {
    ExprPtr expr = bitwiseAnd();

    while (match(TokenType::BIT_XOR)) {
        Token op = previous();
        ExprPtr right = bitwiseAnd();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Bitwise AND: a & b
ExprPtr Parser::bitwiseAnd() {
    ExprPtr expr = equality();

    while (match(TokenType::BIT_AND)) {
        Token op = previous();
        ExprPtr right = equality();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
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
    ExprPtr expr = shift();

    while (match(TokenType::LT, TokenType::GT, TokenType::LEQ, TokenType::GEQ)) {
        Token op = previous();
        ExprPtr right = shift();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Shift: a << b, a >> b
ExprPtr Parser::shift() {
    ExprPtr expr = term();

    while (match(TokenType::LSHIFT, TokenType::RSHIFT)) {
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

// Factor: a * b, a / b, a % b
ExprPtr Parser::factor() {
    ExprPtr expr = unary();

    while (match(TokenType::STAR, TokenType::SLASH, TokenType::PERCENT)) {
        Token op = previous();
        ExprPtr right = unary();
        expr = makeExpr<BinaryExpr>(std::move(expr), std::move(op), std::move(right));
    }

    return expr;
}

// Unary: -a, !a, ~a, ++a, --a
ExprPtr Parser::unary() {
    if (match(TokenType::MINUS, TokenType::NOT, TokenType::BIT_NOT)) {
        Token op = previous();
        ExprPtr right = unary();
        return makeExpr<UnaryExpr>(std::move(op), std::move(right));
    }
    
    // Prefix increment/decrement: ++x, --x
    if (match(TokenType::PLUS_PLUS, TokenType::MINUS_MINUS)) {
        Token op = previous();
        // The next thing should be an identifier
        if (check(TokenType::IDENTIFIER)) {
            Token name = advance();
            return makeExpr<IncrementExpr>(std::move(name), std::move(op), true);
        }
        throw error(peek(), "Expected variable name after '" + op.lexeme + "'.");
    }

    return postfix();
}

// Postfix: a++, a--
ExprPtr Parser::postfix() {
    ExprPtr expr = call();
    
    // Check for postfix increment/decrement
    if (match(TokenType::PLUS_PLUS, TokenType::MINUS_MINUS)) {
        Token op = previous();
        
        if (auto* identExpr = std::get_if<IdentifierExpr>(&expr->node)) {
            Token name = identExpr->name;
            return makeExpr<IncrementExpr>(std::move(name), std::move(op), false);
        }
        
        error(op, "Invalid increment/decrement target.");
    }
    
    return expr;
}

// Function call and array indexing: func(args), arr[index]
ExprPtr Parser::call() {
    ExprPtr expr = primary();

    while (true) {
        if (match(TokenType::LPAREN)) {
            expr = finishCall(std::move(expr));
        } else if (match(TokenType::LBRACKET)) {
            Token bracket = previous();
            ExprPtr index = expression();
            consume(TokenType::RBRACKET, "Expected ']' after index.");
            expr = makeExpr<IndexExpr>(std::move(expr), std::move(bracket), std::move(index));
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

// Parse interpolated string like "hello {name}, you are {age} years old"
ExprPtr Parser::parseInterpString(const std::string& raw) {
    std::vector<std::string> stringParts;
    std::vector<ExprPtr> exprParts;
    
    size_t pos = 0;
    size_t len = raw.length();
    
    while (pos < len) {
        // Find next {
        size_t braceStart = raw.find('{', pos);
        
        if (braceStart == std::string::npos) {
            // No more interpolations, add remaining string
            stringParts.push_back(raw.substr(pos));
            break;
        }
        
        // Add string part before {
        stringParts.push_back(raw.substr(pos, braceStart - pos));
        
        // Find matching }
        size_t braceEnd = raw.find('}', braceStart);
        if (braceEnd == std::string::npos) {
            throw error(previous(), "Unterminated interpolation in string");
        }
        
        // Extract variable name between { and }
        std::string varName = raw.substr(braceStart + 1, braceEnd - braceStart - 1);
        
        // Trim whitespace
        size_t start = varName.find_first_not_of(" \t");
        size_t end = varName.find_last_not_of(" \t");
        if (start != std::string::npos) {
            varName = varName.substr(start, end - start + 1);
        }
        
        // Create identifier token and expression for the variable
        Token varToken(TokenType::IDENTIFIER, varName, previous().line);
        exprParts.push_back(makeExpr<IdentifierExpr>(std::move(varToken)));
        
        pos = braceEnd + 1;
    }
    
    // Make sure we have at least one string part (might be empty)
    if (stringParts.empty()) {
        stringParts.push_back("");
    }
    
    // If stringParts has one more than exprParts, that's correct
    // Otherwise add empty string at end
    while (stringParts.size() <= exprParts.size()) {
        stringParts.push_back("");
    }
    
    return makeExpr<InterpStringExpr>(std::move(stringParts), std::move(exprParts));
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

    // Number literal (integer or float)
    if (match(TokenType::NUMBER)) {
        const auto& lit = previous().literal;
        if (std::holds_alternative<int64_t>(lit)) {
            return makeExpr<LiteralExpr>(std::get<int64_t>(lit));
        } else {
            return makeExpr<LiteralExpr>(std::get<double>(lit));
        }
    }

    // String literal
    if (match(TokenType::STRING)) {
        std::string value = std::get<std::string>(previous().literal);
        return makeExpr<LiteralExpr>(std::move(value));
    }

    // Interpolated string: "hello {name}"
    if (match(TokenType::INTERP_STRING)) {
        std::string raw = std::get<std::string>(previous().literal);
        return parseInterpString(raw);
    }
    
    // Array literal: [1, 2, 3]
    if (match(TokenType::LBRACKET)) {
        std::vector<ExprPtr> elements;
        
        if (!check(TokenType::RBRACKET)) {
            do {
                elements.push_back(expression());
            } while (match(TokenType::COMMA));
        }
        
        consume(TokenType::RBRACKET, "Expected ']' after array elements.");
        return makeExpr<ArrayExpr>(std::move(elements));
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
            case TokenType::SIMP:
            case TokenType::YEET:
                return;
            default:
                advance();
        }
    }
}

} // namespace sigma
