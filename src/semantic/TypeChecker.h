#ifndef SIGMA_TYPE_CHECKER_H
#define SIGMA_TYPE_CHECKER_H

#include <string>
#include <vector>
#include "../ast/AST.h"
#include "Types.h"
#include "SymbolTable.h"

namespace sigma {

// ============================================================================
// Type Checker - Semantic Analysis Phase
// ============================================================================
// 
// Performs type checking and semantic analysis on the AST.
// Reports errors for:
// - Undefined variables
// - Type mismatches
// - Invalid operations
// - Duplicate declarations in same scope
// - Break/continue outside loops
// - Return outside functions
// ============================================================================

class TypeChecker {
public:
    TypeChecker();
    
    // Main entry point: analyze an entire program
    bool analyze(const Program& program);
    
    // Check if errors occurred
    bool hasError() const { return hadError_; }
    
    // Get error messages
    const std::vector<std::string>& getErrors() const { return errors_; }

private:
    SymbolTable symbols_;
    FunctionTable functions_;
    bool hadError_ = false;
    std::vector<std::string> errors_;
    
    // Context tracking
    int loopDepth_ = 0;      // Track nested loops for break/continue
    bool inFunction_ = false; // Track if we're inside a function
    Type currentFunctionReturnType_ = Types::Void();
    std::string currentFile_ = "<unknown>";
    
    // ========================================================================
    // Statement Analysis
    // ========================================================================
    void analyzeStmt(const Stmt& stmt);
    void analyzeVarDecl(const VarDeclStmt& stmt);
    void analyzePrint(const PrintStmt& stmt);
    void analyzeExprStmt(const ExprStmt& stmt);
    void analyzeBlock(const BlockStmt& stmt);
    void analyzeIf(const IfStmt& stmt);
    void analyzeWhile(const WhileStmt& stmt);
    void analyzeFor(const ForStmt& stmt);
    void analyzeFuncDef(const FuncDefStmt& stmt);
    void analyzeReturn(const ReturnStmt& stmt);
    void analyzeBreak(const BreakStmt& stmt);
    void analyzeContinue(const ContinueStmt& stmt);
    void analyzeSwitch(const SwitchStmt& stmt);
    void analyzeTryCatch(const TryCatchStmt& stmt);
    
    // ========================================================================
    // Expression Analysis (returns inferred type)
    // ========================================================================
    Type analyzeExpr(const Expr& expr);
    Type analyzeLiteral(const LiteralExpr& expr);
    Type analyzeIdentifier(const IdentifierExpr& expr);
    Type analyzeBinary(const BinaryExpr& expr);
    Type analyzeUnary(const UnaryExpr& expr);
    Type analyzeCall(const CallExpr& expr);
    Type analyzeGrouping(const GroupingExpr& expr);
    Type analyzeAssign(const AssignExpr& expr);
    Type analyzeLogical(const LogicalExpr& expr);
    Type analyzeCompoundAssign(const CompoundAssignExpr& expr);
    Type analyzeIncrement(const IncrementExpr& expr);
    Type analyzeInterpString(const InterpStringExpr& expr);
    
    // ========================================================================
    // Helper Methods
    // ========================================================================
    void error(int line, const std::string& message);
    void warning(int line, const std::string& message);
    
    // Type checking helpers
    bool checkNumeric(const Type& type, int line, const std::string& context);
    bool checkBoolean(const Type& type, int line, const std::string& context);
    bool checkAssignable(const Type& target, const Type& source, int line);
};

} // namespace sigma

#endif // SIGMA_TYPE_CHECKER_H
