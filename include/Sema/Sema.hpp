#pragma once

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

#include "include/AST/Decl.hpp"
#include "include/AST/DeclBase.hpp"
#include "include/AST/Expr.hpp"
#include "include/AST/Stmt.hpp"
#include "include/AST/Type.hpp"
#include "include/AST/TypeInfo.hpp"
#include "include/Frontend/ASTAllocator.hpp"
#include "include/Sema/Lookup.hpp"
#include "include/Sema/Scope.hpp"

namespace c {

class Sema {
private:
    ASTAllocator& Alloc;

    // Set inside analyzeFunctionDecl. Only inside analyzeReturnStmt to 
    // validate that the return value matches the functions return type.
    const ast::FunctionDecl* CurrentFunction = nullptr;
public:
    Sema(ASTAllocator& alloc)
        : Alloc(alloc) {}

    bool analyze(ast::ModuleDecl* topModule);

    sema::Lookup lookup(sema::Scope* scope, llvm::StringRef name, 
                        bool localOnly = false,
                        sema::Lookup::LookupKind kind = sema::Lookup::Any) {
        return sema::Lookup(scope, name, kind, localOnly);
    }

    bool validateTypeCast(const Type* from, const Type* to, bool isExplicit);

    bool analyzeDecl(ast::Decl* DC, sema::Scope* scope);

    bool analyzeStmt(ast::Stmt* ST, sema::Scope* scope);

    bool analyzeExpr(ast::Expr* EX, sema::Scope* scope, bool allowAssign);
private:
    bool resolveTypeInfo(ast::TypeInfo* info, sema::Scope* scope);

    bool analyzeModuleDecl(ast::ModuleDecl* DC, sema::Scope* scope);

    bool analyzeInitDecl(ast::InitDecl* DC, sema::Scope* scope, bool isVarDecl = false);

    bool analyzeFuncionDecl(ast::FunctionDecl* DC, sema::Scope* scope);

    bool validateTypeDefName(llvm::StringRef name, sema::Scope* scope);

    bool analyzeTypeAliasDecl(ast::TypeAliasDecl* DC, sema::Scope* scope);

    bool analyzeStructDecl(ast::StructDecl* DC, sema::Scope* scope);

    bool analyzeBlockStmt(ast::BlockStmt* ST, sema::Scope* scope);

    bool analyzeReturnStmt(ast::ReturnStmt* ST, sema::Scope* scope);

    bool analyzeConditionalStmtBody(ast::Stmt* body, sema::Scope* scope);

    bool analyzeIfStmt(ast::IfStmt* ST, sema::Scope* scope);

    bool analyzeWhileStmt(ast::WhileStmt* ST, sema::Scope* scope);

    bool analyzeCondition(ast::Expr* EX, sema::Scope* scope);

    bool validateExprIsMutable(ast::Expr* EX);

    bool analyzeBinaryAssign(ast::BinaryOperator* EX, sema::Scope* scope);

    bool analyzeBinaryOperator(ast::BinaryOperator* EX, sema::Scope* scope);

    bool analyzeDereference(ast::UnaryOperator* EX, sema::Scope* scope);

    bool analyzeUnaryAssign(ast::UnaryOperator* EX, sema::Scope* scope);

    bool analyzeUnaryOperator(ast::UnaryOperator* EX, sema::Scope* scope);

    // TODO: Currently doesn't support looking up anything else than
    // InitDecl. This is fine for now but will have to be updated
    // when function pointers among other things, are introduced.
    bool analyzeDeclRef(ast::DeclRefExpr* EX, sema::Scope* scope);
    
    bool analyzeSliceExpr(ast::SliceExpr* EX, sema::Scope* scope);

    bool analyzeAccessExpr(ast::AccessExpr* EX, sema::Scope* scope);

    bool analyzeCallExpr(ast::CallExpr* EX, sema::Scope* scope);

    bool analyzeArrayLiteral(ast::ArrayLiteral* EX, sema::Scope* scope);
};

} // namespace c
