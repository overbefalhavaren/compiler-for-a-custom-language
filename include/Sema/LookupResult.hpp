#pragma once

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"

#include "include/AST/Decl.hpp"
#include "include/Sema/Scope.hpp"

namespace c {
namespace sema {

class LookupResult {
private:
    ast::ModuleDecl* TopModule;
public:
    Lookup(ast::ModuleDecl* module)
        : TopModule(module) {}

    template <typename DeclClass>
    DeclClass* lookupDecl(sema::Scope* scope, llvm::StringRef name) {

    }

    template <typename DeclClass>
    DeclClass* lookupLocal(sema::Scope* scope, llvm::StringRef name) {

    }

    ast::TypeDecl* lookupTypeDecl(sema::Scope* scope, llvm::StringRef name, bool local = false) {
        for (ast::NamedDecl* DC : scope->decls())
            if (llvm::isa<ast::TypeDecl>(DC) && DC->getName() == name)
                return llvm::cast<ast::TypeDecl>(DC);

        if (scope->hasParent())
            return lookupTypeDecl(scope, name, local);
    }

};

} // namespace sema
} // namespace c
