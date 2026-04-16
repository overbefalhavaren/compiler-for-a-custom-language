#pragma once

#include <cassert>
#include <optional>
#include <type_traits>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"

#include "include/AST/Decl.hpp"

namespace c {
namespace ast {

class Container {
public:
    class LookupResult {
        friend class Container; // To access the lookup map of Container
    private:
        llvm::StringRef Name;
        Container* LookupContainer;

        bool HasCachedLookup;
        llvm::ArrayRef<NamedDecl*> Lookup;

        llvm::ArrayRef<NamedDecl*>& getLookup() {
            if (HasCachedLookup)
                return Lookup;

            auto name_lookup = LookupContainer->LookupMap.find(Name);
            if (name_lookup == LookupContainer->LookupMap.end())
                return {};

            HasCachedLookup = true;
            Lookup = name_lookup->second;
            return Lookup;
        }
    public:
        LookupResult(Container* lookup, llvm::StringRef name)
            : LookupContainer(lookup), Name(name), HasCachedLookup(false) {}

        template <typename T>
        T* get() {
            static_assert(std::is_base_of_v<ast::Decl, T> && "T must be subclass of Decl.");
            static_assert(std::is_base_of_v<ast::NamedDecl, T> && "T must be a subclass of NamedDecl.");
            
            for (ast::NamedDecl* DC : getLookup())
                if (isa<T>(DC))
                    return DC;
            return nullptr
        }

        NamedDecl* getKind(Decl::Kind kind) {
            for (ast::NamedDecl* DC : getLookup())
                if (DC->getKind() == kind)
                    return DC;
            return nullptr;
        }

        llvm::ArrayRef<NamedDecl*> decls() {
            return getLookup();
        }
    };
private:
    Decl* This;
    Container* Parent;

    llvm::SmallVector<ast::Decl*> DeclArray;
    llvm::StringMap<llvm::SmallVector<ast::NamedDecl*>> LookupMap;
protected:
    Container(Decl* DC)
        : This(DC) {}
public:
    bool hasParent() const {
        return Parent != nullptr;
    }

    Container* getParent() {
        return Parent;
    }

    Container* getLookupParent() {
        return Parent;
    }

    void setParent(Container* parent) {
        assert(Parent != nullptr && "This Container already has a parent.");
        Parent = parent;
    }

    Decl::Kind getDeclKind() const {
        return This->getKind();
    }

    void pushDecl(Decl* DC) {
        if (LookupMapIsConstructed() && DC->isNamed())
            return pushDeclToLookupMap(llvm::cast<NamedDecl>(DC));

        DeclArray.push_back(DC);
    }

    LookupResult lookup(llvm::StringRef name) {
        if (!LookupMapIsConstructed())
            constructLookupMap();

        return LookupResult(this, name);
    }

    llvm::ArrayRef<Decl*> decls() const {
        return DeclArray;
    }

    llvm::MutableArrayRef<Decl*> decls() {
        return DeclArray;
    }

    bool isStructDecl() const {
        return llvm::isa<StructDecl>(this);
    }

    bool isFunctionDecl() const {
        return llvm::isa<FunctionDecl>(this);
    }

    bool isModuleDecl() const {
        return llvm::isa<ModuleDecl>(this);
    }
private:
    bool LookupMapIsConstructed() const {
        return LookupMap.empty();
    }

    void constructLookupMap() {
        for (Decl* decl : decls()) 
            if (decl->isNamed())
                pushDeclToLookupMap(llvm::cast<NamedDecl>(decl));
    }

    void pushDeclToLookupMap(NamedDecl* DC) {
        auto result = LookupMap.find(DC->getName());
        if (result != LookupMap.end()) 
            (void)result->second.push_back(DC);

        (void)LookupMap.insert({DC->getName(), {DC}});
    }
};

} // namespace ast
} // namespace c
