#include "CodeGen.h"
#include "../utils/Error.h"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>

namespace sigma {

// ============================================================================
// Constructor / Destructor
// ============================================================================

CodeGen::CodeGen() {
    context_ = std::make_unique<llvm::LLVMContext>();
    module_ = std::make_unique<llvm::Module>("sigma_module", *context_);
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
}

CodeGen::~CodeGen() = default;

// ============================================================================
// Main Generation Entry Point
// ============================================================================

bool CodeGen::generate(const Program& program) {
    // First pass: declare all functions
    for (const auto& stmt : program) {
        if (auto* funcDef = std::get_if<FuncDefStmt>(&stmt->node)) {
            // Create function prototype
            std::vector<llvm::Type*> paramTypes(funcDef->params.size(), 
                                                  llvm::Type::getDoubleTy(*context_));
            llvm::FunctionType* funcType = llvm::FunctionType::get(
                llvm::Type::getDoubleTy(*context_), paramTypes, false);
            
            llvm::Function* func = llvm::Function::Create(
                funcType, llvm::Function::ExternalLinkage, 
                funcDef->name.lexeme, module_.get());
            
            functions_[funcDef->name.lexeme] = func;
        }
    }

    // Create main function to hold top-level code
    llvm::Function* mainFunc = createMainFunction();
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", mainFunc);
    builder_->SetInsertPoint(entry);
    currentFunction_ = mainFunc;

    // Second pass: generate code for all statements
    for (const auto& stmt : program) {
        generateStmt(*stmt);
        if (hadError_) return false;
    }

    // Add return 0 to main
    builder_->CreateRet(llvm::ConstantInt::get(*context_, llvm::APInt(32, 0)));

    // Verify the module
    std::string errStr;
    llvm::raw_string_ostream errStream(errStr);
    if (llvm::verifyModule(*module_, &errStream)) {
        error("Module verification failed: " + errStr);
        return false;
    }

    return !hadError_;
}

std::string CodeGen::getIR() const {
    std::string ir;
    llvm::raw_string_ostream stream(ir);
    module_->print(stream, nullptr);
    return ir;
}

// ============================================================================
// Statement Code Generation
// ============================================================================

void CodeGen::generateStmt(const Stmt& stmt) {
    std::visit([this](const auto& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, VarDeclStmt>) {
            generateVarDecl(s);
        } else if constexpr (std::is_same_v<T, PrintStmt>) {
            generatePrint(s);
        } else if constexpr (std::is_same_v<T, ExprStmt>) {
            generateExprStmt(s);
        } else if constexpr (std::is_same_v<T, BlockStmt>) {
            generateBlock(s);
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            generateIf(s);
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            generateWhile(s);
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            generateFor(s);
        } else if constexpr (std::is_same_v<T, FuncDefStmt>) {
            generateFuncDef(s);
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            generateReturn(s);
        } else if constexpr (std::is_same_v<T, BreakStmt>) {
            generateBreak(s);
        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
            generateContinue(s);
        }
    }, stmt.node);
}

// fr x = expression
void CodeGen::generateVarDecl(const VarDeclStmt& stmt) {
    llvm::Value* initVal = generateExpr(*stmt.initializer);
    if (!initVal) return;

    // Determine the type and create appropriate alloca
    llvm::Type* varType;
    std::string typeStr;
    
    if (initVal->getType()->isPointerTy()) {
        // String type - store as pointer
        varType = llvm::PointerType::get(*context_, 0);
        typeStr = "string";
    } else {
        // Number type (default)
        varType = llvm::Type::getDoubleTy(*context_);
        typeStr = "number";
    }

    // Create alloca for variable
    llvm::Function* func = currentFunction_;
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
    llvm::AllocaInst* alloca = tmpBuilder.CreateAlloca(varType, nullptr, stmt.name.lexeme);
    
    builder_->CreateStore(initVal, alloca);
    namedValues_[stmt.name.lexeme] = alloca;
    variableTypes_[stmt.name.lexeme] = typeStr;
}

// say expression
void CodeGen::generatePrint(const PrintStmt& stmt) {
    llvm::Value* value = generateExpr(*stmt.expression);
    if (!value) return;

    llvm::Function* printfFunc = getOrCreatePrintf();

    // Create format string based on value type
    llvm::Value* formatStr;
    std::vector<llvm::Value*> args;

    if (value->getType()->isDoubleTy()) {
        // Print as number: "%g\n"
        formatStr = getOrCreateFormatString("%g\n", "fmt_num");
        args.push_back(formatStr);
        args.push_back(value);
    } else if (value->getType()->isPointerTy()) {
        // Print as string: "%s\n"
        formatStr = getOrCreateFormatString("%s\n", "fmt_str");
        args.push_back(formatStr);
        args.push_back(value);
    } else {
        // Fallback: convert to double
        formatStr = getOrCreateFormatString("%g\n", "fmt_num");
        args.push_back(formatStr);
        llvm::Value* dblVal = builder_->CreateSIToFP(value, 
            llvm::Type::getDoubleTy(*context_), "todbl");
        args.push_back(dblVal);
    }

    builder_->CreateCall(printfFunc, args, "printfcall");
}

void CodeGen::generateExprStmt(const ExprStmt& stmt) {
    generateExpr(*stmt.expression);
}

void CodeGen::generateBlock(const BlockStmt& stmt) {
    // Save current variable scope (simple approach)
    auto savedValues = namedValues_;
    
    for (const auto& s : stmt.statements) {
        generateStmt(*s);
        if (hadError_) return;
    }
    
    // Restore scope (variables declared in block go out of scope)
    // For simplicity, we keep all variables visible (like Python)
}

// lowkey (condition) { ... } highkey { ... }
void CodeGen::generateIf(const IfStmt& stmt) {
    llvm::Value* condVal = generateExpr(*stmt.condition);
    if (!condVal) return;

    // Convert to boolean
    llvm::Value* condBool = toBool(condVal);

    llvm::Function* func = builder_->GetInsertBlock()->getParent();

    // Create blocks
    llvm::BasicBlock* thenBB = llvm::BasicBlock::Create(*context_, "then", func);
    llvm::BasicBlock* elseBB = llvm::BasicBlock::Create(*context_, "else");
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "ifcont");

    if (stmt.elseBranch) {
        builder_->CreateCondBr(condBool, thenBB, elseBB);
    } else {
        builder_->CreateCondBr(condBool, thenBB, mergeBB);
    }

    // Emit then block
    builder_->SetInsertPoint(thenBB);
    generateStmt(*stmt.thenBranch);
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(mergeBB);
    }

    // Emit else block
    if (stmt.elseBranch) {
        func->insert(func->end(), elseBB);
        builder_->SetInsertPoint(elseBB);
        generateStmt(*stmt.elseBranch);
        if (!builder_->GetInsertBlock()->getTerminator()) {
            builder_->CreateBr(mergeBB);
        }
    }

    // Emit merge block
    func->insert(func->end(), mergeBB);
    builder_->SetInsertPoint(mergeBB);
}

// goon (condition) { ... }
void CodeGen::generateWhile(const WhileStmt& stmt) {
    llvm::Function* func = builder_->GetInsertBlock()->getParent();

    // Create blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context_, "whilecond", func);
    llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(*context_, "whilebody");
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context_, "whileend");

    // Push loop info for break/continue
    loopStack_.push({condBB, afterBB});

    // Jump to condition
    builder_->CreateBr(condBB);

    // Emit condition block
    builder_->SetInsertPoint(condBB);
    llvm::Value* condVal = generateExpr(*stmt.condition);
    if (!condVal) return;
    llvm::Value* condBool = toBool(condVal);
    builder_->CreateCondBr(condBool, loopBB, afterBB);

    // Emit loop body
    func->insert(func->end(), loopBB);
    builder_->SetInsertPoint(loopBB);
    generateStmt(*stmt.body);
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(condBB);
    }

    // Emit after block
    func->insert(func->end(), afterBB);
    builder_->SetInsertPoint(afterBB);

    loopStack_.pop();
}

// edge (init, cond, incr) { ... }
void CodeGen::generateFor(const ForStmt& stmt) {
    llvm::Function* func = builder_->GetInsertBlock()->getParent();

    // Generate initializer
    if (stmt.initializer) {
        generateStmt(*stmt.initializer);
    }

    // Create blocks
    llvm::BasicBlock* condBB = llvm::BasicBlock::Create(*context_, "forcond", func);
    llvm::BasicBlock* loopBB = llvm::BasicBlock::Create(*context_, "forbody");
    llvm::BasicBlock* incrBB = llvm::BasicBlock::Create(*context_, "forincr");
    llvm::BasicBlock* afterBB = llvm::BasicBlock::Create(*context_, "forend");

    // Push loop info (continue goes to increment, break goes to after)
    loopStack_.push({incrBB, afterBB});

    // Jump to condition
    builder_->CreateBr(condBB);

    // Emit condition block
    builder_->SetInsertPoint(condBB);
    if (stmt.condition) {
        llvm::Value* condVal = generateExpr(*stmt.condition);
        if (!condVal) return;
        llvm::Value* condBool = toBool(condVal);
        builder_->CreateCondBr(condBool, loopBB, afterBB);
    } else {
        // No condition = infinite loop
        builder_->CreateBr(loopBB);
    }

    // Emit loop body
    func->insert(func->end(), loopBB);
    builder_->SetInsertPoint(loopBB);
    generateStmt(*stmt.body);
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(incrBB);
    }

    // Emit increment block
    func->insert(func->end(), incrBB);
    builder_->SetInsertPoint(incrBB);
    if (stmt.increment) {
        generateExpr(*stmt.increment);
    }
    builder_->CreateBr(condBB);

    // Emit after block
    func->insert(func->end(), afterBB);
    builder_->SetInsertPoint(afterBB);

    loopStack_.pop();
}

// vibe funcName(params) { ... }
void CodeGen::generateFuncDef(const FuncDefStmt& stmt) {
    llvm::Function* func = functions_[stmt.name.lexeme];
    if (!func) {
        error("Function not found: " + stmt.name.lexeme);
        return;
    }

    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*context_, "entry", func);
    
    // Save current state
    llvm::Function* savedFunc = currentFunction_;
    llvm::BasicBlock* savedBlock = builder_->GetInsertBlock();
    auto savedValues = namedValues_;
    auto savedTypes = variableTypes_;

    // Set up for new function
    builder_->SetInsertPoint(entry);
    currentFunction_ = func;
    namedValues_.clear();
    variableTypes_.clear();

    // Create allocas for parameters (all treated as numbers for now)
    size_t idx = 0;
    for (auto& arg : func->args()) {
        std::string paramName = stmt.params[idx].lexeme;
        arg.setName(paramName);
        
        llvm::AllocaInst* alloca = createEntryBlockAlloca(func, paramName);
        builder_->CreateStore(&arg, alloca);
        namedValues_[paramName] = alloca;
        variableTypes_[paramName] = "number";
        idx++;
    }

    // Generate function body
    for (const auto& s : stmt.body) {
        generateStmt(*s);
    }

    // Add default return if not already present
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateRet(llvm::ConstantFP::get(*context_, llvm::APFloat(0.0)));
    }

    // Verify function
    if (llvm::verifyFunction(*func, &llvm::errs())) {
        error("Function verification failed: " + stmt.name.lexeme);
    }

    // Restore state
    currentFunction_ = savedFunc;
    namedValues_ = savedValues;
    variableTypes_ = savedTypes;
    if (savedBlock) {
        builder_->SetInsertPoint(savedBlock);
    }
}

// send expression
void CodeGen::generateReturn(const ReturnStmt& stmt) {
    if (stmt.value) {
        llvm::Value* retVal = generateExpr(*stmt.value);
        if (retVal) {
            builder_->CreateRet(retVal);
        }
    } else {
        builder_->CreateRet(llvm::ConstantFP::get(*context_, llvm::APFloat(0.0)));
    }
}

// mog (break)
void CodeGen::generateBreak(const BreakStmt&) {
    if (loopStack_.empty()) {
        error("'mog' (break) used outside of loop");
        return;
    }
    builder_->CreateBr(loopStack_.top().afterBlock);
}

// skip (continue)
void CodeGen::generateContinue(const ContinueStmt&) {
    if (loopStack_.empty()) {
        error("'skip' (continue) used outside of loop");
        return;
    }
    builder_->CreateBr(loopStack_.top().condBlock);
}

// ============================================================================
// Expression Code Generation
// ============================================================================

llvm::Value* CodeGen::generateExpr(const Expr& expr) {
    return std::visit([this](const auto& e) -> llvm::Value* {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, LiteralExpr>) {
            return generateLiteral(e);
        } else if constexpr (std::is_same_v<T, IdentifierExpr>) {
            return generateIdentifier(e);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            return generateBinary(e);
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return generateUnary(e);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            return generateCall(e);
        } else if constexpr (std::is_same_v<T, GroupingExpr>) {
            return generateGrouping(e);
        } else if constexpr (std::is_same_v<T, AssignExpr>) {
            return generateAssign(e);
        } else if constexpr (std::is_same_v<T, LogicalExpr>) {
            return generateLogical(e);
        }
        return nullptr;
    }, expr.node);
}

llvm::Value* CodeGen::generateLiteral(const LiteralExpr& expr) {
    return std::visit([this](const auto& v) -> llvm::Value* {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, double>) {
            return llvm::ConstantFP::get(*context_, llvm::APFloat(v));
        } else if constexpr (std::is_same_v<T, bool>) {
            return llvm::ConstantFP::get(*context_, llvm::APFloat(v ? 1.0 : 0.0));
        } else if constexpr (std::is_same_v<T, std::string>) {
            return createGlobalString(v, "str");
        } else if constexpr (std::is_same_v<T, std::monostate>) {
            // null/nah -> 0.0
            return llvm::ConstantFP::get(*context_, llvm::APFloat(0.0));
        }
        return nullptr;
    }, expr.value);
}

llvm::Value* CodeGen::generateIdentifier(const IdentifierExpr& expr) {
    auto it = namedValues_.find(expr.name.lexeme);
    if (it == namedValues_.end()) {
        error("Unknown variable: " + expr.name.lexeme);
        return nullptr;
    }
    
    // Load based on the variable's type
    auto typeIt = variableTypes_.find(expr.name.lexeme);
    if (typeIt != variableTypes_.end() && typeIt->second == "string") {
        // Load as pointer for strings
        return builder_->CreateLoad(llvm::PointerType::get(*context_, 0), 
                                     it->second, expr.name.lexeme);
    }
    
    // Default: load as double
    return builder_->CreateLoad(llvm::Type::getDoubleTy(*context_), 
                                 it->second, expr.name.lexeme);
}

llvm::Value* CodeGen::generateBinary(const BinaryExpr& expr) {
    llvm::Value* left = generateExpr(*expr.left);
    llvm::Value* right = generateExpr(*expr.right);
    if (!left || !right) return nullptr;

    switch (expr.op.type) {
        case TokenType::PLUS:
            return builder_->CreateFAdd(left, right, "addtmp");
        case TokenType::MINUS:
            return builder_->CreateFSub(left, right, "subtmp");
        case TokenType::STAR:
            return builder_->CreateFMul(left, right, "multmp");
        case TokenType::SLASH:
            return builder_->CreateFDiv(left, right, "divtmp");
        case TokenType::LT: {
            llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_OLT, left, right, "cmptmp");
            return builder_->CreateUIToFP(cmp, llvm::Type::getDoubleTy(*context_), "booltmp");
        }
        case TokenType::GT: {
            llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_OGT, left, right, "cmptmp");
            return builder_->CreateUIToFP(cmp, llvm::Type::getDoubleTy(*context_), "booltmp");
        }
        case TokenType::LEQ: {
            llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_OLE, left, right, "cmptmp");
            return builder_->CreateUIToFP(cmp, llvm::Type::getDoubleTy(*context_), "booltmp");
        }
        case TokenType::GEQ: {
            llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_OGE, left, right, "cmptmp");
            return builder_->CreateUIToFP(cmp, llvm::Type::getDoubleTy(*context_), "booltmp");
        }
        case TokenType::EQ: {
            llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_OEQ, left, right, "cmptmp");
            return builder_->CreateUIToFP(cmp, llvm::Type::getDoubleTy(*context_), "booltmp");
        }
        case TokenType::NEQ: {
            llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_ONE, left, right, "cmptmp");
            return builder_->CreateUIToFP(cmp, llvm::Type::getDoubleTy(*context_), "booltmp");
        }
        default:
            error("Unknown binary operator");
            return nullptr;
    }
}

llvm::Value* CodeGen::generateUnary(const UnaryExpr& expr) {
    llvm::Value* operand = generateExpr(*expr.operand);
    if (!operand) return nullptr;

    switch (expr.op.type) {
        case TokenType::MINUS:
            return builder_->CreateFNeg(operand, "negtmp");
        case TokenType::NOT: {
            // !x -> (x == 0.0) ? 1.0 : 0.0
            llvm::Value* zero = llvm::ConstantFP::get(*context_, llvm::APFloat(0.0));
            llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_OEQ, operand, zero, "nottmp");
            return builder_->CreateUIToFP(cmp, llvm::Type::getDoubleTy(*context_), "booltmp");
        }
        default:
            error("Unknown unary operator");
            return nullptr;
    }
}

llvm::Value* CodeGen::generateCall(const CallExpr& expr) {
    // Get function name from callee
    auto* identExpr = std::get_if<IdentifierExpr>(&expr.callee->node);
    if (!identExpr) {
        error("Expected function name in call");
        return nullptr;
    }

    std::string funcName = identExpr->name.lexeme;
    llvm::Function* calleeFunc = functions_[funcName];
    
    if (!calleeFunc) {
        calleeFunc = module_->getFunction(funcName);
    }
    
    if (!calleeFunc) {
        error("Unknown function: " + funcName);
        return nullptr;
    }

    // Check argument count
    if (calleeFunc->arg_size() != expr.arguments.size()) {
        error("Wrong number of arguments for function: " + funcName);
        return nullptr;
    }

    // Generate arguments
    std::vector<llvm::Value*> args;
    for (const auto& arg : expr.arguments) {
        llvm::Value* argVal = generateExpr(*arg);
        if (!argVal) return nullptr;
        args.push_back(argVal);
    }

    return builder_->CreateCall(calleeFunc, args, "calltmp");
}

llvm::Value* CodeGen::generateGrouping(const GroupingExpr& expr) {
    return generateExpr(*expr.expression);
}

llvm::Value* CodeGen::generateAssign(const AssignExpr& expr) {
    llvm::Value* value = generateExpr(*expr.value);
    if (!value) return nullptr;

    auto it = namedValues_.find(expr.name.lexeme);
    if (it == namedValues_.end()) {
        error("Unknown variable in assignment: " + expr.name.lexeme);
        return nullptr;
    }

    // Check if type is changing
    std::string newType = value->getType()->isPointerTy() ? "string" : "number";
    auto typeIt = variableTypes_.find(expr.name.lexeme);
    
    if (typeIt != variableTypes_.end() && typeIt->second != newType) {
        // Type is changing - need to create a new alloca with correct type
        llvm::Type* varType = value->getType()->isPointerTy() 
            ? llvm::PointerType::get(*context_, 0)
            : llvm::Type::getDoubleTy(*context_);
        
        llvm::Function* func = currentFunction_;
        llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), func->getEntryBlock().begin());
        llvm::AllocaInst* alloca = tmpBuilder.CreateAlloca(varType, nullptr, expr.name.lexeme);
        
        namedValues_[expr.name.lexeme] = alloca;
        variableTypes_[expr.name.lexeme] = newType;
        builder_->CreateStore(value, alloca);
    } else {
        builder_->CreateStore(value, it->second);
    }
    
    return value;
}

llvm::Value* CodeGen::generateLogical(const LogicalExpr& expr) {
    // Short-circuit evaluation
    llvm::Value* left = generateExpr(*expr.left);
    if (!left) return nullptr;

    llvm::Function* func = builder_->GetInsertBlock()->getParent();
    llvm::Value* leftBool = toBool(left);

    if (expr.op.type == TokenType::OR) {
        // a || b: if a is true, result is 1.0, else evaluate b
        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(*context_, "or.rhs", func);
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "or.merge");

        builder_->CreateCondBr(leftBool, mergeBB, rhsBB);

        // RHS
        builder_->SetInsertPoint(rhsBB);
        llvm::Value* right = generateExpr(*expr.right);
        if (!right) return nullptr;
        llvm::Value* rightBool = toBool(right);
        llvm::Value* rightResult = builder_->CreateUIToFP(rightBool, 
            llvm::Type::getDoubleTy(*context_), "ortmp");
        builder_->CreateBr(mergeBB);
        rhsBB = builder_->GetInsertBlock();

        // Merge
        func->insert(func->end(), mergeBB);
        builder_->SetInsertPoint(mergeBB);
        llvm::PHINode* phi = builder_->CreatePHI(llvm::Type::getDoubleTy(*context_), 2, "orphi");
        phi->addIncoming(llvm::ConstantFP::get(*context_, llvm::APFloat(1.0)), 
                         func->begin()->getNextNode() ? &*std::prev(func->end(), 2) : &func->getEntryBlock());
        phi->addIncoming(rightResult, rhsBB);
        return phi;
    } else {
        // a && b: if a is false, result is 0.0, else evaluate b
        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(*context_, "and.rhs", func);
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "and.merge");

        llvm::BasicBlock* currentBB = builder_->GetInsertBlock();
        builder_->CreateCondBr(leftBool, rhsBB, mergeBB);

        // RHS
        builder_->SetInsertPoint(rhsBB);
        llvm::Value* right = generateExpr(*expr.right);
        if (!right) return nullptr;
        llvm::Value* rightBool = toBool(right);
        llvm::Value* rightResult = builder_->CreateUIToFP(rightBool, 
            llvm::Type::getDoubleTy(*context_), "andtmp");
        builder_->CreateBr(mergeBB);
        rhsBB = builder_->GetInsertBlock();

        // Merge
        func->insert(func->end(), mergeBB);
        builder_->SetInsertPoint(mergeBB);
        llvm::PHINode* phi = builder_->CreatePHI(llvm::Type::getDoubleTy(*context_), 2, "andphi");
        phi->addIncoming(llvm::ConstantFP::get(*context_, llvm::APFloat(0.0)), currentBB);
        phi->addIncoming(rightResult, rhsBB);
        return phi;
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

llvm::AllocaInst* CodeGen::createEntryBlockAlloca(llvm::Function* func, 
                                                   const std::string& varName) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), 
                                  func->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(llvm::Type::getDoubleTy(*context_), nullptr, varName);
}

llvm::Function* CodeGen::getOrCreatePrintf() {
    llvm::Function* printfFunc = module_->getFunction("printf");
    if (!printfFunc) {
        // int printf(const char*, ...) - use opaque pointer (no pointee type)
        llvm::FunctionType* printfType = llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context_),
            {llvm::PointerType::get(*context_, 0)},  // opaque pointer
            true  // varargs
        );
        printfFunc = llvm::Function::Create(printfType, 
            llvm::Function::ExternalLinkage, "printf", module_.get());
    }
    return printfFunc;
}

llvm::Value* CodeGen::getOrCreateFormatString(const std::string& str, const std::string& name) {
    // Check if we already created this format string
    auto it = formatStrings_.find(str);
    if (it != formatStrings_.end()) {
        return it->second;
    }
    
    // Create a new global string constant
    llvm::Value* strPtr = createGlobalString(str, name);
    formatStrings_[str] = strPtr;
    return strPtr;
}

llvm::Value* CodeGen::createGlobalString(const std::string& str, const std::string& name) {
    // Create a constant array for the string (including null terminator)
    llvm::Constant* strConstant = llvm::ConstantDataArray::getString(*context_, str, true);
    
    // Create a global variable
    llvm::GlobalVariable* gv = new llvm::GlobalVariable(
        *module_,
        strConstant->getType(),
        true,  // isConstant
        llvm::GlobalValue::PrivateLinkage,
        strConstant,
        name
    );
    
    // Return a pointer to the first element (GEP to get i8*)
    return llvm::ConstantExpr::getGetElementPtr(
        strConstant->getType(), gv, 
        llvm::ArrayRef<llvm::Constant*>{
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context_), 0),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context_), 0)
        }
    );
}

llvm::Function* CodeGen::createMainFunction() {
    llvm::FunctionType* mainType = llvm::FunctionType::get(
        llvm::Type::getInt32Ty(*context_), {}, false);
    return llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, 
                                   "main", module_.get());
}

llvm::Value* CodeGen::createFCmp(llvm::CmpInst::Predicate pred, 
                                  llvm::Value* left, llvm::Value* right,
                                  const std::string& name) {
    // Create FCmpInst directly to work around potential LLVM version ABI issues
    llvm::Instruction* cmp = llvm::CmpInst::Create(
        llvm::Instruction::FCmp, pred, left, right, name);
    // Insert at the current insert point
    cmp->insertInto(builder_->GetInsertBlock(), builder_->GetInsertBlock()->end());
    return cmp;
}

llvm::Value* CodeGen::toBool(llvm::Value* value) {
    if (value->getType()->isDoubleTy()) {
        llvm::Value* zero = llvm::ConstantFP::get(*context_, llvm::APFloat(0.0));
        llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_ONE, value, zero, "tobool");
        return cmp;
    }
    // Already a boolean (i1)
    return value;
}

void CodeGen::error(const std::string& message) {
    std::cerr << "CodeGen Error: " << message << std::endl;
    hadError_ = true;
}

} // namespace sigma
