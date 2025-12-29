#ifndef SIGMA_AST_PRINTER_H
#define SIGMA_AST_PRINTER_H

#include <string>
#include <sstream>
#include "AST.h"

namespace sigma {

// AST Pretty Printer - prints a human-readable representation of the AST
class ASTPrinter {
public:
    // Print a program (list of statements)
    std::string print(const Program& program) {
        std::ostringstream ss;
        ss << "=== AST ===" << std::endl;
        for (const auto& stmt : program) {
            ss << printStmt(*stmt, 0) << std::endl;
        }
        ss << "===========" << std::endl;
        return ss.str();
    }

private:
    // Indentation helper
    std::string indent(int level) const {
        return std::string(level * 2, ' ');
    }

    // Print a statement
    std::string printStmt(const Stmt& stmt, int level) {
        return std::visit([this, level](const auto& s) { 
            return printStmtNode(s, level); 
        }, stmt.node);
    }

    // Print an expression
    std::string printExpr(const Expr& expr) {
        return std::visit([this](const auto& e) { 
            return printExprNode(e); 
        }, expr.node);
    }

    // ========================================================================
    // Statement Printers
    // ========================================================================

    std::string printStmtNode(const VarDeclStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(fr " << s.name.lexeme << " = ";
        ss << printExpr(*s.initializer) << ")";
        return ss.str();
    }

    std::string printStmtNode(const PrintStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(say " << printExpr(*s.expression) << ")";
        return ss.str();
    }

    std::string printStmtNode(const ExprStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(expr " << printExpr(*s.expression) << ")";
        return ss.str();
    }

    std::string printStmtNode(const BlockStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(block";
        for (const auto& stmt : s.statements) {
            ss << std::endl << printStmt(*stmt, level + 1);
        }
        ss << ")";
        return ss.str();
    }

    std::string printStmtNode(const IfStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(lowkey " << printExpr(*s.condition);
        ss << std::endl << printStmt(*s.thenBranch, level + 1);
        if (s.elseBranch) {
            ss << std::endl << indent(level) << " highkey";
            ss << std::endl << printStmt(*s.elseBranch, level + 1);
        }
        ss << ")";
        return ss.str();
    }

    std::string printStmtNode(const WhileStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(goon " << printExpr(*s.condition);
        ss << std::endl << printStmt(*s.body, level + 1);
        ss << ")";
        return ss.str();
    }

    std::string printStmtNode(const ForStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(edge";
        if (s.initializer) {
            ss << " init:" << printStmt(*s.initializer, 0);
        }
        if (s.condition) {
            ss << " cond:" << printExpr(*s.condition);
        }
        if (s.increment) {
            ss << " incr:" << printExpr(*s.increment);
        }
        ss << std::endl << printStmt(*s.body, level + 1);
        ss << ")";
        return ss.str();
    }

    std::string printStmtNode(const FuncDefStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(vibe " << s.name.lexeme << "(";
        for (size_t i = 0; i < s.params.size(); i++) {
            if (i > 0) ss << ", ";
            ss << s.params[i].lexeme;
        }
        ss << ")";
        for (const auto& stmt : s.body) {
            ss << std::endl << printStmt(*stmt, level + 1);
        }
        ss << ")";
        return ss.str();
    }

    std::string printStmtNode(const ReturnStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(send";
        if (s.value) {
            ss << " " << printExpr(*s.value);
        }
        ss << ")";
        return ss.str();
    }

    std::string printStmtNode(const BreakStmt&, int level) {
        return indent(level) + "(mog)";
    }

    std::string printStmtNode(const ContinueStmt&, int level) {
        return indent(level) + "(skip)";
    }

    std::string printStmtNode(const SwitchStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(simp " << printExpr(*s.expression);
        for (const auto& c : s.cases) {
            ss << std::endl << indent(level + 1) << "(stan ";
            if (c.value) {
                ss << printExpr(*c.value);
            } else {
                ss << "ghost";
            }
            for (const auto& stmt : c.body) {
                ss << std::endl << printStmt(*stmt, level + 2);
            }
            ss << ")";
        }
        ss << ")";
        return ss.str();
    }

    std::string printStmtNode(const TryCatchStmt& s, int level) {
        std::ostringstream ss;
        ss << indent(level) << "(yeet";
        ss << std::endl << printStmt(*s.tryBlock, level + 1);
        ss << std::endl << indent(level) << " caught";
        ss << std::endl << printStmt(*s.catchBlock, level + 1);
        ss << ")";
        return ss.str();
    }

    // ========================================================================
    // Expression Printers
    // ========================================================================

    std::string printExprNode(const LiteralExpr& e) {
        return std::visit([](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return "nah";
            } else if constexpr (std::is_same_v<T, int64_t>) {
                return std::to_string(v);
            } else if constexpr (std::is_same_v<T, double>) {
                std::ostringstream ss;
                ss << v;
                return ss.str();
            } else if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + v + "\"";
            } else if constexpr (std::is_same_v<T, bool>) {
                return v ? "ongod" : "cap";
            }
            return "?";
        }, e.value);
    }

    std::string printExprNode(const IdentifierExpr& e) {
        return e.name.lexeme;
    }

    std::string printExprNode(const BinaryExpr& e) {
        std::ostringstream ss;
        ss << "(" << e.op.lexeme << " ";
        ss << printExpr(*e.left) << " ";
        ss << printExpr(*e.right) << ")";
        return ss.str();
    }

    std::string printExprNode(const UnaryExpr& e) {
        std::ostringstream ss;
        ss << "(" << e.op.lexeme << " ";
        ss << printExpr(*e.operand) << ")";
        return ss.str();
    }

    std::string printExprNode(const CallExpr& e) {
        std::ostringstream ss;
        ss << "(call " << printExpr(*e.callee);
        for (const auto& arg : e.arguments) {
            ss << " " << printExpr(*arg);
        }
        ss << ")";
        return ss.str();
    }

    std::string printExprNode(const GroupingExpr& e) {
        std::ostringstream ss;
        ss << "(group " << printExpr(*e.expression) << ")";
        return ss.str();
    }

    std::string printExprNode(const AssignExpr& e) {
        std::ostringstream ss;
        ss << "(= " << e.name.lexeme << " " << printExpr(*e.value) << ")";
        return ss.str();
    }

    std::string printExprNode(const LogicalExpr& e) {
        std::ostringstream ss;
        ss << "(" << e.op.lexeme << " ";
        ss << printExpr(*e.left) << " ";
        ss << printExpr(*e.right) << ")";
        return ss.str();
    }

    std::string printExprNode(const CompoundAssignExpr& e) {
        std::ostringstream ss;
        ss << "(" << e.op.lexeme << " " << e.name.lexeme << " ";
        ss << printExpr(*e.value) << ")";
        return ss.str();
    }

    std::string printExprNode(const IncrementExpr& e) {
        std::ostringstream ss;
        if (e.isPrefix) {
            ss << "(" << e.op.lexeme << " " << e.name.lexeme << ")";
        } else {
            ss << "(" << e.name.lexeme << " " << e.op.lexeme << ")";
        }
        return ss.str();
    }

    std::string printExprNode(const InterpStringExpr& e) {
        std::ostringstream ss;
        ss << "(interp-string";
        for (size_t i = 0; i < e.stringParts.size(); i++) {
            ss << " \"" << e.stringParts[i] << "\"";
            if (i < e.exprParts.size()) {
                ss << " {" << printExpr(*e.exprParts[i]) << "}";
            }
        }
        ss << ")";
        return ss.str();
    }

    std::string printExprNode(const ArrayExpr& e) {
        std::ostringstream ss;
        ss << "[";
        for (size_t i = 0; i < e.elements.size(); i++) {
            if (i > 0) ss << ", ";
            ss << printExpr(*e.elements[i]);
        }
        ss << "]";
        return ss.str();
    }

    std::string printExprNode(const IndexExpr& e) {
        std::ostringstream ss;
        ss << "(index " << printExpr(*e.object) << " " << printExpr(*e.index) << ")";
        return ss.str();
    }

    std::string printExprNode(const IndexAssignExpr& e) {
        std::ostringstream ss;
        ss << "(index-assign " << printExpr(*e.object) << " " << printExpr(*e.index);
        ss << " " << printExpr(*e.value) << ")";
        return ss.str();
    }
};

} // namespace sigma

#endif // SIGMA_AST_PRINTER_H
