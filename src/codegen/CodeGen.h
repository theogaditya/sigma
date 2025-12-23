#ifndef SIGMA_CODEGEN_H
#define SIGMA_CODEGEN_H

#include <memory>
#include <string>
#include <map>
#include <vector>
#include <stack>

// LLVM headers
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/BasicBlock.h>

#include "../ast/AST.h"

namespace sigma {

// ============================================================================
// CodeGen - LLVM IR Generator
// ============================================================================
// 
// For simplicity, we'll use a "dynamically typed" approach with double as
// the primary numeric type. Booleans are represented as doubles (0.0 = false,
// non-zero = true). Strings will be handled as global constants.
//
// Variables are stored using alloca (stack allocation).
// ============================================================================

class CodeGen {
public:
    CodeGen();
    ~CodeGen();

    // Generate code for an entire program
    bool generate(const Program& program);

    // Get the generated IR as a string
    std::string getIR() const;

    // Get the module (for further processing like JIT or object file generation)
    llvm::Module* getModule() { return module_.get(); }

    // Check if there were errors during code generation
    bool hasError() const { return hadError_; }

private:
    // LLVM core components
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;

    // Current function being compiled
    llvm::Function* currentFunction_ = nullptr;

    // Symbol table for variables (name -> alloca instruction)
    std::map<std::string, llvm::AllocaInst*> namedValues_;
    
    // Type table for variables (name -> type: "number", "string", "bool")
    std::map<std::string, std::string> variableTypes_;

    // Function table (name -> function)
    std::map<std::string, llvm::Function*> functions_;

    // Cache for format strings
    std::map<std::string, llvm::Value*> formatStrings_;

    // For break/continue: stack of loop blocks
    struct LoopInfo {
        llvm::BasicBlock* condBlock;   // For continue
        llvm::BasicBlock* afterBlock;  // For break
    };
    std::stack<LoopInfo> loopStack_;

    // Error state
    bool hadError_ = false;

    // ========================================================================
    // Statement Code Generation
    // ========================================================================
    void generateStmt(const Stmt& stmt);
    void generateVarDecl(const VarDeclStmt& stmt);
    void generatePrint(const PrintStmt& stmt);
    void generateExprStmt(const ExprStmt& stmt);
    void generateBlock(const BlockStmt& stmt);
    void generateIf(const IfStmt& stmt);
    void generateWhile(const WhileStmt& stmt);
    void generateFor(const ForStmt& stmt);
    void generateFuncDef(const FuncDefStmt& stmt);
    void generateReturn(const ReturnStmt& stmt);
    void generateBreak(const BreakStmt& stmt);
    void generateContinue(const ContinueStmt& stmt);

    // ========================================================================
    // Expression Code Generation
    // ========================================================================
    llvm::Value* generateExpr(const Expr& expr);
    llvm::Value* generateLiteral(const LiteralExpr& expr);
    llvm::Value* generateIdentifier(const IdentifierExpr& expr);
    llvm::Value* generateBinary(const BinaryExpr& expr);
    llvm::Value* generateUnary(const UnaryExpr& expr);
    llvm::Value* generateCall(const CallExpr& expr);
    llvm::Value* generateGrouping(const GroupingExpr& expr);
    llvm::Value* generateAssign(const AssignExpr& expr);
    llvm::Value* generateLogical(const LogicalExpr& expr);

    // ========================================================================
    // Helper Functions
    // ========================================================================
    
    // Create alloca instruction in entry block of function
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* func, 
                                              const std::string& varName);

    // Create the printf function declaration
    llvm::Function* getOrCreatePrintf();

    // Create or get a global format string
    llvm::Value* getOrCreateFormatString(const std::string& str, const std::string& name);

    // Create a global string constant
    llvm::Value* createGlobalString(const std::string& str, const std::string& name);

    // Create the main function wrapper
    llvm::Function* createMainFunction();

    // Create floating-point comparison (workaround for LLVM version issues)
    llvm::Value* createFCmp(llvm::CmpInst::Predicate pred, 
                            llvm::Value* left, llvm::Value* right,
                            const std::string& name);

    // Convert boolean condition to i1 (for branches)
    llvm::Value* toBool(llvm::Value* value);

    // Error reporting
    void error(const std::string& message);
};

} // namespace sigma

#endif // SIGMA_CODEGEN_H
