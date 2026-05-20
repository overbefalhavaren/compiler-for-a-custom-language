#pragma once

#include <cstdint>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include "include/AST/DeclBase.hpp"

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
    size_t Depth = 0; // NOTE: Currently unused

    // The parent Scope of this Scope, i.e the Scope this Scope is part of
    Scope* Parent = nullptr;
    Scope* FnProto = nullptr;

    // ast::Container* Entity = nullptr;

    // NOTE: This is currently not how it actually works. Ignore the following comment.
    // This array will always be empty unless the Scope is a FunctionScope.
    // Declared as an array of Decl* so that type conversion for Scope::decls() 
    // will work. In practice this will always be an array of NamedDecl* 
    llvm::SmallVector<ast::NamedDecl*> ScopeDecls = {};
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
        assert(proto->isFunctionPrototypeScope());
        FnProto = proto;
    }

    // void setEntity(ast::Container* entity) {
    //     Entity = entity;
    // }

    // ast::Container* getEntity() {
    //     return Entity;
    // }

    void pushDecl(ast::NamedDecl* DC) {
        ScopeDecls.push_back(DC);
    }

    bool hasDecls() const {
        return !ScopeDecls.empty();
    }

    llvm::ArrayRef<ast::NamedDecl*> decls() const {
        return ScopeDecls;
    }

    bool isModuleScope() const {
        return Flags == ModuleScope;
    }

    bool isStructScope() const {
        return Flags == StructScope;
    }

    bool isFunctionScope() const {
        return Flags == FunctionScope;
    }

    bool isFunctionPrototypeScope() const {
        return Flags == FnPrototypeScope;
    }
};

} // namespace sema
} // namespace c
