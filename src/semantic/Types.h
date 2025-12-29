#ifndef SIGMA_TYPES_H
#define SIGMA_TYPES_H

#include <string>
#include <vector>
#include <memory>
#include <variant>

namespace sigma {

// ============================================================================
// Type System for Sigma Language
// ============================================================================

// Forward declarations
struct FunctionType;

// Basic type kinds
enum class TypeKind {
    Number,     // double (default numeric type)
    Integer,    // int64 (for bitwise operations)
    String,     // string
    Boolean,    // bool
    Null,       // nah/null
    Function,   // function type
    Any,        // unknown/dynamic type
    Void,       // no return value
    Error       // type error placeholder
};

// Convert TypeKind to string for error messages
inline std::string typeKindToString(TypeKind kind) {
    switch (kind) {
        case TypeKind::Number:   return "Number";
        case TypeKind::Integer:  return "Integer";
        case TypeKind::String:   return "String";
        case TypeKind::Boolean:  return "Boolean";
        case TypeKind::Null:     return "Null";
        case TypeKind::Function: return "Function";
        case TypeKind::Any:      return "Any";
        case TypeKind::Void:     return "Void";
        case TypeKind::Error:    return "Error";
        default:                 return "Unknown";
    }
}

// Type representation
struct Type {
    TypeKind kind;
    
    // For function types: parameter types and return type
    std::vector<TypeKind> paramTypes;
    TypeKind returnType = TypeKind::Void;
    
    // Constructors
    explicit Type(TypeKind k = TypeKind::Any) : kind(k) {}
    
    Type(TypeKind ret, std::vector<TypeKind> params) 
        : kind(TypeKind::Function), paramTypes(std::move(params)), returnType(ret) {}
    
    // Comparison
    bool operator==(const Type& other) const {
        if (kind != other.kind) return false;
        if (kind == TypeKind::Function) {
            return returnType == other.returnType && paramTypes == other.paramTypes;
        }
        return true;
    }
    
    bool operator!=(const Type& other) const {
        return !(*this == other);
    }
    
    // Check if this type is compatible with another
    bool isCompatibleWith(const Type& other) const {
        // Any is compatible with everything
        if (kind == TypeKind::Any || other.kind == TypeKind::Any) return true;
        // Error is compatible with everything (to prevent cascading errors)
        if (kind == TypeKind::Error || other.kind == TypeKind::Error) return true;
        // Null is compatible with any reference type (for now, just check equality)
        if (kind == TypeKind::Null || other.kind == TypeKind::Null) return true;
        // Number and Integer are interchangeable
        if ((kind == TypeKind::Number || kind == TypeKind::Integer) &&
            (other.kind == TypeKind::Number || other.kind == TypeKind::Integer)) {
            return true;
        }
        return *this == other;
    }
    
    // Check if type is numeric
    bool isNumeric() const {
        return kind == TypeKind::Number || kind == TypeKind::Integer;
    }
    
    // Convert to string for error messages
    std::string toString() const {
        if (kind == TypeKind::Function) {
            std::string result = "Function(";
            for (size_t i = 0; i < paramTypes.size(); i++) {
                if (i > 0) result += ", ";
                result += typeKindToString(paramTypes[i]);
            }
            result += ") -> " + typeKindToString(returnType);
            return result;
        }
        return typeKindToString(kind);
    }
};

// Predefined type constants for convenience
namespace Types {
    inline Type Number() { return Type(TypeKind::Number); }
    inline Type Integer() { return Type(TypeKind::Integer); }
    inline Type String() { return Type(TypeKind::String); }
    inline Type Boolean() { return Type(TypeKind::Boolean); }
    inline Type Null() { return Type(TypeKind::Null); }
    inline Type Any() { return Type(TypeKind::Any); }
    inline Type Void() { return Type(TypeKind::Void); }
    inline Type Error() { return Type(TypeKind::Error); }
    
    inline Type Function(TypeKind returnType, std::vector<TypeKind> params = {}) {
        return Type(returnType, std::move(params));
    }
}

} // namespace sigma

#endif // SIGMA_TYPES_H
