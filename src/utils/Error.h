#ifndef SIGMA_ERROR_H
#define SIGMA_ERROR_H

#include <string>
#include <iostream>
#include <vector>
#include <sstream>

namespace sigma {

// ============================================================================
// Error Types
// ============================================================================

enum class ErrorType {
    LEXER_ERROR,
    PARSER_ERROR,
    SEMANTIC_ERROR,
    RUNTIME_ERROR
};

inline std::string errorTypeToString(ErrorType type) {
    switch (type) {
        case ErrorType::LEXER_ERROR: return "Lexer Error";
        case ErrorType::PARSER_ERROR: return "Syntax Error";
        case ErrorType::SEMANTIC_ERROR: return "Semantic Error";
        case ErrorType::RUNTIME_ERROR: return "Runtime Error";
        default: return "Error";
    }
}

// ============================================================================
// Source Location
// ============================================================================

struct SourceLocation {
    int line = 1;
    int column = 1;
    std::string filename = "<stdin>";

    std::string toString() const {
        std::ostringstream ss;
        ss << filename << ":" << line << ":" << column;
        return ss.str();
    }
};

// ============================================================================
// Error Structure
// ============================================================================

struct Error {
    ErrorType type;
    std::string message;
    SourceLocation location;
    std::string hint;  // Optional hint for fixing the error

    Error(ErrorType type, std::string message, SourceLocation location, std::string hint = "")
        : type(type), message(std::move(message)), location(std::move(location)), hint(std::move(hint)) {}

    std::string format() const {
        std::ostringstream ss;
        
        // Main error message
        ss << "\033[1;31m" << errorTypeToString(type) << "\033[0m";  // Red bold
        ss << " [Line " << location.line << "]: ";
        ss << message;
        
        // Optional hint
        if (!hint.empty()) {
            ss << "\n  \033[1;36mHint:\033[0m " << hint;  // Cyan
        }
        
        return ss.str();
    }

    std::string formatPlain() const {
        std::ostringstream ss;
        ss << "[" << errorTypeToString(type) << "] ";
        ss << "[Line " << location.line << "]: ";
        ss << message;
        if (!hint.empty()) {
            ss << " (Hint: " << hint << ")";
        }
        return ss.str();
    }
};

// ============================================================================
// Error Reporter - Collects and reports errors
// ============================================================================

class ErrorReporter {
public:
    // Report a simple error (legacy interface)
    static void report(int line, const std::string& where, const std::string& message) {
        std::cerr << "[Line " << line << "] Error" << where << ": " << message << std::endl;
        hadError_ = true;
    }

    static void error(int line, const std::string& message) {
        report(line, "", message);
    }

    // ========================================================================
    // New error reporting interface
    // ========================================================================

    // Add a lexer error
    static void lexerError(int line, const std::string& message, const std::string& hint = "") {
        SourceLocation loc{line, 1, currentFile_};
        errors_.emplace_back(ErrorType::LEXER_ERROR, message, loc, hint);
        hadError_ = true;
    }

    // Add a parser/syntax error
    static void parserError(int line, const std::string& token, const std::string& message, const std::string& hint = "") {
        SourceLocation loc{line, 1, currentFile_};
        std::string fullMessage = message;
        if (!token.empty()) {
            fullMessage = "at '" + token + "': " + message;
        }
        errors_.emplace_back(ErrorType::PARSER_ERROR, fullMessage, loc, hint);
        hadError_ = true;
    }

    // Add a semantic error
    static void semanticError(int line, const std::string& message, const std::string& hint = "") {
        SourceLocation loc{line, 1, currentFile_};
        errors_.emplace_back(ErrorType::SEMANTIC_ERROR, message, loc, hint);
        hadError_ = true;
    }

    // Add a runtime error
    static void runtimeError(const std::string& message) {
        SourceLocation loc{0, 0, "<runtime>"};
        errors_.emplace_back(ErrorType::RUNTIME_ERROR, message, loc, "");
        hadRuntimeError_ = true;
    }

    // ========================================================================
    // Error access and display
    // ========================================================================

    static void printErrors(bool useColor = true) {
        for (const auto& error : errors_) {
            if (useColor) {
                std::cerr << error.format() << std::endl;
            } else {
                std::cerr << error.formatPlain() << std::endl;
            }
        }
    }

    static const std::vector<Error>& getErrors() {
        return errors_;
    }

    static size_t errorCount() {
        return errors_.size();
    }

    // ========================================================================
    // State management
    // ========================================================================

    static void reset() {
        errors_.clear();
        hadError_ = false;
        hadRuntimeError_ = false;
    }

    static bool hadError() {
        return hadError_;
    }

    static bool hadRuntimeError() {
        return hadRuntimeError_;
    }

    static void setCurrentFile(const std::string& filename) {
        currentFile_ = filename;
    }

private:
    static inline std::vector<Error> errors_;
    static inline bool hadError_ = false;
    static inline bool hadRuntimeError_ = false;
    static inline std::string currentFile_ = "<stdin>";
};

// ============================================================================
// Common Error Messages (for consistency)
// ============================================================================

namespace ErrorMessages {
    // Lexer errors
    inline const char* UNEXPECTED_CHARACTER = "Unexpected character";
    inline const char* UNTERMINATED_STRING = "Unterminated string literal";
    inline const char* INVALID_NUMBER = "Invalid number format";

    // Parser errors
    inline const char* EXPECTED_EXPRESSION = "Expected expression";
    inline const char* EXPECTED_IDENTIFIER = "Expected identifier";
    inline const char* EXPECTED_SEMICOLON = "Expected ';' after statement";
    inline const char* EXPECTED_LPAREN = "Expected '(' after";
    inline const char* EXPECTED_RPAREN = "Expected ')' after expression";
    inline const char* EXPECTED_LBRACE = "Expected '{' before block";
    inline const char* EXPECTED_RBRACE = "Expected '}' after block";
    inline const char* INVALID_ASSIGNMENT = "Invalid assignment target";
    inline const char* TOO_MANY_ARGUMENTS = "Cannot have more than 255 arguments";
    inline const char* TOO_MANY_PARAMETERS = "Cannot have more than 255 parameters";

    // Semantic errors
    inline const char* UNDEFINED_VARIABLE = "Undefined variable";
    inline const char* UNDEFINED_FUNCTION = "Undefined function";
    inline const char* ALREADY_DEFINED = "Variable already defined in this scope";
    inline const char* WRONG_ARGUMENT_COUNT = "Wrong number of arguments";
    inline const char* RETURN_OUTSIDE_FUNCTION = "'send' used outside of function";
    inline const char* BREAK_OUTSIDE_LOOP = "'mog' used outside of loop";
    inline const char* CONTINUE_OUTSIDE_LOOP = "'skip' used outside of loop";
}

} // namespace sigma

#endif // SIGMA_ERROR_H
