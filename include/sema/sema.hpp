#pragma once

#include <memory>

#include "llvm/Support/raw_ostream.h" // For llvm::outs()

#include "include/ast/stmt.hpp"
#include "include/ast/module.hpp"
#include "include/diag/diagnostic.hpp"
#include "include/sema/scope.hpp"

namespace c {

class Sema {
private:
    Diagnostic& Diag;
    sema::TopLevelScope GlobalScope;
public:
    Sema(Diagnostic& diag)
        : Diag(diag), GlobalScope() {}

    bool analyzeModule(const Module& src) {
        for (const std::unique_ptr<ast::Stmt>& stmt : src.getStmts()) {
            switch (stmt->getStmtClass()) {
                case ast::Stmt::SC_VarDecl:
                    break;
                case ast::Stmt::SC_TypeDecl:
                    break;
                case ast::Stmt::SC_FunctionDecl:
                    break;
            }
        }
    }
private:

    bool analyzeStmt(ast::Stmt& stmt) {
        switch (stmt.getStmtClass()) {
            
        }
    }

    bool resolveType(ast::TypeExpr& stmt, sema::Scope* scope) {
        std::optional<ast::TypeExpr*> type = scope->lookupType(stmt.getName());
        if (!type.has_value()) {
            llvm::outs() << "Undefined type.\n";
            return true;
        }

        if (!stmt.isa(*type.value())) {
            llvm::outs() << "Invalid type usage.\n";
            return true;
        }

        if (!type.value()->isResolved()) {
            assert(false && "Not Implemented.");
        }
        
        stmt.setResolved(type.value()->getResolved());
        return false;
    }

    bool analyzeTypeDecl(ast::TypeDecl& stmt, sema::Scope* scope) {
        if (scope->lookupType(stmt.getName()).has_value()) {
            llvm::outs() << "A type called '" << stmt.getName().str() << "' already exists.\n";
            return true;
        }

        if (resolveType(*stmt.getTypeExpr(), scope))
            return true;
        
        return false;
    }

    bool analyzeVarDecl(ast::VarDecl& stmt, sema::Scope* scope) {
        if (scope->lookupVariable(stmt.getName()).has_value()) {
            llvm::outs() << "A variable called '" << stmt.getName().str() << "' already exists.\n";
            return true;
        }

        if (stmt.isAutoTyped()) {
            if (!stmt.isa<ast::CallExpr>()) {
                llvm::outs() << "Only CallExpr can be automatically type evaluated.\n";
                return true;
            }

            // TODO: Implement sema method for CallExpr
            assert(false && "Not implemented.");
            return false;
        }

        if (resolveType(*stmt.getTypeExpr(), scope))
            return true;

        if (analyzeStmt(*stmt.getValue()))
            return true;
        
        return false;
    }

    bool analyzeFunctionDecl(ast::FunctionDecl& stmt, sema::Scope* scope) {
        if (stmt.hasReturn() && resolveType(*stmt.getTypeExpr(), scope)) {
            llvm::outs() << "\n";
            return true;
        }
        
        
    }

    bool analyzeStructDecl(ast::StructDecl& stmt) {
        if (GlobalScope.lookupStruct(stmt.getName()).has_value()) {
            llvm::outs() << "A struct called '" << stmt.getName().str() << "' already exists.\n";
            return true;
        }

        
        for (std::unique_ptr<ast::VarDecl>& attr : stmt.getAttrs())
            if (!)
    }

};

} // namespace c
