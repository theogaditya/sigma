#include "TestFramework.h"
#include "../src/lexer/Lexer.h"
#include "../src/parser/Parser.h"
#include "../src/ast/AST.h"
#include "../src/utils/Error.h"
#include <string>

using namespace sigma;
using namespace sigma::test;

// Helper function to parse code
std::vector<StmtPtr> parse(const std::string& code) {
    ErrorReporter::reset();
    Lexer lexer(code);
    auto tokens = lexer.scanTokens();
    Parser parser(tokens);
    return parser.parse();
}

// Helper to get statement variant
template<typename T>
T& getStmt(const StmtPtr& stmt) {
    return std::get<T>(stmt->node);
}

// Helper to check statement type
template<typename T>
bool isStmtType(const StmtPtr& stmt) {
    return std::holds_alternative<T>(stmt->node);
}

// Helper to get expression variant
template<typename T>
T& getExpr(const ExprPtr& expr) {
    return std::get<T>(expr->node);
}

// Helper to check expression type
template<typename T>
bool isExprType(const ExprPtr& expr) {
    return std::holds_alternative<T>(expr->node);
}

// ============================================================================
// Variable Declaration Tests
// ============================================================================

TEST(Parser_SimpleVariableDeclaration) {
    auto stmts = parse("fr x = 5");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<VarDeclStmt>(stmts[0]));
    
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_EQ(varDecl.name.lexeme, "x");
    EXPECT_TRUE(isExprType<LiteralExpr>(varDecl.initializer));
    
    auto& lit = getExpr<LiteralExpr>(varDecl.initializer);
    // Integer literals now stored as int64_t
    EXPECT_TRUE(std::holds_alternative<int64_t>(lit.value));
    EXPECT_EQ(std::get<int64_t>(lit.value), 5);
    
    return true;
}

TEST(Parser_StringVariableDeclaration) {
    auto stmts = parse("fr name = \"hello\"");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_EQ(varDecl.name.lexeme, "name");
    EXPECT_TRUE(isExprType<LiteralExpr>(varDecl.initializer));
    
    auto& lit = getExpr<LiteralExpr>(varDecl.initializer);
    EXPECT_TRUE(std::holds_alternative<std::string>(lit.value));
    EXPECT_EQ(std::get<std::string>(lit.value), "hello");
    
    return true;
}

TEST(Parser_BooleanVariables) {
    auto stmts = parse("fr a = ongod\nfr b = cap");
    
    EXPECT_EQ(stmts.size(), 2u);
    
    auto& decl1 = getStmt<VarDeclStmt>(stmts[0]);
    auto& lit1 = getExpr<LiteralExpr>(decl1.initializer);
    EXPECT_TRUE(std::holds_alternative<bool>(lit1.value));
    EXPECT_EQ(std::get<bool>(lit1.value), true);
    
    auto& decl2 = getStmt<VarDeclStmt>(stmts[1]);
    auto& lit2 = getExpr<LiteralExpr>(decl2.initializer);
    EXPECT_TRUE(std::holds_alternative<bool>(lit2.value));
    EXPECT_EQ(std::get<bool>(lit2.value), false);
    
    return true;
}

// ============================================================================
// Expression Tests
// ============================================================================

TEST(Parser_BinaryExpression) {
    auto stmts = parse("fr x = 1 + 2");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<BinaryExpr>(varDecl.initializer));
    
    auto& binary = getExpr<BinaryExpr>(varDecl.initializer);
    EXPECT_TRUE(binary.op.type == TokenType::PLUS);
    
    return true;
}

TEST(Parser_ComparisonExpression) {
    auto stmts = parse("fr x = 5 > 3");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<BinaryExpr>(varDecl.initializer));
    
    auto& binary = getExpr<BinaryExpr>(varDecl.initializer);
    EXPECT_TRUE(binary.op.type == TokenType::GT);
    
    return true;
}

TEST(Parser_UnaryExpression) {
    auto stmts = parse("fr x = -5");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<UnaryExpr>(varDecl.initializer));
    
    auto& unary = getExpr<UnaryExpr>(varDecl.initializer);
    EXPECT_TRUE(unary.op.type == TokenType::MINUS);
    
    return true;
}

TEST(Parser_LogicalExpression) {
    auto stmts = parse("fr x = cap && nah");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<LogicalExpr>(varDecl.initializer));
    
    auto& logical = getExpr<LogicalExpr>(varDecl.initializer);
    EXPECT_TRUE(logical.op.type == TokenType::AND);
    
    return true;
}

TEST(Parser_GroupingExpression) {
    auto stmts = parse("fr x = (1 + 2) * 3");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<BinaryExpr>(varDecl.initializer));
    
    // Outer should be multiplication
    auto& outer = getExpr<BinaryExpr>(varDecl.initializer);
    EXPECT_TRUE(outer.op.type == TokenType::STAR);
    
    // Left should be grouping with addition inside
    EXPECT_TRUE(isExprType<GroupingExpr>(outer.left));
    
    return true;
}

// ============================================================================
// Control Flow Tests
// ============================================================================

TEST(Parser_IfStatement) {
    auto stmts = parse("lowkey (ongod) { say \"yes\" }");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<IfStmt>(stmts[0]));
    
    auto& ifStmt = getStmt<IfStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<LiteralExpr>(ifStmt.condition));
    auto& lit = getExpr<LiteralExpr>(ifStmt.condition);
    EXPECT_EQ(std::get<bool>(lit.value), true);
    
    return true;
}

TEST(Parser_IfElseStatement) {
    auto stmts = parse("lowkey (ongod) { say \"yes\" } highkey { say \"no\" }");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& ifStmt = getStmt<IfStmt>(stmts[0]);
    EXPECT_TRUE(ifStmt.elseBranch != nullptr);
    
    return true;
}

TEST(Parser_WhileLoop) {
    auto stmts = parse("goon (ongod) { say \"loop\" }");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<WhileStmt>(stmts[0]));
    
    auto& whileStmt = getStmt<WhileStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<LiteralExpr>(whileStmt.condition));
    
    return true;
}

TEST(Parser_ForLoop) {
    auto stmts = parse("edge (fr i = 0, i < 10, i = i + 1) { say i }");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<ForStmt>(stmts[0]));
    
    auto& forStmt = getStmt<ForStmt>(stmts[0]);
    EXPECT_TRUE(forStmt.initializer != nullptr);
    EXPECT_TRUE(forStmt.condition != nullptr);
    EXPECT_TRUE(forStmt.increment != nullptr);
    
    return true;
}

// ============================================================================
// Function Tests
// ============================================================================

TEST(Parser_FunctionDeclaration) {
    auto stmts = parse("vibe add(a, b) { send a + b }");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<FuncDefStmt>(stmts[0]));
    
    auto& func = getStmt<FuncDefStmt>(stmts[0]);
    EXPECT_EQ(func.name.lexeme, "add");
    EXPECT_EQ(func.params.size(), 2u);
    EXPECT_EQ(func.params[0].lexeme, "a");
    EXPECT_EQ(func.params[1].lexeme, "b");
    
    return true;
}

TEST(Parser_FunctionNoParams) {
    auto stmts = parse("vibe greet() { say \"hello\" }");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& func = getStmt<FuncDefStmt>(stmts[0]);
    EXPECT_EQ(func.name.lexeme, "greet");
    EXPECT_EQ(func.params.size(), 0u);
    
    return true;
}

TEST(Parser_FunctionCall) {
    auto stmts = parse("add(1, 2)");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<ExprStmt>(stmts[0]));
    
    auto& exprStmt = getStmt<ExprStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<CallExpr>(exprStmt.expression));
    
    auto& call = getExpr<CallExpr>(exprStmt.expression);
    EXPECT_EQ(call.arguments.size(), 2u);
    
    return true;
}

TEST(Parser_ReturnStatement) {
    auto stmts = parse("vibe test() { send 42 }");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& func = getStmt<FuncDefStmt>(stmts[0]);
    EXPECT_EQ(func.body.size(), 1u);
    EXPECT_TRUE(isStmtType<ReturnStmt>(func.body[0]));
    
    return true;
}

// ============================================================================
// Print Statement Tests
// ============================================================================

TEST(Parser_PrintStatement) {
    auto stmts = parse("say \"hello\"");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<PrintStmt>(stmts[0]));
    
    return true;
}

TEST(Parser_PrintExpression) {
    auto stmts = parse("say 1 + 2");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& print = getStmt<PrintStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<BinaryExpr>(print.expression));
    
    return true;
}

// ============================================================================
// Assignment Tests
// ============================================================================

TEST(Parser_VariableAssignment) {
    auto stmts = parse("x = 10");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<ExprStmt>(stmts[0]));
    
    auto& exprStmt = getStmt<ExprStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<AssignExpr>(exprStmt.expression));
    
    auto& assign = getExpr<AssignExpr>(exprStmt.expression);
    EXPECT_EQ(assign.name.lexeme, "x");
    
    return true;
}

TEST(Parser_CompoundAssignment) {
    auto stmts = parse("x += 5");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& exprStmt = getStmt<ExprStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<CompoundAssignExpr>(exprStmt.expression));
    
    auto& compound = getExpr<CompoundAssignExpr>(exprStmt.expression);
    EXPECT_TRUE(compound.op.type == TokenType::PLUS_EQ);
    
    return true;
}

// ============================================================================
// Block Statement Tests
// ============================================================================

TEST(Parser_BlockStatement) {
    auto stmts = parse("{ fr x = 1\n fr y = 2 }");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<BlockStmt>(stmts[0]));
    
    auto& block = getStmt<BlockStmt>(stmts[0]);
    EXPECT_EQ(block.statements.size(), 2u);
    
    return true;
}

// ============================================================================
// Break and Continue Tests
// ============================================================================

TEST(Parser_BreakStatement) {
    auto stmts = parse("goon (ongod) { mog }");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& whileStmt = getStmt<WhileStmt>(stmts[0]);
    // Body is a single BlockStmt, get its contents
    auto& bodyBlock = getStmt<BlockStmt>(whileStmt.body);
    EXPECT_EQ(bodyBlock.statements.size(), 1u);
    EXPECT_TRUE(isStmtType<BreakStmt>(bodyBlock.statements[0]));
    
    return true;
}

TEST(Parser_ContinueStatement) {
    auto stmts = parse("goon (ongod) { skip }");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& whileStmt = getStmt<WhileStmt>(stmts[0]);
    auto& bodyBlock = getStmt<BlockStmt>(whileStmt.body);
    EXPECT_EQ(bodyBlock.statements.size(), 1u);
    EXPECT_TRUE(isStmtType<ContinueStmt>(bodyBlock.statements[0]));
    
    return true;
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(Parser_MissingExpression) {
    auto stmts = parse("fr x =");
    
    EXPECT_TRUE(ErrorReporter::hadError());
    
    return true;
}

TEST(Parser_MissingClosingBrace) {
    auto stmts = parse("lowkey cap { say \"test\"");
    
    EXPECT_TRUE(ErrorReporter::hadError());
    
    return true;
}
// ============================================================================
// Array Tests
// ============================================================================

TEST(Parser_ArrayLiteral) {
    auto stmts = parse("fr arr = [1, 2, 3]");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<VarDeclStmt>(stmts[0]));
    
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_EQ(varDecl.name.lexeme, "arr");
    EXPECT_TRUE(isExprType<ArrayExpr>(varDecl.initializer));
    
    auto& arr = getExpr<ArrayExpr>(varDecl.initializer);
    EXPECT_EQ(arr.elements.size(), 3u);
    
    return true;
}

TEST(Parser_EmptyArray) {
    auto stmts = parse("fr arr = []");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<ArrayExpr>(varDecl.initializer));
    
    auto& arr = getExpr<ArrayExpr>(varDecl.initializer);
    EXPECT_EQ(arr.elements.size(), 0u);
    
    return true;
}

TEST(Parser_ArrayIndexAccess) {
    auto stmts = parse("fr x = arr[0]");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<IndexExpr>(varDecl.initializer));
    
    auto& idx = getExpr<IndexExpr>(varDecl.initializer);
    EXPECT_TRUE(isExprType<IdentifierExpr>(idx.object));
    EXPECT_TRUE(isExprType<LiteralExpr>(idx.index));
    
    return true;
}

TEST(Parser_ArrayIndexAssignment) {
    auto stmts = parse("arr[0] = 42");
    
    EXPECT_EQ(stmts.size(), 1u);
    EXPECT_TRUE(isStmtType<ExprStmt>(stmts[0]));
    
    auto& exprStmt = getStmt<ExprStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<IndexAssignExpr>(exprStmt.expression));
    
    return true;
}

TEST(Parser_NestedArrayAccess) {
    auto stmts = parse("fr x = arr[i + 1]");
    
    EXPECT_EQ(stmts.size(), 1u);
    auto& varDecl = getStmt<VarDeclStmt>(stmts[0]);
    EXPECT_TRUE(isExprType<IndexExpr>(varDecl.initializer));
    
    auto& idx = getExpr<IndexExpr>(varDecl.initializer);
    EXPECT_TRUE(isExprType<BinaryExpr>(idx.index));
    
    return true;
}