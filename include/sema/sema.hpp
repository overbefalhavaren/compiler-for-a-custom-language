#pragma once

#include "llvm/ADT/StringRef.h"

#include "include/Sema/Scope.hpp"

namespace c {

class Sema {

public:
    bool analyze() {

    }

    bool isTypeConvertable(const Type* from, const Type* to);

    void lookupName();

    void lookupLocal();

    void lookupClosest();

    bool analyzeDecl(ast::Decl* DC, sema::Scope* scope) {

    }

    bool analyzeStmt(ast::Stmt* ST, sema::Scope* scope) {

    }
private:
    ast::NamedDecl* lookupDeclInScope(ast::Decl::Kind kind, llvm::StringRef name, sema::Scope* scope) {
        if (scope->isContainerDecl()) 
            return scope->getEntity()->lookup(name).getKind(kind);

        for (ast::NamedDecl* DC : scope->decls()) 
            if (DC->getKind() == kind && DC->getName() == name)
                return DC;

        return nullptr;
    }

    bool toBuiltinKind(llvm::StringRef name, BuiltinType::BuiltinKind& result) {

    }

    bool resolveTypeInfo(ast::TypeInfo* info, sema::Scope* scope) {
        
    }

    bool analyzeInitDecl(ast::InitDecl* DC, sema::Scope* scope, bool isVarDecl = false) {
        if (findShadowingDecl(DC, scope)) {
            llvm::outs() << "A variable called '" << DC->getName().str() << "' already exists.\n";
            return true;
        }

        scope->pushDecl(DC);

        if (!DC->hasInit()) {
            if (isVarDecl) {
                llvm::outs() << "Variable declarations must have an init value.\n";
                return true;
            }

            if (DC->isAutoTyped()) {
                llvm::outs() << "Automatically type evalueated Decl must have a value.\n";
                return true;
            }

            return false;
        }

        if () // FIXME: Analyze init value
            return true;

        if (DC->isAutoTyped()) {
            if (DC->getInit()->isTypeDependant()) {
                llvm::outs() << "Type dependant expression can't be automatically evaluated.\n";
                return true;
            }

            // FIXME: Maybe check for void function call

            DC->getTypeInfo()->setType(DC->getInit()->getType());
            return false;
        }

        if (DC->getInit()->isTypeDependant()) {
            // FIXME: 
            llvm_unreachable("Currently no type dependant expressions (i think).");
        } else if (isTypeConvertable(DC->getInit()->getType(), DC->getType())) {
            llvm::outs() << "Invalid type conversion\n";
            return true;
        }

        return false;
    }

    bool analyzeFuncionDecl(ast::FunctionDecl* DC, sema::Scope* scope) {
        if (findShadowingDecl(DC, scope)) {
            llvm::outs() << "A function called '" << DC->getName().str() << "' already exists.\n";
            return true;
        }

        scope->pushDecl(DC);

        sema::Scope proto; // FIXME:
        if (DC->hasParams())
            for (ast::ParamDecl* param : DC->parameters()) {
                if (analyzeInitDecl(param, &proto))
                    return true;
                proto.pushDecl(param);
            }

        if (!DC->isVoid() && resolveTypeInfo(DC->getTypeInfo(), &proto))
            return true;
    
        if (analyzeBlockStmt(DC->getBody(), &proto))
            return true;

        return false;
    }

    bool validate() {

    }

    bool analyzeTypeAliasDecl(ast::TypeAliasDecl* DC, sema::Scope* scope) {
        BuiltinType::BuiltinKind _;
        if (toBuiltinKind(DC->getName(), _)) {
            llvm::outs() << "Type alias name conflicts with builtin.\n";
            return true;
        }

        if (findShadowingDecl(DC, scope)) {
            llvm::outs() << "A type called '" << DC->getName().str() << "' already exists.\n";
            return true;
        }

        scope->pushDecl(DC);

        if (resolveTypeInfo(DC->getTypeInfo(), scope))
            return true;

        return false;
    }

    bool analyzeStructDecl(ast::StructDecl* DC, sema::Scope* scope) {
        if () {
            
        }

        sema::Scope body; // FIXME:
        for (ast::Decl* d : DC->decls()) {
            if (auto FD = llvm::dyn_cast<ast::FieldDecl>(d)) {
                if (analyzeInitDecl(FD, &body))
                    return true;
            } else if (auto TD = llvm::dyn_cast<ast::TypeAliasDecl>(d)) {
                if (analyzeTypeAliasDecl(TD, &body))
                    return true;
            } else {
                llvm::outs() << "Only field declarations and type aliases are allowed in struct declarations.\n";
                return true;
            }

            // Can only be FieldDecl or TypeAliasDecl, which are both NamedDecl
            body.pushDecl(llvm::cast<ast::NamedDecl>(d));
        }

        return false;
    }

    // FIXME:
    bool analyzeBlockStmt(ast::BlockStmt* ST, sema::Scope* scope) {
        sema::Scope body; // FIXME:
        for (ast::Stmt* s : ST->stmts()) {
            if (auto d = llvm::dyn_cast<ast::DeclStmt>(s)) {
                if (!llvm::isa<ast::VarDecl>(d) || !llvm::isa<ast::TypeAliasDecl>(d)) {
                    llvm::outs() << "Only variable declarations and type aliases are allowed in function bodies.\n";
                    return true;
                }

                if (analyzeDecl(d->getDecl(), &body))
                    return true;

                body.pushDecl(d->getDecl());
            } else if (analyzeStmt(s, &body))
                return true;
        }

        return false;
    }
};

} // namespace c
