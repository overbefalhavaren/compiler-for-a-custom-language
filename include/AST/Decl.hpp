#pragma once

#include <cassert>
#include <type_traits>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"

#include "include/AST/DeclBase.hpp"
#include "include/IO/SrcSpan.hpp"

namespace c {
namespace ast {

class Expr;      // include/AST/Expr.hpp
class TypeInfo;  // include/AST/TypeInfo.hpp
class BlockStmt; // include/AST/Stmt.hpp
class FieldDecl;

class TypeAliasDecl : public TypeDecl {
private:
    TypeInfo* Info;
public:
    static constexpr Kind ClassKind = TypeAliasDeclKind;

    TypeAliasDecl(SrcSpan span, llvm::StringRef name, TypeInfo* info)
        : TypeDecl(ClassKind, span, name), Info(info) {}

    static bool classof(const Decl* DC) {
        return DC->getKind() == ClassKind;
    }

    TypeInfo* getTypeInfo() const {
        return Info;
    }
};

class StructDecl : public TypeDecl, public Container {
public:
    static constexpr Kind ClassKind = StructDeclKind;
private:
    llvm::SmallVector<FieldDecl*> Fields = {};
public:
    StructDecl(llvm::StringRef name)
        : TypeDecl(ClassKind, SrcSpan(), name), Container(this) {}

    static bool classof(const Decl* DC) {
        return DC->getKind() == ClassKind;
    }
    
    void setFields(llvm::SmallVector<FieldDecl*>&& fields) {
        // Added to the Container in the parser.
        Fields = std::move(fields);
    }

    size_t getAmntFields() const {
        return Fields.size();
    }

    FieldDecl* getFieldDecl(size_t idx) {
        return Fields[idx];
    }

    const FieldDecl* getFieldDecl(size_t idx) const {
        return Fields[idx];
    }

    llvm::MutableArrayRef<FieldDecl*> fields() {
        return llvm::MutableArrayRef<FieldDecl*>(Fields.begin(), Fields.end());
    }

    llvm::ArrayRef<FieldDecl*> fields() const {
        return Fields;
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

    static bool classof(const Decl* DC) {
        return DC->getKind() == ClassKind;
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

    ParamDecl(SrcSpan span, llvm::StringRef name, TypeInfo* info, Expr* init)
        : InitDecl(ClassKind, span, name, info, init) {}

    static bool classof(const Decl* DC) {
        return DC->getKind() == ClassKind;
    }

    // NOTE: Mutability will be added later on
    bool isMutable() const {
        return false;
    }
};

class FieldDecl : public InitDecl {
public:
    static constexpr Kind ClassKind = FieldDeclKind;
private:
    size_t Index;
    StructDecl* Parent = nullptr;
public:
    FieldDecl(SrcSpan span, llvm::StringRef name, size_t idx, TypeInfo* info, Expr* init)
        : InitDecl(ClassKind, span, name, info, init), Index(idx) {}

    static bool classof(const Decl* DC) {
        return DC->getKind() == ClassKind;
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
    llvm::SmallVector<ParamDecl*> Parameters = {};
    BlockStmt* Body = nullptr;
public:
    FunctionDecl(llvm::StringRef name)
        : ValueDecl(ClassKind, SrcSpan(), name, nullptr), Container(this) {}
    
    static bool classof(const Decl* DC) {
        return DC->getKind() == ClassKind;
    }

    bool isVoid() const {
        return getTypeInfo() == nullptr;
    }

    bool hasBody() const {
        return Body != nullptr;
    }

    void setBody(BlockStmt* body) {
        Body = body;
    }

    BlockStmt* getBody() const {
        return Body;
    }

    bool hasParams() const {
        return !Parameters.empty();
    }

    void setParams(llvm::SmallVector<ParamDecl*>&& params) {
        Parameters = std::move(params);
    }
    
    size_t getAmntParams() const {
        return Parameters.size();
    }

    ParamDecl* getParamDecl(size_t idx) {
        return Parameters[idx];
    }

    const ParamDecl* getParamDecl(size_t idx) const {
        return Parameters[idx];
    }

    llvm::ArrayRef<ParamDecl*> parameters() const {
        return Parameters;
    }

    llvm::MutableArrayRef<ParamDecl*> parameters() {
        return llvm::MutableArrayRef<ParamDecl*>(Parameters.begin(), Parameters.end());
    }
};

class ModuleDecl : public NamedDecl, public Container {
public:
    static constexpr Kind ClassKind = ModuleDeclKind;

    ModuleDecl(SrcSpan span, llvm::StringRef name)
        : NamedDecl(ClassKind, span, name), Container(this) {}

    static bool classof(const Decl* DC) {
        return DC->getKind() == ClassKind;
    } 
};

} // namespace ast
} // namespace c
