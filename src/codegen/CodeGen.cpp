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
    
    // Initialize global scope
    pushScope();
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
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            generateSwitch(s);
        } else if constexpr (std::is_same_v<T, TryCatchStmt>) {
            generateTryCatch(s);
        }
    }, stmt.node);
}

// fr x = expression
void CodeGen::generateVarDecl(const VarDeclStmt& stmt) {
    // Check if it's an array literal
    if (auto* arrayExpr = std::get_if<ArrayExpr>(&stmt.initializer->node)) {
        size_t size = arrayExpr->elements.size();
        
        // Create array type
        llvm::ArrayType* arrayType = llvm::ArrayType::get(
            llvm::Type::getDoubleTy(*context_), size);
        
        // Allocate array
        llvm::Function* func = currentFunction_;
        llvm::AllocaInst* alloca = builder_->CreateAlloca(arrayType, nullptr, stmt.name.lexeme);
        
        // Initialize elements
        for (size_t i = 0; i < size; i++) {
            llvm::Value* elemVal = generateExpr(*arrayExpr->elements[i]);
            if (!elemVal) return;
            
            // Ensure it's a double
            if (!elemVal->getType()->isDoubleTy()) {
                if (elemVal->getType()->isIntegerTy()) {
                    elemVal = builder_->CreateSIToFP(elemVal, llvm::Type::getDoubleTy(*context_), "todbl");
                }
            }
            
            // Store element
            llvm::Value* indices[] = {
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0),
                llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), i)
            };
            llvm::Value* elemPtr = builder_->CreateGEP(arrayType, alloca, indices, "elemptr");
            builder_->CreateStore(elemVal, elemPtr);
        }
        
        // Use custom declaration with array size
        VariableInfo info(alloca, "array", currentScopeDepth_, size);
        scopeStack_.back()[stmt.name.lexeme] = info;
        return;
    }
    
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

    // Create alloca for variable (in current scope)
    llvm::Function* func = currentFunction_;
    llvm::AllocaInst* alloca = createEntryBlockAlloca(func, stmt.name.lexeme, varType);
    
    builder_->CreateStore(initVal, alloca);
    declareVariable(stmt.name.lexeme, alloca, typeStr);
}

// say expression
void CodeGen::generatePrint(const PrintStmt& stmt) {
    // Check if it's an interpolated string - handle specially
    if (auto* interpExpr = std::get_if<InterpStringExpr>(&stmt.expression->node)) {
        generateInterpStringPrint(*interpExpr);
        return;
    }
    
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

// Print interpolated string: say "hello {name}, you are {age} years old"
void CodeGen::generateInterpStringPrint(const InterpStringExpr& expr) {
    llvm::Function* printfFunc = getOrCreatePrintf();
    
    // Build format string and collect values
    std::string formatStr;
    std::vector<llvm::Value*> values;
    
    for (size_t i = 0; i < expr.stringParts.size(); i++) {
        formatStr += expr.stringParts[i];
        
        if (i < expr.exprParts.size()) {
            llvm::Value* val = generateExpr(*expr.exprParts[i]);
            if (!val) continue;
            
            // Check the type and add appropriate format specifier
            if (val->getType()->isDoubleTy()) {
                formatStr += "%g";
                values.push_back(val);
            } else if (val->getType()->isPointerTy()) {
                // Assume it's a string
                formatStr += "%s";
                values.push_back(val);
            } else {
                formatStr += "%g";
                values.push_back(val);
            }
        }
    }
    
    // Add newline
    formatStr += "\n";
    
    // Create format string
    llvm::Value* fmtStr = createGlobalString(formatStr, "interp_fmt");
    
    // Build args list
    std::vector<llvm::Value*> args;
    args.push_back(fmtStr);
    for (auto& v : values) {
        args.push_back(v);
    }
    
    builder_->CreateCall(printfFunc, args, "printfcall");
}

void CodeGen::generateExprStmt(const ExprStmt& stmt) {
    generateExpr(*stmt.expression);
}

void CodeGen::generateBlock(const BlockStmt& stmt) {
    // Create new scope for block
    pushScope();
    
    for (const auto& s : stmt.statements) {
        generateStmt(*s);
        if (hadError_) break;
    }
    
    // Restore scope (variables declared in block go out of scope)
    popScope();
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
    auto savedScopeStack = scopeStack_;
    int savedScopeDepth = currentScopeDepth_;

    // Set up for new function with fresh scope
    builder_->SetInsertPoint(entry);
    currentFunction_ = func;
    scopeStack_.clear();
    currentScopeDepth_ = 0;
    pushScope();  // Function scope

    // Create allocas for parameters (all treated as numbers for now)
    size_t idx = 0;
    for (auto& arg : func->args()) {
        std::string paramName = stmt.params[idx].lexeme;
        arg.setName(paramName);
        
        llvm::AllocaInst* alloca = createEntryBlockAlloca(func, paramName);
        builder_->CreateStore(&arg, alloca);
        declareVariable(paramName, alloca, "number");
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
    scopeStack_ = savedScopeStack;
    currentScopeDepth_ = savedScopeDepth;
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
        } else if constexpr (std::is_same_v<T, CompoundAssignExpr>) {
            return generateCompoundAssign(e);
        } else if constexpr (std::is_same_v<T, IncrementExpr>) {
            return generateIncrement(e);
        } else if constexpr (std::is_same_v<T, InterpStringExpr>) {
            return generateInterpString(e);
        } else if constexpr (std::is_same_v<T, ArrayExpr>) {
            return generateArray(e);
        } else if constexpr (std::is_same_v<T, IndexExpr>) {
            return generateIndex(e);
        } else if constexpr (std::is_same_v<T, IndexAssignExpr>) {
            return generateIndexAssign(e);
        }
        return nullptr;
    }, expr.node);
}

llvm::Value* CodeGen::generateLiteral(const LiteralExpr& expr) {
    return std::visit([this](const auto& v) -> llvm::Value* {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, int64_t>) {
            // Integer literal - convert to double for consistency
            return llvm::ConstantFP::get(*context_, llvm::APFloat(static_cast<double>(v)));
        } else if constexpr (std::is_same_v<T, double>) {
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
    VariableInfo* varInfo = lookupVariable(expr.name.lexeme);
    if (!varInfo) {
        error("Unknown variable: " + expr.name.lexeme);
        return nullptr;
    }
    
    // Arrays: return the pointer to the array (don't load)
    if (varInfo->type == "array") {
        return varInfo->allocaInst;
    }
    
    // Load based on the variable's type
    if (varInfo->type == "string") {
        // Load as pointer for strings
        return builder_->CreateLoad(llvm::PointerType::get(*context_, 0), 
                                     varInfo->allocaInst, expr.name.lexeme);
    }
    
    // Default: load as double
    return builder_->CreateLoad(llvm::Type::getDoubleTy(*context_), 
                                 varInfo->allocaInst, expr.name.lexeme);
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
        case TokenType::PERCENT: {
            // Modulo for floats: a % b = a - floor(a/b) * b
            return builder_->CreateFRem(left, right, "modtmp");
        }
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
        // Bitwise operators (convert to int, operate, convert back)
        case TokenType::BIT_AND: {
            llvm::Value* leftInt = builder_->CreateFPToSI(left, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* rightInt = builder_->CreateFPToSI(right, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* result = builder_->CreateAnd(leftInt, rightInt, "andtmp");
            return builder_->CreateSIToFP(result, llvm::Type::getDoubleTy(*context_), "tofp");
        }
        case TokenType::BIT_OR: {
            llvm::Value* leftInt = builder_->CreateFPToSI(left, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* rightInt = builder_->CreateFPToSI(right, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* result = builder_->CreateOr(leftInt, rightInt, "ortmp");
            return builder_->CreateSIToFP(result, llvm::Type::getDoubleTy(*context_), "tofp");
        }
        case TokenType::BIT_XOR: {
            llvm::Value* leftInt = builder_->CreateFPToSI(left, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* rightInt = builder_->CreateFPToSI(right, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* result = builder_->CreateXor(leftInt, rightInt, "xortmp");
            return builder_->CreateSIToFP(result, llvm::Type::getDoubleTy(*context_), "tofp");
        }
        case TokenType::LSHIFT: {
            llvm::Value* leftInt = builder_->CreateFPToSI(left, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* rightInt = builder_->CreateFPToSI(right, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* result = builder_->CreateShl(leftInt, rightInt, "shltmp");
            return builder_->CreateSIToFP(result, llvm::Type::getDoubleTy(*context_), "tofp");
        }
        case TokenType::RSHIFT: {
            llvm::Value* leftInt = builder_->CreateFPToSI(left, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* rightInt = builder_->CreateFPToSI(right, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* result = builder_->CreateAShr(leftInt, rightInt, "shrtmp");
            return builder_->CreateSIToFP(result, llvm::Type::getDoubleTy(*context_), "tofp");
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
        case TokenType::BIT_NOT: {
            // ~x -> bitwise NOT (convert to int, NOT, convert back)
            llvm::Value* intVal = builder_->CreateFPToSI(operand, llvm::Type::getInt64Ty(*context_), "toint");
            llvm::Value* result = builder_->CreateNot(intVal, "bitnot");
            return builder_->CreateSIToFP(result, llvm::Type::getDoubleTy(*context_), "tofp");
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

    VariableInfo* varInfo = lookupVariable(expr.name.lexeme);
    if (!varInfo) {
        error("Unknown variable in assignment: " + expr.name.lexeme);
        return nullptr;
    }

    // Check if type is changing
    std::string newType = value->getType()->isPointerTy() ? "string" : "number";
    
    if (varInfo->type != newType) {
        // Type is changing - need to create a new alloca with correct type
        llvm::Type* varType = value->getType()->isPointerTy() 
            ? llvm::PointerType::get(*context_, 0)
            : llvm::Type::getDoubleTy(*context_);
        
        llvm::AllocaInst* alloca = createEntryBlockAlloca(currentFunction_, expr.name.lexeme, varType);
        
        // Update the variable in current scope
        varInfo->allocaInst = alloca;
        varInfo->type = newType;
        builder_->CreateStore(value, alloca);
    } else {
        builder_->CreateStore(value, varInfo->allocaInst);
    }
    
    return value;
}

llvm::Value* CodeGen::generateLogical(const LogicalExpr& expr) {
    // Short-circuit evaluation
    llvm::Value* left = generateExpr(*expr.left);
    if (!left) return nullptr;

    llvm::Function* func = builder_->GetInsertBlock()->getParent();
    llvm::Value* leftBool = toBool(left);
    
    // Store current block for PHI node
    llvm::BasicBlock* entryBB = builder_->GetInsertBlock();

    if (expr.op.type == TokenType::OR) {
        // a || b: if a is true, result is 1.0, else evaluate b
        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(*context_, "or.rhs", func);
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "or.merge", func);

        builder_->CreateCondBr(leftBool, mergeBB, rhsBB);

        // RHS evaluation
        builder_->SetInsertPoint(rhsBB);
        llvm::Value* right = generateExpr(*expr.right);
        if (!right) return nullptr;
        llvm::Value* rightBool = toBool(right);
        llvm::Value* rightResult = builder_->CreateUIToFP(rightBool, 
            llvm::Type::getDoubleTy(*context_), "ortmp");
        llvm::BasicBlock* rhsEndBB = builder_->GetInsertBlock();  // May have changed
        builder_->CreateBr(mergeBB);

        // Merge block
        builder_->SetInsertPoint(mergeBB);
        llvm::PHINode* phi = builder_->CreatePHI(llvm::Type::getDoubleTy(*context_), 2, "orphi");
        phi->addIncoming(llvm::ConstantFP::get(*context_, llvm::APFloat(1.0)), entryBB);
        phi->addIncoming(rightResult, rhsEndBB);
        return phi;
    } else {
        // a && b: if a is false, result is 0.0, else evaluate b
        llvm::BasicBlock* rhsBB = llvm::BasicBlock::Create(*context_, "and.rhs", func);
        llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "and.merge", func);

        builder_->CreateCondBr(leftBool, rhsBB, mergeBB);

        // RHS evaluation
        builder_->SetInsertPoint(rhsBB);
        llvm::Value* right = generateExpr(*expr.right);
        if (!right) return nullptr;
        llvm::Value* rightBool = toBool(right);
        llvm::Value* rightResult = builder_->CreateUIToFP(rightBool, 
            llvm::Type::getDoubleTy(*context_), "andtmp");
        llvm::BasicBlock* rhsEndBB = builder_->GetInsertBlock();  // May have changed
        builder_->CreateBr(mergeBB);

        // Merge block
        builder_->SetInsertPoint(mergeBB);
        llvm::PHINode* phi = builder_->CreatePHI(llvm::Type::getDoubleTy(*context_), 2, "andphi");
        phi->addIncoming(llvm::ConstantFP::get(*context_, llvm::APFloat(0.0)), entryBB);
        phi->addIncoming(rightResult, rhsEndBB);
        return phi;
    }
}

// Compound assignment: x += 5, x -= 3, etc.
llvm::Value* CodeGen::generateCompoundAssign(const CompoundAssignExpr& expr) {
    // Get current value
    VariableInfo* varInfo = lookupVariable(expr.name.lexeme);
    if (!varInfo) {
        error("Unknown variable in compound assignment: " + expr.name.lexeme);
        return nullptr;
    }
    
    llvm::Value* currentVal = builder_->CreateLoad(llvm::Type::getDoubleTy(*context_), 
                                                    varInfo->allocaInst, expr.name.lexeme);
    llvm::Value* rhs = generateExpr(*expr.value);
    if (!rhs) return nullptr;
    
    llvm::Value* result = nullptr;
    switch (expr.op.type) {
        case TokenType::PLUS_EQ:
            result = builder_->CreateFAdd(currentVal, rhs, "addtmp");
            break;
        case TokenType::MINUS_EQ:
            result = builder_->CreateFSub(currentVal, rhs, "subtmp");
            break;
        case TokenType::STAR_EQ:
            result = builder_->CreateFMul(currentVal, rhs, "multmp");
            break;
        case TokenType::SLASH_EQ:
            result = builder_->CreateFDiv(currentVal, rhs, "divtmp");
            break;
        case TokenType::PERCENT_EQ:
            result = builder_->CreateFRem(currentVal, rhs, "modtmp");
            break;
        default:
            error("Unknown compound assignment operator");
            return nullptr;
    }
    
    builder_->CreateStore(result, varInfo->allocaInst);
    return result;
}

// Increment/Decrement: x++, ++x, x--, --x
llvm::Value* CodeGen::generateIncrement(const IncrementExpr& expr) {
    VariableInfo* varInfo = lookupVariable(expr.name.lexeme);
    if (!varInfo) {
        error("Unknown variable in increment/decrement: " + expr.name.lexeme);
        return nullptr;
    }
    
    llvm::Value* currentVal = builder_->CreateLoad(llvm::Type::getDoubleTy(*context_), 
                                                    varInfo->allocaInst, expr.name.lexeme);
    
    llvm::Value* one = llvm::ConstantFP::get(*context_, llvm::APFloat(1.0));
    llvm::Value* newVal = nullptr;
    
    if (expr.op.type == TokenType::PLUS_PLUS) {
        newVal = builder_->CreateFAdd(currentVal, one, "inctmp");
    } else {
        newVal = builder_->CreateFSub(currentVal, one, "dectmp");
    }
    
    builder_->CreateStore(newVal, varInfo->allocaInst);
    
    // Prefix returns new value, postfix returns old value
    return expr.isPrefix ? newVal : currentVal;
}

// Interpolated string: "hello {name}, you are {age} years old"
llvm::Value* CodeGen::generateInterpString(const InterpStringExpr& expr) {
    // Build a combined format string and collect values
    std::string formatStr;
    std::vector<llvm::Value*> values;
    
    for (size_t i = 0; i < expr.stringParts.size(); i++) {
        formatStr += expr.stringParts[i];
        
        if (i < expr.exprParts.size()) {
            llvm::Value* val = generateExpr(*expr.exprParts[i]);
            if (!val) continue;
            
            // Check the type and add appropriate format specifier
            if (val->getType()->isDoubleTy()) {
                formatStr += "%g";
                values.push_back(val);
            } else if (val->getType()->isPointerTy()) {
                // Assume it's a string
                formatStr += "%s";
                values.push_back(val);
            } else {
                formatStr += "%g";
                values.push_back(val);
            }
        }
    }
    
    // Return the format string as a global string (for use in print)
    // Store the values in a way we can retrieve them for printf
    // For now, just return the format string - the print statement will handle it
    return createGlobalString(formatStr, "interp_str");
}

// Array literal: [1, 2, 3]
llvm::Value* CodeGen::generateArray(const ArrayExpr& expr) {
    size_t size = expr.elements.size();
    
    // Create array type (array of doubles for now)
    llvm::ArrayType* arrayType = llvm::ArrayType::get(
        llvm::Type::getDoubleTy(*context_), size);
    
    // Allocate array on stack
    llvm::AllocaInst* arrayAlloca = builder_->CreateAlloca(arrayType, nullptr, "array");
    
    // Initialize elements
    for (size_t i = 0; i < size; i++) {
        llvm::Value* elemVal = generateExpr(*expr.elements[i]);
        if (!elemVal) return nullptr;
        
        // Ensure it's a double
        if (!elemVal->getType()->isDoubleTy()) {
            if (elemVal->getType()->isIntegerTy()) {
                elemVal = builder_->CreateSIToFP(elemVal, llvm::Type::getDoubleTy(*context_), "todbl");
            }
        }
        
        // Get pointer to array element
        llvm::Value* indices[] = {
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0),
            llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), i)
        };
        llvm::Value* elemPtr = builder_->CreateGEP(arrayType, arrayAlloca, indices, "elemptr");
        builder_->CreateStore(elemVal, elemPtr);
    }
    
    // Return pointer to the array (cast to generic pointer)
    return arrayAlloca;
}

// Array index access: arr[0]
llvm::Value* CodeGen::generateIndex(const IndexExpr& expr) {
    // Get the array name from the identifier
    auto* identExpr = std::get_if<IdentifierExpr>(&expr.object->node);
    if (!identExpr) {
        error("Array index access requires an identifier");
        return nullptr;
    }
    
    VariableInfo* varInfo = lookupVariable(identExpr->name.lexeme);
    if (!varInfo || varInfo->type != "array") {
        error("Variable is not an array: " + identExpr->name.lexeme);
        return nullptr;
    }
    
    llvm::Value* indexVal = generateExpr(*expr.index);
    if (!indexVal) return nullptr;
    
    // Convert index to integer
    llvm::Value* indexInt = builder_->CreateFPToSI(indexVal, 
        llvm::Type::getInt64Ty(*context_), "idx");
    
    // Create the array type using stored size
    llvm::ArrayType* arrayType = llvm::ArrayType::get(
        llvm::Type::getDoubleTy(*context_), varInfo->arraySize);
    
    // Get element pointer
    llvm::Value* indices[] = {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0),
        indexInt
    };
    
    llvm::Value* elemPtr = builder_->CreateGEP(arrayType, varInfo->allocaInst, indices, "elemptr");
    
    // Load and return the element
    return builder_->CreateLoad(llvm::Type::getDoubleTy(*context_), elemPtr, "elem");
}

// Array index assignment: arr[0] = value
llvm::Value* CodeGen::generateIndexAssign(const IndexAssignExpr& expr) {
    // Get the array name from the identifier
    auto* identExpr = std::get_if<IdentifierExpr>(&expr.object->node);
    if (!identExpr) {
        error("Array index assignment requires an identifier");
        return nullptr;
    }
    
    VariableInfo* varInfo = lookupVariable(identExpr->name.lexeme);
    if (!varInfo || varInfo->type != "array") {
        error("Variable is not an array: " + identExpr->name.lexeme);
        return nullptr;
    }
    
    llvm::Value* indexVal = generateExpr(*expr.index);
    if (!indexVal) return nullptr;
    
    llvm::Value* value = generateExpr(*expr.value);
    if (!value) return nullptr;
    
    // Convert index to integer
    llvm::Value* indexInt = builder_->CreateFPToSI(indexVal, 
        llvm::Type::getInt64Ty(*context_), "idx");
    
    // Create the array type using stored size
    llvm::ArrayType* arrayType = llvm::ArrayType::get(
        llvm::Type::getDoubleTy(*context_), varInfo->arraySize);
    
    // Get element pointer
    llvm::Value* indices[] = {
        llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0),
        indexInt
    };
    
    llvm::Value* elemPtr = builder_->CreateGEP(arrayType, varInfo->allocaInst, indices, "elemptr");
    
    // Store the value
    builder_->CreateStore(value, elemPtr);
    
    return value;
}

// Switch statement
void CodeGen::generateSwitch(const SwitchStmt& stmt) {
    llvm::Value* switchVal = generateExpr(*stmt.expression);
    if (!switchVal) return;
    
    llvm::Function* func = builder_->GetInsertBlock()->getParent();
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*context_, "switch.end");
    llvm::BasicBlock* defaultBB = nullptr;
    
    // Find default case
    for (const auto& c : stmt.cases) {
        if (c.isDefault) {
            defaultBB = llvm::BasicBlock::Create(*context_, "switch.default", func);
            break;
        }
    }
    
    if (!defaultBB) {
        defaultBB = mergeBB;
    }
    
    // Generate case blocks
    std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>> caseBlocks;
    for (const auto& c : stmt.cases) {
        if (!c.isDefault) {
            llvm::BasicBlock* caseBB = llvm::BasicBlock::Create(*context_, "switch.case", func);
            llvm::Value* caseVal = generateExpr(*c.value);
            caseBlocks.push_back({caseVal, caseBB});
        }
    }
    
    // Create cascading comparisons (since we use floating point, can't use LLVM switch)
    for (size_t i = 0; i < caseBlocks.size(); i++) {
        llvm::Value* cmp = createFCmp(llvm::CmpInst::FCMP_OEQ, switchVal, caseBlocks[i].first, "cmptmp");
        
        llvm::BasicBlock* nextBB = (i + 1 < caseBlocks.size()) 
            ? llvm::BasicBlock::Create(*context_, "switch.next", func) 
            : defaultBB;
        
        builder_->CreateCondBr(cmp, caseBlocks[i].second, nextBB);
        builder_->SetInsertPoint(nextBB);
    }
    
    // If no cases, jump to default
    if (caseBlocks.empty()) {
        builder_->CreateBr(defaultBB);
    }
    
    // Generate case bodies
    size_t caseIdx = 0;
    for (const auto& c : stmt.cases) {
        llvm::BasicBlock* caseBB;
        if (c.isDefault) {
            caseBB = defaultBB;
        } else {
            caseBB = caseBlocks[caseIdx++].second;
        }
        
        builder_->SetInsertPoint(caseBB);
        for (const auto& s : c.body) {
            generateStmt(*s);
        }
        
        if (!builder_->GetInsertBlock()->getTerminator()) {
            builder_->CreateBr(mergeBB);
        }
    }
    
    // Continue after switch
    func->insert(func->end(), mergeBB);
    builder_->SetInsertPoint(mergeBB);
}

// Try-catch statement
// NOTE: Full exception handling requires LLVM's invoke/landingpad which needs
// personality functions and proper exception runtime support.
// This simplified version executes try block and skips catch on success.
void CodeGen::generateTryCatch(const TryCatchStmt& stmt) {
    // For now, we implement a basic version that:
    // 1. Executes the try block
    // 2. The catch block is generated but only reachable if we add explicit error checking
    
    llvm::Function* func = builder_->GetInsertBlock()->getParent();
    
    llvm::BasicBlock* tryBB = llvm::BasicBlock::Create(*context_, "try", func);
    llvm::BasicBlock* catchBB = llvm::BasicBlock::Create(*context_, "catch", func);
    llvm::BasicBlock* endBB = llvm::BasicBlock::Create(*context_, "tryend", func);
    
    // Jump to try block
    builder_->CreateBr(tryBB);
    
    // Try block
    builder_->SetInsertPoint(tryBB);
    generateStmt(*stmt.tryBlock);
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(endBB);  // Success - skip catch
    }
    
    // Catch block (currently unreachable without proper exception support)
    builder_->SetInsertPoint(catchBB);
    generateStmt(*stmt.catchBlock);
    if (!builder_->GetInsertBlock()->getTerminator()) {
        builder_->CreateBr(endBB);
    }
    
    // Continue after try-catch
    builder_->SetInsertPoint(endBB);
    
    // TODO: Implement proper LLVM exception handling with invoke/landingpad
    // This requires linking with a C++ exception runtime
}

// ============================================================================
// Scope Management
// ============================================================================

void CodeGen::pushScope() {
    scopeStack_.push_back({});
    currentScopeDepth_++;
}

void CodeGen::popScope() {
    if (!scopeStack_.empty()) {
        scopeStack_.pop_back();
        currentScopeDepth_--;
    }
}

VariableInfo* CodeGen::lookupVariable(const std::string& name) {
    // Search from innermost to outermost scope
    for (auto it = scopeStack_.rbegin(); it != scopeStack_.rend(); ++it) {
        auto varIt = it->find(name);
        if (varIt != it->end()) {
            return &varIt->second;
        }
    }
    return nullptr;
}

void CodeGen::declareVariable(const std::string& name, llvm::AllocaInst* alloca, const std::string& type) {
    if (!scopeStack_.empty()) {
        scopeStack_.back()[name] = VariableInfo(alloca, type, currentScopeDepth_);
    }
}

// ============================================================================
// Helper Functions
// ============================================================================

llvm::AllocaInst* CodeGen::createEntryBlockAlloca(llvm::Function* func, 
                                                   const std::string& varName,
                                                   llvm::Type* type) {
    llvm::IRBuilder<> tmpBuilder(&func->getEntryBlock(), 
                                  func->getEntryBlock().begin());
    if (!type) {
        type = llvm::Type::getDoubleTy(*context_);
    }
    return tmpBuilder.CreateAlloca(type, nullptr, varName);
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
    // Check if we already created this exact string (deduplication)
    auto it = stringLiterals_.find(str);
    if (it != stringLiterals_.end()) {
        return it->second;
    }
    
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
    llvm::Value* strPtr = llvm::ConstantExpr::getGetElementPtr(
        strConstant->getType(), gv, 
        llvm::ArrayRef<llvm::Constant*>{
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context_), 0),
            llvm::ConstantInt::get(llvm::Type::getInt32Ty(*context_), 0)
        }
    );
    
    // Cache for future use
    stringLiterals_[str] = strPtr;
    return strPtr;
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
