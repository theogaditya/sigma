#include "TypeChecker.h"
#include "../utils/Error.h"
#include <sstream>

namespace sigma {

TypeChecker::TypeChecker() {}

// ============================================================================
// Main Analysis Entry Point
// ============================================================================

bool TypeChecker::analyze(const Program& program) {
    // First pass: collect all function declarations
    for (const auto& stmt : program) {
        if (auto* funcDef = std::get_if<FuncDefStmt>(&stmt->node)) {
            // Build parameter types (all numbers for now)
            std::vector<TypeKind> paramTypes(funcDef->params.size(), TypeKind::Number);
            std::vector<std::string> paramNames;
            for (const auto& param : funcDef->params) {
                paramNames.push_back(param.lexeme);
            }
            
            Type funcType = Types::Function(TypeKind::Number, paramTypes);
            
            if (!functions_.declare(funcDef->name.lexeme, funcType, paramNames, funcDef->name.line)) {
                error(funcDef->name.line, "Function '" + funcDef->name.lexeme + "' is already declared");
            }
        }
    }
    
    // Second pass: analyze all statements
    for (const auto& stmt : program) {
        analyzeStmt(*stmt);
    }
    
    return !hadError_;
}

// ============================================================================
// Statement Analysis
// ============================================================================

void TypeChecker::analyzeStmt(const Stmt& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, VarDeclStmt>) {
            analyzeVarDecl(s);
        } else if constexpr (std::is_same_v<T, PrintStmt>) {
            analyzePrint(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            analyzeExprStmt(s);
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
            analyzeBlock(s);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            analyzeIf(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            analyzeWhile(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            analyzeFor(s);
        } else if constexpr (std::is_same_v<T, FuncDefStmt>) {
            analyzeFuncDef(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            analyzeReturn(s);
        } else if constexpr (std::is_same_v<T, BreakStmt>) {
            analyzeBreak(s);
        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
            analyzeContinue(s);
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            analyzeSwitch(s);
        } else if constexpr (std::is_same_v<T, TryCatchStmt>) {
            analyzeTryCatch(s);
        }
    }, stmt.node);
}

void TypeChecker::analyzeVarDecl(const VarDeclStmt& stmt) {
    // Analyze initializer first
    Type initType = analyzeExpr(*stmt.initializer);
    
    // Check for redeclaration in current scope
    if (symbols_.lookupLocal(stmt.name.lexeme)) {
        error(stmt.name.line, "Variable '" + stmt.name.lexeme + 
              "' is already declared in this scope");
        return;
    }
    
    // Declare variable with inferred type
    symbols_.declare(stmt.name.lexeme, initType, stmt.name.line);
}

void TypeChecker::analyzePrint(const PrintStmt& stmt) {
    Type exprType = analyzeExpr(*stmt.expression);
    // Print accepts any type, so no type checking needed
    (void)exprType;
}

void TypeChecker::analyzeExprStmt(const ExprStmt& stmt) {
    analyzeExpr(*stmt.expression);
}

void TypeChecker::analyzeBlock(const BlockStmt& stmt) {
    symbols_.pushScope();
    
    for (const auto& s : stmt.statements) {
        analyzeStmt(*s);
    }
    
    symbols_.popScope();
}

void TypeChecker::analyzeIf(const IfStmt& stmt) {
    Type condType = analyzeExpr(*stmt.condition);
    
    // Condition should be boolean-like (we allow numbers as booleans)
    if (!condType.isNumeric() && condType.kind != TypeKind::Boolean && 
        condType.kind != TypeKind::Any && condType.kind != TypeKind::Error) {
        error(0, "Condition in 'lowkey' must be a boolean or number, got " + condType.toString());
    }
    
    analyzeStmt(*stmt.thenBranch);
    
    if (stmt.elseBranch) {
        analyzeStmt(*stmt.elseBranch);
    }
}

void TypeChecker::analyzeWhile(const WhileStmt& stmt) {
    Type condType = analyzeExpr(*stmt.condition);
    
    if (!condType.isNumeric() && condType.kind != TypeKind::Boolean && 
        condType.kind != TypeKind::Any && condType.kind != TypeKind::Error) {
        error(0, "Condition in 'goon' must be a boolean or number, got " + condType.toString());
    }
    
    loopDepth_++;
    analyzeStmt(*stmt.body);
    loopDepth_--;
}

void TypeChecker::analyzeFor(const ForStmt& stmt) {
    // For loop creates its own scope for the initializer
    symbols_.pushScope();
    
    if (stmt.initializer) {
        analyzeStmt(*stmt.initializer);
    }
    
    if (stmt.condition) {
        Type condType = analyzeExpr(*stmt.condition);
        if (!condType.isNumeric() && condType.kind != TypeKind::Boolean && 
            condType.kind != TypeKind::Any && condType.kind != TypeKind::Error) {
            error(0, "Condition in 'edge' must be a boolean or number, got " + condType.toString());
        }
    }
    
    if (stmt.increment) {
        analyzeExpr(*stmt.increment);
    }
    
    loopDepth_++;
    analyzeStmt(*stmt.body);
    loopDepth_--;
    
    symbols_.popScope();
}

void TypeChecker::analyzeFuncDef(const FuncDefStmt& stmt) {
    FunctionInfo* funcInfo = functions_.lookup(stmt.name.lexeme);
    if (!funcInfo) {
        error(stmt.name.line, "Function '" + stmt.name.lexeme + "' not found (internal error)");
        return;
    }
    
    // Enter function scope
    bool wasInFunction = inFunction_;
    Type savedReturnType = currentFunctionReturnType_;
    inFunction_ = true;
    currentFunctionReturnType_ = Type(funcInfo->type.returnType);
    
    symbols_.pushScope();
    
    // Declare parameters
    for (size_t i = 0; i < stmt.params.size(); i++) {
        Type paramType = (i < funcInfo->type.paramTypes.size()) 
            ? Type(funcInfo->type.paramTypes[i]) 
            : Types::Number();
        symbols_.declare(stmt.params[i].lexeme, paramType, stmt.params[i].line);
    }
    
    // Analyze function body
    for (const auto& s : stmt.body) {
        analyzeStmt(*s);
    }
    
    symbols_.popScope();
    
    // Restore context
    inFunction_ = wasInFunction;
    currentFunctionReturnType_ = savedReturnType;
}

void TypeChecker::analyzeReturn(const ReturnStmt& stmt) {
    if (!inFunction_) {
        error(stmt.keyword.line, "'send' (return) used outside of function");
        return;
    }
    
    if (stmt.value) {
        Type returnType = analyzeExpr(*stmt.value);
        // For now, all functions return numbers
        if (!returnType.isNumeric() && returnType.kind != TypeKind::Any && 
            returnType.kind != TypeKind::Error) {
            // Warning: returning non-numeric from function
            warning(stmt.keyword.line, "Function returns " + returnType.toString() + 
                    ", but Number was expected");
        }
    }
}

void TypeChecker::analyzeBreak(const BreakStmt& stmt) {
    if (loopDepth_ == 0) {
        error(stmt.keyword.line, "'mog' (break) used outside of loop");
    }
}

void TypeChecker::analyzeContinue(const ContinueStmt& stmt) {
    if (loopDepth_ == 0) {
        error(stmt.keyword.line, "'skip' (continue) used outside of loop");
    }
}

void TypeChecker::analyzeSwitch(const SwitchStmt& stmt) {
    Type switchType = analyzeExpr(*stmt.expression);
    
    for (const auto& caseStmt : stmt.cases) {
        if (caseStmt.value) {
            Type caseType = analyzeExpr(*caseStmt.value);
            if (!switchType.isCompatibleWith(caseType)) {
                error(stmt.keyword.line, "Case type " + caseType.toString() + 
                      " doesn't match switch expression type " + switchType.toString());
            }
        }
        
        for (const auto& s : caseStmt.body) {
            analyzeStmt(*s);
        }
    }
}

void TypeChecker::analyzeTryCatch(const TryCatchStmt& stmt) {
    // Note: Try-catch is not fully implemented in code generation
    warning(stmt.keyword.line, "'yeet/caught' (try-catch) is not fully implemented");
    
    analyzeStmt(*stmt.tryBlock);
    analyzeStmt(*stmt.catchBlock);
}

// ============================================================================
// Expression Analysis
// ============================================================================

Type TypeChecker::analyzeExpr(const Expr& expr) {
    return std::visit([this](const auto& e) -> Type {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            return analyzeLiteral(e);
        } else if constexpr (std::is_same_v<T, IdentifierExpr>) {
            return analyzeIdentifier(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return analyzeBinary(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return analyzeUnary(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return analyzeCall(e);
        } else if constexpr (std::is_same_v<T, GroupingExpr>) {
            return analyzeGrouping(e);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
            return analyzeAssign(e);
        } else if constexpr (std::is_same_v<T, LogicalExpr>) {
            return analyzeLogical(e);
        } else if constexpr (std::is_same_v<T, CompoundAssignExpr>) {
            return analyzeCompoundAssign(e);
        } else if constexpr (std::is_same_v<T, IncrementExpr>) {
            return analyzeIncrement(e);
        } else if constexpr (std::is_same_v<T, InterpStringExpr>) {
            return analyzeInterpString(e);
        }
        return Types::Error();
    }, expr.node);
}

Type TypeChecker::analyzeLiteral(const LiteralExpr& expr) {
    return std::visit([](const auto& v) -> Type {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) {
            return Types::Number();
        } else if constexpr (std::is_same_v<T, bool>) {
            return Types::Boolean();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return Types::String();
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            return Types::Null();
        }
        return Types::Any();
    }, expr.value);
}

Type TypeChecker::analyzeIdentifier(const IdentifierExpr& expr) {
    Symbol* sym = symbols_.lookup(expr.name.lexeme);
    if (!sym) {
        // Check if it's a function name
        if (functions_.exists(expr.name.lexeme)) {
            FunctionInfo* func = functions_.lookup(expr.name.lexeme);
            return func->type;
        }
        error(expr.name.line, "Undefined variable '" + expr.name.lexeme + "'");
        return Types::Error();
    }
    return sym->type;
}

Type TypeChecker::analyzeBinary(const BinaryExpr& expr) {
    Type leftType = analyzeExpr(*expr.left);
    Type rightType = analyzeExpr(*expr.right);
    
    switch (expr.op.type) {
        // Arithmetic operators require numeric operands
        case TokenType::PLUS:
            // Plus can also concatenate strings
            if (leftType.kind == TypeKind::String || rightType.kind == TypeKind::String) {
                return Types::String();
            }
            if (!checkNumeric(leftType, expr.op.line, "left operand of '+'")) return Types::Error();
            if (!checkNumeric(rightType, expr.op.line, "right operand of '+'")) return Types::Error();
            return Types::Number();
            
        case TokenType::MINUS:
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::PERCENT:
            if (!checkNumeric(leftType, expr.op.line, "left operand")) return Types::Error();
            if (!checkNumeric(rightType, expr.op.line, "right operand")) return Types::Error();
            return Types::Number();
            
        // Comparison operators
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LEQ:
        case TokenType::GEQ:
            if (!checkNumeric(leftType, expr.op.line, "left operand of comparison")) return Types::Error();
            if (!checkNumeric(rightType, expr.op.line, "right operand of comparison")) return Types::Error();
            return Types::Boolean();
            
        // Equality operators work on any type
        case TokenType::EQ:
        case TokenType::NEQ:
            if (!leftType.isCompatibleWith(rightType)) {
                warning(expr.op.line, "Comparing incompatible types: " + 
                        leftType.toString() + " and " + rightType.toString());
            }
            return Types::Boolean();
            
        // Bitwise operators require integers
        case TokenType::BIT_AND:
        case TokenType::BIT_OR:
        case TokenType::BIT_XOR:
        case TokenType::LSHIFT:
        case TokenType::RSHIFT:
            if (!checkNumeric(leftType, expr.op.line, "left operand of bitwise operator")) return Types::Error();
            if (!checkNumeric(rightType, expr.op.line, "right operand of bitwise operator")) return Types::Error();
            return Types::Number();
            
        default:
            error(expr.op.line, "Unknown binary operator");
            return Types::Error();
    }
}

Type TypeChecker::analyzeUnary(const UnaryExpr& expr) {
    Type operandType = analyzeExpr(*expr.operand);
    
    switch (expr.op.type) {
        case TokenType::MINUS:
            if (!checkNumeric(operandType, expr.op.line, "operand of unary '-'")) return Types::Error();
            return Types::Number();
            
        case TokenType::NOT:
            // Logical NOT: any value can be used as boolean
            return Types::Boolean();
            
        case TokenType::BIT_NOT:
            if (!checkNumeric(operandType, expr.op.line, "operand of '~'")) return Types::Error();
            return Types::Number();
            
        default:
            error(expr.op.line, "Unknown unary operator");
            return Types::Error();
    }
}

Type TypeChecker::analyzeCall(const CallExpr& expr) {
    // Get function name
    auto* identExpr = std::get_if<IdentifierExpr>(&expr.callee->node);
    if (!identExpr) {
        error(expr.paren.line, "Expected function name in call");
        return Types::Error();
    }
    
    FunctionInfo* funcInfo = functions_.lookup(identExpr->name.lexeme);
    if (!funcInfo) {
        error(expr.paren.line, "Undefined function '" + identExpr->name.lexeme + "'");
        return Types::Error();
    }
    
    // Check argument count
    if (expr.arguments.size() != funcInfo->type.paramTypes.size()) {
        error(expr.paren.line, "Function '" + identExpr->name.lexeme + "' expects " + 
              std::to_string(funcInfo->type.paramTypes.size()) + " arguments, got " + 
              std::to_string(expr.arguments.size()));
        return Types::Error();
    }
    
    // Check argument types
    for (size_t i = 0; i < expr.arguments.size(); i++) {
        Type argType = analyzeExpr(*expr.arguments[i]);
        Type expectedType = Type(funcInfo->type.paramTypes[i]);
        
        if (!argType.isCompatibleWith(expectedType)) {
            error(expr.paren.line, "Argument " + std::to_string(i + 1) + " of function '" + 
                  identExpr->name.lexeme + "' expects " + expectedType.toString() + 
                  ", got " + argType.toString());
        }
    }
    
    return Type(funcInfo->type.returnType);
}

Type TypeChecker::analyzeGrouping(const GroupingExpr& expr) {
    return analyzeExpr(*expr.expression);
}

Type TypeChecker::analyzeAssign(const AssignExpr& expr) {
    Type valueType = analyzeExpr(*expr.value);
    
    Symbol* sym = symbols_.lookup(expr.name.lexeme);
    if (!sym) {
        error(expr.name.line, "Undefined variable '" + expr.name.lexeme + "' in assignment");
        return Types::Error();
    }
    
    if (sym->isConst) {
        error(expr.name.line, "Cannot assign to constant '" + expr.name.lexeme + "'");
        return Types::Error();
    }
    
    // Update the variable's type (dynamic typing)
    sym->type = valueType;
    
    return valueType;
}

Type TypeChecker::analyzeLogical(const LogicalExpr& expr) {
    Type leftType = analyzeExpr(*expr.left);
    Type rightType = analyzeExpr(*expr.right);
    
    // Logical operators accept any values (truthy/falsy)
    (void)leftType;
    (void)rightType;
    
    return Types::Boolean();
}

Type TypeChecker::analyzeCompoundAssign(const CompoundAssignExpr& expr) {
    Symbol* sym = symbols_.lookup(expr.name.lexeme);
    if (!sym) {
        error(expr.name.line, "Undefined variable '" + expr.name.lexeme + "' in compound assignment");
        return Types::Error();
    }
    
    if (sym->isConst) {
        error(expr.name.line, "Cannot modify constant '" + expr.name.lexeme + "'");
        return Types::Error();
    }
    
    Type valueType = analyzeExpr(*expr.value);
    
    if (!checkNumeric(sym->type, expr.name.line, "variable in compound assignment")) {
        return Types::Error();
    }
    if (!checkNumeric(valueType, expr.op.line, "value in compound assignment")) {
        return Types::Error();
    }
    
    return Types::Number();
}

Type TypeChecker::analyzeIncrement(const IncrementExpr& expr) {
    Symbol* sym = symbols_.lookup(expr.name.lexeme);
    if (!sym) {
        error(expr.name.line, "Undefined variable '" + expr.name.lexeme + "' in increment/decrement");
        return Types::Error();
    }
    
    if (sym->isConst) {
        error(expr.name.line, "Cannot modify constant '" + expr.name.lexeme + "'");
        return Types::Error();
    }
    
    if (!checkNumeric(sym->type, expr.name.line, "variable in increment/decrement")) {
        return Types::Error();
    }
    
    return Types::Number();
}

Type TypeChecker::analyzeInterpString(const InterpStringExpr& expr) {
    // Analyze all expressions within the interpolated string
    for (const auto& exprPart : expr.exprParts) {
        analyzeExpr(*exprPart);
        // Any type can be interpolated into a string
    }
    return Types::String();
}

// ============================================================================
// Helper Methods
// ============================================================================

void TypeChecker::error(int line, const std::string& message) {
    hadError_ = true;
    std::ostringstream ss;
    ss << "[Line " << line << "] Semantic Error: " << message;
    errors_.push_back(ss.str());
    ErrorReporter::semanticError(line, message);
}

void TypeChecker::warning(int line, const std::string& message) {
    std::ostringstream ss;
    ss << "[Line " << line << "] Warning: " << message;
    errors_.push_back(ss.str());
    // Optionally print warning
    std::cerr << "\033[1;33mWarning\033[0m [Line " << line << "]: " << message << std::endl;
}

bool TypeChecker::checkNumeric(const Type& type, int line, const std::string& context) {
    if (type.kind == TypeKind::Error || type.kind == TypeKind::Any) {
        return true; // Don't cascade errors
    }
    if (!type.isNumeric()) {
        error(line, "Expected numeric type for " + context + ", got " + type.toString());
        return false;
    }
    return true;
}

bool TypeChecker::checkBoolean(const Type& type, int line, const std::string& context) {
    if (type.kind == TypeKind::Error || type.kind == TypeKind::Any) {
        return true; // Don't cascade errors
    }
    if (type.kind != TypeKind::Boolean && !type.isNumeric()) {
        error(line, "Expected boolean type for " + context + ", got " + type.toString());
        return false;
    }
    return true;
}

bool TypeChecker::checkAssignable(const Type& target, const Type& source, int line) {
    if (!target.isCompatibleWith(source)) {
        error(line, "Cannot assign " + source.toString() + " to " + target.toString());
        return false;
    }
    return true;
}

} // namespace sigma
