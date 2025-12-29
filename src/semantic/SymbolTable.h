#ifndef SIGMA_SYMBOL_TABLE_H
#define SIGMA_SYMBOL_TABLE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include "Types.h"

namespace sigma {

// ============================================================================
// Symbol Information
// ============================================================================

struct Symbol {
    std::string name;
    Type type;
    int scopeDepth;
    int line;           // Line where declared
    bool isConst;       // For future const support
    bool isInitialized; // Whether the variable has been assigned
    
    // Default constructor needed for unordered_map
    Symbol() : scopeDepth(0), line(0), isConst(false), isInitialized(false) {}
    
    Symbol(const std::string& n, Type t, int depth, int ln, bool constant = false)
        : name(n), type(t), scopeDepth(depth), line(ln), 
          isConst(constant), isInitialized(true) {}
};

// ============================================================================
// Symbol Table with Scope Management
// ============================================================================

class SymbolTable {
public:
    SymbolTable() {
        // Start with global scope
        pushScope();
    }
    
    // Scope management
    void pushScope() {
        scopes_.emplace_back();
        currentDepth_++;
    }
    
    void popScope() {
        if (!scopes_.empty()) {
            scopes_.pop_back();
            currentDepth_--;
        }
    }
    
    int currentDepth() const { return currentDepth_; }
    
    // Symbol operations
    bool declare(const std::string& name, Type type, int line, bool isConst = false) {
        // Check if already declared in current scope
        if (!scopes_.empty()) {
            auto& currentScope = scopes_.back();
            if (currentScope.find(name) != currentScope.end()) {
                return false; // Already declared in this scope
            }
            currentScope[name] = Symbol(name, type, currentDepth_, line, isConst);
        }
        return true;
    }
    
    // Look up a symbol (searches from innermost to outermost scope)
    Symbol* lookup(const std::string& name) {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            auto found = it->find(name);
            if (found != it->end()) {
                return &found->second;
            }
        }
        return nullptr;
    }
    
    // Look up only in current scope
    Symbol* lookupLocal(const std::string& name) {
        if (!scopes_.empty()) {
            auto& currentScope = scopes_.back();
            auto found = currentScope.find(name);
            if (found != currentScope.end()) {
                return &found->second;
            }
        }
        return nullptr;
    }
    
    // Check if a symbol exists
    bool exists(const std::string& name) const {
        for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
            if (it->find(name) != it->end()) {
                return true;
            }
        }
        return false;
    }
    
    // Update symbol type (for type inference)
    bool updateType(const std::string& name, Type newType) {
        Symbol* sym = lookup(name);
        if (sym) {
            sym->type = newType;
            return true;
        }
        return false;
    }

private:
    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    int currentDepth_ = 0;
};

// ============================================================================
// Function Table
// ============================================================================

struct FunctionInfo {
    std::string name;
    Type type;  // Function type with params and return
    std::vector<std::string> paramNames;
    int line;
    bool isDefined;
    
    // Default constructor needed for unordered_map
    FunctionInfo() : line(0), isDefined(false) {}
    
    FunctionInfo(const std::string& n, Type t, std::vector<std::string> params, int ln)
        : name(n), type(t), paramNames(std::move(params)), line(ln), isDefined(true) {}
};

class FunctionTable {
public:
    bool declare(const std::string& name, Type type, 
                 std::vector<std::string> paramNames, int line) {
        if (functions_.find(name) != functions_.end()) {
            return false; // Already declared
        }
        functions_[name] = FunctionInfo(name, type, std::move(paramNames), line);
        return true;
    }
    
    FunctionInfo* lookup(const std::string& name) {
        auto it = functions_.find(name);
        if (it != functions_.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    bool exists(const std::string& name) const {
        return functions_.find(name) != functions_.end();
    }

private:
    std::unordered_map<std::string, FunctionInfo> functions_;
};

} // namespace sigma

#endif // SIGMA_SYMBOL_TABLE_H
