#include "include/AST/Expr.hpp"

#include "include/AST/Decl.hpp"
#include "include/AST/DeclBase.hpp"

namespace c {
namespace ast {

bool Expr::isTypeDependant() const {
    // Currently we don't have any Expr that's type dependant
    return false; 
}

bool Expr::isStorageLocation() const {
    // NOTE: This switch statement only includes expression kinds that 
    // are implemented and have classes implemented for them.
    switch (getKind()) {
        default:
            llvm_unreachable("Expr subclass has no case implemented.");

        case UnaryOperatorKind:
            if (llvm::cast<UnaryOperator>(this)->isDerefOp())
                return true;
            return false;
        case BinaryOperatorKind:
            // Expressions like "a = b = c" are allowed
            if (llvm::cast<BinaryOperator>(this)->isAssignOp()) // FIXME: Might be wrong
                return true;
            return false;

        // TODO: When we implement things like function pointers, namespaces, 
        // templates and etc, DeclRefExpr will be able to point to almost any
        // kind of NamedDecl. Logic for this will need to be implemented.
        case DeclRefExprKind:
            // Conditional assertion to try save future headache.
            if (auto DC = llvm::cast<DeclRefExpr>(this)->getDecl())
                // Only VarDecl and ParamDecl have logic implemented for them.
                assert(llvm::isa<InitDecl>(DC));
            return true;

        // TODO: When we add functions to structs in the future AccessExpr 
        // might also point to a function. This will need its own logic 
        // since a function can't be assigned to.
        case AccessExprKind:
            // No assertion here since the class methods will probably have
            // to change when logic for method access is implemented.
            return true;

        case SliceExprKind:
            // Both normal array access and the slice syntax return an
            // assignable memory location.
            return true;
        
        case CallExprKind:
        case ArrayLiteralKind:
        case FloatingLiteralKind:
        case IntegerLiteralKind:
        case BooleanLiteralKind:
            return false;
    }
}

bool Expr::isModifiableValue() const {
    if (!isStorageLocation())
        return false;
    if (llvm::isa<DeclRefExpr>(this)) {
        const Decl* DC = llvm::cast<DeclRefExpr>(this)->getDecl();
        if (auto VD = llvm::dyn_cast<VarDecl>(DC))
            return VD->isMutable();
        if (auto PD = llvm::dyn_cast<ParamDecl>(DC))
            return PD->isMutable();
    } else if (llvm::isa<QualExpr>(this)) {
        return llvm::cast<QualExpr>(this)->isModifiableValue();
    }

    return true;
}

bool Expr::isLiteralExpr() const {
    return getKind() == ArrayLiteralKind ||
           getKind() == StringLiteralKind ||
           getKind() == CharLiteralKind ||
           getKind() == IntegerLiteralKind ||
           getKind() == FloatingLiteralKind ||
           getKind() == BooleanLiteralKind;
}

} // namespace c
} // namespace ast
