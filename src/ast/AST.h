#ifndef SIGMA_AST_H
#define SIGMA_AST_H

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <cstdint>
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

// Literal values: numbers (int/float), strings, booleans, null
struct LiteralExpr {
    // Value can be: int64_t, double (float), string, bool, or monostate (nah/null)
    std::variant<std::monostate, int64_t, double, std::string, bool> value;

    explicit LiteralExpr(int64_t val) : value(val) {}
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

// Compound assignment: x += 5, x -= 3, etc.
struct CompoundAssignExpr {
    Token name;
    Token op;       // +=, -=, *=, /=, %=
    ExprPtr value;

    CompoundAssignExpr(Token name, Token op, ExprPtr value)
        : name(std::move(name)), op(std::move(op)), value(std::move(value)) {}
};

// Increment/Decrement: x++, ++x, x--, --x
struct IncrementExpr {
    Token name;
    Token op;       // ++ or --
    bool isPrefix;  // true for ++x, false for x++

    IncrementExpr(Token name, Token op, bool isPrefix)
        : name(std::move(name)), op(std::move(op)), isPrefix(isPrefix) {}
};

// Interpolated string: "hello {name}, you are {age} years old"
// Parts alternate between string literals and expressions
struct InterpStringExpr {
    std::vector<std::string> stringParts;  // Static string parts
    std::vector<ExprPtr> exprParts;        // Expressions to interpolate
    
    InterpStringExpr(std::vector<std::string> stringParts, std::vector<ExprPtr> exprParts)
        : stringParts(std::move(stringParts)), exprParts(std::move(exprParts)) {}
};
// Array literal: [1, 2, 3] or [\"a\", \"b\", \"c\"]
struct ArrayExpr {
    std::vector<ExprPtr> elements;
    
    explicit ArrayExpr(std::vector<ExprPtr> elements) 
        : elements(std::move(elements)) {}
};

// Array/string index access: arr[0], str[1]
struct IndexExpr {
    ExprPtr object;    // The array or string being indexed
    Token bracket;     // The '[' token for error reporting
    ExprPtr index;     // The index expression
    
    IndexExpr(ExprPtr object, Token bracket, ExprPtr index)
        : object(std::move(object)), bracket(std::move(bracket)), index(std::move(index)) {}
};

// Array index assignment: arr[0] = value
struct IndexAssignExpr {
    ExprPtr object;    // The array being indexed
    Token bracket;     // The '[' token for error reporting
    ExprPtr index;     // The index expression
    ExprPtr value;     // The value to assign
    
    IndexAssignExpr(ExprPtr object, Token bracket, ExprPtr index, ExprPtr value)
        : object(std::move(object)), bracket(std::move(bracket)), 
          index(std::move(index)), value(std::move(value)) {}
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
    LogicalExpr,
    CompoundAssignExpr,
    IncrementExpr,
    InterpStringExpr,
    ArrayExpr,
    IndexExpr,
    IndexAssignExpr
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

// Switch case: stan value: { ... }
struct SwitchCase {
    ExprPtr value;      // nullptr for ghost (default)
    std::vector<StmtPtr> body;
    bool isDefault;

    SwitchCase(ExprPtr value, std::vector<StmtPtr> body, bool isDefault = false)
        : value(std::move(value)), body(std::move(body)), isDefault(isDefault) {}
};

// Switch statement: simp (expr) { stan val: { ... } ghost: { ... } }
struct SwitchStmt {
    Token keyword;
    ExprPtr expression;
    std::vector<SwitchCase> cases;

    SwitchStmt(Token keyword, ExprPtr expression, std::vector<SwitchCase> cases)
        : keyword(std::move(keyword)), expression(std::move(expression)), cases(std::move(cases)) {}
};

// Try-catch statement: yeet { ... } caught { ... }
struct TryCatchStmt {
    Token keyword;
    StmtPtr tryBlock;
    StmtPtr catchBlock;

    TryCatchStmt(Token keyword, StmtPtr tryBlock, StmtPtr catchBlock)
        : keyword(std::move(keyword)), tryBlock(std::move(tryBlock)), catchBlock(std::move(catchBlock)) {}
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
    ContinueStmt,
    SwitchStmt,
    TryCatchStmt
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
