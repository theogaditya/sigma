#ifndef SIGMA_AST_H
#define SIGMA_AST_H

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include "../lexer/Token.h"

namespace sigma {

// Forward declarations
struct Expr;
struct Stmt;

// Smart pointer types for AST nodes
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

// ============================================================================
// EXPRESSION NODES
// ============================================================================

// Literal values: numbers, strings, booleans, null
struct LiteralExpr {
    // Value can be: double (number), string, bool, or monostate (nah/null)
    std::variant<std::monostate, double, std::string, bool> value;

    explicit LiteralExpr(double val) : value(val) {}
    explicit LiteralExpr(std::string val) : value(std::move(val)) {}
    explicit LiteralExpr(bool val) : value(val) {}
    LiteralExpr() : value(std::monostate{}) {}  // null/nah
};

// Variable reference: x, myVar, etc.
struct IdentifierExpr {
    Token name;

    explicit IdentifierExpr(Token name) : name(std::move(name)) {}
};

// Binary expression: a + b, x == y, etc.
struct BinaryExpr {
    ExprPtr left;
    Token op;
    ExprPtr right;

    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
};

// Unary expression: -x, !flag
struct UnaryExpr {
    Token op;
    ExprPtr operand;

    UnaryExpr(Token op, ExprPtr operand)
        : op(std::move(op)), operand(std::move(operand)) {}
};

// Function call: add(1, 2)
struct CallExpr {
    ExprPtr callee;          // The function being called (usually an identifier)
    Token paren;             // The '(' token for error reporting
    std::vector<ExprPtr> arguments;

    CallExpr(ExprPtr callee, Token paren, std::vector<ExprPtr> arguments)
        : callee(std::move(callee)), paren(std::move(paren)), arguments(std::move(arguments)) {}
};

// Grouped expression: (a + b)
struct GroupingExpr {
    ExprPtr expression;

    explicit GroupingExpr(ExprPtr expression) : expression(std::move(expression)) {}
};

// Assignment expression: x = 5 (when used as expression, not declaration)
struct AssignExpr {
    Token name;
    ExprPtr value;

    AssignExpr(Token name, ExprPtr value)
        : name(std::move(name)), value(std::move(value)) {}
};

// Logical expression: a && b, x || y (short-circuit evaluation)
struct LogicalExpr {
    ExprPtr left;
    Token op;
    ExprPtr right;

    LogicalExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
};

// Expression variant - holds any expression type
using ExprVariant = std::variant<
    LiteralExpr,
    IdentifierExpr,
    BinaryExpr,
    UnaryExpr,
    CallExpr,
    GroupingExpr,
    AssignExpr,
    LogicalExpr
>;

// Expression wrapper
struct Expr {
    ExprVariant node;

    template<typename T>
    explicit Expr(T&& node) : node(std::forward<T>(node)) {}
};

// ============================================================================
// STATEMENT NODES
// ============================================================================

// Variable declaration: fr x = 10
struct VarDeclStmt {
    Token name;
    ExprPtr initializer;

    VarDeclStmt(Token name, ExprPtr initializer)
        : name(std::move(name)), initializer(std::move(initializer)) {}
};

// Print statement: say "hello"
struct PrintStmt {
    ExprPtr expression;

    explicit PrintStmt(ExprPtr expression) : expression(std::move(expression)) {}
};

// Expression statement: function_call(); or just an expression
struct ExprStmt {
    ExprPtr expression;

    explicit ExprStmt(ExprPtr expression) : expression(std::move(expression)) {}
};

// Block statement: { ... }
struct BlockStmt {
    std::vector<StmtPtr> statements;

    explicit BlockStmt(std::vector<StmtPtr> statements) 
        : statements(std::move(statements)) {}
};

// If statement: lowkey (cond) { ... } highkey { ... }
struct IfStmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch;  // nullptr if no highkey

    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch = nullptr)
        : condition(std::move(condition))
        , thenBranch(std::move(thenBranch))
        , elseBranch(std::move(elseBranch)) {}
};

// While statement: goon (cond) { ... }
struct WhileStmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr condition, StmtPtr body)
        : condition(std::move(condition)), body(std::move(body)) {}
};

// For statement: edge (init; cond; incr) { ... }
struct ForStmt {
    StmtPtr initializer;   // Can be VarDeclStmt or ExprStmt or nullptr
    ExprPtr condition;     // Can be nullptr (infinite loop)
    ExprPtr increment;     // Can be nullptr
    StmtPtr body;

    ForStmt(StmtPtr initializer, ExprPtr condition, ExprPtr increment, StmtPtr body)
        : initializer(std::move(initializer))
        , condition(std::move(condition))
        , increment(std::move(increment))
        , body(std::move(body)) {}
};

// Function definition: vibe add(a, b) { ... }
struct FuncDefStmt {
    Token name;
    std::vector<Token> params;
    std::vector<StmtPtr> body;

    FuncDefStmt(Token name, std::vector<Token> params, std::vector<StmtPtr> body)
        : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
};

// Return statement: send value
struct ReturnStmt {
    Token keyword;  // For error reporting
    ExprPtr value;  // Can be nullptr for bare "send"

    ReturnStmt(Token keyword, ExprPtr value = nullptr)
        : keyword(std::move(keyword)), value(std::move(value)) {}
};

// Break statement: mog
struct BreakStmt {
    Token keyword;  // For error reporting

    explicit BreakStmt(Token keyword) : keyword(std::move(keyword)) {}
};

// Continue statement: skip
struct ContinueStmt {
    Token keyword;  // For error reporting

    explicit ContinueStmt(Token keyword) : keyword(std::move(keyword)) {}
};

// Statement variant - holds any statement type
using StmtVariant = std::variant<
    VarDeclStmt,
    PrintStmt,
    ExprStmt,
    BlockStmt,
    IfStmt,
    WhileStmt,
    ForStmt,
    FuncDefStmt,
    ReturnStmt,
    BreakStmt,
    ContinueStmt
>;

// Statement wrapper
struct Stmt {
    StmtVariant node;

    template<typename T>
    explicit Stmt(T&& node) : node(std::forward<T>(node)) {}
};

// ============================================================================
// HELPER FUNCTIONS FOR CREATING AST NODES
// ============================================================================

// Expression creation helpers
template<typename T, typename... Args>
ExprPtr makeExpr(Args&&... args) {
    return std::make_unique<Expr>(T(std::forward<Args>(args)...));
}

// Statement creation helpers
template<typename T, typename... Args>
StmtPtr makeStmt(Args&&... args) {
    return std::make_unique<Stmt>(T(std::forward<Args>(args)...));
}

// Program is a list of statements
using Program = std::vector<StmtPtr>;

} // namespace sigma

#endif // SIGMA_AST_H
