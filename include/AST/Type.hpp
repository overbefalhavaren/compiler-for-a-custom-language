#pragma once

#include "llvm/Support/Casting.h"

namespace c {

namespace ast {
class StructDecl;
} // namespace ast

// Forward declaration for metods of Type
class BuiltinType;

class Type {
public:
    enum Kind {
        firstType,

        PointerTypeKind = firstType,
        BuiltinTypeKind,
        EnumTypeKind,       // NOTE: Planned but currently not implemented
        ArrayTypeKind,
        StructTypeKind,
        TemplateTypeKind,   // NOTE: Planned but currently not implemented

        lastType = TemplateTypeKind
    };
private:
    Kind TypeKind;
public:
    Type(Kind TK)
        : TypeKind(TK) {}
    
    static bool classof(const Type* d) {
        return true;
    }

    inline Kind getKind() const {
        return TypeKind;
    }

    bool isNamedTy() const {
        return isBuiltinTy() ||
               isStructTy();
    }

    bool isBuiltinTy() const {
        return TypeKind == BuiltinTypeKind;
    }

    bool isPointerTy() const {
        return TypeKind == PointerTypeKind;
    }

    bool isArrayTy() const {
        return TypeKind == ArrayTypeKind;
    }

    bool isStructTy() const {
        return TypeKind == StructTypeKind;
    }

    bool isIntegerTy() const;

    bool isFloatingTy() const;

    bool isBooleanTy() const;
};

class BuiltinType : public Type {
public:
    static constexpr Kind ClassKind = BuiltinTypeKind;

    enum BuiltinKind {
        Bool,
        I8,
        I16,
        I32,
        I64,
        U8,
        U16,
        U32,
        U64,
        F32,
        F64
    };
private:
    BuiltinKind BK;
public:
    BuiltinType(BuiltinKind kind)
        : Type(ClassKind), BK(kind) {}

    static bool classof(const Type* d) {
        return d->getKind() == ClassKind;
    }

    static uint8_t getAmntBuiltinTypes() {
        // Enum starts at 0, not 1
        return (size_t)F64 + 1;
    }

    BuiltinKind getBuiltinKind() const {
        return BK;
    }

    bool isSigned() const {
        return BK == I8 ||
               BK == I16 ||
               BK == I32 ||
               BK == I64;
    }

    bool isUnsigned() const {
        return BK == U8 ||
               BK == U16 ||
               BK == U32 ||
               BK == U64;
    }

    bool isIntegerTy() const {
        return isSigned() || isUnsigned();
    }

    bool isFloatingTy() const {
        return BK == F32 ||
               BK == F64;
    }

    bool isBooleanTy() const {
        return BK == Bool;
    }
};

class PointerType : public Type {
public:
    static constexpr Kind ClassKind = PointerTypeKind;
private:
    bool IsRawPtr;
    Type* Pointee;
protected:
    PointerType(bool isRawPtr, Type* pointee)
        : Type(ClassKind), IsRawPtr(isRawPtr), Pointee(pointee) {}
public:
    static bool classof(const Type* d) {
        return d->getKind() == PointerTypeKind;
    }

    bool isRaw() const {
        return IsRawPtr;
    }

    Type* getPointee() {
        return Pointee;
    }

    const Type* getPointee() const {
        return Pointee;
    }
};

class ArrayType : public Type {
public:
    static constexpr Kind ClassKind = ArrayTypeKind;
private:
    Type* ElemType;
    size_t InitSize;
public:
    ArrayType(Type* ET, size_t size)
        : Type(ClassKind), ElemType(ET), InitSize(size) {}

    static bool classof(const Type* d) {
        return d->getKind() == ClassKind;
    }

    size_t getInitSize() const {
        return InitSize;
    }

    void setInitSize(size_t size) {
        InitSize = size;
    }

    Type* getElemType() {
        return ElemType;
    }

    const Type* getElemType() const {
        return ElemType;
    }
};

class StructType : public Type {
public:
    static constexpr Kind ClassKind = StructTypeKind;
private:
    ast::StructDecl* DC;
public:
    StructType(ast::StructDecl* decl)
        : Type(ClassKind), DC(decl) {}

    static bool classof(const Type* d) {
        return d->getKind() == ClassKind;
    }

    ast::StructDecl* getDecl() {
        return DC;
    }

    const ast::StructDecl* getDecl() const {
        return DC;
    }
};

// Okay I get it, it's ugly. But it works.
inline bool Type::isIntegerTy() const {
    if (isBuiltinTy())
        return llvm::cast<BuiltinType>(this)->isIntegerTy();
    return false;
}

inline bool Type::isFloatingTy() const {
    if (isBuiltinTy())
        return llvm::cast<BuiltinType>(this)->isFloatingTy();
    return false;
}

inline bool Type::isBooleanTy() const {
    if (isBuiltinTy())
        return llvm::cast<BuiltinType>(this)->isBooleanTy();
    return false;
}

} // namsepace c
