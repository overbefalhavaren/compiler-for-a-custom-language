#include "include/AST/Stmt.hpp"

#include "llvm/Support/ErrorHandling.h"

namespace c {
namespace ast {

llvm::StringRef Stmt::getStmtKindName() const {
    switch (StmtKind) {
        default:
            llvm_unreachable("Stmt::Kind missing case.");

        case IfStmtKind:
            return "IfStmt";
        case WhileStmtKind:
            return "WhileStmt";

        case BlockStmtKind:
            return "BlockStmt";
        
        case MatchStmtKind:
            return "MatchStmt";
        
        case ReturnStmtKind:
            return "ReturnStmt";
        case DeclStmtKind:
            return "DeclStmt";

        case AccessExprKind:
            return "AccessExpr";
        case SliceExprKind:
            return "SliceExpr";

        case BinaryOperatorKind:
            return "BinaryOperator";
        case UnaryOperatorKind:
            return "UnaryOperator";

        case CallExprKind:
            return "CallExpr";
        case DeclRefExprKind:
            return "DeclRefExpr";

        case ArrayLiteralKind:
            return "ArrayLiteral";
        case IntegerLiteralKind:
            return "IntegerLiteral";
        case FloatingLiteralKind:
            return "FloatingLiteral";
        case BooleanLiteralKind:
            return "BooleanLiteral";
    }
}

} // namespace ast
} // namespace c
