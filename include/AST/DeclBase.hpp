#pragma once

#include <cassert>
#include <optional>
#include <type_traits>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"

#include "include/AST/Type.hpp"
#include "include/AST/TypeInfo.hpp"
#include "include/IO/SrcSpan.hpp"

#include "llvm/Support/raw_ostream.h"

namespace c {
namespace ast {

class StructDecl;   // include/AST/Decl.hpp
class FunctionDecl; // include/AST/Decl.hpp
class ModuleDecl;   // include/AST/Decl.hpp
class Container;

class Decl {
public:
    enum Kind {
        firstDecl,
        firstNamedDecl = firstDecl,
        
        firstTypeDecl = firstDecl,
        TypeAliasDeclKind = firstTypeDecl,
        EnumDeclKind,   // NOTE: Planned but currently not implemented
        TraitDeclKind,  // NOTE: Planned but currently not implemented
        StructDeclKind,
        lastTypeDecl = StructDeclKind,
        
        firstValueDecl,

        firstInitDecl = firstValueDecl,
        VarDeclKind = firstInitDecl,
        ParamDeclKind,
        FieldDeclKind,
        lastInitDecl = FieldDeclKind,

        FunctionDeclKind,
        lastValueDecl = FunctionDeclKind,
        
        ModuleDeclKind,
        lastNamedDecl = ModuleDeclKind,

        ImportDeclKind,  // NOTE: Planned but currently not implemented
        ExportDeclKind,  // NOTE: Planned but currently not implemented
        ImplDeclKind,    // NOTE: Planned but currently not implemented
        lastDecl = ImplDeclKind
    };
private:
    Kind DeclKind;
    SrcSpan Span;
    Container* DeclContainer = nullptr;
protected:
    Decl(Kind DK, SrcSpan span)
        : DeclKind(DK), Span(span) {}
public:
    static bool classof(const Decl* d) {
        return true;
    }

    Kind getKind() const {
        return DeclKind;
    }

    llvm::StringRef getDeclKindName() const;

    void setSpan(SrcSpan span) {
        Span = span;
    }

    SrcSpan getSpan() const {
        return Span;
    }

    SrcLoc getStartLoc() const {
        return Span.getStart();
    }

    SrcLoc getEndLoc() const {
        return Span.getEnd();
    }

    void setContainer(Container* DC) {
        DeclContainer = DC;
    }

    Container* getContainer() {
        return DeclContainer;
    }

    const Container* getContainer() const {
        return DeclContainer;
    }
};

class NamedDecl : public Decl {
private:
    llvm::StringRef Name;
protected:
    NamedDecl(Kind DK, SrcSpan span, llvm::StringRef name)
        : Decl(DK, span), Name(name) {}
public:
    static bool classof(const Decl* d) {
        return d->getKind() >= firstNamedDecl && 
               d->getKind() <= lastNamedDecl;
    }

    llvm::StringRef getName() const {
        return Name;
    }
};

class TypeDecl : public NamedDecl {
private:
    const Type* Ty;
protected:
    TypeDecl(Kind DK, SrcSpan span, llvm::StringRef name)
        : NamedDecl(DK, span, name), Ty(nullptr) {}
public:
    static bool classof(const Decl* d) {
        return d->getKind() >= firstTypeDecl && 
               d->getKind() <= lastTypeDecl;
    }

    const Type* getDeclType() const {
        return Ty;
    }

    void setDeclType(const Type* DT) {
        Ty = DT;
    }
};

class ValueDecl : public NamedDecl {
private:
    TypeInfo* Info;
protected:
    ValueDecl(Kind DK, SrcSpan span, llvm::StringRef name, TypeInfo* info)
        : NamedDecl(DK, span, name), Info(info) {}
public:
    static bool classof(const Decl* d) {
        return d->getKind() >= firstValueDecl && 
               d->getKind() <= lastValueDecl;
    }

    void setTypeInfo(TypeInfo* info) {
        Info = info;
    }

    TypeInfo* getTypeInfo() {
        return Info;
    }

    const TypeInfo* getTypeInfo() const {
        return Info;
    }

    const Type* getType() const {
        assert(Info != nullptr);
        return Info->getType();
    }
};

class InitDecl : public ValueDecl {
private:
    Expr* Init;
protected:
    InitDecl(Kind DK, SrcSpan span, llvm::StringRef name, TypeInfo* info, Expr* init)
        : ValueDecl(DK, span, name, info), Init(init) {}
public:
    static bool classof(const Decl* d) {
        return d->getKind() >= firstInitDecl && 
               d->getKind() <= lastInitDecl;
    }

    bool isAutoTyped() const {
        return getTypeInfo()->isImplicitTy();
    }

    bool hasInit() const {
        return Init != nullptr;
    }

    Expr* getInit() {
        return Init;
    }

    const Expr* getInit() const {
        return Init;
    }
};

class Container {
public:
    class LookupResult {
        friend class Container; // To access the lookup map of Container
    private:
        llvm::StringRef Name;
        Container* LookupContainer;

        bool HasCachedLookup = false;
        llvm::ArrayRef<NamedDecl*> Lookup = {};

        llvm::ArrayRef<NamedDecl*>& getLookup() {
            if (HasCachedLookup)
                return Lookup;

            auto name_lookup = LookupContainer->LookupMap.find(Name);
            if (name_lookup == LookupContainer->LookupMap.end())
                return Lookup; // Null value, since just doing "{}" gives a compiler error

            HasCachedLookup = true;
            Lookup = name_lookup->second;
            return Lookup;
        }
    public:
        LookupResult(Container* lookup, llvm::StringRef name)
            : LookupContainer(lookup), Name(name) {}

        template <typename T>
        T* get() {
            static_assert(std::is_base_of_v<Decl, T> && "T must be subclass of Decl.");
            static_assert(std::is_base_of_v<NamedDecl, T> && "T must be a subclass of NamedDecl.");
            
            for (NamedDecl* DC : getLookup())
                if (isa<T>(DC))
                    return llvm::cast<T>(DC);
            return nullptr;
        }

        NamedDecl* getKind(Decl::Kind kind) {
            for (NamedDecl* DC : getLookup())
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
    Container* Parent = nullptr;

    llvm::SmallVector<Decl*> DeclArray = {};
    llvm::StringMap<llvm::SmallVector<NamedDecl*>> LookupMap = {};
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
        Parent = parent;
    }

    Decl::Kind getDeclKind() const {
        return This->getKind();
    }

    void pushDecl(Decl* DC) {
        if (LookupMapIsConstructed() && llvm::isa<NamedDecl>(DC))
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

    // bool isStructDecl() const {
    //     return llvm::isa<StructDecl>(This);
    // }

    // bool isFunctionDecl() const {
    //     return llvm::isa<FunctionDecl>(This);
    // }

    // bool isModuleDecl() const {
    //     return llvm::isa<ModuleDecl>(This);
    // }
private:
    bool LookupMapIsConstructed() const {
        return !LookupMap.empty();
    }

    void constructLookupMap() {
        for (Decl* DC : decls()) 
            if (llvm::isa<NamedDecl>(DC))
                pushDeclToLookupMap(llvm::cast<NamedDecl>(DC));
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
