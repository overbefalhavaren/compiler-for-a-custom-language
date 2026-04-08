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
        FunctionScope
    };
private:
    ScopeFlags Flag;
    size_t Depth;

    Scope* Parent;
    Scope* FnParent;

    ast::Container* Entity;

    llvm::SmallVector<ast::NamedDecl*> ScopeDecls;
public:
    // Scope(Scope* parent); // FIXME:

    bool hasParent() const {
        return Parent != nullptr;
    }

    size_t getDepth() const {
        return Depth;
    }

    Scope* getParent() {
        return Parent;
    }

    Scope* getFnParent() {
        return Parent;
    }

    ast::Container* getEntity() {
        return Entity;
    }

    void pushDecl(ast::NamedDecl* DC) {
        ScopeDecls.push_back(DC);
    }

    llvm::ArrayRef<ast::NamedDecl*> decls() {
        // FIXME:
    }

    bool isContainerDecl() const {
        // FIXME:
    }

    bool isFunctionScope() const {
        // FIXME:
    }
};

} // namespace sema
} // namespace c
