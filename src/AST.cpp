#include "include/AST/DeclBase.hpp"
#include "include/AST/Expr.hpp"
#include "include/AST/Stmt.hpp"

#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Decl.hpp"

#include "src/Debug.hpp"

namespace c {
namespace ast {

llvm::ArrayRef<NamedDecl*>& Container::LookupResult::getLookup() {
    if (HasCachedLookup)
        return Lookup;

    auto name_lookup = LookupContainer.LookupMap.find(Name);
    if (name_lookup == LookupContainer.LookupMap.end())
        return Lookup; // Null value, since just doing "{}" gives a compiler error

    HasCachedLookup = true;
    Lookup = name_lookup->second;
    return Lookup;
}

template <typename T>
T* Container::LookupResult::get() {
    static_assert(std::is_base_of_v<Decl, T> && "T must be subclass of Decl.");
    static_assert(std::is_base_of_v<NamedDecl, T> && "T must be a subclass of NamedDecl.");
    
    for (NamedDecl* DC : getLookup())
        if (isa<T>(DC))
            return llvm::cast<T>(DC);
    return nullptr;
}

NamedDecl* Container::LookupResult::getKind(Decl::Kind kind) {
    for (NamedDecl* DC : getLookup())
        if (DC->getKind() == kind)
            return DC;
    return nullptr;
}

void Container::constructLookupMap() {
    for (Decl* DC : decls()) 
        if (llvm::isa<NamedDecl>(DC))
            pushDeclToLookupMap(llvm::cast<NamedDecl>(DC));
}

void Container::pushDeclToLookupMap(NamedDecl* DC) {
    auto result = LookupMap.find(DC->getName());
    if (result != LookupMap.end())
        (void)result->second.push_back(DC);

    (void)LookupMap.insert({DC->getName(), {DC}});
}

bool Expr::isTypeDependant() const {
    // Currently we don't have any Expr that's type dependant
    return false; 
}

bool Expr::isPlace() const {
    // NOTE: This switch statement only includes expression kinds that 
    // are implemented and have classes implemented for them.
    switch (getKind()) {
        default:
            llvm_unreachable("Expr subclass has no case implemented.");

        case UnaryOperatorKind:
            if (llvm::cast<UnaryOperator>(this)->isDerefOp())
                return true;
            return false;

        case CallExprKind:
            if (llvm::isa<StructDecl>(llvm::cast<CallExpr>(this)->getCalleeDecl()))
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
        
        case BinaryOperatorKind:
        case ArrayLiteralKind:
        case FloatingLiteralKind:
        case IntegerLiteralKind:
        case BooleanLiteralKind:
            return false;
    }
}

bool Expr::isMutablePlace() const {
    if (!isPlace())
        return false;
    if (llvm::isa<DeclRefExpr>(this)) {
        const Decl* DC = llvm::cast<DeclRefExpr>(this)->getDecl();
        if (auto VD = llvm::dyn_cast<VarDecl>(DC))
            return VD->isMutable();
        else if (auto PD = llvm::dyn_cast<ParamDecl>(DC))
            return PD->isMutable();
    } else if (auto qual = llvm::dyn_cast<QualExpr>(this))
        return qual->getBase()->isMutablePlace();
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

llvm::StringRef Decl::getDeclKindName() const {
    switch (DeclKind) {
        default:
            llvm_unreachable("Decl::Kind missing case.");
        
        case StructDeclKind:
            return "StructDecl";
        case TypeAliasDeclKind:
            return "TypeAliasDecl";
        
        case VarDeclKind:
            return "VarDecl";
        case ParamDeclKind:
            return "ParamDecl";
        case FieldDeclKind:
            return "FieldDecl";

        case FunctionDeclKind:
            return "FunctionDecl";

        case ModuleDeclKind:
            return "ModuleDecl";
    }
}

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
