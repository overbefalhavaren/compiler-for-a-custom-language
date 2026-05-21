#pragma once

#include <cassert>

#include "llvm/ADT/StringRef.h"

#include "include/AST/Type.hpp"
#include "include/IO/SrcSpan.hpp"

namespace c {
namespace ast {

class Expr; // include/AST/Expr.hpp

class TypeInfo {
public:
    enum Kind {
        // Implicit
        Implicit,

        // A named type, like a builtin, an alias, an enum, a struct etc
        Named,

        // Pointers and references. Grouped under Adress
        RawPointer,
        RefPointer,

        Array,
    };
private:
    Kind TypeKind;
    SrcSpan Span = SrcSpan();
    const Type* Resolved = nullptr;

    union {
        struct {
            llvm::StringRef Name;
        } UNamed;
        struct {
            TypeInfo* Pointee;
        } UAdress;
        struct {
            TypeInfo* ElemType;
            size_t Size;
        } UArray;
    };

    TypeInfo(SrcSpan span, Kind kind)
        : Span(span), TypeKind(kind) {} 
public:
    static TypeInfo CreateImplicit(SrcLoc expectedLocForType) {
        return TypeInfo(SrcSpan(expectedLocForType), Implicit);
    }

    static TypeInfo CreateNamed(SrcSpan span, llvm::StringRef name) {
        TypeInfo result(span, Named);
        result.UNamed.Name = name;
        return result;
    }

    static TypeInfo CreatePointer(SrcSpan span, bool isRaw, TypeInfo* pointee) {
        TypeInfo result(span, isRaw ? RawPointer : RefPointer);
        result.UAdress.Pointee = pointee;
        return result;
    }

    static TypeInfo CreateArray(SrcSpan span, TypeInfo* elemType, size_t size) {
        TypeInfo result(span, Array);
        result.UArray.ElemType = elemType;
        result.UArray.Size = size;
        return result;
    }

    bool isResolved() const {
        return Resolved != nullptr;
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

    const Type* getType() const {
        return Resolved;
    }

    void setType(const Type* T) {
        Resolved = T;
    }

    llvm::StringRef getName() const {
        assert(isNamedTy() && "This TypeInfo does not refer to a named type.");
        return UNamed.Name;
    }

    TypeInfo* getPointee() {
        assert((isPointerTy() || isArrayTy()) && "Only pointers, references and arrays point to a type.");
        if (isArrayTy()) 
            return UArray.ElemType;
        return UAdress.Pointee;
    }

    const TypeInfo* getPointee() const {
        assert((isPointerTy() || isArrayTy()) && "Only pointers, references and arrays point to a type.");
        if (isArrayTy())
            return UArray.ElemType;
        return UAdress.Pointee;
    }

    size_t getArraySize() const {
        assert(isArrayTy() && "Only array types have a size.");
        return UArray.Size;
    }

    bool isTypeCompatible(const Type* T) const {
        if (isImplicitTy()) {
            return true;
        } if (isPointerTy() && T->isPointerTy()) {
            const PointerType* ty = llvm::cast<PointerType>(T);
            if (isRawPointerTy() && ty->isRaw())
                return false;
            return UAdress.Pointee->isTypeCompatible(ty->getPointee());
        } else if (isNamedTy() && T->isNamedTy()) {
            return true;
        } else if (isArrayTy() && T->isArrayTy()) {
            return true;
        }
        return false;
    }

    bool isImplicitTy() const {
        return TypeKind == Implicit;
    }

    bool isNamedTy() const {
        return TypeKind == Named;
    }

    bool isPointerTy() const {
        return TypeKind == RawPointer ||
               TypeKind == RefPointer;
    }

    bool isRawPointerTy() const {
        return TypeKind == RawPointer;
    }

    bool isArrayTy() const {
        return TypeKind == Array;
    }
};

} // namespace ast
} // namespace c
