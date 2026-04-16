#pragma once

#include <cassert>

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ErrorHandling.h"

#include "include/AST/Expr.hpp"
#include "include/AST/Type.hpp"
#include "include/IO/SrcSpan.hpp"

namespace c {
namespace ast {

class TypeInfo {
public:
    enum Kind {
        // A named type, like a builtin, an alias, an enum, a struct etc
        Named,

        // Pointers and references. Grouped under Adress
        RawPointer,
        RefPointer,

        Array,
    };
private:
    Kind TypeKind;
    SrcSpan Span;
    const Type* Resolved;

    union {
        struct {
            llvm::StringRef Name;
        } Named;
        struct {
            TypeInfo* Pointee;
        } Adress;
        struct {
            TypeInfo* ElemType;
            Expr* Size;
        } Array;
    };

    TypeInfo(SrcSpan span, Kind kind, 
        llvm::StringRef name = "", 
        TypeInfo* pointee = nullptr, 
        Expr* arraySize = nullptr
    ) : Span(span), TypeKind(kind), Resolved(nullptr) {
        if (isNamedTy()) {
            Named.Name = name;
        } else if (isAdressTy()) {
            Adress.Pointee = pointee;
        } else if (isArrayTy()) {
            Array.ElemType = pointee;
            Array.Size = arraySize;
        } else
            llvm_unreachable("");
    } 
public:
    static TypeInfo CreateNamed(SrcSpan span, llvm::StringRef name) {
        return TypeInfo(span, Kind::Named, name);
    }

    static TypeInfo CreatePointer(SrcSpan span, bool isRaw, TypeInfo* pointee) {
        Kind kind = isRaw ? RawPointer | RefPointer;
        return TypeInfo(span, kind, "", pointee);
    }

    static TypeInfo CreateArray(SrcSpan span, TypeInfo* elemType, Expr* size) {
        return TypeInfo(span, Kind::Array, "", elemType, size);
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
        assert(!isResolved() && "Type is already resolved");
        Resolved = T;
    }

    llvm::StringRef getName() const {
        assert(isNamedTy() && "This TypeInfo does not refer to a named type.");
        return Named.Name;
    }

    TypeInfo* getPointeeType() {
        assert((isPointerTy() || isArrayTy()) && "Only pointers, references and arrays point to a type.");
        if (isArrayTy()) 
            return Array.ElemType;
        return Adress.Pointee;
    }

    const TypeInfo* getPointeeType() const {
        assert((isPointerTy() || isArrayTy()) && "Only pointers, references and arrays point to a type.");
        if (isArrayTy())
            return Array.ElemType;
        return Adress.Pointee;
    }

    Expr* getArraySize() {
        assert(isArrayTy() && "Only array types have a size.");
        return Array.Size;
    }

    const Expr* getArraySize() const {
        assert(isArrayTy() && "Only array types have a size.");
        return Array.Size;
    }

    bool isTypeCompatible(const Type* T) const {
        if (isPointerTy() && T->isPointerTy()) {
            const PointerType* ty = llvm::cast<PointerType>(T);
            if (isRawPointerTy() && ty->isRaw())
                return false;
            return Adress.Pointee->isTypeCompatible(ty->getPointee());
        } else if (isNamedTy() && T->isNamedTy()) {
            return true;
        } else if (isArrayTy() && T->isArrayTy()) {
            return true;
        }
        return false;
    }

    bool isNamedTy() const {
        return TypeKind == Kind::Named;
    }

    bool isPointerTy() const {
        return TypeKind == Kind::RawPointer ||
               TypeKind == Kind::RefPointer;
    }

    bool isRawPointerTy() const {
        return TypeKind == Kind::RawPointer;
    }

    bool isArrayTy() const {
        return TypeKind == Kind::Array;
    }
};

} // namespace ast
} // namespace c
