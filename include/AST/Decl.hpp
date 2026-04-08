#pragma once

#include <cassert>
#include <type_traits>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include "include/AST/Container.hpp"
#include "include/AST/Expr.hpp"
#include "include/AST/Stmt.hpp"
#include "include/AST/TypeInfo.hpp"
#include "include/IO/SrcSpan.hpp"

namespace c {
namespace ast {

class Decl {
public:
    enum Kind {
        firstDecl,
        firstNamedDecl = firstDecl,
        
        firstTypeDecl = firstDecl,
        EnumDeclKind = firstTypeDecl,   // NOTE: Planned but currently not implemented
        TraitDeclKind,                  // NOTE: Planned but currently not implemented
        StructDeclKind,
        lastTypeDecl = StructDeclKind,
        
        firstValueDecl,

        firstInitDecl = firstValueDecl,
        VarDeclKind = firstInitDecl,
        ParamDeclKind,
        FieldDeclKind,
        lastInitDecl = FieldDeclKind,

        TypeAliasDeclKind,
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
    Container* DeclContainer;
    Decl* NextDecl;
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
        assert(!DeclContainer && "This Decl already has a Container.");
        DeclContainer = DC;
    }

    Container* getContainer() {
        return DeclContainer;
    }

    const Container* getContainer() const {
        return DeclContainer;
    }

    void setNextDecl(Decl* DC) {
        NextDecl = DC;
    }

    Decl* getNextDecl() {
        return NextDecl;
    }

    const Decl* getNextDecl() const {
        return NextDecl;
    }

    bool isNamed() const {
        return NamedDecl::classof(this);
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
        return d->getKind() <= firstNamedDecl && 
               d->getKind() <= lastNamedDecl;
    }

    llvm::StringRef getName() const {
        return Name;
    }
};

class TypeDecl : public NamedDecl {
private:
    Type* Ty;
protected:
    TypeDecl(Kind DK, SrcSpan span, llvm::StringRef name)
        : NamedDecl(DK, span, name), Ty(nullptr) {}
public:
    static bool classof(const Decl* d) {
        return d->getKind() <= firstTypeDecl && 
               d->getKind() <= lastTypeDecl;
    }

    const Type* getDeclType() const {
        return Ty;
    }

    void setDeclType(Type* DT) {
        Ty = DT;
    }
};

class TypeAliasDecl : public TypeDecl {
private:
    TypeInfo* Info;
public:
    static constexpr Kind ClassKind = TypeAliasDeclKind;

    TypeAliasDecl(SrcSpan span, llvm::StringRef name, TypeInfo* info)
        : TypeDecl(ClassKind, span, name), Info(info) {}

    static bool classof(const Decl* d) {
        return d->getKind() == ClassKind;
    }

    TypeInfo* getTypeInfo() const {
        return Info;
    }
};

class StructDecl : public TypeDecl, public Container {
public:
    static constexpr Kind ClassKind = StructDeclKind;
private:
    llvm::SmallVector<FieldDecl*> Fields;
public:
    StructDecl(llvm::StringRef name)
        : TypeDecl(ClassKind, SrcSpan(), name), Container(this) {}

    static bool classof(const Decl* d) {
        return d->getKind() == ClassKind;
    }
    
    void setFields(llvm::SmallVector<FieldDecl*>&& fields) {
        assert(Fields.size() != 0 && "This StructDecl already has fields");
        Fields = std::move(fields);
    }

    size_t getAmntFields() const {
        return Fields.size();
    }

    FieldDecl* getFieldDecl(size_t i) {
        return Fields[i];
    }

    const FieldDecl* getFieldDecl(size_t i) const {
        return Fields[i];
    }

    llvm::MutableArrayRef<FieldDecl*> fields() {
        return llvm::MutableArrayRef<FieldDecl*>(Fields.begin(), Fields.end());
    }

    llvm::ArrayRef<FieldDecl*> fields() const {
        return Fields;
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
        return d->getKind() <= firstValueDecl && 
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
        return d->getKind() <= firstInitDecl && 
               d->getKind() <= lastInitDecl;
    }

    bool isAutoTyped() const {
        return getTypeInfo() == nullptr;
    }

    bool hasInit() const {
        return Init != nullptr;
    }

    const Type* getType() const {
        if (isAutoTyped())
            return Init->getType();
        return getTypeInfo()->getType();
    }

    Expr* getInit() {
        return Init;
    }

    const Expr* getInit() const {
        return Init;
    }
};

class VarDecl : public InitDecl {
public:
    static constexpr Kind ClassKind = VarDeclKind;

    enum Mutability {
        Constant,
        Immutable,
        Mutable
    };
private:
    Mutability M;
public:
    VarDecl(SrcSpan span, llvm::StringRef name, Mutability m, TypeInfo* info, Expr* init)
        : InitDecl(ClassKind, span, name, info, init), M(m) {}

    static bool classof(const Decl* d) {
        return d->getKind() == ClassKind;
    }

    bool isConstant() const {
        return M == Constant;
    }

    bool isMutable() const {
        return M == Mutable;
    }
};

class ParamDecl : public InitDecl {
public:
    static constexpr Kind ClassKind = ParamDeclKind;

    enum Mutability {
        MutableReference,
        ImmutableReference,
        MovedValue
    };
private:
    Mutability M;
public:
    ParamDecl(SrcSpan span, llvm::StringRef name, Mutability m, TypeInfo* info, Expr* init)
        : InitDecl(ClassKind, span, name, info, init), M(m) {}

    static bool classof(const Decl* d) {
        return d->getKind() == ClassKind;
    }

    bool isMutable() const {
        return M != ImmutableReference;
    }
};

class FieldDecl : public InitDecl {
public:
    static constexpr Kind ClassKind = FieldDeclKind;
private:
    size_t Index;
    StructDecl* Parent;
public:
    FieldDecl(SrcSpan span, llvm::StringRef name, size_t idx, TypeInfo* info, Expr* init)
        : InitDecl(ClassKind, span, name, info, init), Index(idx) {}

    static bool classof(const Decl* d) {
        return d->getKind() == ClassKind;
    }

    size_t getFieldIndex() const {
        return Index;
    }

    void setParent(StructDecl* parent) {
        Parent = parent;
    }

    StructDecl* getParent() {
        return Parent;
    }

    const StructDecl* getParent() const {
        return Parent;
    }
};

class FunctionDecl : public ValueDecl, public Container {
public:
    static constexpr Kind ClassKind = FunctionDeclKind;
private:
    llvm::SmallVector<ParamDecl*> Parameters;
    BlockStmt* Body;
public:
    FunctionDecl(llvm::StringRef name)
        : ValueDecl(ClassKind, SrcSpan(), name, nullptr), Container(this) {}
    
    static bool classof(const Decl* d) {
        return d->getKind() == ClassKind;
    }

    bool isVoid() const {
        return getTypeInfo() == nullptr;
    }

    bool hasBody() const {
        return Body != nullptr;
    }

    void setBody(BlockStmt* body) {
        assert(Body == nullptr && "This FunctionDecl already has a body");
        Body = body;
    }

    BlockStmt* getBody() const {
        return Body;
    }

    bool hasParams() const {
        return Parameters.size() != 0;
    }

    void setParams(llvm::SmallVector<ParamDecl*> params) {
        assert(Parameters.size() == 0 && "This FunctionDecl already has parameters defined");
        Parameters = params;
    }
    
    size_t getAmntParams() const {
        return Parameters.size();
    }

    ParamDecl* getParamDecl(size_t i) {
        return Parameters[i];
    }

    const ParamDecl* getParamDecl(size_t i) const {
        return Parameters[i];
    }

    llvm::MutableArrayRef<ParamDecl*> parameters() {
        return llvm::MutableArrayRef<ParamDecl*>(Parameters.begin(), Parameters.end());
    }

    llvm::ArrayRef<ParamDecl*> parameters() const {
        return Parameters;
    }
};

} // namespace ast
} // namespace c
