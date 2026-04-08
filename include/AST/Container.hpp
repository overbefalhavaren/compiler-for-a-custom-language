#pragma once

#include <cassert>
#include <optional>
#include <type_traits>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"

#include "include/AST/Decl.hpp"

namespace c {
namespace ast {

class Container {
public:
    class LookupResult {
        friend class Container;
    private:
        llvm::StringRef Name;
        Container* LookupContainer;
        std::optional<llvm::DenseMap<Decl::Kind, NamedDecl*>&> Lookup;

        LookupResult(Container* lookup, llvm::StringRef name)
            : LookupContainer(lookup), Name(name) {}

        NamedDecl* lookup(Decl::Kind kind) {
            if (Lookup.has_value())
                return Lookup.value().lookup(kind);

            auto name_lookup = LookupContainer->LookupMap.find(Name);
            if (name_lookup == LookupContainer->LookupMap.end())
                return nullptr;

            (void)Lookup.emplace(name_lookup->second);
            return Lookup.value().lookup(kind);
        }
    public:
        template <typename T>
        T* get() {
            static_assert(std::is_base_of<ast::Decl, T>::value && "T must be subclass of Decl.");
            return static_cast<T*>(lookup(T::ClassKind));
        }

        NamedDecl* getKind(Decl::Kind kind) {
            return lookup(kind);
        }

         // FIXME: "auto" in function declarations is retarded
        auto decls() {
            if (Lookup.has_value())
                return Lookup.value().values();

            auto name_lookup = LookupContainer->LookupMap.find(Name);
            if (name_lookup == LookupContainer->LookupMap.end())
                return;

            Lookup.emplace(name_lookup->second);
            return Lookup.value().values();
        }
    };

    class DeclIterator {
    private:
        Decl* Current;
    public: 
        DeclIterator(Decl* start)
            : Current(start) {}

        Decl* operator*() const {
            return Current;
        }

        DeclIterator& operator++() {
            Current = Current->getNextDecl();
            return *this;
        }

        DeclIterator operator++(int) {
            DeclIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator!=(const DeclIterator& other) const {
            return Current != other.Current;
        }

        bool operator==(const DeclIterator& other) const {
            return Current == other.Current;
        }
    };

    class DeclRange {
    private:
        Decl* Start;
    public:
        DeclRange(Decl* start)
            : Start(start) {}

        DeclIterator begin() {
            return DeclIterator(Start);
        }

        DeclIterator end() {
            return DeclIterator(nullptr);
        }
    };
private:
    Decl* This;
    Container* Parent;

    Decl* FirstDecl;
    Decl* LastDecl;

    llvm::StringMap<llvm::DenseMap<Decl::Kind, NamedDecl*>> LookupMap;
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

        if (!FirstDecl) {
            FirstDecl = LastDecl = DC;
            return;
        }

        LastDecl->setNextDecl(DC);
        LastDecl = DC;
    }

    LookupResult lookup(llvm::StringRef name) {
        if (!LookupMapIsConstructed())
            constructLookupMap();

        return LookupResult(this, name);
    }

    DeclRange decls() {
        return DeclRange(FirstDecl);
    }

    bool hasLookupParentFallbacl() const {
        // FIXME:
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
            (void)result->second.insert({DC->getKind(), DC});

        (void)LookupMap.insert({DC->getName(), {{DC->getKind(), DC}}});
    }
};

} // namespace ast
} // namespace c
