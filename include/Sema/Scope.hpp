#pragma once

#include <cstdint>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include "include/AST/Container.hpp"
#include "include/AST/Decl.hpp"

namespace c {
namespace sema {

class Scope {
public:
    enum ScopeFlags {
        ModuleScope,
        StructScope,
        FunctionScope,
        FnPrototypeScope,
    };
private:
    ScopeFlags Flags;
    size_t Depth;

    // The parent Scope of this Scope, i.e the Scope this Scope is part of
    Scope* Parent;
    Scope* FnProto;

    ast::Container* Entity;

    // NOTE: This is currently not how it actually works.
    // This array will always be empty unless the Scope is a FunctionScope.
    // Declared as an array of Decl* so that type conversion for Scope::decls() 
    // will work. In practice this will always be an array of NamedDecl* 
    llvm::SmallVector<ast::NamedDecl*> ScopeDecls;
public:
    Scope(Scope* parent, ScopeFlags flags)
        : Parent(parent), Flags(flags) {}

    bool hasParent() const {
        return Parent != nullptr;
    }

    ScopeFlags getFlags() const {
        return Flags;
    }

    size_t getDepth() const {
        return Depth;
    }

    Scope* getParent() {
        return Parent;
    }

    Scope* getFnPrototype() const {
        return FnProto;
    }

    void setFnPrototype(Scope* proto) {
        FnProto = proto;
    }

    void setEntity(ast::Container* entity) {
        Entity = entity;
    }

    ast::Container* getEntity() {
        return Entity;
    }

    void pushDecl(ast::NamedDecl* DC) {
        ScopeDecls.push_back(DC);
    }

    llvm::ArrayRef<ast::NamedDecl*> decls() {
        // FIXME: This is a terrible way of doing it but it works and 
        // right now i need to get shit done.
        if (ScopeDecls.empty() && Entity != nullptr)
            for (ast::Decl* DC : Entity->decls())
                if (ast::NamedDecl* ND = llvm::dyn_cast<ast::NamedDecl>(DC))
                    ScopeDecls.push_back(ND);
        
        return ScopeDecls;
    }
};

} // namespace sema
} // namespace c
