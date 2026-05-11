#pragma once

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Decl.hpp"
#include "include/Sema/Scope.hpp"

#include "llvm/Support/raw_ostream.h"

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
    T* getAsSingle() const {
        // Check T is a valid subclass of NamedDecl.
        // FIXME: Causes compiler error:
        // error C2572: 'c::Sema::analyzeInitDecl': redefinition of default argument: parameter 1
        // (void)llvm::cast<ast::NamedDecl, T>();
        static_assert(std::is_base_of_v<ast::Decl, T> && "T must be subclass of Decl.");
        static_assert(std::is_base_of_v<ast::NamedDecl, T> && "T must be a subclass of NamedDecl.");

        for (ast::NamedDecl* DC : Decls)
            if (llvm::isa<T>(DC))
                return llvm::cast<T>(DC);
        return nullptr;
    }

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

    bool LocalOnly;
    LookupKind Kind;
    llvm::StringRef Name;
    Scope* LookupScope;
public:
    Lookup(Scope* lookup, llvm::StringRef name, LookupKind kind = Any, bool localOnly = false)
        : LookupScope(lookup), Name(name), Kind(kind), LocalOnly(localOnly) {}

    LookupResult find() const {
        return doLookup(true);
    }

    LookupResult findAll() const {
        return doLookup(false);
    }

    LookupResult findKind(ast::Decl::Kind kind) const {
        Scope* current = LookupScope;
        while (true) {
            for (ast::NamedDecl* DC : current->decls())
                if (DC->getKind() == kind && DC->getName() == Name)
                    return LookupResult(DC);

            if (!current->hasParent())
                return LookupResult();
            if (current->isFunctionPrototypeScope() && LocalOnly)
                return LookupResult();
            current = current->getParent();
        }
    }
private:
    LookupResult doLookup(bool isSingle) const {
        LookupResult result;

        LookupFilter is_lookup_kind = getLookupFilter();
        Scope* current = getStartScope();

        while (true) {
            for (ast::NamedDecl* DC : current->decls()) {
                if (is_lookup_kind(DC) && DC->getName() == Name) {
                    if (isSingle)
                        return LookupResult(DC);

                    result.pushDecl(DC);
                }
            }

            if (!current->hasParent())
                break;
            if (current->isFunctionPrototypeScope() && LocalOnly)
                break;

            current = current->getParent();
        }

        return result;
    }

    Scope* getStartScope() const {
        switch (Kind) {
            default:
                llvm_unreachable("LookupKind flag is missing a case.");
            case Any:
            case Value:
                return LookupScope;
            case Type:
                if (LookupScope->isFunctionScope())
                    // TODO: Remove the note when templates are implemented.
                    // NOTE: Functions currently can't have templates. 
                    // Get  the prototype of the function instead of the functions 
                    // parent scope because the function can have a template. 
                    return LookupScope->getFnPrototype();
                return LookupScope;
        }
    }
 
    // This language is too stupid to understand you can just use llvm::isa.
    // So here I am implementing my own wrapper function around llvm::isa... 
    // I love my life...
    template <typename T>
    static bool isa(ast::Decl* DC) {
        return llvm::isa<T>(DC);
    }

    LookupFilter getLookupFilter() const {
        switch (Kind) {
            default:
                // If you reach this point then congratulations, you are officially stupid.
                llvm_unreachable("LookupKind flag is missing a case.");

            case Any:
                // Lambda function are stupid but let's use them anyway!
                return [](ast::Decl* DC) {
                    return true;
                };
            case Value:
                // Captures VarDecl, ParamDecl, FieldDecl and FunctionDecl.
                // Because function pointers are fun... (that's what *she* said)
                return &isa<ast::ValueDecl>;
            case Type:
                // TODO: Update this when more TypeDecl subclasses are added
                // Captures TypeAliasDecl and StructDecl
                // Because type are just... built different... or somthing...
                return &isa<ast::TypeDecl>;
        }
    }
};

} // namespace sema
} // namespace c
