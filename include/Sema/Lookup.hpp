#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include "include/AST/Decl.hpp"
#include "include/Sema/Scope.hpp"

namespace c {
namespace sema {

class LookupResult {
private:
    llvm::SmallVector<ast::NamedDecl*, 1> Decls = {}; 
public:
    LookupResult() = default;
    LookupResult(ast::NamedDecl* Single)
        : Decls({Single}) {}

    operator bool() const {
        return !isEmpty();
    }

    bool isEmpty() const {
        return Decls.size() == 0;
    }

    bool isSingle() const {
        return Decls.size() == 1;
    }

    bool isAmbiguous() const {
        return Decls.size() > 1;
    }

    void pushDecl(ast::NamedDecl* DC) {
        Decls.push_back(DC);
    }

    ast::NamedDecl* getAsSingle() const {
        return isEmpty() ? nullptr : Decls[0];
    }
    
    template <typename T> 
    T* getAsSingle() const;

    llvm::ArrayRef<ast::NamedDecl*> matches() const {
        return Decls;
    }
};

class Lookup {
public:
    enum LookupKind {
        Any,    // Looks for any kind of Decl.
        Value,  // Looks for ValueDecl, including FunctionDecl because function pointers is a thing.
        Type,   // Looks for TypeDecl.
    };
private:
    // Function pointers are stupid but let's use them 
    // anyway because otherwise we'd break DRY.
    using LookupFilter = bool(*)(ast::Decl*); // Man, this syntax is a dumpsterfire

    bool LocalOnly = false;
    LookupKind Kind = Any;
    llvm::StringRef Name = "";
    Scope* LookupScope = nullptr;
public:
    Lookup(Scope* lookup, llvm::StringRef name, LookupKind kind = Any, bool localOnly = false)
        : LookupScope(lookup), Name(name), Kind(kind), LocalOnly(localOnly) {}

    LookupResult find() const {
        return doLookup(true);
    }

    LookupResult findAll() const {
        return doLookup(false);
    }

    LookupResult findKind(ast::Decl::Kind kind) const;
protected:
    LookupResult doLookup(bool isSingle) const;

    Scope* getStartScope() const;

    LookupFilter getLookupFilter() const;
};

} // namespace sema
} // namespace c
